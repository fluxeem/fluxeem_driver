// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#include <fluxeem/driver/camera/base/i_camera.hpp>
#include <fluxeem/driver/camera/ev_camera_service.hpp>
#include <fluxeem/hal/tools/camera_tool.h>
#include <fluxeem/hal/tools/tool_info.h>

#include <atomic>
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <mutex>
#include <sstream>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <thread>
#include <chrono>

#include <opencv2/opencv.hpp>

// 前向声明
class CameraControlUI;

// 全局UI指针，用于轨迹条回调
static CameraControlUI* g_ui_instance = nullptr;

// 工具参数运行时状态
struct ParamState {
    std::string name;
    fluxeem::ToolParameterType type;
    int int_min{0}, int_max{100};
    float float_min{0.0f}, float_max{1.0f};
    std::vector<std::string> enum_options;
    int current_pos{0};  // 轨迹条当前位置
    int last_pos{-1};    // 上一次位置，用于检测变化
};

// 工具UI信息
struct ToolUI {
    fluxeem::ToolType type;
    std::string name;
    std::string description;
    std::vector<ParamState> params;
    std::shared_ptr<fluxeem::CameraTool> tool;
    bool available{false};
};

// 显示帧结构
struct DisplayFrame {
    cv::Mat image;
    std::mutex mutex;
    uint64_t event_count{0};
    uint64_t last_timestamp{0};
};

struct StatisticsInfo {
    std::atomic<uint64_t> bandwidth_bytes{0};
    std::atomic<uint64_t> events_count{0};
};

// 相机控制UI类
class CameraControlUI {
public:
    CameraControlUI() {
        g_ui_instance = this;
    }
    ~CameraControlUI() {
        g_ui_instance = nullptr;
    }

    bool initialize() {
        // 发现相机
        fluxeem::EvCameraService camera_manager;
        const auto descs = camera_manager.listCameras();

        if (descs.empty()) {
            std::cerr << "No camera found. Please plug in a camera and try again." << std::endl;
            return false;
        }

        std::cout << "Found cameras:" << std::endl;
        for (const auto& d : descs) {
            std::cout << "  - " << d.serial << " (" << d.product << ")" << std::endl;
        }

        // 打开第一个相机
        const std::string serial_to_open = descs.front().serial;
        std::cout << "Opening camera: " << serial_to_open << std::endl;

        cam_ = camera_manager.open(serial_to_open);
        if (!cam_) {
            std::cerr << "Failed to open camera." << std::endl;
            return false;
        }

        // 获取相机分辨率
        width_ = cam_->getWidth();
        height_ = cam_->getHeight();
        std::cout << "Camera resolution: " << width_ << " x " << height_ << std::endl;

        // 初始化工具
        initTools();

        // 注册统计回调
        cam_->setStatisticsCallback(
            [this](const fluxeem::EvCameraStatisticInfo info) {
                statistics_.bandwidth_bytes.store(info.bandwidth_byte, std::memory_order_relaxed);
                statistics_.events_count.store(info.events_count, std::memory_order_relaxed);
            });

        // 注册事件回调
        setupEventCallback();

        // 启动相机
        if (!cam_->start()) {
            std::cerr << "Failed to start camera." << std::endl;
            return false;
        }

        // 创建UI窗口
        createUI();

        return true;
    }

    void run() {
        std::cout << "\n=== Camera Control UI Started ===" << std::endl;
        std::cout << "Controls:" << std::endl;
        std::cout << "  [0-9] Select tool" << std::endl;
        std::cout << "  [B] Toggle BOOL params of selected tool" << std::endl;
        std::cout << "  [R] Reset display" << std::endl;
        std::cout << "  [S] Save current frame (PNG)" << std::endl;
        std::cout << "  [Space] Start/stop recording (RAW)" << std::endl;
        std::cout << "  [Q]/[Esc] Exit" << std::endl;
        std::cout << "===================================\n" << std::endl;

        last_update_time_ = std::chrono::steady_clock::now();

        while (running_ && cam_->isConnected()) {
            // 处理参数变化（轨迹条）
            processParamChanges();

            // 更新显示
            updateDisplay();

            // 处理键盘输入（增加延迟到30ms以提高稳定性）
            int key = cv::waitKey(30);
            if (key != -1) {
                handleKeyboard(key);
            }
        }

        cleanup();
    }

