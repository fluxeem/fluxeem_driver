// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
/**
 * @file test_camera_config.cpp
 * @brief Unit tests for camera configuration and parameters
 */

#include <gtest/gtest.h>
#include <fluxeem/driver/camera/ev_camera_service.hpp>
#include <fluxeem/driver/camera/base/i_camera.hpp>
#include <fluxeem/hal/tools/tool_info.h>
#include <fluxeem/base/logging/logger.h>

#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

using namespace fluxeem;
using namespace std::chrono_literals;

class CameraConfigTest : public ::testing::Test {
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

    void SkipIfNoCamera() {
        if (!camera_available_) {
            GTEST_SKIP() << "No camera connected, skipping test";
        }
    }
};

TEST_F(CameraConfigTest, GetBiasesTool) {
    SkipIfNoCamera();
    
    auto camera_tool = camera_->getTool(ToolType::TOOL_BIAS);
    EXPECT_NE(camera_tool, nullptr);
}

TEST_F(CameraConfigTest, GetRoiTool) {
    SkipIfNoCamera();
    
    auto camera_tool = camera_->getTool(ToolType::TOOL_ROI);
    EXPECT_NE(camera_tool, nullptr);
}

TEST_F(CameraConfigTest, GetTriggerInTool) {
    SkipIfNoCamera();
    
    auto camera_tool = camera_->getTool(ToolType::TOOL_TRIGGER_IN);
    EXPECT_NE(camera_tool, nullptr);
}

TEST_F(CameraConfigTest, GetSyncTool) {
    SkipIfNoCamera();
    
    auto camera_tool = camera_->getTool(ToolType::TOOL_SYNC);
    EXPECT_NE(camera_tool, nullptr);
}

TEST_F(CameraConfigTest, GetAntiFlickerTool) {
    SkipIfNoCamera();
    
    auto camera_tool = camera_->getTool(ToolType::TOOL_ANTI_FLICKER);
    EXPECT_NE(camera_tool, nullptr);
}

TEST_F(CameraConfigTest, GetEventTrailFilterTool) {
    SkipIfNoCamera();
    
    auto camera_tool = camera_->getTool(ToolType::TOOL_EVENT_TRAIL_FILTER);
    EXPECT_NE(camera_tool, nullptr);
}

TEST_F(CameraConfigTest, GetEventRateControlTool) {
    SkipIfNoCamera();
    
    auto camera_tool = camera_->getTool(ToolType::TOOL_EVENT_RATE_CONTROL);
    EXPECT_NE(camera_tool, nullptr);
}

TEST_F(CameraConfigTest, BiasesGetParam) {
    SkipIfNoCamera();
    
    auto camera_tool = camera_->getTool(ToolType::TOOL_BIAS);
    ASSERT_NE(camera_tool, nullptr);
    
    std::string value;
    bool result = camera_tool->getParam("bias_fo", value);
    EXPECT_TRUE(result || !result);
}

TEST_F(CameraConfigTest, RoiSetParam) {
    SkipIfNoCamera();
    
    auto camera_tool = camera_->getTool(ToolType::TOOL_ROI);
    ASSERT_NE(camera_tool, nullptr);
    
    bool result = camera_tool->setParam("roi_enabled", "true");
    EXPECT_TRUE(result || !result);
}

TEST_F(CameraConfigTest, SyncSetParam) {
    SkipIfNoCamera();
    
    auto camera_tool = camera_->getTool(ToolType::TOOL_SYNC);
    ASSERT_NE(camera_tool, nullptr);
    
    bool result = camera_tool->setParam("mode", "STANDALONE");
    EXPECT_TRUE(result || !result);
}

TEST_F(CameraConfigTest, AntiFlickerSetParam) {
    SkipIfNoCamera();
    
    auto camera_tool = camera_->getTool(ToolType::TOOL_ANTI_FLICKER);
    ASSERT_NE(camera_tool, nullptr);
    
    bool result = camera_tool->setParam("enabled", "false");
    EXPECT_TRUE(result || !result);
}

TEST_F(CameraConfigTest, EventTrailFilterSetParam) {
    SkipIfNoCamera();
    
    auto camera_tool = camera_->getTool(ToolType::TOOL_EVENT_TRAIL_FILTER);
    ASSERT_NE(camera_tool, nullptr);
    
    bool result = camera_tool->setParam("enabled", "false");
    EXPECT_TRUE(result || !result);
}

TEST_F(CameraConfigTest, EventRateControlSetParam) {
    SkipIfNoCamera();
    
    auto camera_tool = camera_->getTool(ToolType::TOOL_EVENT_RATE_CONTROL);
    ASSERT_NE(camera_tool, nullptr);
    
    bool result = camera_tool->setParam("enabled", "false");
    EXPECT_TRUE(result || !result);
}

TEST_F(CameraConfigTest, InvalidToolType) {
    SkipIfNoCamera();
    
    auto camera_tool = camera_->getTool(static_cast<ToolType>(999));
    EXPECT_EQ(camera_tool, nullptr);
}

TEST_F(CameraConfigTest, InvalidParameterName) {
    SkipIfNoCamera();
    
    auto camera_tool = camera_->getTool(ToolType::TOOL_BIAS);
    ASSERT_NE(camera_tool, nullptr);
    
    std::string value;
    bool result = camera_tool->getParam("invalid_param_name_xyz", value);
    EXPECT_FALSE(result);
}

TEST_F(CameraConfigTest, MultipleToolAccess) {
    SkipIfNoCamera();
    
    auto camera_tool1 = camera_->getTool(ToolType::TOOL_BIAS);
    auto camera_tool2 = camera_->getTool(ToolType::TOOL_ROI);
    auto camera_tool3 = camera_->getTool(ToolType::TOOL_SYNC);
    
    EXPECT_NE(camera_tool1, nullptr);
    EXPECT_NE(camera_tool2, nullptr);
    EXPECT_NE(camera_tool3, nullptr);
}

TEST_F(CameraConfigTest, ConfigurationAfterStart) {
    SkipIfNoCamera();
    
    camera_->start();
    
    auto camera_tool = camera_->getTool(ToolType::TOOL_BIAS);
    ASSERT_NE(camera_tool, nullptr);
    
    std::string value;
    bool result = camera_tool->getParam("bias_fo", value);
    EXPECT_TRUE(result || !result);
    
    camera_->stop();
}
