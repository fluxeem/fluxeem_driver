// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#include <fluxeem/hal/data_stream/evt3_decoder.h>
#include <fluxeem/base/logging/logger.h>
#include <fluxeem/base/define/evt3_types.h>
#include <fluxeem/hal/data_stream/evt3_validator.h>

namespace fluxeem
{
	EVT3Decoder::EVT3Decoder(uint16_t width, uint16_t height)
		: width_(width), height_(height), validator_(height, width)
	{
		cd_event_forwarder_ = std::make_unique<DecodedEventForwarder<Event2D>>(this);
		remain_raw_data_ = std::make_unique<std::vector<Evt3Raw::RawEvent>>();
	}

	uint32_t EVT3Decoder::decode(std::vector<uint8_t> &buffer)
	{
		if (buffer.data() == nullptr)
		{
			return 0;
		}
		RawData *start = reinterpret_cast<RawData *>(buffer.data());
		RawData *end = start + buffer.size() / sizeof(RawData);
		return decode(start, end);
	}

	uint32_t EVT3Decoder::decode(void *startRawData, void *endRawData)
	{
		// Update bandwidth statistics via injected collector
		if (statistics_) {
			statistics_->addBandwidth(
				static_cast<uint64_t>(std::distance(static_cast<char *>(startRawData),
													static_cast<char *>(endRawData))));
		}

		decoded_events_count_ = 0;

		const Evt3Raw::RawEvent *cur_raw_ev =
			reinterpret_cast<const Evt3Raw::RawEvent *>(startRawData);
		const Evt3Raw::RawEvent *const raw_ev_end =
			reinterpret_cast<const Evt3Raw::RawEvent *>(endRawData);

		// Handle remaining data from previous decode call
		if (raw_data_missing_count_ > 0)
		{
			const auto available = static_cast<size_t>(std::distance(cur_raw_ev, raw_ev_end));

			if (available >= raw_data_missing_count_)
			{
				// We have enough data to complete the partial event
				remain_raw_data_->insert(remain_raw_data_->end(),
										 cur_raw_ev,
										 cur_raw_ev + raw_data_missing_count_);
				cur_raw_ev += raw_data_missing_count_;
				raw_data_missing_count_ = 0;

				const Evt3Raw::RawEvent *remain_begin = remain_raw_data_->data();
				decodeImpl(remain_begin, remain_begin + remain_raw_data_->size());
				remain_raw_data_->clear();
			}
			else
			{
				// Still not enough data, store and wait
				remain_raw_data_->insert(remain_raw_data_->end(),
										 cur_raw_ev,
										 cur_raw_ev + available);
				raw_data_missing_count_ -= available;
				return 0;
			}
		}

		// Decode main data
		raw_data_missing_count_ = decodeImpl(cur_raw_ev, raw_ev_end);

		// Store partial event data at end of buffer
		if (raw_data_missing_count_ > 0)
		{
			remain_raw_data_->insert(remain_raw_data_->end(), cur_raw_ev, raw_ev_end);
		}

		cd_event_forwarder_->flush();
		return static_cast<uint32_t>(decoded_events_count_);
	}

