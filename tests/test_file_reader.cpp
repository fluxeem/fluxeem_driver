// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
/**
 * @file test_file_reader.cpp
 * @brief Unit tests for file reader functionality
 */

#include <gtest/gtest.h>
#include <fluxeem/driver/file_reader/raw_file_reader.h>
#include <fluxeem/base/logging/logger.h>
#include <fluxeem/base/define/event_type.h>

#include <fstream>
#include <filesystem>

using namespace fluxeem;

class FileReaderTest : public ::testing::Test {
protected:
    std::string test_data_dir_ = "test_data";
    std::string test_file_path_;
    
    void SetUp() override {
        test_file_path_ = test_data_dir_ + "/test_events.raw";
        
        if (!std::filesystem::exists(test_data_dir_)) {
            std::filesystem::create_directory(test_data_dir_);
        }
    }
    
    void TearDown() override {
    }
    
    void CreateTestFile(const std::string& content) {
        std::ofstream file(test_file_path_, std::ios::binary);
        file << content;
        file.close();
    }
    
    void RemoveTestFile() {
        if (std::filesystem::exists(test_file_path_)) {
            std::filesystem::remove(test_file_path_);
        }
    }
    
    void SkipIfNoTestFile() {
        if (!std::filesystem::exists(test_file_path_)) {
            GTEST_SKIP() << "Test file not found: " << test_file_path_;
        }
    }
};

TEST_F(FileReaderTest, CreateReader) {
    SkipIfNoTestFile();
    
    auto reader = std::make_unique<RawFileReader>(test_file_path_);
    EXPECT_NE(reader, nullptr);
}

TEST_F(FileReaderTest, OpenNonExistentFile) {
    std::string invalid_path = "non_existent_file.raw";
    
    EXPECT_THROW({
        auto reader = std::make_unique<RawFileReader>(invalid_path);
    }, std::exception);
}

TEST_F(FileReaderTest, GetFileInfo) {
    SkipIfNoTestFile();
    
    auto reader = std::make_unique<RawFileReader>(test_file_path_);
    
    EvFileInfo file_info;
    bool result = reader->getFileMetadata(file_info);
    
    if (result) {
        EXPECT_GT(file_info.width, 0);
        EXPECT_GT(file_info.height, 0);
        EXPECT_GT(file_info.max_events, 0);
    }
}

TEST_F(FileReaderTest, GetWidthHeight) {
    SkipIfNoTestFile();
    
    auto reader = std::make_unique<RawFileReader>(test_file_path_);
    
    uint16_t width = reader->getWidth();
    uint16_t height = reader->getHeight();
    
    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
}

TEST_F(FileReaderTest, ReadEvents) {
    SkipIfNoTestFile();
    
    auto reader = std::make_unique<RawFileReader>(test_file_path_);
    
    uint64_t n_events = 100;
    auto events = reader->readEvents(n_events);
    
    if (events && !events->empty()) {
        EXPECT_LE(events->size(), n_events);
        
        for (const auto& event : *events) {
            EXPECT_LT(event.x, reader->getWidth());
            EXPECT_LT(event.y, reader->getHeight());
        }
    }
}

TEST_F(FileReaderTest, ReadEventsFromTimestamp) {
    SkipIfNoTestFile();
    
    auto reader = std::make_unique<RawFileReader>(test_file_path_);
    
    EvFileInfo file_info;
    if (reader->getFileMetadata(file_info) && file_info.max_events > 0) {
        Timestamp start = file_info.start_timestamp + 1000;
        uint64_t n_events = 50;
        
        auto events = reader->readEventsFromTimestamp(start, n_events);
        
        if (events && !events->empty()) {
            for (const auto& event : *events) {
                EXPECT_GE(event.timestamp, start);
            }
        }
    }
}

