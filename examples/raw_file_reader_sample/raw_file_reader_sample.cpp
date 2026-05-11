/**
 * @file raw_file_reader_sample.cpp
 * @brief RAW 文件交互式回放示例
 *
 * 功能:
 *   - 读取 .raw 文件并以可视化方式回放事件数据
 *   - 双缓冲事件累积渲染，画面流畅
 *   - 底部 HUD 进度条（左=起始时间，右=结束时间，中=当前时间）
 *   - 支持播放/暂停、变速、前进/后退跳转
 *   - 播放结束自动暂停，按空格从头重播
 *
 * 键盘控制:
 *   Space     播放/暂停（播放结束后按空格重播）
 *   +/=       加速 (x2, 最高 x16)
 *   -         减速 (x0.5, 最低 x0.125)
 *   Right →   快进 1 秒
 *   Left  ←   快退 1 秒
 *   R         重置速度为 1x
 *   Q / ESC   退出
 *
 * 用法:
 *   raw_file_reader_sample <path_to_raw_file>
 */

#include <fluxeem/driver/file_reader/ev_file_reader.h>
#include <fluxeem/base/logging/logger.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include <opencv2/opencv.hpp>

// ═══════════════════════ EventAnalyzer ═══════════════════════
// 双缓冲事件累积渲染器：img 持续累积事件像素，显示时 swap 给主线程

class EventAnalyzer {
public:
    cv::Mat img;       // 事件累积画布
    cv::Mat img_swap;  // 显示交换缓冲

    cv::Vec3b color_bg  = cv::Vec3b(0x70, 0x70, 0x70);
    cv::Vec3b color_on  = cv::Vec3b(0xff, 0xff, 0xff);
    cv::Vec3b color_off = cv::Vec3b(0x00, 0x00, 0x00);

    void setup(uint16_t width, uint16_t height) {
        img      = cv::Mat(height, width, CV_8UC3);
        img_swap = cv::Mat(height, width, CV_8UC3);
        img.setTo(color_bg);
        img_swap.setTo(color_bg);
    }

    /// 将累积画面交换到 display，并重置累积画布
    void getDisplayFrame(cv::Mat& display) {
        std::swap(img, img_swap);
        img.setTo(color_bg);
        img_swap.copyTo(display);
    }

    /// 将一批事件累积绘制到 img 上
    void processEvents(const std::shared_ptr<fluxeem::EventBatch>& events,
                       uint16_t width, uint16_t height) {
        if (!events || events->empty()) return;
        for (const auto& ev : *events) {
            if (ev.x >= width || ev.y >= height) continue;
            img.at<cv::Vec3b>(ev.y, ev.x) = ev.polarity ? color_on : color_off;
        }
    }

    /// 重置画布（用于 seek 跳转后清屏）
    void reset() {
        img.setTo(color_bg);
        img_swap.setTo(color_bg);
    }
};

// ═══════════════════════ 工具函数 ═══════════════════════

/// 将微秒时长格式化为 mm:ss.ms
static std::string formatTime(int64_t us)
{
    if (us < 0) us = 0;
    int64_t total_ms = us / 1000;
    int ms  = static_cast<int>(total_ms % 1000);
    int sec = static_cast<int>((total_ms / 1000) % 60);
    int min = static_cast<int>(total_ms / 60000);
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << min << ":"
        << std::setfill('0') << std::setw(2) << sec << "."
        << std::setfill('0') << std::setw(3) << ms;
    return oss.str();
}

