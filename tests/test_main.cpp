// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
/**
 * @file test_main.cpp
 * @brief Main entry point for all tests
 */

#include <gtest/gtest.h>

int main(int argc, char **argv) {
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);

    // 设置测试失败时的行为:继续执行后续测试
    return RUN_ALL_TESTS();
}