    // 处理轨迹条参数变化
    void processParamChanges() {
        if (selected_tool_idx_ < 0 || selected_tool_idx_ >= static_cast<int>(tools_.size()))
            return;

        auto& tool = tools_[selected_tool_idx_];
        if (!tool.available || !tool.tool)
            return;

        // 检查每个参数的位置变化
        for (auto& param : tool.params) {
            if (param.current_pos != param.last_pos) {
                // 位置发生变化，应用新值
                applyParamChange(tool, param);
                param.last_pos = param.current_pos;
            }
        }
    }

    // 应用参数变化到相机
    void applyParamChange(ToolUI& tool, ParamState& param) {
        switch (param.type) {
            case fluxeem::ToolParameterType::INT: {
                int actual_value = param.current_pos + param.int_min;
                if (tool.tool->setParam(param.name, actual_value)) {
                    std::cout << "[PARAM] " << param.name << " = " << actual_value << std::endl;
                } else {
                    std::cerr << "[ERROR] Failed to set " << param.name << std::endl;
                }
                break;
            }
            case fluxeem::ToolParameterType::BOOL: {
                bool val = (param.current_pos != 0);
                if (tool.tool->setParam(param.name, val)) {
                    std::cout << "[PARAM] " << param.name << " = " << (val ? "true" : "false") << std::endl;
                } else {
                    std::cerr << "[ERROR] Failed to set " << param.name << std::endl;
                }
                break;
            }
            case fluxeem::ToolParameterType::FLOAT: {
                float actual_value = (param.current_pos / 100.0f) + param.float_min;
                if (tool.tool->setParam(param.name, actual_value)) {
                    std::cout << "[PARAM] " << param.name << " = " << actual_value << std::endl;
                } else {
                    std::cerr << "[ERROR] Failed to set " << param.name << std::endl;
                }
                break;
            }
            case fluxeem::ToolParameterType::ENUM: {
                if (param.current_pos >= 0 && param.current_pos < static_cast<int>(param.enum_options.size())) {
                    std::string val = param.enum_options[param.current_pos];
                    if (tool.tool->setParam(param.name, val)) {
                        std::cout << "[PARAM] " << param.name << " = " << val << std::endl;
                    } else {
                        std::cerr << "[ERROR] Failed to set " << param.name << std::endl;
                    }
                }
                break;
            }
            default:
                break;
        }
    }

private:
    void initTools() {
        // 初始化所有工具类型
        const std::vector<std::pair<fluxeem::ToolType, std::string>> tool_types = {
            {fluxeem::ToolType::TOOL_BIAS, "Biases"},
            {fluxeem::ToolType::TOOL_ROI, "ROI"},
            {fluxeem::ToolType::TOOL_TRIGGER_IN, "Trigger In"},
            {fluxeem::ToolType::TOOL_ANTI_FLICKER, "Anti Flicker"},
            {fluxeem::ToolType::TOOL_EVENT_TRAIL_FILTER, "Trail Filter"},
            {fluxeem::ToolType::TOOL_EVENT_RATE_CONTROL, "Rate Control"},
            {fluxeem::ToolType::TOOL_SYNC, "Sync"},
        };

        for (const auto& [type, name] : tool_types) {
            ToolUI tool_ui;
            tool_ui.type = type;
            tool_ui.name = name;

            // 尝试获取工具
            tool_ui.tool = cam_->getTool(type);
            tool_ui.available = (tool_ui.tool != nullptr);

            if (tool_ui.available) {
                auto info = tool_ui.tool->getToolInfo();
                tool_ui.description = info.description;

                // 获取所有参数信息
                auto all_param_info = tool_ui.tool->getAllParamInfo();

                for (const auto& param_name : info.parameter_names) {
                    ParamState param_state;
                    param_state.name = param_name;

                    auto it = all_param_info.find(param_name);
                    if (it != all_param_info.end()) {
                        param_state.type = it->second.type;

                        // 获取详细参数信息并读取当前值
                        switch (it->second.type) {
                            case fluxeem::ToolParameterType::INT: {
                                fluxeem::IntParameterInfo int_info;
                                if (tool_ui.tool->getParamInfo(param_name, int_info)) {
                                    param_state.int_min = int_info.min;
                                    param_state.int_max = int_info.max;

                                    // 读取当前值
                                    int current_val;
                                    if (tool_ui.tool->getParam(param_name, current_val)) {
                                        param_state.current_pos = current_val - int_info.min;
                                    } else {
                                        param_state.current_pos = int_info.default_value - int_info.min;
                                    }
                                }
                                break;
                            }
                            case fluxeem::ToolParameterType::FLOAT: {
                                fluxeem::FloatParameterInfo float_info;
                                if (tool_ui.tool->getParamInfo(param_name, float_info)) {
                                    param_state.float_min = float_info.min;
                                    param_state.float_max = float_info.max;

                                    // 读取当前值
                                    float current_val;
                                    if (tool_ui.tool->getParam(param_name, current_val)) {
                                        param_state.current_pos = static_cast<int>((current_val - float_info.min) * 100);
                                    } else {
                                        param_state.current_pos = static_cast<int>((float_info.default_value - float_info.min) * 100);
                                    }
                                }
                                break;
                            }
                            case fluxeem::ToolParameterType::BOOL: {
                                fluxeem::BoolParameterInfo bool_info;
                                if (tool_ui.tool->getParamInfo(param_name, bool_info)) {
                                    // 读取当前值
                                    bool current_val;
                                    if (tool_ui.tool->getParam(param_name, current_val)) {
                                        param_state.current_pos = current_val ? 1 : 0;
                                    } else {
                                        param_state.current_pos = bool_info.default_value ? 1 : 0;
                                    }
                                }
                                break;
                            }
                            case fluxeem::ToolParameterType::ENUM: {
                                fluxeem::EnumParameterInfo enum_info;
                                if (tool_ui.tool->getParamInfo(param_name, enum_info)) {
                                    param_state.enum_options = enum_info.options;

                                    // 读取当前值
                                    std::string current_val;
                                    if (tool_ui.tool->getParam(param_name, current_val)) {
                                        for (size_t i = 0; i < enum_info.options.size(); ++i) {
                                            if (enum_info.options[i] == current_val) {
                                                param_state.current_pos = static_cast<int>(i);
                                                break;
                                            }
                                        }
                                    } else {
                                        for (size_t i = 0; i < enum_info.options.size(); ++i) {
                                            if (enum_info.options[i] == enum_info.default_value) {
                                                param_state.current_pos = static_cast<int>(i);
                                                break;
                                            }
                                        }
                                    }
                                }
                                break;
                            }
                            default:
                                break;
                        }
                    }

                    param_state.last_pos = param_state.current_pos;
                    tool_ui.params.push_back(param_state);
                }
            }

            tools_.push_back(tool_ui);
        }

        // 选择第一个可用的工具
        for (size_t i = 0; i < tools_.size(); ++i) {
            if (tools_[i].available) {
                selected_tool_idx_ = static_cast<int>(i);
                break;
            }
        }
    }