/// 在画面底部绘制 HUD 进度条
static void drawHUD(cv::Mat& frame,
                    fluxeem::Timestamp cur_ts,
                    fluxeem::Timestamp start_ts,
                    fluxeem::Timestamp end_ts,
                    uint64_t cur_event,
                    uint64_t max_events,
                    uint64_t frame_ev_count,
                    double decode_mb_per_sec,
                    double decode_mev_per_sec,
                    double speed,
                    bool paused,
                    bool finished)
{
    const int w = frame.cols;
    const int h = frame.rows;

    // ── 半透明 HUD 底栏 ──
    constexpr int hud_h = 62;
    int hud_y = std::max(0, h - hud_h);
    cv::Mat roi = frame(cv::Rect(0, hud_y, w, h - hud_y));
    cv::Mat dark(roi.size(), roi.type(), cv::Scalar(0, 0, 0));
    cv::addWeighted(dark, 0.60, roi, 0.40, 0, roi);

    // ── 进度条 ──
    constexpr int margin = 10;
    constexpr int bar_h  = 5;
    int bar_y = hud_y + 7;
    int bar_w = w - margin * 2;

    // 槽
    cv::rectangle(frame, {margin, bar_y}, {margin + bar_w, bar_y + bar_h},
                  cv::Scalar(60, 60, 60), cv::FILLED);

    // 填充
    double progress = (end_ts > start_ts)
        ? std::clamp(double(cur_ts - start_ts) / double(end_ts - start_ts), 0.0, 1.0)
        : 0.0;
    int fill = static_cast<int>(bar_w * progress);
    if (fill > 0)
        cv::rectangle(frame, {margin, bar_y}, {margin + fill, bar_y + bar_h},
                      cv::Scalar(80, 220, 120), cv::FILLED);

    // 指示点
    cv::circle(frame, {margin + fill, bar_y + bar_h / 2}, 4,
               cv::Scalar(255, 255, 255), cv::FILLED);

    // ── 第一行：起始时间 | 当前时间 | 结束时间 ──
    constexpr double fs = 0.38;
    int row1 = bar_y + bar_h + 15;
    int baseline = 0;

    // 左：起始
    cv::putText(frame, formatTime(0), {margin, row1},
                cv::FONT_HERSHEY_SIMPLEX, fs, cv::Scalar(150, 150, 150), 1);

    // 右：结束
    std::string end_str = formatTime(end_ts - start_ts);
    auto end_sz = cv::getTextSize(end_str, cv::FONT_HERSHEY_SIMPLEX, fs, 1, &baseline);
    cv::putText(frame, end_str, {w - margin - end_sz.width, row1},
                cv::FONT_HERSHEY_SIMPLEX, fs, cv::Scalar(150, 150, 150), 1);

    // 中：当前
    std::string cur_str = formatTime(cur_ts - start_ts);
    auto cur_sz = cv::getTextSize(cur_str, cv::FONT_HERSHEY_SIMPLEX, fs + 0.04, 1, &baseline);
    cv::putText(frame, cur_str, {(w - cur_sz.width) / 2, row1},
                cv::FONT_HERSHEY_SIMPLEX, fs + 0.04, cv::Scalar(255, 255, 255), 1);

    // ── 第二行：状态 | 事件统计 ──
    int row2 = row1 + 16;

    // 左：状态 + 速度
    std::ostringstream st;
    if (finished)       st << ">> END  [Space: Replay]";
    else if (paused)    st << "|| PAUSED";
    else                st << "> PLAYING";
    st << "  " << std::fixed << std::setprecision(speed < 1.0 ? 3 : 1) << speed << "x";

    cv::Scalar st_color = finished ? cv::Scalar(0, 140, 255)
                        : paused   ? cv::Scalar(0, 200, 255)
                                   : cv::Scalar(80, 255, 160);
    cv::putText(frame, st.str(), {margin, row2},
                cv::FONT_HERSHEY_SIMPLEX, fs, st_color, 1);

    // 右：解码统计（每秒）
    std::ostringstream ev;
    ev << "decode:" << std::fixed << std::setprecision(2)
       << decode_mb_per_sec << " MB/s"
       << "  |  " << decode_mev_per_sec << " Mev/s"
       << "  frame_ev:" << frame_ev_count
       << "  #" << cur_event << "/" << max_events;
    std::string ev_str = ev.str();
    auto ev_sz = cv::getTextSize(ev_str, cv::FONT_HERSHEY_SIMPLEX, fs, 1, &baseline);
    cv::putText(frame, ev_str, {w - margin - ev_sz.width, row2},
                cv::FONT_HERSHEY_SIMPLEX, fs, cv::Scalar(190, 190, 190), 1);
}

