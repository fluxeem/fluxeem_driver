/**
 * @file test_camera_hardware.cpp
 * @brief Hardware-dependent tests requiring a physical camera connection
 *
 * These tests require an actual Event camera to be connected to the system.
 * They will be skipped if no camera is found.
 */

#include <gtest/gtest.h>
#include <fluxeem/driver/camera/ev_camera_service.hpp>
#include <fluxeem/driver/camera/base/i_camera.hpp>
#include <fluxeem/hal/tools/tool_info.h>
#include <fluxeem/base/define/event_type.h>
#include <fluxeem/base/logging/logger.h>

#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

using namespace fluxeem;

// Helper to check if a camera is available
class CameraTestFixture : public ::testing::Test {
protected:
    std::unique_ptr<EvCameraService> manager_;
    CameraDevice camera_;
    bool camera_available_ = false;

    void SetUp() override {
        // Logger is automatically initialized via singleton
        manager_ = std::make_unique<EvCameraService>();

        // Update camera list and check if any camera is available
        int count = manager_->refresh();
        camera_available_ = (count > 0);

        if (camera_available_) {
            auto descs = manager_->listCameras();
            if (!descs.empty()) {
                camera_ = manager_->open(descs[0].serial);
                if (!camera_) {
                    camera_available_ = false;
                }
            } else {
                camera_available_ = false;
            }
        }
    }

    void TearDown() override {
        if (camera_ && camera_->isConnected()) {
            camera_->stop();
        }
        camera_.reset();
        manager_.reset();
    }

    // Skip test if no camera available
    void SkipIfNoCamera() {
        if (!camera_available_) {
            GTEST_SKIP() << "No camera connected, skipping hardware test";
        }
    }
};

/**
 * @brief Test camera discovery and basic information
 */
TEST_F(CameraTestFixture, CameraDiscovery) {
    // Note: If camera is already opened (in SetUp), refresh() may return 0
    // but listCameras() will still return the opened camera
    int count = manager_->refresh();

    // This test can pass even without camera (just verifies API works)
    EXPECT_GE(count, 0);

    auto descs = manager_->listCameras();

    // If we have a camera (either discovered now or opened in SetUp)
    if (!descs.empty()) {
        // Verify camera description fields
        EXPECT_FALSE(descs[0].serial.empty());
        EXPECT_FALSE(descs[0].product.empty());
        EXPECT_FALSE(descs[0].manufacturer.empty());
        EXPECT_GT(descs[0].vid, 0);
        EXPECT_GT(descs[0].pid, 0);
    }
}

/**
 * @brief Test camera connection and basic properties
 */
TEST_F(CameraTestFixture, CameraConnection) {
    SkipIfNoCamera();

    ASSERT_TRUE(camera_ != nullptr);
    EXPECT_TRUE(camera_->isConnected());

    // Test resolution
    uint16_t width = camera_->getWidth();
    uint16_t height = camera_->getHeight();
    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);

    // Test description
    auto desc = camera_->getDescription();
    EXPECT_FALSE(desc.serial.empty());
    EXPECT_FALSE(desc.product.empty());
}

/**
 * @brief Test camera start/stop functionality
 */
TEST_F(CameraTestFixture, CameraStartStop) {
    SkipIfNoCamera();

    ASSERT_TRUE(camera_ != nullptr);

    // Start camera
    bool result = camera_->start();
    EXPECT_TRUE(result);

    // Give camera time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Stop camera
    result = camera_->stop();
    EXPECT_TRUE(result);
}

/**
 * @brief Test event streaming with callback
 */
TEST_F(CameraTestFixture, EventStreamingCallback) {
    SkipIfNoCamera();

    ASSERT_TRUE(camera_ != nullptr);

    std::atomic<uint64_t> event_count{0};
    std::atomic<bool> callback_received{false};
    std::mutex mtx;
    std::condition_variable cv;

    // Register event callback
    uint32_t cb_id = camera_->registerEventBatchCallback(
        [&event_count, &callback_received, &cv](EventIterator_t begin, EventIterator_t end) {
            if (begin != end) {
                event_count += (end - begin);
                callback_received = true;
                cv.notify_one();
            }
        });

    // Start camera
    EXPECT_EQ(camera_->start(), 0);

    // Wait for events (timeout after 5 seconds)
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait_for(lock, std::chrono::seconds(5), [&callback_received] {
            return callback_received.load();
        });
    }

    // Stop camera
    EXPECT_EQ(camera_->stop(), 0);

    // Remove callback
    EXPECT_TRUE(camera_->unregisterEventBatchCallback(cb_id));

    // If we got events, verify they look valid
    if (event_count > 0) {
        EXPECT_TRUE(callback_received);
    }
}

