// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
/**
 * @file test_camera_tools.cpp
 * @brief Tests for camera tools and parameter info
 */

#include <gtest/gtest.h>
#include <fluxeem/hal/tools/tool_info.h>
#include <fluxeem/base/define/camera_types.h>

using namespace fluxeem;

class ToolInfoTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ToolInfoTest, ToolTypeToString) {
    EXPECT_NO_THROW({
        std::string bias_str = to_string(ToolType::TOOL_BIAS);
        std::string trigger_str = to_string(ToolType::TOOL_TRIGGER_IN);
        std::string roi_str = to_string(ToolType::TOOL_ROI);

        EXPECT_FALSE(bias_str.empty());
        EXPECT_FALSE(trigger_str.empty());
        EXPECT_FALSE(roi_str.empty());
    });
}

TEST_F(ToolInfoTest, ToolParameterTypeToString) {
    EXPECT_EQ(ToolParameterTypeToString(ToolParameterType::INT), "INT");
    EXPECT_EQ(ToolParameterTypeToString(ToolParameterType::FLOAT), "FLOAT");
    EXPECT_EQ(ToolParameterTypeToString(ToolParameterType::BOOL), "BOOL");
    EXPECT_EQ(ToolParameterTypeToString(ToolParameterType::STRING), "STRING");
    EXPECT_EQ(ToolParameterTypeToString(ToolParameterType::ENUM), "ENUM");
}

TEST_F(ToolInfoTest, BasicParameterInfoStruct) {
    BasicParameterInfo info;
    info.name = "test_param";
    info.description = "A test parameter";
    info.type = ToolParameterType::INT;

    std::string str = info.toString();
    EXPECT_NE(str.find("test_param"), std::string::npos);
    EXPECT_NE(str.find("A test parameter"), std::string::npos);
    EXPECT_NE(str.find("INT"), std::string::npos);
}

TEST_F(ToolInfoTest, IntParameterInfoStruct) {
    IntParameterInfo info;
    info.min = 0;
    info.max = 100;
    info.default_value = 50;
    info.unit = "ms";

    EXPECT_EQ(info.min, 0);
    EXPECT_EQ(info.max, 100);
    EXPECT_EQ(info.default_value, 50);
    EXPECT_EQ(info.unit, "ms");

    // Test constraintValue
    EXPECT_EQ(info.constraintValue(-10), 0);    // Below min
    EXPECT_EQ(info.constraintValue(150), 100);  // Above max
    EXPECT_EQ(info.constraintValue(75), 75);    // Within range
}

TEST_F(ToolInfoTest, FloatParameterInfoStruct) {
    FloatParameterInfo info;
    info.min = 0.0f;
    info.max = 1.0f;
    info.default_value = 0.5f;
    info.unit = "ratio";

    EXPECT_FLOAT_EQ(info.min, 0.0f);
    EXPECT_FLOAT_EQ(info.max, 1.0f);
    EXPECT_FLOAT_EQ(info.default_value, 0.5f);
    EXPECT_EQ(info.unit, "ratio");

    // Test constraintValue
    EXPECT_FLOAT_EQ(info.constraintValue(-0.5f), 0.0f);   // Below min
    EXPECT_FLOAT_EQ(info.constraintValue(1.5f), 1.0f);    // Above max
    EXPECT_FLOAT_EQ(info.constraintValue(0.75f), 0.75f);  // Within range
}

TEST_F(ToolInfoTest, BoolParameterInfoStruct) {
    BoolParameterInfo info;
    info.default_value = true;

    std::string str = info.toString();
    EXPECT_NE(str.find("1"), std::string::npos);  // true -> "1"
}

TEST_F(ToolInfoTest, EnumParameterInfoStruct) {
    EnumParameterInfo info;
    info.options = {"option1", "option2", "option3"};
    info.default_value = "option2";

    EXPECT_EQ(info.options.size(), 3);
    EXPECT_EQ(info.options[0], "option1");
    EXPECT_EQ(info.options[1], "option2");
    EXPECT_EQ(info.options[2], "option3");
    EXPECT_EQ(info.default_value, "option2");
}

TEST_F(ToolInfoTest, StringParameterInfoStruct) {
    StringParameterInfo info;
    info.default_value = "default_string";

    std::string str = info.toString();
    EXPECT_NE(str.find("default_string"), std::string::npos);
}

class CameraTypesTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(CameraTypesTest, CameraDescriptionStruct) {
    CameraDescription desc;
    desc.serial = "EVK4-001";
    desc.product = "EVK4";
    desc.manufacturer = "Prophesee";
    desc.vid = 0x04B4;
    desc.pid = 0x00F4;
    desc.interface_type = InterfaceType::USB;
    desc.firmware_version = "2.1.0";

    EXPECT_EQ(desc.serial, "EVK4-001");
    EXPECT_EQ(desc.product, "EVK4");
    EXPECT_EQ(desc.manufacturer, "Prophesee");
    EXPECT_EQ(desc.vid, 0x04B4);
    EXPECT_EQ(desc.pid, 0x00F4);
    EXPECT_EQ(desc.interface_type, InterfaceType::USB);
    EXPECT_EQ(desc.firmware_version, "2.1.0");
}

TEST_F(CameraTypesTest, InterfaceTypeEnum) {
    EXPECT_EQ(static_cast<int>(InterfaceType::USB), 0);
    EXPECT_EQ(static_cast<int>(InterfaceType::MIPI), 1);
}

TEST_F(CameraTypesTest, BatchConditionTypeEnum) {
    EXPECT_EQ(static_cast<int>(BatchConditionType::NO_CONDITION), 0);
    EXPECT_EQ(static_cast<int>(BatchConditionType::N_EVENTS), 1);
    EXPECT_EQ(static_cast<int>(BatchConditionType::N_US), 2);
}