// ═══════════════════════ main ═══════════════════════

int main(int argc, char* argv[])
{
    // ── 参数 ──
    const std::string desc =
        "RAW file interactive playback viewer.\n"
        "Press 'q' or ESC to quit.\n"
        "Space: Play/Pause  +/-: Speed  Arrow: Seek +/-1s  R: Reset speed\n";

    if (argc < 2) {
        std::cout << desc;
        std::cerr << "Usage: " << argv[0] << " <event_file_path>\n";
        return 1;
    }
    std::cout << desc << std::endl;

    const std::string event_file_path = argv[1];
    std::cout << "Event file: " << event_file_path << std::endl;

    // ── 1. 打开文件 ──
    fluxeem::EvFile reader = fluxeem::EvFileReader::createFileReader(event_file_path);
    if (!reader) {
        LOG_ERROR("Failed to create file reader: %s", event_file_path.c_str());
        return 1;
    }
    if (!reader->open()) {
        LOG_ERROR("open() failed!");
        return 1;
    }

    const uint16_t width  = reader->getWidth();
    const uint16_t height = reader->getHeight();

    fluxeem::Timestamp ts_start = 0, ts_end = 0;
    uint64_t max_events = 0;
    reader->getStartTime(ts_start);
    reader->getEndTime(ts_end);
    reader->getEventCount(max_events);

    fluxeem::EvFileInfo file_info{};
    reader->getFileMetadata(file_info);

    LOG_INFO("Resolution: %d x %d", width, height);
    LOG_INFO("Duration  : %s", formatTime(ts_end - ts_start).c_str());
    LOG_INFO("Events    : %llu", max_events);
    LOG_INFO("Start ts  : %llu", ts_start);
    LOG_INFO("End   ts  : %llu", ts_end);

    // ── 2. 初始化显示 ──
    EventAnalyzer analyzer;
    analyzer.setup(width, height);

    constexpr int kFps = 30;
    constexpr int kWaitMs = static_cast<int>(1000.0 / kFps);
    constexpr fluxeem::Timestamp kBaseInterval = 1000000 / kFps; // ~33333 us
    constexpr fluxeem::Timestamp kSeekStep = 1000000;            // 1 秒
    constexpr fluxeem::Timestamp kStatsWindowUs = 1000000;       // 事件时间 1 秒

    const std::string window_name = "Fluxeem RAW Playback";
    cv::namedWindow(window_name, cv::WINDOW_NORMAL);
    cv::resizeWindow(window_name, width, height);

    cv::Mat display;

    // ── 3. 播放状态 ──
    double playback_speed = 1.0;
    bool paused   = false;
    bool finished = false;
    double decode_mb_per_sec = 0.0;
    double decode_mev_per_sec = 0.0;
    fluxeem::Timestamp last_stats_event_ts = ts_start;

    reader->seekToTimestamp(ts_start);

    // ── 4. 主循环 ──
    bool stop_application = false;
    while (!stop_application)
    {
        uint64_t frame_ev_count = 0;

        if (!paused && !finished)
        {
            // 根据当前倍速计算本帧读取的时间间隔
            fluxeem::Timestamp interval = static_cast<fluxeem::Timestamp>(kBaseInterval * playback_speed);
            if (interval < 1) interval = 1;

            auto events = reader->readEventsByTimeInterval(interval);
            frame_ev_count = (events && !events->empty()) ? events->size() : 0;
            analyzer.processEvents(events, width, height);

            // 检测是否播放到末尾
            if (reader->isEndReached())
            {
                finished = true;
                paused   = true;
                std::cout << "Playback finished. Press Space to replay.\n";
            }
        }

        const fluxeem::Timestamp cur_ts_for_stats = reader->getCurrentTimestamp();
        if (cur_ts_for_stats >= last_stats_event_ts &&
            (cur_ts_for_stats - last_stats_event_ts) >= kStatsWindowUs) {
            uint64_t decode_bandwidth_bytes = 0;
            uint64_t decode_events_count = 0;
            if (reader->getDecodeStatistics(decode_bandwidth_bytes, decode_events_count)) {
                decode_mb_per_sec = static_cast<double>(decode_bandwidth_bytes) / (1024.0 * 1024.0);
                decode_mev_per_sec = static_cast<double>(decode_events_count) / 1000000.0;
            } else {
                decode_mb_per_sec = 0.0;
                decode_mev_per_sec = 0.0;
            }
            last_stats_event_ts = cur_ts_for_stats;
        }

        // 取出显示帧（暂停时也 swap，保证画面不卡死，但累积画布为空所以显示上一帧内容）
        if (!paused)
        {
            analyzer.getDisplayFrame(display);
        }
        // paused 时保持 display 不变，显示暂停前的最后一帧

        if (!display.empty())
        {
            // 绘制 HUD
            fluxeem::Timestamp cur_ts = reader->getCurrentTimestamp();
            uint64_t cur_ev = reader->getCurrentEventIndex();
            drawHUD(display, cur_ts, ts_start, ts_end, cur_ev, max_events,
                    frame_ev_count, decode_mb_per_sec, decode_mev_per_sec,
                    playback_speed, paused, finished);

            cv::imshow(window_name, display);
        }

        // ── 键盘处理 ──
        int key = cv::waitKey(kWaitMs) & 0xFFFF;

        switch (key)
        {
        case 'q': case 'Q': case 27: // ESC
            stop_application = true;
            break;

        case ' ': // 空格
            if (finished) {
                // 播放结束 → 从头重播
                reader->seekToTimestamp(ts_start);
                analyzer.reset();
                display.release();
                finished = false;
                paused   = false;
                decode_mb_per_sec = 0.0;
                decode_mev_per_sec = 0.0;
                last_stats_event_ts = ts_start;
                uint64_t flush_bw = 0;
                uint64_t flush_ev = 0;
                reader->getDecodeStatistics(flush_bw, flush_ev);
                std::cout << "Replaying from start.\n";
            } else {
                paused = !paused;
                std::cout << (paused ? "Paused.\n" : "Playing.\n");
            }
            break;

        case '+': case '=': // 加速
            if (playback_speed < 16.0) playback_speed *= 2.0;
            std::cout << "Speed: " << playback_speed << "x\n";
            break;

        case '-': case '_': // 减速
            if (playback_speed > 0.125) playback_speed /= 2.0;
            std::cout << "Speed: " << playback_speed << "x\n";
            break;

        case 'r': case 'R': // 重置速度
            playback_speed = 1.0;
            std::cout << "Speed reset: 1x\n";
            break;

        case 0x0027: // Right → 快进 1s
        case 0xFF53:
        {
            fluxeem::Timestamp target = std::min(reader->getCurrentTimestamp() + kSeekStep, ts_end);
            reader->seekToTimestamp(target);
            analyzer.reset();
            last_stats_event_ts = target;
            decode_mb_per_sec = 0.0;
            decode_mev_per_sec = 0.0;
            uint64_t flush_bw = 0;
            uint64_t flush_ev = 0;
            reader->getDecodeStatistics(flush_bw, flush_ev);
            std::cout << "Seek >> " << formatTime(target - ts_start) << "\n";
            break;
        }

        case 0x0025: // Left ← 快退 1s
        case 0xFF51:
        {
            fluxeem::Timestamp cur = reader->getCurrentTimestamp();
            fluxeem::Timestamp target = (cur - ts_start > kSeekStep) ? cur - kSeekStep : ts_start;
            reader->seekToTimestamp(target);
            analyzer.reset();
            finished = false;
            paused   = false;
            last_stats_event_ts = target;
            decode_mb_per_sec = 0.0;
            decode_mev_per_sec = 0.0;
            uint64_t flush_bw = 0;
            uint64_t flush_ev = 0;
            reader->getDecodeStatistics(flush_bw, flush_ev);
            std::cout << "Seek << " << formatTime(target - ts_start) << "\n";
            break;
        }

        default:
            break;
        }
    }

    cv::destroyAllWindows();
    std::cout << "Application exited.\n";
    return 0;
}