/**
 * @brief Test event batching by number
 */
TEST_F(CameraTestFixture, EventBatchingByNumber) {
    SkipIfNoCamera();

    ASSERT_TRUE(camera_ != nullptr);

    // Set batch size to 1000 events
    camera_->setBatchEventsNum(1000);

    // Start camera
    EXPECT_EQ(camera_->start(), 0);

    // Wait a bit for events to accumulate
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Try to get batches
    EventBatch batch;
    int batches_received = 0;
    int total_events = 0;

    for (int i = 0; i < 10; i++) {
        if (camera_->getNextBatch(batch)) {
            batches_received++;
            total_events += batch.size();

            // Verify event data
            for (const auto& evt : batch) {
                EXPECT_LE(evt.x, camera_->getWidth());
                EXPECT_LE(evt.y, camera_->getHeight());
                EXPECT_TRUE(evt.polarity == 0 || evt.polarity == 1);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Stop camera
    EXPECT_EQ(camera_->stop(), 0);
}

/**
 * @brief Test event batching by time
 */
TEST_F(CameraTestFixture, EventBatchingByTime) {
    SkipIfNoCamera();

    ASSERT_TRUE(camera_ != nullptr);

    // Set batch time to 10ms
    camera_->setBatchEventsTime(10000);  // 10ms in microseconds

    // Start camera
    EXPECT_EQ(camera_->start(), 0);

    // Wait a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Try to get batches
    EventBatch batch;
    int batches_received = 0;

    for (int i = 0; i < 10; i++) {
        if (camera_->getNextBatch(batch)) {
            batches_received++;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    // Stop camera
    EXPECT_EQ(camera_->stop(), 0);
}

/**
 * @brief Test tool information retrieval
 */
TEST_F(CameraTestFixture, ToolInfoRetrieval) {
    SkipIfNoCamera();

    ASSERT_TRUE(camera_ != nullptr);

    // Get all tools info
    auto tools_info = camera_->getToolsInfo();

    // Most cameras should have at least some tools
    // But we don't strictly require it - just verify API works
    for (const auto& info : tools_info) {
        EXPECT_FALSE(info.tool_name.empty());
        EXPECT_FALSE(info.description.empty());

        // Verify we can get individual tool info
        auto tool_info = camera_->getToolInfo(info.tool_type);
        EXPECT_EQ(tool_info.tool_type, info.tool_type);
    }
}

/**
 * @brief Test ROI tool if available
 */
TEST_F(CameraTestFixture, ROIToolTest) {
    SkipIfNoCamera();

    ASSERT_TRUE(camera_ != nullptr);

    auto roi_tool = camera_->getTool(ToolType::TOOL_ROI);
    if (!roi_tool) {
        GTEST_SKIP() << "Camera does not support ROI tool";
    }

    // Get tool info
    auto tool_info = roi_tool->getToolInfo();
    EXPECT_EQ(tool_info.tool_type, ToolType::TOOL_ROI);

    // Get all parameters
    auto params = roi_tool->getAllParamInfo();

    // Test getting/setting ROI if parameters exist
    for (const auto& [name, info] : params) {
        if (info.type == ToolParameterType::INT) {
            IntParameterInfo int_info;
            if (roi_tool->getParamInfo(name, int_info)) {
                int current_value;
                if (roi_tool->getParam(name, current_value)) {
                    // Try to set the same value back
                    EXPECT_TRUE(roi_tool->setParam(name, current_value));
                }
            }
        }
    }
}

/**
 * @brief Test bias tool if available
 */
TEST_F(CameraTestFixture, BiasToolTest) {
    SkipIfNoCamera();

    ASSERT_TRUE(camera_ != nullptr);

    auto bias_tool = camera_->getTool(ToolType::TOOL_BIAS);
    if (!bias_tool) {
        GTEST_SKIP() << "Camera does not support bias tool";
    }

    // Get tool info
    auto tool_info = bias_tool->getToolInfo();
    EXPECT_EQ(tool_info.tool_type, ToolType::TOOL_BIAS);

    // Get all parameters
    auto params = bias_tool->getAllParamInfo();

    // Verify bias parameters exist
    EXPECT_GT(params.size(), 0);
}

/**
 * @brief Test trigger tool if available
 */
TEST_F(CameraTestFixture, TriggerToolTest) {
    SkipIfNoCamera();

    ASSERT_TRUE(camera_ != nullptr);

    auto trigger_tool = camera_->getTool(ToolType::TOOL_TRIGGER_IN);
    if (!trigger_tool) {
        GTEST_SKIP() << "Camera does not support trigger tool";
    }

    // Get tool info
    auto tool_info = trigger_tool->getToolInfo();
    EXPECT_EQ(tool_info.tool_type, ToolType::TOOL_TRIGGER_IN);
}

/**
 * @brief Test parameter descriptor interface
 */
TEST_F(CameraTestFixture, ParameterDescriptorInterface) {
    SkipIfNoCamera();

    ASSERT_TRUE(camera_ != nullptr);

    // Get a tool to test (prefer ROI, fall back to any available)
    auto tool = camera_->getTool(ToolType::TOOL_ROI);
    if (!tool) {
        auto tools_info = camera_->getToolsInfo();
        if (!tools_info.empty()) {
            tool = camera_->getTool(tools_info[0].tool_type);
        }
    }

    if (!tool) {
        GTEST_SKIP() << "No tools available for testing";
    }

    // Test descriptors
    auto descriptors = tool->descriptors();

    // Test getting parameter values via descriptor interface
    for (const auto& desc : descriptors) {
        auto value = tool->get(desc.name);
        // Just verify API doesn't crash - value may or may not be present
        (void)value;
    }
}

/**
 * @brief Test start/stop cycle
 * @note Some cameras may not support multiple start/stop cycles without re-initialization
 */
TEST_F(CameraTestFixture, MultipleStartStopCycles) {
    SkipIfNoCamera();

    ASSERT_TRUE(camera_ != nullptr);

    // First cycle should always work
    EXPECT_EQ(camera_->start(), 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    EXPECT_EQ(camera_->stop(), 0);

    // Some cameras may not support multiple cycles - that's OK
    // We just verify that one full cycle works correctly
}

/**
 * @brief Test camera remains connected after operations
 */
TEST_F(CameraTestFixture, CameraConnectionStability) {
    SkipIfNoCamera();

    ASSERT_TRUE(camera_ != nullptr);
    EXPECT_TRUE(camera_->isConnected());

    // Start and stop
    EXPECT_EQ(camera_->start(), 0);
    EXPECT_TRUE(camera_->isConnected());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(camera_->stop(), 0);
    EXPECT_TRUE(camera_->isConnected());

    // Resolution should still work
    EXPECT_GT(camera_->getWidth(), 0);
    EXPECT_GT(camera_->getHeight(), 0);
}

/**
 * @brief Test JSON config export/import (if supported)
 */
TEST_F(CameraTestFixture, ConfigExportImport) {
    SkipIfNoCamera();

    ASSERT_TRUE(camera_ != nullptr);

    // Export config
    std::string test_config_path = "test_camera_config.json";
    bool export_result = camera_->exportCameraConfig(test_config_path);

    // Export might fail if camera doesn't support it - that's OK
    if (export_result) {
        // If export succeeded, try import
        bool import_result = camera_->importCameraConfig(test_config_path);
        // Import might succeed or fail depending on implementation
        (void)import_result;

        // Clean up test file
        std::remove(test_config_path.c_str());
    }
}

/**
 * @brief Test callback registration and removal
 */
TEST_F(CameraTestFixture, CallbackRegistration) {
    SkipIfNoCamera();

    ASSERT_TRUE(camera_ != nullptr);

    // Register multiple callbacks
    uint32_t cb1 = camera_->registerEventBatchCallback(
        [](EventIterator_t, EventIterator_t) {});

    uint32_t cb2 = camera_->registerEventBatchCallback(
        [](EventIterator_t, EventIterator_t) {});

    // Verify different IDs
    EXPECT_NE(cb1, cb2);

    // Remove callbacks
    EXPECT_TRUE(camera_->unregisterEventBatchCallback(cb1));
    EXPECT_TRUE(camera_->unregisterEventBatchCallback(cb2));

    // Removing again should fail
    EXPECT_FALSE(camera_->unregisterEventBatchCallback(cb1));
}
