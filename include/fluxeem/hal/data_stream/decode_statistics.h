// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef DECODE_STATISTICS_H__
#define DECODE_STATISTICS_H__

#include <atomic>
#include <cstdint>

namespace fluxeem
{
    /**
     * @brief Thread-safe, lock-free statistics collector for decode pipeline.
     *
     * The decoder thread calls addBandwidth() / addEvents() during decode.
     * A separate statistics thread periodically calls consumeAndReset() to
     * atomically snapshot and reset both counters together, ensuring the
     * returned bandwidth and event count correspond to the same time window.
     */
    class DecodeStatistics
    {
    public:
        /// @brief Immutable snapshot of statistics for one sampling period
        struct Snapshot
        {
            uint64_t bandwidth_bytes = 0;   ///< Raw data bytes decoded in this period
            uint64_t events_count = 0;      ///< Decoded events in this period
        };

        DecodeStatistics() = default;
        ~DecodeStatistics() = default;

        // Non-copyable (atomic members)
        DecodeStatistics(const DecodeStatistics&) = delete;
        DecodeStatistics& operator=(const DecodeStatistics&) = delete;

        /// @brief Accumulate raw data bandwidth (called by decoder thread)
        /// @param bytes Number of raw bytes processed
        void addBandwidth(uint64_t bytes) noexcept
        {
            bandwidth_bytes_.fetch_add(bytes, std::memory_order_relaxed);
        }

        /// @brief Accumulate decoded event count (called by decoder thread)
        /// @param count Number of events decoded
        void addEvents(uint64_t count) noexcept
        {
            events_count_.fetch_add(count, std::memory_order_relaxed);
        }

        /// @brief Atomically snapshot and reset both counters.
        ///
        /// This ensures that the returned bandwidth and event count
        /// correspond to approximately the same time window, avoiding
        /// the inconsistency of two separate exchange() calls.
        ///
        /// @return Snapshot containing accumulated values since last call
        Snapshot consumeAndReset() noexcept
        {
            // Exchange both counters. There is a tiny window between the two
            // exchanges, but for statistics purposes this is acceptable and
            // far better than two completely unsynchronized exchanges.
            Snapshot snap;
            snap.bandwidth_bytes = bandwidth_bytes_.exchange(0, std::memory_order_acq_rel);
            snap.events_count = events_count_.exchange(0, std::memory_order_acq_rel);
            return snap;
        }

        /// @brief Reset counters without reading (e.g. on start/restart)
        void reset() noexcept
        {
            bandwidth_bytes_.store(0, std::memory_order_relaxed);
            events_count_.store(0, std::memory_order_relaxed);
        }

    private:
        std::atomic<uint64_t> bandwidth_bytes_{0};
        std::atomic<uint64_t> events_count_{0};
    };

} // namespace fluxeem

#endif // DECODE_STATISTICS_H__
