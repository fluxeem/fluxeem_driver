// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef RAW_FILE_READER_H
#define RAW_FILE_READER_H

#include <atomic>
#include <functional>
#include <mutex>
#include <queue>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <fstream>
#include <fluxeem/base/define/base_define.h>
#include <fluxeem/driver/file_reader/ev_file_reader.h>
#include <fluxeem/hal/data_stream/decoder.hpp>
#include <fluxeem/hal/data_stream/evt3_decoder.h>
#include <fluxeem/hal/data_stream/decode_factory.hpp>
#include <fluxeem/base/define/event_type.h>

namespace fluxeem
{
    /**
     * @brief RAW file reader for EVT3-encoded event data.
     *
     * This class is NOT thread-safe. All public methods must be called
     * sequentially from the same thread. No internal mutex is held;
     * external synchronization is required if shared across threads.
     */
    class FLUXEEM_API RawFileReader : public EvFileReader
    {
        struct Bookmark {
            std::streamsize byte_offset{ 0 };    ///< Byte offset in the raw file
            EVT3Decoder::EVT3DecoderStatus status;   ///< Decoder state snapshot for replay
            Timestamp timestamp{ 0 };       ///< Timestamp at this bookmark
            uint32_t event_count{ 0 };      ///< Cumulative event count from file start
        };
        using BookmarkMap = std::map<Timestamp, Bookmark>;
        using BookmarkEventsMap = std::map<uint64_t, Timestamp>;
    public:
        explicit RawFileReader(const std::string& filepath);

        ~RawFileReader();

        bool open() override;

        bool getStartTime(Timestamp &start_timestamp) override;

        bool getEndTime(Timestamp &end_timestamp) override;

        bool getEventCount(uint64_t &num) override;

        bool isEndReached() override;

        bool seekToTimestamp(Timestamp t) override;

        bool seekToEventIndex(uint64_t n_event) override;

        Timestamp getCurrentTimestamp() override;

        uint64_t getCurrentEventIndex() override;

        std::shared_ptr<EventBatch> readEvents(uint64_t n) override;

        std::shared_ptr<EventBatch> readEventsFromTimestamp(Timestamp start, uint64_t n) override;

        std::shared_ptr<EventBatch> readEventsFromEventIndex(uint64_t start_event_num, uint64_t n) override;

        std::shared_ptr<EventBatch> readEventsByTimeIntervalFromTimestamp(Timestamp start, Timestamp interval) override;

        std::shared_ptr<EventBatch> readEventsByTimeInterval(Timestamp interval) override;

        std::shared_ptr<EventBatch> readEventsByTimeIntervalFromEventIndex(uint64_t event_num, Timestamp interval) override;

        bool exportEventsToRaw(Timestamp start, Timestamp end, const std::string& out_file_path) override;

        uint32_t registerTriggerInCallback(EvTriggerInCallback cb) override;

		bool unregisterTriggerInCallback(uint32_t callback_id) override;

        uint16_t getWidth() const override { return file_info_.width; }

        uint16_t getHeight() const override { return file_info_.height; }

        bool getDecodeStatistics(uint64_t& bandwidth_bytes, uint64_t& events_count) override;

        bool getFileMetadata(EvFileInfo& out_file_info) override;

    private:
        std::unique_ptr<Decoder> decoder_;

        /// @brief Helper to access EVT3Decoder-specific methods (decode status)
        EVT3Decoder& evt3Decoder() { return static_cast<EVT3Decoder&>(*decoder_); }

        std::string input_file_path_;
        std::string index_file_path_;
        std::ifstream input_stream_;

        static constexpr uint64_t kReadChunkSizeBytes = 128 * 1024;  ///< 128 KB per read
        std::vector<uint8_t> io_buffer_;  ///< Reusable I/O buffer for hot-path reads (seekToTimestamp, readAndDecode, etc.)
        std::streamsize data_start_offset_;
        std::shared_ptr<EventBatch> decoded_events_buffer_;
        std::shared_ptr<EventBatch> cached_events_buffer_;
        std::shared_ptr<EventBatch> read_events_buffer_;
        uint64_t current_event_index_ = 0;
        Timestamp current_timestamp_ = 0;
        Timestamp start_timestamp_ = 0;
        Timestamp end_timestamp_ = 0;
        uint64_t total_event_count_ = 0;
        uint64_t decode_bandwidth_accum_bytes_ = 0;
        uint64_t decode_events_accum_count_ = 0;
        EvFileInfo file_info_{};   ///< Zero-initialized; populated from file header

        std::streampos file_size_bytes_;

        // trigger in callbacks
        std::unordered_map<uint32_t, EvTriggerInCallback> trigger_in_callbacks_;
		uint32_t next_trigger_callback_id_ = 0;
		std::mutex trigger_in_callbacks_mutex_;

        // ---- Bookmark index ----
        BookmarkMap bookmarks_by_timestamp_;
        BookmarkEventsMap timestamp_by_event_count_;
        static constexpr Timestamp kBookmarkPeriodUs = 2000; ///< One bookmark every 2 ms
        /// Index file format version. Bump this whenever the bookmark
        /// serialization format changes so that stale .flxidx files are
        /// automatically invalidated and rebuilt.
        static constexpr uint32_t kIndexFileVersion = 2;

        void serializeBookmarkMap(const BookmarkMap& bookmarks, const std::string& filename);
        void loadBookmarksFromFile(const std::string& filename);
        bool checkBookmarksFromFile(const std::string& filename);
        void initBookmark();
        static uint8_t crc8(const std::vector<uint8_t>& data);

        bool getRawFileBookmark(Timestamp ts, Bookmark& bm);
        bool getRawFileBookmarkByNEvents(uint64_t n_event, Bookmark& bm);

        /// @brief Read and decode events until stop_condition returns true
        bool readAndDecodeEvents(std::function<bool()> stop_condition);

        /// @brief Parse the text header of a .raw file into EvFileInfo
        static bool readFileHeader(std::ifstream& file_stream, EvFileInfo& file_info);
    };

} // namespace fluxeem

#endif //RAW_FILE_READER_H
