#ifndef __EVT3_DECODER_HPP__
#define __EVT3_DECODER_HPP__

#include <fluxeem/hal/data_stream/decoder.hpp>

#include <array>
#include <atomic>
#include <bit>
#include <bitset>
#include <map>
#include <memory>
#include <stdexcept>

#include <fluxeem/base/define/evt3_types.h>
#include <fluxeem/hal/data_stream/decode_statistics.h>
#include <fluxeem/hal/data_stream/evt3_validator.h>

namespace fluxeem
{
	/// @brief Number of bits in timestamp LSB field
	constexpr uint16_t kNumBitsInTimestampLSB = 12;

	/// @brief Number of bits in high timestamp LSB field
	constexpr uint16_t kNumBitsInHighTimestampLSB = 12;

	/// @brief Mask for extracting polarity bit from VECT_BASE_X
	constexpr uint16_t kPolarityMask = 1 << (kNumBitsInTimestampLSB - 1);

	/// @brief Mask for extracting X coordinate (excluding polarity bit)
	constexpr uint16_t kNotPolarityMask = ~kPolarityMask;

	/// @brief Count trailing zeros for non-zero value
	/// @param val Input value (must not be zero)
	/// @return Number of trailing zero bits
	/// @throws std::invalid_argument if val is zero
	inline uint32_t countTrailingZeros(uint32_t val)
	{
		if (val == 0) {
			throw std::invalid_argument("Input value must not be zero.");
		}
		return std::countr_zero(val);
	}

	/// @brief Decoder for EVT3 format event data from Event cameras
	/// 
	/// This decoder handles the EVT3 binary format which includes:
	/// - CD (Change Detection) events with X, Y coordinates, polarity and timestamp
	/// - Vector encoded events for efficient bandwidth usage
	/// - Timestamp events (high and low parts)
	/// - External trigger events
	/// - System/master events
	class FLUXEEM_API EVT3Decoder : public Decoder
	{
	public:
		/// @brief Decoder status containing internal state for each event type
		class EVT3DecoderStatus : public DecoderStatus
		{
		public:
			EVT3DecoderStatus() { status_.fill(0); }

			explicit EVT3DecoderStatus(const std::array<uint16_t, 16>& init_status)
				: status_(init_status) {}

			uint16_t& operator[](std::size_t index) { return status_[index]; }
			const uint16_t& operator[](std::size_t index) const { return status_[index]; }

		private:
			std::array<uint16_t, 16> status_;
		};

		/// @brief Construct EVT3 decoder with sensor dimensions
		/// @param width Sensor width in pixels
		/// @param height Sensor height in pixels
		EVT3Decoder(uint16_t width, uint16_t height);

		~EVT3Decoder() = default;

		// Non-copyable, non-movable
		EVT3Decoder(const EVT3Decoder&) = delete;
		EVT3Decoder& operator=(const EVT3Decoder&) = delete;
		EVT3Decoder(EVT3Decoder&&) = delete;
		EVT3Decoder& operator=(EVT3Decoder&&) = delete;

		/// @brief Decode raw event data from a vector buffer
		/// @param buffer Raw data buffer
		/// @return Number of decoded events
		uint32_t decode(std::vector<uint8_t>& buffer) override;

		/// @brief Decode raw event data from memory range
		/// @param startRawData Pointer to start of raw data
		/// @param endRawData Pointer to end of raw data
		/// @return Number of decoded events
		uint32_t decode(void* startRawData, void* endRawData) override;

		/// @brief Decode only timestamp information (skip event generation)
		/// @param startRawData Pointer to start of raw data
		/// @param endRawData Pointer to end of raw data
		/// @return true on success
		bool decodeTimestamp(void* startRawData, void* endRawData) override;

		/// @brief Reset decoder state to initial values
		void reset();

		/// @brief Get the last decoded timestamp
		/// @return Last timestamp in microseconds, or 0 if not set
		Timestamp getLastTimestamp() override;

		/// @brief Set the last timestamp manually
		/// @param ts Timestamp value to set
		void setLastTimestamp(Timestamp& ts) override;

		/// @brief Get the first decoded timestamp
		/// @return First timestamp in microseconds, or 0 if not set
		Timestamp getFirstTimestamp() const;

		/// @brief Set custom trigger timestamp
		/// @param ts Timestamp to trigger at
		void setCustomTriggerIn(Timestamp ts) override;

		/// @brief Get raw event size in bytes
		/// @return Size of one raw event
		uint8_t getRawEventSizeBytes() const;

		/// @brief Get current decoder status
		EVT3DecoderStatus getDecodeStatus() const;

		/// @brief Set decoder status (for state restoration)
		/// @param status Status to restore
		void setDecodeStatus(EVT3DecoderStatus status);

		/// @brief Add decoded events to output buffer
		/// @param begin Iterator to first event
		/// @param end Iterator past last event
		void addEventToBuffer(EventIterator_t begin, EventIterator_t end);

	private:
		/// @brief Master event types in EVT3 format
		enum class Evt3MasterEventTypes : uint16_t
		{
			MASTER_IN_CD_EVENT_COUNT = 0x014,
			MASTER_RATE_CONTROL_CD_EVENT_COUNT = 0x016
		};

