// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef __DECODER_HPP__
#define __DECODER_HPP__

#include <cstdint>
#include <vector>
#include <memory>
#include <functional>

#include <fluxeem/base/define/base_define.h>
#include <fluxeem/base/define/event_type.h>

namespace fluxeem
{
	class DecodeStatistics;  // Forward declaration

	class FLUXEEM_API DecoderStatus
	{
	};

	class FLUXEEM_API Decoder
	{
	public:
		virtual ~Decoder() = default;

		void setEventBatchHandleCallback(EventBatchHandleCallback cb)
		{
			event_batch_handling_callback_ = cb;
		}

		void setTriggerInCallback(EvTriggerInCallback cb)
		{
			ev_trigger_in_handling_callback_ = cb;
		}

		/// @brief Inject an external statistics collector (non-owning)
		/// @param stats Pointer to DecodeStatistics owned by caller (must outlive decoder)
		void setStatistics(DecodeStatistics* stats) noexcept
		{
			statistics_ = stats;
		}

		/// @brief Get the injected statistics collector
		DecodeStatistics* getStatistics() const noexcept { return statistics_; }

		virtual uint32_t decode(std::vector<uint8_t>& buffer) = 0;

		virtual uint32_t decode(void* startRawData, void* endRawData) = 0;

		virtual bool decodeTimestamp(void* startRawData, void* endRawData) = 0;

		virtual Timestamp getLastTimestamp() = 0;

		virtual void setLastTimestamp(Timestamp& ts) = 0;

		virtual uint8_t getRawEventSizeBytes() const = 0;

		virtual void setCustomTriggerIn(Timestamp ts) {}

		/// @brief Suppress the next N timestamp jump/rollback warnings.
		/// Call this after a seek operation so the inevitable first discontinuity
		/// is silently ignored; subsequent anomalies will still be reported.
		/// @param count Number of warnings to suppress (default 1)
		virtual void suppressNextTsJumpWarning(uint32_t count = 1) {}

	protected:
		EventBatchHandleCallback event_batch_handling_callback_ = nullptr;
		EvTriggerInCallback ev_trigger_in_handling_callback_ = nullptr;
		DecodeStatistics* statistics_ = nullptr;  ///< Non-owning, injected by owner
	};

} // namespace fluxeem

#endif // __DECODER_HPP__
