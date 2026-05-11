/**
 * @file test_camera_basic.cpp
 * @brief Unit tests for camera basic functionality
 */

#include <gtest/gtest.h>
#include <fluxeem/driver/camera/ev_camera_service.hpp>
#include <fluxeem/driver/camera/base/i_camera.hpp>
#include <fluxeem/base/define/event_type.h>
#include <fluxeem/base/logging/logger.h>

#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

using namespace fluxeem;
using namespace std::chrono_literals;

class CameraBasicTest : public ::testing::Test {
protected:
    std::unique_ptr<EvCameraService> manager_;
    CameraDevice camera_;
    bool camera_available_ = false;
    std::string test_serial_;

    void SetUp() override {
        manager_ = std::make_unique<EvCameraService>();
        int count = manager_->refresh();
        camera_available_ = (count > 0);

        if (camera_available_) {
            auto descs = manager_->listCameras();
            if (!descs.empty()) {
                test_serial_ = descs[0].serial;
                camera_ = manager_->open(test_serial_);
                if (!camera_) {
                    camera_available_ = false;
                }
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

    void SkipIfNoCamera() {
        if (!camera_available_) {
            GTEST_SKIP() << "No camera connected, skipping test";
        }
    }
};

TEST_F(CameraBasicTest, CameraManagerCreation) {
    ASSERT_NE(manager_, nullptr);
}

TEST_F(CameraBasicTest, CameraDiscovery) {
    int count = manager_->refresh();
    EXPECT_GE(count, 0);

    auto descs = manager_->listCameras();
    if (count > 0) {
        EXPECT_EQ(descs.size(), static_cast<size_t>(count));
        
        for (const auto& desc : descs) {
            EXPECT_FALSE(desc.serial.empty());
            EXPECT_FALSE(desc.product.empty());
        }
    }
}

TEST_F(CameraBasicTest, CameraOpen) {
    SkipIfNoCamera();
    
    ASSERT_NE(camera_, nullptr);
    EXPECT_TRUE(camera_->isConnected());
}

TEST_F(CameraBasicTest, CameraResolution) {
    SkipIfNoCamera();
    
    uint16_t width = camera_->getWidth();
    uint16_t height = camera_->getHeight();
    
    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
    EXPECT_LE(width, 4096);
    EXPECT_LE(height, 4096);
}

TEST_F(CameraBasicTest, CameraStartStop) {
    SkipIfNoCamera();
    
    int result = camera_->start();
    EXPECT_EQ(result, 0);
    
    std::this_thread::sleep_for(100ms);
    
    EXPECT_TRUE(camera_->isConnected());
    
    result = camera_->stop();
    EXPECT_EQ(result, 0);
}

TEST_F(CameraBasicTest, CameraEventCallback) {
    SkipIfNoCamera();
    
    std::atomic<uint64_t> event_count{0};
    std::atomic<bool> callback_called{false};
    
    uint32_t cb_id = camera_->registerEventBatchCallback(
        [&event_count, &callback_called](EventIterator_t begin, EventIterator_t end) {
            callback_called = true;
            event_count += static_cast<uint64_t>(std::distance(begin, end));
        });

    int result = camera_->start();
    EXPECT_EQ(result, 0);
    
    std::this_thread::sleep_for(1000ms);
    
    camera_->stop();
    
    camera_->unregisterEventBatchCallback(cb_id);
    
    EXPECT_TRUE(callback_called || event_count > 0);
}

TEST_F(CameraBasicTest, CameraMultipleCallbacks) {
    SkipIfNoCamera();
    
    std::atomic<int> callback1_count{0};
    std::atomic<int> callback2_count{0};
    
    uint32_t cb1 = camera_->registerEventBatchCallback(
        [&callback1_count](EventIterator_t begin, EventIterator_t end) {
            callback1_count++;
        });
    
    uint32_t cb2 = camera_->registerEventBatchCallback(
        [&callback2_count](EventIterator_t begin, EventIterator_t end) {
            callback2_count++;
        });
    
    EXPECT_NE(cb1, cb2);
    
    camera_->start();
    std::this_thread::sleep_for(300ms);
    camera_->stop();
    
    camera_->unregisterEventBatchCallback(cb1);
    camera_->unregisterEventBatchCallback(cb2);
}

TEST_F(CameraBasicTest, CameraInvalidSerial) {
    auto camera = manager_->open("invalid_serial_12345");
    EXPECT_EQ(camera, nullptr);
}

TEST_F(CameraBasicTest, CameraDoubleStart) {
    SkipIfNoCamera();
    
    int result1 = camera_->start();
    EXPECT_EQ(result1, 0);
    
    int result2 = camera_->start();
    
    camera_->stop();
}