	bool EVT3Decoder::decodeTimestamp(void *startRawData, void *endRawData)
	{
		const Evt3Raw::RawEvent *cur_raw_ev =
			reinterpret_cast<const Evt3Raw::RawEvent *>(startRawData);
		const Evt3Raw::RawEvent *const raw_ev_end =
			reinterpret_cast<const Evt3Raw::RawEvent *>(endRawData);

		while (cur_raw_ev < raw_ev_end)
		{
			const Evt3EventTypes_4bits type =
				static_cast<Evt3EventTypes_4bits>(cur_raw_ev->type);

			if (type == Evt3EventTypes_4bits::EVT_TIME_HIGH)
			{
				const Evt3Raw::Event_Time *ev_timehigh =
					reinterpret_cast<const Evt3Raw::Event_Time *>(cur_raw_ev);
				static constexpr uint64_t kMaxTimestamp = 1ULL << 11;

				new_timestamp_tmp_ = last_timestamp_;
				validator_.validateTimeHigh(new_timestamp_tmp_.bitfield_time.high,
											ev_timehigh->time);

				new_timestamp_tmp_.bitfield_time.loop +=
					(new_timestamp_tmp_.bitfield_time.high >= kMaxTimestamp + ev_timehigh->time);
				new_timestamp_tmp_.bitfield_time.low =
					(new_timestamp_tmp_.bitfield_time.high == ev_timehigh->time)
						? new_timestamp_tmp_.bitfield_time.low
						: 0;
				new_timestamp_tmp_.bitfield_time.high = ev_timehigh->time;

				// Stricter threshold for timestamp-only decode (10 seconds)
				const bool valid = (new_timestamp_tmp_.time >= last_timestamp_.time) &&
								   !(new_timestamp_tmp_.bitfield_time.loop != 0 &&
									 new_timestamp_tmp_.time - last_timestamp_.time > 10000000);

				if (valid)
				{
					last_timestamp_ = new_timestamp_tmp_;
					last_timestamp_high_set_ = true;
				}
			}
			else if (type == Evt3EventTypes_4bits::EVT_TIME_LOW)
			{
				decode_state_[(int)type] = cur_raw_ev->content;
				last_timestamp_.bitfield_time.low = cur_raw_ev->content;
				last_timestamp_low_set_ = true;
				validator_.stateUpdate(cur_raw_ev);
			}
			++cur_raw_ev;
		}
		return true;
	}

	void EVT3Decoder::reset()
	{
		// Reset timestamp state
		last_timestamp_ = {0};
		first_timestamp_ = {0};
		new_timestamp_tmp_ = {0};
		last_timestamp_low_set_ = false;
		last_timestamp_high_set_ = false;
		first_timestamp_set_ = false;

		// Reset decode state
		raw_data_missing_count_ = 0;
		remain_raw_data_->clear();
		decoded_events_count_ = 0;
		is_valid_cd_event_ = false;
		is_cd_stream_ = false;
		decode_state_ = EVT3DecoderStatus();
	}

	// ============================================================================
	// Event Handlers (Inline implementations for performance)
	// ============================================================================

	inline int EVT3Decoder::handleAddrX(const Evt3Raw::RawEvent *cur_raw_ev)
	{
		if (is_valid_cd_event_)
		{
			const Evt3Raw::Event_PosX *ev_posx =
				reinterpret_cast<const Evt3Raw::Event_PosX *>(cur_raw_ev);

			if (ev_posx->x < width_)
			{
				cd_event_forwarder_->forward(
					static_cast<uint16_t>(ev_posx->x),
					decode_state_[(int)Evt3EventTypes_4bits::EVT_ADDR_Y],
					static_cast<uint8_t>(ev_posx->pol),
					last_timestamp_.time);
			}
		}
		return 1;
	}

	inline int EVT3Decoder::handleVect12(const Evt3Raw::RawEvent *cur_raw_ev,
										 const Evt3Raw::RawEvent *raw_ev_end)
	{
		constexpr uint32_t kVect12Size = sizeof(Evt3Raw::Event_Vect12_12_8) / sizeof(RawData);
		constexpr uint16_t kNumBits = 32;

		// Check if we have enough data
		if (cur_raw_ev + kVect12Size > raw_ev_end)
		{
			return -static_cast<int>(std::distance(raw_ev_end, cur_raw_ev + kVect12Size));
		}

		if (!is_valid_cd_event_)
		{
			return static_cast<int>(kVect12Size);
		}

		int next_offset;
		if (validator_.validateVect12By12By8Pattern(
				cur_raw_ev,
				decode_state_[(int)Evt3EventTypes_4bits::VECT_BASE_X] & kNotPolarityMask,
				next_offset))
		{

			const Evt3Raw::Event_Vect12_12_8 *ev_vect =
				reinterpret_cast<const Evt3Raw::Event_Vect12_12_8 *>(cur_raw_ev);

			Evt3Raw::Mask m;
			m.m.valid1 = ev_vect->valid1;
			m.m.valid2 = ev_vect->valid2;
			m.m.valid3 = ev_vect->valid3;

			uint32_t valid = m.valid;
			const uint16_t last_x = decode_state_[(int)Evt3EventTypes_4bits::VECT_BASE_X] & kNotPolarityMask;

			if (last_x < width_)
			{
				cd_event_forwarder_->reserve(32);

				const uint8_t polarity = static_cast<uint8_t>(
					(decode_state_[(int)Evt3EventTypes_4bits::VECT_BASE_X] & kPolarityMask) != 0);
				const uint16_t addr_y = decode_state_[(int)Evt3EventTypes_4bits::EVT_ADDR_Y];
				const Timestamp ts = last_timestamp_.time;

				while (valid)
				{
					const uint16_t off = countTrailingZeros(valid);
					valid &= ~(1u << off);
					cd_event_forwarder_->forward_unsafe(
						static_cast<uint16_t>(last_x + off), addr_y, polarity, ts);
				}
			}
		}

		if (validator_.hasValidVectBase())
		{
			decode_state_[(int)Evt3EventTypes_4bits::VECT_BASE_X] += kNumBits;
		}

		return next_offset;
	}