    void setupEventCallback() {
        accumulation_ = cv::Mat(height_, width_, CV_8UC3, cv::Scalar(0, 0, 0));
        window_initialized_ = false;

        const uint32_t cb_id = cam_->registerEventBatchCallback(
            [this](fluxeem::EventIterator_t begin, fluxeem::EventIterator_t end) {
                if (begin == end) return;

                std::lock_guard<std::mutex> acc_lock(accumulation_mutex_);

                if (!window_initialized_) {
                    first_timestamp_ = begin->timestamp;
                    window_initialized_ = true;
                }

                uint64_t count = 0;
                for (auto it = begin; it != end; ++it) {
                    if (it->timestamp < first_timestamp_) {
                        first_timestamp_ = it->timestamp;
                        accumulation_.setTo(cv::Scalar(0, 0, 0));
                    }

                    if (it->x < width_ && it->y < height_) {
                        if (it->polarity == 1) {
                            accumulation_.at<cv::Vec3b>(it->y, it->x) = cv::Vec3b(0, 255, 0);
                        } else {
                            accumulation_.at<cv::Vec3b>(it->y, it->x) = cv::Vec3b(0, 0, 255);
                        }
                    }
                    count++;
                }

                const uint64_t last_timestamp = (end - 1)->timestamp;
                const uint64_t time_span = last_timestamp - first_timestamp_;

                if (time_span >= display_time_window_us_) {
                    cv::Mat image = accumulation_.clone();

                    {
                        std::lock_guard<std::mutex> frame_lock(display_frame_.mutex);
                        display_frame_.image = std::move(image);
                        display_frame_.event_count = count;
                        display_frame_.last_timestamp = last_timestamp;
                    }

                    accumulation_.setTo(cv::Scalar(0, 0, 0));
                    first_timestamp_ = last_timestamp;
                }
            });

        (void)cb_id;
    }