TEST_F(FileReaderTest, ReadEventsFromEventIndex) {
    SkipIfNoTestFile();
    
    auto reader = std::make_unique<RawFileReader>(test_file_path_);
    
    EvFileInfo file_info;
    if (reader->getFileMetadata(file_info) && file_info.max_events > 100) {
        uint64_t start_event = 50;
        uint64_t n_events = 100;
        
        auto events = reader->readEventsFromEventIndex(start_event, n_events);
        
        if (events && !events->empty()) {
            EXPECT_LE(events->size(), n_events);
        }
    }
}

TEST_F(FileReaderTest, ReadEventsByTimeInterval) {
    SkipIfNoTestFile();
    
    auto reader = std::make_unique<RawFileReader>(test_file_path_);
    
    Timestamp interval = 10000;
    auto events = reader->readEventsByTimeInterval(interval);
    
    if (events && !events->empty()) {
        Timestamp start_ts = events->front().timestamp;
        Timestamp end_ts = events->back().timestamp;
        
        EXPECT_LE(end_ts - start_ts, interval);
    }
}

TEST_F(FileReaderTest, SequentialRead) {
    SkipIfNoTestFile();
    
    auto reader = std::make_unique<RawFileReader>(test_file_path_);
    
    uint64_t batch_size = 100;
    int batch_count = 0;
    const int max_batches = 10;
    
    while (batch_count < max_batches) {
        auto events = reader->readEvents(batch_size);
        
        if (!events || events->empty()) {
            break;
        }
        
        batch_count++;
    }
    
    EXPECT_GT(batch_count, 0);
}

TEST_F(FileReaderTest, EventDataValidity) {
    SkipIfNoTestFile();
    
    auto reader = std::make_unique<RawFileReader>(test_file_path_);
    
    auto events = reader->readEvents(1000);
    
    if (events && !events->empty()) {
        uint16_t width = reader->getWidth();
        uint16_t height = reader->getHeight();
        
        for (const auto& event : *events) {
            EXPECT_LT(event.x, width);
            EXPECT_LT(event.y, height);
            EXPECT_TRUE(event.polarity == 0 || event.polarity == 1);
        }
    }
}

TEST_F(FileReaderTest, TimestampMonotonicity) {
    SkipIfNoTestFile();
    
    auto reader = std::make_unique<RawFileReader>(test_file_path_);
    
    auto events = reader->readEvents(1000);
    
    if (events && events->size() > 1) {
        Timestamp prev_ts = events->front().timestamp;
        
        for (size_t i = 1; i < events->size(); ++i) {
            EXPECT_GE((*events)[i].timestamp, prev_ts);
            prev_ts = (*events)[i].timestamp;
        }
    }
}

TEST_F(FileReaderTest, MultipleReaders) {
    SkipIfNoTestFile();
    
    auto reader1 = std::make_unique<RawFileReader>(test_file_path_);
    auto reader2 = std::make_unique<RawFileReader>(test_file_path_);
    
    auto events1 = reader1->readEvents(100);
    auto events2 = reader2->readEvents(100);
    
    if (events1 && events2 && !events1->empty() && !events2->empty()) {
        EXPECT_EQ(events1->size(), events2->size());
        
        for (size_t i = 0; i < events1->size(); ++i) {
            EXPECT_EQ((*events1)[i].x, (*events2)[i].x);
            EXPECT_EQ((*events1)[i].y, (*events2)[i].y);
            EXPECT_EQ((*events1)[i].timestamp, (*events2)[i].timestamp);
            EXPECT_EQ((*events1)[i].polarity, (*events2)[i].polarity);
        }
    }
}

TEST_F(FileReaderTest, ReadAfterReset) {
    SkipIfNoTestFile();
    
    auto reader = std::make_unique<RawFileReader>(test_file_path_);
    
    auto events1 = reader->readEvents(100);
    
    reader = std::make_unique<RawFileReader>(test_file_path_);
    
    auto events2 = reader->readEvents(100);
    
    if (events1 && events2 && !events1->empty() && !events2->empty()) {
        EXPECT_EQ(events1->size(), events2->size());
    }
}