	inline bool EVT3Decoder::validateAndUpdateTimestamp(const Evt3Timestamp &new_ts)
	{
		// Check for timestamp rollback
		if (new_ts.time < last_timestamp_.time)
		{
			uint32_t remaining = ts_jump_suppress_count_.load(std::memory_order_relaxed);
			if (remaining > 0) {
				ts_jump_suppress_count_.fetch_sub(1, std::memory_order_relaxed);
			} else {
				LOG_WARN("Timestamp rollback detected. new: %llu, last: %llu",
						 new_ts.time, last_timestamp_.time);
			}
			return false;
		}

		// Check for timestamp jump (threshold: 1 second)
		if (new_ts.bitfield_time.loop != 0 &&
			new_ts.time - last_timestamp_.time > 1000000)
		{
			uint32_t remaining = ts_jump_suppress_count_.load(std::memory_order_relaxed);
			if (remaining > 0) {
				ts_jump_suppress_count_.fetch_sub(1, std::memory_order_relaxed);
			} else {
				LOG_WARN("Timestamp jump detected. new: %llu, last: %llu",
						 new_ts.time, last_timestamp_.time);
			}
			return false;
		}

		return true;
	}

	inline int EVT3Decoder::handleTimeHigh(const Evt3Raw::RawEvent *cur_raw_ev)
	{
		const Evt3Raw::Event_Time *ev_timehigh =
			reinterpret_cast<const Evt3Raw::Event_Time *>(cur_raw_ev);
		static constexpr uint64_t kMaxTimestamp = 1ULL << 11;

		new_timestamp_tmp_ = last_timestamp_;
		validator_.validateTimeHigh(new_timestamp_tmp_.bitfield_time.high, ev_timehigh->time);

		// Handle timestamp overflow (loop increment)
		new_timestamp_tmp_.bitfield_time.loop +=
			(new_timestamp_tmp_.bitfield_time.high >= kMaxTimestamp + ev_timehigh->time);

		// Avoid momentary time discrepancies - reset low if high changed
		new_timestamp_tmp_.bitfield_time.low =
			(new_timestamp_tmp_.bitfield_time.high == ev_timehigh->time)
				? new_timestamp_tmp_.bitfield_time.low
				: 0;

		new_timestamp_tmp_.bitfield_time.high = ev_timehigh->time;

		// Validate and update (logs warnings but still accepts)
		validateAndUpdateTimestamp(new_timestamp_tmp_);

		last_timestamp_ = new_timestamp_tmp_;
		last_timestamp_high_set_ = true;

		return 1;
	}

	inline int EVT3Decoder::handleTimeLow(const Evt3Raw::RawEvent *cur_raw_ev)
	{
		constexpr int type_index = static_cast<int>(Evt3EventTypes_4bits::EVT_TIME_LOW);
		decode_state_[type_index] = cur_raw_ev->content;
		last_timestamp_.bitfield_time.low = cur_raw_ev->content;
		last_timestamp_low_set_ = true;
		validator_.stateUpdate(cur_raw_ev);

		// Set first timestamp when both high and low are set
		if (!first_timestamp_set_ && last_timestamp_high_set_ && last_timestamp_low_set_)
		{
			first_timestamp_ = last_timestamp_;
			first_timestamp_set_ = true;
		}

		return 1;
	}