    void createUI() {
        // 主显示窗口
        cv::namedWindow(window_name_, cv::WINDOW_NORMAL);
        cv::resizeWindow(window_name_, width_, height_);

        // 控制面板窗口（仅用于轨迹条控件）
        cv::namedWindow(control_window_name_, cv::WINDOW_NORMAL);
        cv::resizeWindow(control_window_name_, 560, 340);

        // 信息面板窗口（显示工具信息和参数值）
        cv::namedWindow(info_window_name_, cv::WINDOW_NORMAL);
        cv::resizeWindow(info_window_name_, 440, 680);

        // 为当前选中的工具创建参数轨迹条
        refreshToolControls();
    }

    void refreshToolControls() {
        // 重新创建窗口以清除旧轨迹条
        cv::destroyWindow(control_window_name_);
        cv::namedWindow(control_window_name_, cv::WINDOW_NORMAL);

        // 如果选中的工具可用，为其参数创建轨迹条
        int trackbar_count = 0;
        std::vector<const ParamState*> bool_params;
        trackbar_labels_.clear();
        if (selected_tool_idx_ >= 0 && selected_tool_idx_ < static_cast<int>(tools_.size())) {
            auto& tool = tools_[selected_tool_idx_];
            if (tool.available) {
                int param_idx = 0;
                for (auto& param : tool.params) {
                    if (param.type == fluxeem::ToolParameterType::BOOL) {
                        bool_params.push_back(&param);
                        continue;  // BOOL 参数用键盘切换，不使用轨迹条
                    }
                    if (param_idx >= 8) break; // 最多显示8个参数
                    trackbar_labels_.push_back(param.name);
                    createParamTrackbar(param, selected_tool_idx_, param_idx);
                    param_idx++;
                    trackbar_count++;
                }
            }
        }

        // 在控制面板标题显示 enable/BOOL 状态，便于与轨迹条窗口一起观察
        cv::setWindowTitle(control_window_name_, buildControlWindowTitle(bool_params));

        // 注册鼠标回调，实现悬停显示完整参数名
        cv::setMouseCallback(control_window_name_, onControlPanelMouse, nullptr);

        // 构建图例图像，显示在轨迹条下方
        buildLegendImage(bool_params, trackbar_count);
        if (!legend_image_.empty()) {
            cv::imshow(control_window_name_, legend_image_);
        }

        // 根据轨迹条数量设置初始窗口尺寸
        const int legend_h = legend_image_.empty() ? 0 : legend_image_.rows;
        const int window_height = std::max(300, legend_h + 30);
        cv::resizeWindow(control_window_name_, 560, window_height);
    }

    std::string buildControlWindowTitle(const std::vector<const ParamState*>& bool_params) const {
        std::ostringstream oss;
        oss << "Camera Control Panel";

        if (selected_tool_idx_ >= 0 && selected_tool_idx_ < static_cast<int>(tools_.size())) {
            oss << " | Tool: " << tools_[selected_tool_idx_].name;
        }

        if (!bool_params.empty()) {
            oss << " | ";
            for (size_t i = 0; i < bool_params.size(); ++i) {
                const auto* p = bool_params[i];
                oss << p->name << "=" << ((p->current_pos != 0) ? "ON" : "OFF");
                if (i + 1 < bool_params.size()) {
                    oss << ", ";
                }
            }
            oss << " (press b)";
        }

        return oss.str();
    }

    void createParamTrackbar(ParamState& param, int tool_idx, int param_idx) {
        // 计算轨迹条范围
        int max_pos = 0;
        switch (param.type) {
            case fluxeem::ToolParameterType::INT:
                max_pos = param.int_max - param.int_min;
                break;
            case fluxeem::ToolParameterType::BOOL:
                max_pos = 1;
                break;
            case fluxeem::ToolParameterType::FLOAT:
                max_pos = static_cast<int>((param.float_max - param.float_min) * 100);
                break;
            case fluxeem::ToolParameterType::ENUM:
                max_pos = static_cast<int>(param.enum_options.size()) - 1;
                if (max_pos < 0) max_pos = 0;
                break;
            default:
                return;
        }

        // 使用短编号作为轨迹条标签，避免 OpenCV 控件截断长名
        std::string trackbar_name = "[" + std::to_string(param_idx) + "] " + param.name;
        cv::createTrackbar(trackbar_name, control_window_name_,
                          &param.current_pos, max_pos,
                          onTrackbarChange);
    }

