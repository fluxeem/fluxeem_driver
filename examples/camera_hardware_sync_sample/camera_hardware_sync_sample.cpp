// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
/**
 * @file camera_hardware_sync_sample.cpp
 * @brief 简化版多相机硬件同步示例
 * 
 * 使用方法：
 *   camera_hardware_sync_sample <mode> <serial_number>
 *   
 *   mode: MASTER / SLAVE / STANDALONE
 *   serial_number: 相机序列号
 * 
 * 示例：
 *   camera_hardware_sync_sample MASTER 00000000
 *   camera_hardware_sync_sample SLAVE 11111111
 *   camera_hardware_sync_sample STANDALONE 22222222
 */

#include <fluxeem/driver/camera/ev_camera_service.hpp>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <string>
#include <opencv2/opencv.hpp>

using namespace std::chrono_literals;

// 双缓冲显示
struct DisplayBuffer {
    cv::Mat back_buf;
    cv::Mat front_buf;
    std::mutex mutex;
    uint64_t window_start_ts = 0;
    bool initialized = false;
};

constexpr uint64_t kDisplayWindowUs = 33'000; // 30 FPS

int main(int argc, char* argv[])
{
    // 检查参数
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <mode> <serial_number>" << std::endl;
        std::cout << "  mode: MASTER / SLAVE / STANDALONE" << std::endl;
        std::cout << "  serial_number: camera serial number" << std::endl;
        std::cout << "\nExample:" << std::endl;
        std::cout << "  " << argv[0] << " MASTER 00000000" << std::endl;
        std::cout << "  " << argv[0] << " SLAVE 11111111" << std::endl;
        return 1;
    }

    std::string mode = argv[1];
    std::string serial = argv[2];

    // 验证模式
    if (mode != "MASTER" && mode != "SLAVE" && mode != "STANDALONE") {
        std::cerr << "Error: Invalid mode '" << mode << "'" << std::endl;
        std::cerr << "Valid modes: MASTER, SLAVE, STANDALONE" << std::endl;
        return 1;
    }

    try {
        // 初始化相机管理器
        fluxeem::EvCameraService manager;
        manager.refresh();
        auto descs = manager.listCameras();

        if (descs.empty()) {
            std::cerr << "Error: No camera found" << std::endl;
            return 1;
        }

        // 查找指定序列号的相机
        bool found = false;
        for (const auto& d : descs) {
            if (d.serial == serial) {
                found = true;
                break;
            }
        }

        if (!found) {
            std::cerr << "Error: Camera with serial '" << serial << "' not found" << std::endl;
            std::cout << "Available cameras:" << std::endl;
            for (const auto& d : descs) {
                std::cout << "  - " << d.serial << std::endl;
            }
            return 1;
        }

        // 打开相机
        std::cout << "Opening camera: " << serial << std::endl;
        auto camera = manager.open(serial);
        if (!camera) {
            std::cerr << "Error: Failed to open camera" << std::endl;
            return 1;
        }

        uint16_t width = camera->getWidth();
        uint16_t height = camera->getHeight();
        std::cout << "Camera resolution: " << width << "x" << height << std::endl;

        // 设置同步模式
        auto sync_tool = camera->getTool(fluxeem::ToolType::TOOL_SYNC);
        if (sync_tool) {
            if (sync_tool->setParam("mode", mode)) {
                std::cout << "Sync mode set to: " << mode << std::endl;
            } else {
                std::cerr << "Warning: Failed to set sync mode" << std::endl;
            }
        } else {
            std::cerr << "Warning: TOOL_SYNC not available" << std::endl;
        }

        // 创建显示缓冲
        DisplayBuffer display;
        display.back_buf = cv::Mat(height, width, CV_8UC3, cv::Scalar(20, 20, 20));
        display.front_buf = cv::Mat(height, width, CV_8UC3, cv::Scalar(20, 20, 20));

        // 统计信息（由统计回调线程写入，主循环读取）
        struct StatisticsInfo {
            std::atomic<uint64_t> bandwidth_bytes{0};
            std::atomic<uint64_t> events_count{0};
        } statistics;

        camera->setStatisticsCallback(
            [&statistics](const fluxeem::EvCameraStatisticInfo info) {
                statistics.bandwidth_bytes.store(info.bandwidth_byte, std::memory_order_relaxed);
                statistics.events_count.store(info.events_count, std::memory_order_relaxed);
            });

        // 注册事件回调
        uint32_t cb_id = camera->registerEventBatchCallback(
            [&display, width, height](fluxeem::EventIterator_t begin, fluxeem::EventIterator_t end) {
                if (begin == end) return;

                if (!display.initialized) {
                    display.window_start_ts = begin->timestamp;
                    display.initialized = true;
                }

                // 绘制事件
                for (auto it = begin; it != end; ++it) {
                    if (it->timestamp < display.window_start_ts) {
                        display.window_start_ts = it->timestamp;
                        display.back_buf.setTo(cv::Scalar(20, 20, 20));
                    }

                    if (it->x < width && it->y < height) {
                        display.back_buf.at<cv::Vec3b>(it->y, it->x) =
                            (it->polarity == 1)
                                ? cv::Vec3b(0, 255, 0)   // 绿色：正极性
                                : cv::Vec3b(0, 0, 255);  // 红色：负极性
                    }
                }

                uint64_t last_ts = (end - 1)->timestamp;
                uint64_t time_span = last_ts - display.window_start_ts;

                if (time_span >= kDisplayWindowUs) {
                    std::lock_guard<std::mutex> lock(display.mutex);
                    std::swap(display.back_buf, display.front_buf);
                    display.back_buf.setTo(cv::Scalar(20, 20, 20));
                    display.window_start_ts = last_ts;
                }
            });

        // 启动相机
        std::cout << "Starting camera..." << std::endl;
        if (!camera->start()) {
            std::cerr << "Error: Failed to start camera" << std::endl;
            return 1;
        }

        std::cout << "\n========================================" << std::endl;
        std::cout << "Camera started successfully!" << std::endl;
        std::cout << "Mode: " << mode << std::endl;
        std::cout << "Serial: " << serial << std::endl;
        std::cout << "Press SPACE to start/stop recording" << std::endl;
        std::cout << "Press 'q' or ESC to quit" << std::endl;
        std::cout << "========================================\n" << std::endl;

        // 创建显示窗口
        std::string window_name = "Event Camera - " + mode + " [" + serial + "]";
        cv::namedWindow(window_name, cv::WINDOW_NORMAL);
        cv::resizeWindow(window_name, 640, 480);

        // 主循环
        std::atomic<bool> running{true};
        bool is_recording = false;
        while (running && camera->isConnected()) {
            // 获取显示帧
            cv::Mat frame;
            {
                std::lock_guard<std::mutex> lock(display.mutex);
                if (!display.front_buf.empty()) {
                    frame = display.front_buf.clone();
                }
            }

            if (frame.empty()) {
                frame = cv::Mat(height, width, CV_8UC3, cv::Scalar(20, 20, 20));
            }

            // 添加 HUD
            cv::Scalar mode_color;
            if (mode == "MASTER")       mode_color = cv::Scalar(0, 200, 255);   // 橙黄
            else if (mode == "SLAVE")   mode_color = cv::Scalar(255, 180, 0);   // 蓝紫
            else                        mode_color = cv::Scalar(180, 180, 180); // 灰白

            cv::putText(frame, mode, cv::Point(10, 30), 
                       cv::FONT_HERSHEY_SIMPLEX, 0.8, mode_color, 2);
            cv::putText(frame, "S/N: " + serial, cv::Point(10, 55), 
                       cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(200, 200, 200), 1);

            const uint64_t bw = statistics.bandwidth_bytes.load(std::memory_order_relaxed);
            const uint64_t ev = statistics.events_count.load(std::memory_order_relaxed);
            char stat_text[128];
            std::snprintf(stat_text,
                          sizeof(stat_text),
                          "%.2f MB/s  |  %.2f Mev/s",
                          static_cast<double>(bw) / 1024.0 / 1024.0,
                          static_cast<double>(ev) / 1000000.0);
            cv::putText(frame,
                        stat_text,
                        cv::Point(10, 80),
                        cv::FONT_HERSHEY_SIMPLEX,
                        0.5,
                        cv::Scalar(0, 255, 0),
                        1);

            if (is_recording) {
                cv::circle(frame, cv::Point(frame.cols - 20, 20), 8,
                           cv::Scalar(0, 0, 255), -1);
                cv::putText(frame, "REC",
                            cv::Point(frame.cols - 65, 26),
                            cv::FONT_HERSHEY_SIMPLEX, 0.5,
                            cv::Scalar(0, 0, 255), 1);
            }

            // 显示
            cv::imshow(window_name, frame);

            // 按键处理
            int key = cv::waitKey(10);
            if (key == 'q' || key == 27) {
                running = false;
            } else if (key == ' ') {
                if (!is_recording) {
                    auto now = std::chrono::system_clock::now();
                    auto time_t_now = std::chrono::system_clock::to_time_t(now);
                    std::tm tm_now{};
#ifdef _WIN32
                    localtime_s(&tm_now, &time_t_now);
#else
                    localtime_r(&time_t_now, &tm_now);
#endif
                    char time_buf[64];
                    std::strftime(time_buf, sizeof(time_buf), "%Y%m%d_%H%M%S", &tm_now);
                    std::string file_path = std::string("recording_") + mode + "_" + serial + "_" + time_buf + ".raw";

                    bool ret = camera->startRecording(file_path);
                    if (ret) {
                        is_recording = true;
                        std::cout << "Recording started: " << file_path << std::endl;
                    } else {
                        std::cerr << "Failed to start recording." << std::endl;
                    }
                } else {
                    camera->stopRecording();
                    is_recording = false;
                    std::cout << "Recording stopped." << std::endl;
                }
            }
        }

        // 停止相机
        std::cout << "\nStopping camera..." << std::endl;
        cv::destroyAllWindows();
        camera->unregisterEventBatchCallback(cb_id);

        if (is_recording) {
            camera->stopRecording();
            std::cout << "Recording stopped on exit." << std::endl;
        }

        camera->stop();

        // 恢复为 STANDALONE 模式
        if (sync_tool && mode != "STANDALONE") {
            sync_tool->setParam("mode", "STANDALONE");
            std::cout << "Camera reset to STANDALONE mode" << std::endl;
        }

        std::cout << "Done!" << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 100;
    }
}