	inline int EVT3Decoder::handleExtTrigger(const Evt3Raw::RawEvent *cur_raw_ev)
	{
		const Evt3Raw::Event_ExtTrigger *ev_trigger =
			reinterpret_cast<const Evt3Raw::Event_ExtTrigger *>(cur_raw_ev);

		if (ev_trigger_in_handling_callback_)
		{
			// Flush pending events on falling edge
			if (ev_trigger->pol == 0)
			{
				cd_event_forwarder_->flush();
			}

			ev_trigger_in_handling_callback_(EventTriggerIn(
				static_cast<short>(ev_trigger->id),
				static_cast<short>(ev_trigger->pol),
				static_cast<Timestamp>(last_timestamp_.time)));
		}
		return 1;
	}

	inline int EVT3Decoder::handleOthers(const Evt3Raw::RawEvent *&cur_raw_ev,
										 const Evt3Raw::RawEvent *raw_ev_end)
	{
		const uint16_t master_type = cur_raw_ev->content;

		switch (master_type)
		{
		case static_cast<uint16_t>(Evt3MasterEventTypes::MASTER_RATE_CONTROL_CD_EVENT_COUNT):
		case static_cast<uint16_t>(Evt3MasterEventTypes::MASTER_IN_CD_EVENT_COUNT):
		{
			constexpr uint32_t kEvtSize =
				1 + sizeof(Evt3Raw::Event_Continue12_12_4) / sizeof(Evt3Raw::RawEvent);

			if (cur_raw_ev + kEvtSize > raw_ev_end)
			{
				return -static_cast<int>(std::distance(raw_ev_end, cur_raw_ev + kEvtSize));
			}

			++cur_raw_ev;
			int next_offset;
			validator_.validateContinue12By12By4Pattern(cur_raw_ev, next_offset);
			return 1 + next_offset;
		}
		default:
			return 1;
		}
	}

	inline int EVT3Decoder::handleStateUpdate(const Evt3Raw::RawEvent *cur_raw_ev,
											  Evt3EventTypes_4bits type)
	{
		decode_state_[(int)type] = cur_raw_ev->content;

		// Track event type (CD vs EM):
		// - EVT_ADDR_Y (0x0): is_cd_stream_ = true (CD event)
		// - 0x1: is_cd_stream_ = false (EM event)
		// - type >= EVT_ADDR_X: keep is_cd_stream_ unchanged
		if (type < Evt3EventTypes_4bits::EVT_ADDR_X) {
			is_cd_stream_ = (type == Evt3EventTypes_4bits::EVT_ADDR_Y);
		}

		// Validate that event is within sensor bounds
		is_valid_cd_event_ = is_cd_stream_ && (decode_state_[(int)Evt3EventTypes_4bits::EVT_ADDR_Y] < height_);

		// Update time low if this is a time low event
		if (type == Evt3EventTypes_4bits::EVT_TIME_LOW)
		{
			last_timestamp_.bitfield_time.low = decode_state_[(int)Evt3EventTypes_4bits::EVT_TIME_LOW];
		}

		validator_.stateUpdate(cur_raw_ev);

		return 1;
	}

	int EVT3Decoder::decodeImpl(const Evt3Raw::RawEvent *&cur_raw_ev,
								const Evt3Raw::RawEvent *const raw_ev_end)
	{
		while (cur_raw_ev < raw_ev_end)
		{
			const Evt3EventTypes_4bits type =
				static_cast<Evt3EventTypes_4bits>(cur_raw_ev->type);

			int consumed = 0;

			switch (type)
			{
			case Evt3EventTypes_4bits::EVT_ADDR_X:
				consumed = handleAddrX(cur_raw_ev);
				break;

			case Evt3EventTypes_4bits::VECT_12:
				consumed = handleVect12(cur_raw_ev, raw_ev_end);
				if (consumed < 0)
				{
					return -consumed; // Return missing data count
				}
				break;

			case Evt3EventTypes_4bits::EVT_TIME_HIGH:
				consumed = handleTimeHigh(cur_raw_ev);
				break;

			case Evt3EventTypes_4bits::EVT_TIME_LOW:
				consumed = handleTimeLow(cur_raw_ev);
				break;

			case Evt3EventTypes_4bits::EXT_TRIGGER:
				consumed = handleExtTrigger(cur_raw_ev);
				break;

			case Evt3EventTypes_4bits::OTHERS:
			{
				int result = handleOthers(cur_raw_ev, raw_ev_end);
				if (result < 0)
				{
					return -result; // Return missing data count
				}
				consumed = result;
				break;
			}

			default:
				// State update events: EVT_ADDR_Y, VECT_BASE_X, VECT_8, CONTINUED_4, etc.
				consumed = handleStateUpdate(cur_raw_ev, type);
				break;
			}

			cur_raw_ev += consumed;
		}

		return 0; // All events decoded successfully
	}