    // 构建图例图像：参数名对照表 + BOOL 状态
    void buildLegendImage(const std::vector<const ParamState*>& bool_params, int trackbar_count) {
        const int lh = 22;
        const int col_width = 260;
        const int cols = 2;
        const int font_face = cv::FONT_HERSHEY_SIMPLEX;
        const double text_scale = 0.55;
        const int text_thickness = 1;
        const int text_line_type = cv::LINE_AA;
        const int rows_per_col = std::max(1, (trackbar_count + cols - 1) / cols);

        int total_items = trackbar_count + static_cast<int>(bool_params.size());
        int section_rows = (total_items + cols - 1) / cols;
        int legend_height = 30 + section_rows * lh;
        if (!bool_params.empty()) legend_height += lh;  // "press b" hint
        legend_height = std::max(legend_height, 80);

        legend_image_ = cv::Mat(legend_height, col_width * cols + 20, CV_8UC3, cv::Scalar(48, 48, 48));

        auto drawClearText = [&](const std::string& text, const cv::Point& org, const cv::Scalar& color) {
            // 先画深色描边再画亮色正文，提升低分辨率窗口下的可读性
            cv::putText(legend_image_, text, org, font_face, text_scale,
                        cv::Scalar(20, 20, 20), text_thickness + 2, text_line_type);
            cv::putText(legend_image_, text, org, font_face, text_scale,
                        color, text_thickness, text_line_type);
        };

        // 画参数名对照表（两列布局）
        for (int i = 0; i < trackbar_count; ++i) {
            int col = i / rows_per_col;
            int row = i % rows_per_col;
            int x = 15 + col * col_width;
            int y = 24 + row * lh;
            std::string label = "[" + std::to_string(i) + "] " + trackbar_labels_[i];
            drawClearText(label, cv::Point(x, y), cv::Scalar(240, 240, 240));
        }

        // 在对照表下方显示 BOOL 参数状态
        if (!bool_params.empty()) {
            int y_start = 20 + rows_per_col * lh + 8;
            for (size_t i = 0; i < bool_params.size(); ++i) {
                int col = static_cast<int>(i) / rows_per_col;
                int row = static_cast<int>(i) % rows_per_col;
                int x = 15 + col * col_width;
                int y = y_start + row * lh + 2;
                const auto* p = bool_params[i];
                const std::string state = (p->current_pos != 0) ? "ON" : "OFF";
                drawClearText(p->name + " = " + state + "  (press b)",
                              cv::Point(x, y),
                              (p->current_pos != 0) ? cv::Scalar(150, 255, 150) : cv::Scalar(190, 190, 190));
            }
        }
    }

    // 鼠标回调：悬停时在窗口标题显示完整参数名
    static void onControlPanelMouse(int event, int x, int y, int /*flags*/, void* /*userdata*/) {
        if (event != cv::EVENT_MOUSEMOVE || !g_ui_instance) return;

        // OpenCV trackbar 区域大约每个 55px 高
        const int per_trackbar = 55;
        int idx = y / per_trackbar;

        auto& self = *g_ui_instance;
        if (idx >= 0 && idx < static_cast<int>(self.trackbar_labels_.size())) {
            std::string title = "Param: " + self.trackbar_labels_[idx];
            cv::setWindowTitle(self.control_window_name_, title);
        }
    }

    // 静态轨迹条回调
    static void onTrackbarChange(int pos, void* userdata) {
        // OpenCV 的回调不会给我们 userdata，我们需要使用全局实例
        // 实际的变化检测在 processParamChanges 中处理
        (void)pos;
        (void)userdata;
    }

    void updateDisplay() {
        cv::Mat current_image;
        uint64_t event_count = 0;

        {
            std::lock_guard<std::mutex> lock(display_frame_.mutex);
            if (!display_frame_.image.empty()) {
                current_image = display_frame_.image.clone();
                event_count = display_frame_.event_count;
            }
        }

        if (!current_image.empty()) {
            drawInfo(current_image, event_count);
            cv::imshow(window_name_, current_image);
        }

        // 更新信息面板（动态计算所需高度，避免文字溢出或重叠）
        int panel_height = calculatePanelHeight();
        cv::Mat info_bg(panel_height, 420, CV_8UC3, cv::Scalar(40, 40, 40));
        drawControlPanel(info_bg);
        cv::imshow(info_window_name_, info_bg);
    }

