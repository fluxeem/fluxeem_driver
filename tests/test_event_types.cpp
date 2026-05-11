// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
/**
 * @file test_event_types.cpp
 * @brief Tests for event types (Event2D, EventTriggerIn, etc.)
 */

#include <gtest/gtest.h>
#include <fluxeem/base/define/event_type.h>

using namespace fluxeem;

class Event2DTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

TEST_F(Event2DTest, DefaultConstructor) {
    Event2D event;
    EXPECT_EQ(event.x, 0);
    EXPECT_EQ(event.y, 0);
    EXPECT_EQ(event.polarity, 0);
    EXPECT_EQ(event.timestamp, 0);
}

TEST_F(Event2DTest, ParameterizedConstructor) {
    Event2D event(100, 200, 1, 12345678);
    EXPECT_EQ(event.x, 100);
    EXPECT_EQ(event.y, 200);
    EXPECT_EQ(event.polarity, 1);
    EXPECT_EQ(event.timestamp, 12345678);
}

TEST_F(Event2DTest, PolarityValues) {
    // Test polarity 0 (decrease)
    Event2D event_decrease(50, 50, 0, 1000);
    EXPECT_EQ(event_decrease.polarity, 0);

    // Test polarity 1 (increase)
    Event2D event_increase(50, 50, 1, 1000);
    EXPECT_EQ(event_increase.polarity, 1);
}

TEST_F(Event2DTest, BoundaryValues) {
    // Test max uint16_t values for x and y
    Event2D event(65535, 65535, 1, UINT64_MAX);
    EXPECT_EQ(event.x, 65535);
    EXPECT_EQ(event.y, 65535);
    EXPECT_EQ(event.timestamp, UINT64_MAX);
}

class EventTriggerInTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(EventTriggerInTest, DefaultConstructor) {
    EventTriggerIn trigger;
    EXPECT_EQ(trigger.id, 0);
    EXPECT_EQ(trigger.polarity, 0);
    EXPECT_EQ(trigger.timestamp, 0);
}

TEST_F(EventTriggerInTest, ParameterizedConstructor) {
    EventTriggerIn trigger(5, 1, 9876543210);
    EXPECT_EQ(trigger.id, 5);
    EXPECT_EQ(trigger.polarity, 1);
    EXPECT_EQ(trigger.timestamp, 9876543210);
}

class EvCameraStatisticInfoTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(EvCameraStatisticInfoTest, DefaultConstructor) {
    EvCameraStatisticInfo info;
    EXPECT_EQ(info.bandwidth_byte, 0);
    EXPECT_EQ(info.events_count, 0);
}

TEST_F(EvCameraStatisticInfoTest, ParameterizedConstructor) {
    EvCameraStatisticInfo info(1024000, 50000);
    EXPECT_EQ(info.bandwidth_byte, 1024000);
    EXPECT_EQ(info.events_count, 50000);
}

TEST_F(EvCameraStatisticInfoTest, LargeValues) {
    EvCameraStatisticInfo info(UINT64_MAX, UINT64_MAX);
    EXPECT_EQ(info.bandwidth_byte, UINT64_MAX);
    EXPECT_EQ(info.events_count, UINT64_MAX);
}
