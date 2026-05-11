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

	/// @brief A base class to decode raw event stream from camera
	///
	/// Statistics collection is decoupled from the decoder. The owner
	/// (e.g. EventCamera) creates a DecodeStatistics object and
	/// injects it via setStatistics(). The decoder accumulates counters
	/// through that pointer during decode.
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

		/// @brief Only the time data in the original data is parsed.
		/// Event decode is not performed, and event callbacks are not triggered.
		/// It is generally used for timestamp location
		virtual bool decodeTimestamp(void* startRawData, void* endRawData) = 0;

		virtual Timestamp getLastTimestamp() = 0;

		virtual void setLastTimestamp(Timestamp& ts) = 0;

		virtual uint8_t getRawEventSizeBytes() const = 0;

		virtual void setCustomTriggerIn(Timestamp ts) {}

	protected:
		EventBatchHandleCallback event_batch_handling_callback_ = nullptr;
		EvTriggerInCallback ev_trigger_in_handling_callback_ = nullptr;
		DecodeStatistics* statistics_ = nullptr;  ///< Non-owning, injected by owner
	};

} // namespace fluxeem

#endif // __DECODER_HPP__