		/// @brief Bitfield structure for timestamp components
		struct BitfieldTimestamp
		{
			uint64_t low  : 12;  ///< Low 12 bits of timestamp
			uint64_t high : 12;  ///< High 12 bits of timestamp
			uint64_t loop : 40;  ///< Loop counter for overflow
		};

		/// @brief Union for timestamp access (bitfield or raw value)
		struct Evt3Timestamp
		{
			union
			{
				BitfieldTimestamp bitfield_time;
				Timestamp time;
			};
		};

		/// @brief Internal decode implementation
		/// @param cur_raw_ev Reference to current event pointer (updated during decode)
		/// @param raw_ev_end Pointer past last event
		/// @return Number of missing bytes if incomplete event at end, 0 otherwise
		int decodeImpl(const Evt3Raw::RawEvent*& cur_raw_ev,
					   const Evt3Raw::RawEvent* const raw_ev_end);

		/// @brief Handle EVT_ADDR_X event - single pixel event
		/// @return Number of raw events consumed (always 1)
		inline int handleAddrX(const Evt3Raw::RawEvent* cur_raw_ev);

		/// @brief Handle VECT_12 event - vector of 12-bit encoded pixels
		/// @return Number of raw events consumed, or negative value if more data needed
		inline int handleVect12(const Evt3Raw::RawEvent* cur_raw_ev,
								const Evt3Raw::RawEvent* raw_ev_end);

		/// @brief Handle EVT_TIME_HIGH event - high bits of timestamp
		/// @return Number of raw events consumed (always 1)
		inline int handleTimeHigh(const Evt3Raw::RawEvent* cur_raw_ev);

		/// @brief Handle EVT_TIME_LOW event - low bits of timestamp
		/// @return Number of raw events consumed (always 1)
		inline int handleTimeLow(const Evt3Raw::RawEvent* cur_raw_ev);

		/// @brief Handle EXT_TRIGGER event - external trigger signal
		/// @return Number of raw events consumed (always 1)
		inline int handleExtTrigger(const Evt3Raw::RawEvent* cur_raw_ev);

		/// @brief Handle OTHERS event - master events and system events
		/// @return Number of raw events consumed, or negative value if more data needed
		inline int handleOthers(const Evt3Raw::RawEvent*& cur_raw_ev,
								const Evt3Raw::RawEvent* raw_ev_end);

		/// @brief Handle default state update events (EVT_ADDR_Y, VECT_BASE_X, etc.)
		/// @return Number of raw events consumed (always 1)
		inline int handleStateUpdate(const Evt3Raw::RawEvent* cur_raw_ev,
									 Evt3EventTypes_4bits type);

		/// @brief Validate timestamp and detect anomalies
		/// @return true if timestamp is valid, false if it should be discarded
		inline bool validateAndUpdateTimestamp(const Evt3Timestamp& new_ts);

		/// @brief Helper class for buffering and forwarding decoded events
		///
		/// For performance, events are buffered and forwarded in batches rather than
		/// one at a time. This reduces callback overhead significantly.
		///
		/// @tparam Event Event type to forward
		/// @tparam BUFFER_SIZE Maximum events to buffer before forwarding
		template <typename Event, int BUFFER_SIZE = 320>
		class DecodedEventForwarder
		{
		public:
			/// @brief Constructor
			/// @param event_decoder Parent decoder instance
			explicit DecodedEventForwarder(EVT3Decoder* event_decoder);

			/// @brief Forward event with buffer overflow check
			/// @param args Arguments for Event constructor
			template <typename... Args>
			void forward(Args&&... args);

			/// @brief Forward event without overflow check (use after reserve())
			/// @param args Arguments for Event constructor
			template <typename... Args>
			void forward_unsafe(Args&&... args);

			/// @brief Flush all buffered events to callback
			void flush();

			/// @brief Reserve space in buffer, flushing if necessary
			/// @param size Number of events to reserve (must be <= BUFFER_SIZE)
			void reserve(int size);

		private:
			void addEvents();

			EVT3Decoder* event_decoder_;
			std::array<Event, BUFFER_SIZE> ev_buf_;
			typename std::array<Event, BUFFER_SIZE>::iterator ev_it_;
		};

		// Event forwarder
		std::unique_ptr<DecodedEventForwarder<Event2D>> cd_event_forwarder_;

		// Validator for data integrity
		BasicCheckValidator validator_;

		// Partial event buffer (for handling split events across buffers)
		std::unique_ptr<std::vector<Evt3Raw::RawEvent>> remain_raw_data_;
		size_t raw_data_missing_count_ = 0;

		// Sensor dimensions
		const uint16_t width_;
		const uint16_t height_;

		// Timestamp state
		Evt3Timestamp last_timestamp_ = {0};
		Evt3Timestamp new_timestamp_tmp_ = {0};
		Evt3Timestamp first_timestamp_ = {0};
		bool last_timestamp_low_set_ = false;
		bool last_timestamp_high_set_ = false;
		bool first_timestamp_set_ = false;

		// Decode state
		bool is_valid_cd_event_ = false;
		bool is_cd_stream_ = false;
		EVT3DecoderStatus decode_state_;
		uint64_t decoded_events_count_ = 0;

		// Custom trigger
		std::atomic<uint64_t> custom_trigger_in_by_ts_ = 0;
	};

} // namespace fluxeem

#endif