	uint8_t EVT3Decoder::getRawEventSizeBytes() const
	{
		return sizeof(Evt3Raw::RawEvent);
	}

	EVT3Decoder::EVT3DecoderStatus EVT3Decoder::getDecodeStatus() const
	{
		return decode_state_;
	}

	void EVT3Decoder::setDecodeStatus(EVT3DecoderStatus status)
	{
		decode_state_ = status;
		remain_raw_data_->clear();
	}

	Timestamp EVT3Decoder::getLastTimestamp()
	{
		if (last_timestamp_high_set_ && last_timestamp_low_set_)
		{
			return last_timestamp_.time;
		}
		return 0;
	}

	void EVT3Decoder::setLastTimestamp(Timestamp &ts)
	{
		last_timestamp_.time = ts;
	}

	void EVT3Decoder::suppressNextTsJumpWarning(uint32_t count)
	{
		ts_jump_suppress_count_.store(count, std::memory_order_relaxed);
	}

	Timestamp EVT3Decoder::getFirstTimestamp() const
	{
		if (first_timestamp_set_ && last_timestamp_low_set_ && last_timestamp_high_set_)
		{
			return first_timestamp_.time;
		}
		return 0;
	}

	void EVT3Decoder::addEventToBuffer(EventIterator_t begin, EventIterator_t end)
	{
		const auto count = std::distance(begin, end);
		decoded_events_count_ += count;

		// Accumulate event count in external statistics collector
		if (statistics_) {
			statistics_->addEvents(static_cast<uint64_t>(count));
		}

		if (event_batch_handling_callback_)
		{
			event_batch_handling_callback_(begin, end);
		}
	}

	template <typename Event, int BUFFER_SIZE>
	EVT3Decoder::DecodedEventForwarder<Event, BUFFER_SIZE>::DecodedEventForwarder(
		EVT3Decoder *event_decoder)
		: event_decoder_(event_decoder), ev_it_(ev_buf_.begin())
	{
	}

	template <typename Event, int BUFFER_SIZE>
	template <typename... Args>
	void EVT3Decoder::DecodedEventForwarder<Event, BUFFER_SIZE>::forward(Args &&...args)
	{
		*ev_it_ = Event(std::forward<Args>(args)...);

		if (++ev_it_ == ev_buf_.end())
		{
			addEvents();
		}
	}

	template <typename Event, int BUFFER_SIZE>
	template <typename... Args>
	void EVT3Decoder::DecodedEventForwarder<Event, BUFFER_SIZE>::forward_unsafe(Args &&...args)
	{
		*ev_it_ = Event(std::forward<Args>(args)...);
		++ev_it_;
	}

	template <typename Event, int BUFFER_SIZE>
	void EVT3Decoder::DecodedEventForwarder<Event, BUFFER_SIZE>::flush()
	{
		if (ev_it_ != ev_buf_.begin())
		{
			addEvents();
		}
	}

	template <typename Event, int BUFFER_SIZE>
	void EVT3Decoder::DecodedEventForwarder<Event, BUFFER_SIZE>::reserve(int size)
	{
		// Reserve room for (size + 1) events:
		// - 'size' events via forward_unsafe()
		// - 1 additional event via forward() before overflow check
		if (std::distance(ev_it_, ev_buf_.end()) < size + 1)
		{
			addEvents();
		}
	}

	template <typename Event, int BUFFER_SIZE>
	void EVT3Decoder::DecodedEventForwarder<Event, BUFFER_SIZE>::addEvents()
	{
		event_decoder_->addEventToBuffer(
			ev_buf_.data(),
			ev_buf_.data() + std::distance(ev_buf_.begin(), ev_it_));
		ev_it_ = ev_buf_.begin();
	}

} // namespace fluxeem