#include <fluxeem/driver/file_reader/ev_file_reader.h>
#include <fluxeem/base/logging/logger.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

class EventAnalyzer {
public:
    cv::Mat img;
    cv::Mat img_swap;

    cv::Vec3b color_bg  = cv::Vec3b(0x70, 0x70, 0x70);
    cv::Vec3b color_on  = cv::Vec3b(0xff, 0xff, 0xff);
    cv::Vec3b color_off = cv::Vec3b(0x00, 0x00, 0x00);

    void setup(uint16_t width, uint16_t height) {
        img = cv::Mat(height, width, CV_8UC3);
        img_swap = cv::Mat(height, width, CV_8UC3);
        img.setTo(color_bg);
        img_swap.setTo(color_bg);
    }

    void getDisplayFrame(cv::Mat& display) {
        std::swap(img, img_swap);
        img.setTo(color_bg);
        img_swap.copyTo(display);
    }

    void processEvents(const std::shared_ptr<fluxeem::EventBatch>& events,
                       uint16_t width,
                       uint16_t height) {
        if (!events || events->empty()) return;
        for (const auto& ev : *events) {
            if (ev.x >= width || ev.y >= height) continue;
            img.at<cv::Vec3b>(ev.y, ev.x) = ev.polarity ? color_on : color_off;
        }
    }

    void reset() {
        img.setTo(color_bg);
        img_swap.setTo(color_bg);
    }
};

static std::string buildDefaultOutputPath()
{
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now{};
#ifdef _WIN32
    localtime_s(&tm_now, &t);
#else
    localtime_r(&t, &tm_now);
#endif

    char time_buf[64];
    std::strftime(time_buf, sizeof(time_buf), "%Y%m%d_%H%M%S", &tm_now);
    return std::string("slow_motion_") + time_buf + ".mp4";
}

static bool tryParsePositiveDouble(const std::string& s, double& value)
{
    char* end_ptr = nullptr;
    const double parsed = std::strtod(s.c_str(), &end_ptr);
    if (end_ptr == s.c_str() || *end_ptr != '\0' || parsed <= 0.0) {
        return false;
    }
    value = parsed;
    return true;
}