    void drawInfo(cv::Mat& image, uint64_t event_count) {
        // 显示事件统计信息
        char events_text[128];
        std::snprintf(events_text,
                      sizeof(events_text),
                      "Events (Window): %llu",
                      static_cast<unsigned long long>(event_count));

        char timestamp_text[128];
        std::snprintf(timestamp_text,
                      sizeof(timestamp_text),
                      "Timestamp (us): %llu",
                      static_cast<unsigned long long>(display_frame_.last_timestamp));

        const uint64_t bw = statistics_.bandwidth_bytes.load(std::memory_order_relaxed);
        const uint64_t ev = statistics_.events_count.load(std::memory_order_relaxed);
        char stat_text[128];
        std::snprintf(stat_text,
                      sizeof(stat_text),
                      "Rate: %.2f MB/s  |  %.2f Mev/s",
                      static_cast<double>(bw) / 1024.0 / 1024.0,
                      static_cast<double>(ev) / 1000000.0);

        cv::putText(image, events_text, cv::Point(10, 30),
                   cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
        cv::putText(image, timestamp_text, cv::Point(10, 60),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(200, 200, 200), 1);
        cv::putText(image, stat_text, cv::Point(10, 85),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);

        if (is_recording_) {
            cv::circle(image, cv::Point(image.cols - 20, 20), 8,
                       cv::Scalar(0, 0, 255), -1);
            cv::putText(image, "REC",
                        cv::Point(image.cols - 65, 26),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5,
                        cv::Scalar(0, 0, 255), 1);
        }

        // 显示图例
        cv::rectangle(image, cv::Point(10, height_ - 40), cv::Point(30, height_ - 20), cv::Scalar(0, 255, 0), -1);
        cv::putText(image, "ON", cv::Point(35, height_ - 25),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);

        cv::rectangle(image, cv::Point(70, height_ - 40), cv::Point(90, height_ - 20), cv::Scalar(0, 0, 255), -1);
        cv::putText(image, "OFF", cv::Point(95, height_ - 25),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
    }

    // 动态计算信息面板所需高度
    int calculatePanelHeight() {
        const int lh = 20;
        const int gap = 8;
        int h = 15;  // 顶部边距

        h += lh + gap + 2 + gap;           // 标题 + 分隔线
        h += lh + lh + lh + gap + 2 + gap; // 相机信息 + 录制状态 + 分隔线
        h += lh + gap;                      // "Selected Tool" 标题

        if (selected_tool_idx_ >= 0 && selected_tool_idx_ < static_cast<int>(tools_.size())) {
            const auto& tool = tools_[selected_tool_idx_];
            h += lh;  // 工具名
            if (tool.available && !tool.description.empty()) h += lh;
            if (tool.available && !tool.params.empty()) {
                h += gap;
                h += static_cast<int>(tool.params.size()) * lh;
            }
        }

        h += gap + 2 + gap;                // 分隔线
        h += lh + gap;                      // "Tools" 标题
        h += static_cast<int>(std::min(tools_.size(), static_cast<size_t>(10))) * lh;
        h += gap + 2 + gap;                // 分隔线
        h += lh + gap;                      // "Shortcuts" 标题
        h += 6 * lh;
        h += 15;                            // 底部边距

        return std::max(h, 200);
    }

    // 获取参数当前值的字符串表示
    std::string getParamValueString(const ParamState& param) const {
        switch (param.type) {
            case fluxeem::ToolParameterType::INT:
                return std::to_string(param.current_pos + param.int_min);
            case fluxeem::ToolParameterType::BOOL:
                return param.current_pos ? "ON" : "OFF";
            case fluxeem::ToolParameterType::FLOAT: {
                float val = (param.current_pos / 100.0f) + param.float_min;
                char buf[32];
                std::snprintf(buf, sizeof(buf), "%.2f", val);
                return std::string(buf);
            }
            case fluxeem::ToolParameterType::ENUM:
                if (param.current_pos >= 0 && param.current_pos < static_cast<int>(param.enum_options.size()))
                    return param.enum_options[param.current_pos];
                return "?";
            default:
                return "?";
        }
    }

    void drawControlPanel(cv::Mat& panel) {
        int y = 15;
        const int lh = 20;   // 统一行高
        const int gap = 8;   // 统一段落间距

        // 标题
        cv::putText(panel, "Camera Control Panel", cv::Point(10, y += lh),
                   cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(255, 255, 255), 1);
        y += gap;
        cv::line(panel, cv::Point(10, y), cv::Point(410, y), cv::Scalar(100, 100, 100), 1);
        y += gap;

        // 相机信息
        cv::putText(panel, "Resolution: " + std::to_string(width_) + "x" + std::to_string(height_),
                   cv::Point(10, y += lh), cv::FONT_HERSHEY_SIMPLEX, 0.42, cv::Scalar(200, 200, 200), 1);
        cv::putText(panel, "Status: " + std::string(cam_->isConnected() ? "Connected" : "Disconnected"),
                   cv::Point(10, y += lh), cv::FONT_HERSHEY_SIMPLEX, 0.42,
                   cam_->isConnected() ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255), 1);
        cv::putText(panel, "Recording: " + std::string(is_recording_ ? "ON" : "OFF"),
               cv::Point(10, y += lh), cv::FONT_HERSHEY_SIMPLEX, 0.42,
               is_recording_ ? cv::Scalar(0, 0, 255) : cv::Scalar(180, 180, 180), 1);
        y += gap;
        cv::line(panel, cv::Point(10, y), cv::Point(410, y), cv::Scalar(100, 100, 100), 1);
        y += gap;

        // 当前选中工具
        cv::putText(panel, "Selected Tool", cv::Point(10, y += lh),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
        y += gap;

        if (selected_tool_idx_ >= 0 && selected_tool_idx_ < static_cast<int>(tools_.size())) {
            const auto& tool = tools_[selected_tool_idx_];
            cv::putText(panel, tool.name + (tool.available ? "" : " [N/A]"),
                       cv::Point(10, y += lh), cv::FONT_HERSHEY_SIMPLEX, 0.45,
                       tool.available ? cv::Scalar(0, 255, 255) : cv::Scalar(128, 128, 128), 1);

            if (tool.available && !tool.description.empty()) {
                std::string desc = tool.description;
                if (desc.length() > 55) desc = desc.substr(0, 55) + "...";
                cv::putText(panel, desc, cv::Point(10, y += lh),
                           cv::FONT_HERSHEY_SIMPLEX, 0.35, cv::Scalar(180, 180, 180), 1);
            }

            // 显示当前各参数值（与轨迹条联动）
            if (tool.available && !tool.params.empty()) {
                y += gap;
                for (const auto& param : tool.params) {
                    std::string val_str = getParamValueString(param);
                    std::string display_name = param.name;
                    if (display_name.length() > 40) display_name = display_name.substr(0, 40) + "..";
                    cv::putText(panel, display_name + ": " + val_str,
                               cv::Point(15, y += lh), cv::FONT_HERSHEY_SIMPLEX, 0.38,
                               cv::Scalar(180, 220, 180), 1);
                }
            }
        }

        y += gap;
        cv::line(panel, cv::Point(10, y), cv::Point(410, y), cv::Scalar(100, 100, 100), 1);
        y += gap;

        // 所有工具列表
        cv::putText(panel, "Tools (press 0-9)", cv::Point(10, y += lh),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
        y += gap;

        for (size_t i = 0; i < tools_.size() && i < 10; ++i) {
            const auto& tool = tools_[i];
            std::string marker = (i == static_cast<size_t>(selected_tool_idx_)) ? "> " : "  ";
            std::string status = tool.available ? "" : " [N/A]";
            cv::putText(panel, marker + std::to_string(i) + ": " + tool.name + status,
                       cv::Point(10, y += lh), cv::FONT_HERSHEY_SIMPLEX, 0.42,
                       tool.available ? cv::Scalar(200, 200, 200) : cv::Scalar(100, 100, 100), 1);
        }

        y += gap;
        cv::line(panel, cv::Point(10, y), cv::Point(410, y), cv::Scalar(100, 100, 100), 1);
        y += gap;

        // 快捷键
        cv::putText(panel, "Shortcuts", cv::Point(10, y += lh),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
        y += gap;

        const std::vector<std::string> shortcuts = {
            "[0-9] Select tool",
            "[B] Toggle BOOL params",
            "[R] Reset display",
            "[S] Save frame (PNG)",
            "[Space] Start/stop recording (RAW)",
            "[Q]/[Esc] Exit"
        };

        for (const auto& shortcut : shortcuts) {
            cv::putText(panel, shortcut, cv::Point(15, y += lh),
                       cv::FONT_HERSHEY_SIMPLEX, 0.38, cv::Scalar(200, 200, 200), 1);
        }
    }

    void handleKeyboard(int key) {
        // 数字键选择工具
        if (key >= '0' && key <= '9') {
            int tool_idx = key - '0';
            if (tool_idx < static_cast<int>(tools_.size()) && tool_idx != selected_tool_idx_) {
                selected_tool_idx_ = tool_idx;
                refreshToolControls();
                std::cout << "[TOOL] Switched to: " << tools_[selected_tool_idx_].name << std::endl;
            }
            return;
        }

        switch (key) {
            case 'q':
            case 'Q':
            case 27: // ESC
                running_ = false;
                break;

            case 'b':
            case 'B':
                toggleBoolParamsOfSelectedTool();
                break;

            case 'r':
            case 'R':
                resetDisplay();
                std::cout << "[ACTION] Display reset" << std::endl;
                break;

            case 's':
            case 'S':
                saveFrame();
                break;

            case ' ':
                toggleRecording();
                break;

            default:
                break;
        }
    }

    void toggleBoolParamsOfSelectedTool() {
        if (selected_tool_idx_ < 0 || selected_tool_idx_ >= static_cast<int>(tools_.size())) {
            return;
        }

        auto& tool = tools_[selected_tool_idx_];
        if (!tool.available || !tool.tool) {
            std::cout << "[WARN] Selected tool is not available" << std::endl;
            return;
        }

        bool toggled = false;
        for (auto& param : tool.params) {
            if (param.type != fluxeem::ToolParameterType::BOOL) {
                continue;
            }

            param.current_pos = (param.current_pos == 0) ? 1 : 0;
            applyParamChange(tool, param);
            param.last_pos = param.current_pos;
            toggled = true;
        }

        if (!toggled) {
            std::cout << "[INFO] Selected tool has no BOOL parameters" << std::endl;
            return;
        }

        // 立即刷新控制面板，显示最新 BOOL 状态
        refreshToolControls();
    }

    void resetDisplay() {
        std::lock_guard<std::mutex> lock(accumulation_mutex_);
        accumulation_.setTo(cv::Scalar(0, 0, 0));
        window_initialized_ = false;
    }

    void saveFrame() {
        cv::Mat current_image;
        {
            std::lock_guard<std::mutex> lock(display_frame_.mutex);
            if (!display_frame_.image.empty()) {
                current_image = display_frame_.image.clone();
            }
        }

        if (!current_image.empty()) {
            const std::string filename = "frame_" + std::to_string(cv::getTickCount()) + ".png";
            if (cv::imwrite(filename, current_image)) {
                std::cout << "[ACTION] Frame saved to: " << filename << std::endl;
            } else {
                std::cerr << "[ERROR] Failed to save frame" << std::endl;
            }
        }
    }

    void toggleRecording() {
        if (!cam_) {
            return;
        }

        if (!is_recording_) {
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
            const std::string file_path = std::string("recording_control_ui_") + time_buf + ".raw";

            bool ret = cam_->startRecording(file_path);
            if (ret) {
                is_recording_ = true;
                recording_file_path_ = file_path;
                std::cout << "[ACTION] Recording started: " << file_path << std::endl;
            } else {
                std::cerr << "[ERROR] Failed to start recording." << std::endl;
            }
        } else {
            cam_->stopRecording();
            is_recording_ = false;
            std::cout << "[ACTION] Recording stopped: "
                      << recording_file_path_ << std::endl;
            recording_file_path_.clear();
        }
    }

    void cleanup() {
        cv::destroyAllWindows();

        if (cam_) {
            if (is_recording_) {
                cam_->stopRecording();
                std::cout << "[ACTION] Recording stopped on exit." << std::endl;
                is_recording_ = false;
                recording_file_path_.clear();
            }
            cam_->stop();
        }
        std::cout << "\n=== Camera stopped ===" << std::endl;
    }

private:
    std::shared_ptr<fluxeem::ICamera> cam_;
    uint16_t width_{0};
    uint16_t height_{0};

    DisplayFrame display_frame_;
    std::atomic<bool> running_{true};

    // 显示相关
    cv::Mat accumulation_;
    std::mutex accumulation_mutex_;  // 保护 accumulation_ 及相关变量的线程安全
    uint64_t first_timestamp_{0};
    bool window_initialized_{false};
    uint64_t display_time_window_us_{33000};

    // UI相关
    std::string window_name_{"Camera Control - Event View"};
    std::string control_window_name_{"Camera Control Panel"};
    std::string info_window_name_{"Camera Info"};

    // 工具
    std::vector<ToolUI> tools_;
    int selected_tool_idx_{0};

    // 控制面板图例
    std::vector<std::string> trackbar_labels_;  // 轨迹条编号 -> 完整参数名
    cv::Mat legend_image_;                      // 参数名对照表图像

    // 时间
    std::chrono::steady_clock::time_point last_update_time_;

    // 统计与录制
    StatisticsInfo statistics_;
    bool is_recording_{false};
    std::string recording_file_path_;
};

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    try {
        CameraControlUI ui;

        if (!ui.initialize()) {
            return 1;
        }

        ui.run();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 100;
    }
}
