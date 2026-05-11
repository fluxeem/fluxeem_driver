#include <fluxeem/driver/camera/ev_camera_service.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <string>

#include <opencv2/opencv.hpp>

// 显示帧结构 - 回调生成图像，主循环只负责显示
struct DisplayFrame {
    cv::Mat image;
    std::mutex mutex;
};

int main(int argc, char* argv[])
{
    try
    {
        fluxeem::EvCameraService camera_manager;

        const auto descs = camera_manager.listCameras();

        if (descs.empty())
        {
            LOG_WARN("No camera found. Please plug in a camera and try again.");
            return 1;
        }

        LOG_INFO("Found cameras:");
        for (const auto& d : descs)
        {
            LOG_INFO("Product: %s  Serial: %s  Firmware: %s", d.product.c_str(), d.serial.c_str(), d.firmware_version.empty() ? "(unknown)" : d.firmware_version.c_str());
        }

        // 选择要打开的相机：
        // - 优先使用命令行参数 argv[1]
        // - 否则打开第一个发现的相机
        std::string serial_to_open = descs.front().serial;

        LOG_INFO("Opening camera serial=%s", serial_to_open.c_str());
        auto cam = camera_manager.open(serial_to_open);
        if (!cam)
        {
            LOG_ERROR("Open camera failed.");
            return 2;
        }

        // 获取相机分辨率
        const uint16_t width = cam->getWidth();
        const uint16_t height = cam->getHeight();
        LOG_INFO("Camera resolution: %d x %d", width, height);

        // 创建显示窗口
        const std::string window_name = "Fluxeem Live Viewer";
        cv::namedWindow(window_name, cv::WINDOW_NORMAL);
        cv::resizeWindow(window_name, width, height);

        // 显示帧 - 回调生成图像，主循环只负责显示
        DisplayFrame display_frame;
        std::atomic<bool> running{true};
        bool is_recording = false; 

        // 统计信息（由统计回调线程写入，主循环读取）
        struct StatisticsInfo {
            std::atomic<uint64_t> bandwidth_bytes{0};   // 字节/秒
            std::atomic<uint64_t> events_count{0};      // 事件数/秒
        } statistics;

        // 注册统计信息回调
        cam->setStatisticsCallback(
            [&statistics](const fluxeem::EvCameraStatisticInfo info) {
                statistics.bandwidth_bytes.store(info.bandwidth_byte, std::memory_order_relaxed);
                statistics.events_count.store(info.events_count, std::memory_order_relaxed);
            });

        // 显示参数：每 33ms (约30 FPS) 刷新一次
        constexpr uint64_t display_time_window_us = 33000;  // 微秒

        // 注册事件回调：按时间窗直接在画布上累积事件，避免大向量反复分配
        cv::Mat accumulation(height, width, CV_8UC3, cv::Scalar(0, 0, 0));
        uint64_t first_timestamp = 0;
        bool window_initialized = false;

        const uint32_t cb_id = cam->registerEventBatchCallback(
            [&display_frame,
             &accumulation,
             &window_initialized,
             &first_timestamp,
             width,
             height,
             display_time_window_us](fluxeem::EventIterator_t begin, fluxeem::EventIterator_t end)
            {
                if (begin == end) {
                    return;
                }

                // 初始化时间窗起点
                if (!window_initialized) {
                    first_timestamp = begin->timestamp;
                    window_initialized = true;
                }

                // 把事件直接绘制到累积画布
                for (auto it = begin; it != end; ++it) {
                    // 处理可能出现的时间戳回跳（如设备重启/时基复位）
                    if (it->timestamp < first_timestamp) {
                        first_timestamp = it->timestamp;
                        accumulation.setTo(cv::Scalar(0, 0, 0));
                    }

                    if (it->x < width && it->y < height) {
                        if (it->polarity == 1) {
                            accumulation.at<cv::Vec3b>(it->y, it->x) = cv::Vec3b(0, 255, 0);  // 绿色
                        } else {
                            accumulation.at<cv::Vec3b>(it->y, it->x) = cv::Vec3b(0, 0, 255);  // 红色
                        }
                    }
                }

                const uint64_t last_timestamp = (end - 1)->timestamp;
                const uint64_t time_span = last_timestamp - first_timestamp;

                if (time_span >= display_time_window_us) {
                    cv::Mat image = accumulation.clone();

                    // 更新显示帧
                    {
                        std::lock_guard<std::mutex> lock(display_frame.mutex);
                        display_frame.image = std::move(image);
                    }

                    accumulation.setTo(cv::Scalar(0, 0, 0));
                    first_timestamp = last_timestamp;
                }
            });

        // auto bias_tool = cam->getTool(fluxeem::ToolType::TOOL_BIAS);
        // bias_tool->setParam("bias_diff_off", 100);
        // bias_tool->setParam("bias_diff_on", 100);

        if (!cam->start())
        {
            LOG_ERROR("Start camera failed.");
            return 3;
        }

        LOG_INFO("Camera started. Press 'q' in the window or Ctrl+C to exit.");
        LOG_INFO("Press ' ' to start/stop recording.");

        while (running && cam->isConnected())
        {
            // 获取最新图像并显示
            cv::Mat current_image;
            {
                std::lock_guard<std::mutex> lock(display_frame.mutex);
                if (!display_frame.image.empty()) {
                    current_image = display_frame.image.clone();
                }
            }

            if (!current_image.empty()) {
                const uint64_t bw = statistics.bandwidth_bytes.load(std::memory_order_relaxed);
                const uint64_t ev = statistics.events_count.load(std::memory_order_relaxed);

                cv::putText(current_image,
                            std::to_string(bw / 1024.0 / 1024.0).substr(0, 5) + " MB/s  |  " +
                            std::to_string(ev / 1000000.0).substr(0, 5) + " Mev/s",
                            cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                            cv::Scalar(0, 255, 0), 1);

                // 录制状态指示
                if (is_recording) {
                    cv::circle(current_image, cv::Point(current_image.cols - 20, 20), 8,
                               cv::Scalar(0, 0, 255), -1);  // 红色实心圆
                    cv::putText(current_image, "REC",
                                cv::Point(current_image.cols - 65, 26),
                                cv::FONT_HERSHEY_SIMPLEX, 0.5,
                                cv::Scalar(0, 0, 255), 1);
                }

                cv::imshow(window_name, current_image);
            }

            // 检查按键
            const int key = cv::waitKey(5);
            if (key == 'q' || key == 27) // 'q' 或 ESC
            {
                running = false;
                break;
            }
            else if (key == ' ')
            {
                if (!is_recording)
                {
                    // 生成带时间戳的文件名
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
                    std::string file_path = std::string("recording_") + time_buf + ".raw";

                    bool ret = cam->startRecording(file_path);
                    if (ret)
                    {
                        is_recording = true;
                        LOG_INFO("Recording started: %s", file_path.c_str());
                    }
                    else
                    {
                        LOG_ERROR("Failed to start recording.");
                    }
                }
                else
                {
                    cam->stopRecording();
                    is_recording = false;
                    LOG_INFO("Recording stopped.");
                }
            }
        }

        cv::destroyAllWindows();

        // 退出前停止录制
        if (is_recording)
        {
            cam->stopRecording();
            LOG_INFO("Recording stopped on exit.");
        }

        LOG_INFO("Camera disconnected.");
        (void)cam->stop();
        return 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception: %s", e.what());
        return 100;
    }
}