static void drawHud(cv::Mat& frame,
                    double speed,
                    bool slow_mode,
                    bool saving,
                    double slow_factor,
                    uint64_t saved_frames)
{
    const cv::Scalar text_color = cv::Scalar(230, 230, 230);
    const cv::Scalar mode_color = slow_mode ? cv::Scalar(80, 255, 160) : cv::Scalar(120, 120, 120);

    cv::putText(frame,
                std::string("Mode: ") + (slow_mode ? "SLOW" : "NORMAL"),
                cv::Point(12, 28),
                cv::FONT_HERSHEY_SIMPLEX,
                0.65,
                mode_color,
                2);

    std::ostringstream speed_text;
    speed_text << "Speed: " << std::fixed << std::setprecision(2) << speed << "x"
               << "  (Slow factor: x" << std::setprecision(1) << slow_factor << ")";
    cv::putText(frame,
                speed_text.str(),
                cv::Point(12, 54),
                cv::FONT_HERSHEY_SIMPLEX,
                0.55,
                text_color,
                1);

    cv::putText(frame,
                std::string("Saving video: ") + (saving ? "ON" : "OFF"),
                cv::Point(12, 80),
                cv::FONT_HERSHEY_SIMPLEX,
                0.55,
                saving ? cv::Scalar(0, 200, 255) : cv::Scalar(140, 140, 140),
                1);

    std::ostringstream frames_text;
    frames_text << "Saved frames: " << saved_frames;
    cv::putText(frame,
                frames_text.str(),
                cv::Point(12, 106),
                cv::FONT_HERSHEY_SIMPLEX,
                0.55,
                text_color,
                1);
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_raw_file> [output_video_path] [slow_factor]" << std::endl;
        std::cerr << "   or: " << argv[0] << " <path_to_raw_file> [slow_factor]" << std::endl;
        return 1;
    }

    const std::string event_file_path = argv[1];
    std::string output_video_path = buildDefaultOutputPath();
    double slow_factor = 100.0;

    if (argc >= 3) {
        double maybe_factor = 0.0;
        if (tryParsePositiveDouble(argv[2], maybe_factor)) {
            slow_factor = maybe_factor;
        } else {
            output_video_path = argv[2];
        }
    }

    if (argc >= 4) {
        double parsed_factor = 0.0;
        if (!tryParsePositiveDouble(argv[3], parsed_factor)) {
            std::cerr << "Invalid slow_factor: " << argv[3] << std::endl;
            return 1;
        }
        slow_factor = parsed_factor;
    }

    if (slow_factor < 1.0) {
        slow_factor = 1.0;
    }

    fluxeem::EvFile reader = fluxeem::EvFileReader::createFileReader(event_file_path);
    if (!reader || !reader->open()) {
        LOG_ERROR("Failed to open RAW file: %s", event_file_path.c_str());
        return 1;
    }

    const uint16_t width = reader->getWidth();
    const uint16_t height = reader->getHeight();

    fluxeem::Timestamp ts_start = 0;
    fluxeem::Timestamp ts_end = 0;
    reader->getStartTime(ts_start);
    reader->getEndTime(ts_end);
    reader->seekToTimestamp(ts_start);

    constexpr int kFps = 30;
    constexpr int kWaitMs = static_cast<int>(1000.0 / kFps);
    constexpr fluxeem::Timestamp kBaseIntervalUs = 1000000 / kFps;
    constexpr double kNormalSpeed = 1.0;
    const double kSlowSpeed = 1.0 / slow_factor;

    EventAnalyzer analyzer;
    analyzer.setup(width, height);

    const std::string window_name = "Slow Motion Playback Sample";
    cv::namedWindow(window_name, cv::WINDOW_NORMAL);
    cv::resizeWindow(window_name, width, height);

    cv::VideoWriter writer;
    bool slow_mode = false;
    bool finished = false;
    uint64_t saved_frames = 0;

    const std::vector<int> codec_candidates = {
        cv::VideoWriter::fourcc('a', 'v', 'c', '1'),
        cv::VideoWriter::fourcc('H', '2', '6', '4'),
        cv::VideoWriter::fourcc('m', 'p', '4', 'v')
    };

    bool writer_opened = false;
    int selected_fourcc = 0;
    for (int candidate : codec_candidates) {
        if (writer.open(output_video_path,
                        candidate,
                        static_cast<double>(kFps),
                        cv::Size(width, height),
                        true)) {
            writer_opened = true;
            selected_fourcc = candidate;
            break;
        }
    }

    if (!writer.isOpened()) {
        std::cerr << "Failed to open output video: " << output_video_path << std::endl;
        return 2;
    }

    char codec_name[5] = {
        static_cast<char>(selected_fourcc & 0xFF),
        static_cast<char>((selected_fourcc >> 8) & 0xFF),
        static_cast<char>((selected_fourcc >> 16) & 0xFF),
        static_cast<char>((selected_fourcc >> 24) & 0xFF),
        '\0'
    };

    std::cout << "Controls:" << std::endl;
    std::cout << "  [Space] Toggle slow motion" << std::endl;
    std::cout << "  [Q]/[Esc] Exit" << std::endl;
    std::cout << "Output video: " << output_video_path << std::endl;
    std::cout << "Codec: " << codec_name << std::endl;
    std::cout << "Slow factor: x" << std::fixed << std::setprecision(1) << slow_factor
              << " (speed=" << std::setprecision(2) << kSlowSpeed << "x)" << std::endl;
    std::cout << "Video recording started from playback begin." << std::endl;

    cv::Mat display;

    while (true) {
        const double speed = slow_mode ? kSlowSpeed : kNormalSpeed;
        bool end_reached_this_iteration = false;

        if (!finished) {
            fluxeem::Timestamp interval = static_cast<fluxeem::Timestamp>(kBaseIntervalUs * speed);
            if (interval < 1) interval = 1;

            auto events = reader->readEventsByTimeInterval(interval);
            analyzer.processEvents(events, width, height);

            if (reader->isEndReached()) {
                finished = true;
                end_reached_this_iteration = true;
                std::cout << "Playback finished." << std::endl;
            }
        }

        analyzer.getDisplayFrame(display);
        if (!display.empty()) {
            drawHud(display, speed, slow_mode, true, slow_factor, saved_frames);
            cv::imshow(window_name, display);

            if (writer.isOpened()) {
                writer.write(display);
                ++saved_frames;
            }
        }

        if (end_reached_this_iteration) {
            break;
        }

        int key = cv::waitKey(kWaitMs) & 0xFFFF;
        if (key == 'q' || key == 'Q' || key == 27) {
            break;
        }

        if (key == ' ') {
            slow_mode = !slow_mode;
            std::cout << "Slow motion " << (slow_mode ? "ON" : "OFF") << std::endl;
        }
    }

    if (writer.isOpened()) {
        writer.release();
    }
    cv::destroyAllWindows();

    std::cout << "Saved playback video: " << output_video_path
              << " (frames=" << saved_frames << ")" << std::endl;

    return 0;
}
