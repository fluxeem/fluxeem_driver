// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
/**
 * @file i_camera.hpp
 * @author Fluxeem
 * @since 1.0.0
 * \~english @brief Abstract camera interface for event-based sensors
 * \~chinese @brief 基于事件的传感器相机抽象接口
 * \~english @details Declares ICamera — the common contract that every event camera driver
 *                  must satisfy. Covers device lifecycle (start / stop / close), synchronous
 *                  event batch retrieval, file recording, tool access and callback registration.
 * \~chinese @details 声明 ICamera —— 每个事件相机驱动必须满足的通用契约。
 *                  涵盖设备生命周期（启动/停止/关闭）、同步事件批次获取、文件录制、
 *                  工具访问和回调注册。
 * @ingroup fluxeem_camera_api
 */

#ifndef I_CAMERA_HPP
#define I_CAMERA_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <fluxeem/base/define/base_define.h>
#include <fluxeem/base/define/camera_types.h>
#include <fluxeem/base/define/event_type.h>
#include <fluxeem/hal/tools/camera_tool.h>
#include <fluxeem/hal/tools/tool_info.h>
#include <fluxeem/base/logging/logger.h>

namespace fluxeem
{

    class EvCameraService;

    /**
     * \~english @brief Streaming state of the camera device
     * \~chinese @brief 相机设备的流式传输状态
     * \~english @details Either STOPPED (idle, no data flowing) or STARTED (actively streaming events)
     * \~chinese @details STOPPED（空闲、无数据流）或 STARTED（正在流式传输事件）
     * @ingroup fluxeem_camera_api
     */
#ifdef _WIN32
    enum FLUXEEM_API CameraStatus
#else
    enum CameraStatus
#endif // _WIN32
    {
        STOPPED = 0,    ///< \~english Device is idle \~chinese 设备空闲
        STARTED = 1     ///< \~english Device is streaming \~chinese 设备正在流式传输
    };

    /**
     * \~english @brief Pure-virtual interface that every Fluxeem event camera must implement
     * \~chinese @brief 每个 Fluxeem 事件相机必须实现的纯虚接口
     * \~english @details Exposes the complete programming surface for an event camera:
     *                  - Lifecycle: start(), stop(), close()
     *                  - Data:      getNextBatch(), registerEventBatchCallback()
     *                  - Recording: startRecording(), stopRecording()
     *                  - Tools:     getTool(), getToolsInfo()
     *                  - Config:    exportCameraConfig(), importCameraConfig()
     *
     *                  Concrete camera drivers derive from this class and override all pure
     *                  virtual methods.
     * \~chinese @details 暴露事件相机的完整编程接口：
     *                  - 生命周期：start()、stop()、close()
     *                  - 数据获取：getNextBatch()、registerEventBatchCallback()
     *                  - 录制：startRecording()、stopRecording()
     *                  - 工具：getTool()、getToolsInfo()
     *                  - 配置：exportCameraConfig()、importCameraConfig()
     *
     *                  具体相机驱动需继承此类并覆写所有纯虚方法。
     * @ingroup fluxeem_camera_api
     */
    class FLUXEEM_API ICamera
    {
    private:
        friend EvCameraService;  ///< \~english Allow service to access internals \~chinese 允许服务类访问内部成员

    public:
        /**
         * \~english @brief Initialize from a camera descriptor
         * \~english @param camera_desc Device metadata (serial, product name, VID/PID, etc.)
         * \~chinese @brief 从相机描述符初始化
         * \~chinese @param camera_desc 设备元数据（序列号、产品名、VID/PID 等）
         */
        ICamera(CameraDescription camera_desc);

        /**
         * \~english @brief Polymorphic destructor
         * \~chinese @brief 多态析构函数
         */
        virtual ~ICamera() = default;

        /**
         * \~english @brief Probe whether the device is still reachable on the bus
         * \~english @return true  — device is present and responding
         * \~english @return false — device has been unplugged or is unresponsive
         * \~chinese @brief 探测设备在总线上是否仍可达
         * \~chinese @return true  — 设备在线且正常响应
         * \~chinese @return false — 设备已拔出或无响应
         */
        virtual bool isConnected() const = 0;

        /**
         * \~english @brief Read-only access to the camera descriptor
         * \~english @return Const reference to the stored CameraDescription
         * \~chinese @brief 只读访问相机描述符
         * \~chinese @return 所存 CameraDescription 的常量引用
         */
        const CameraDescription& getDescription() const
        {
            return camera_desc_;
        }

        /**
         * \~english @brief Install a user callback for event-batch notification
         * \~english @param cb  Callable that receives begin/end iterators of an EventBatch
         * \~english @return Opaque callback handle; pass to unregisterEventBatchCallback() to remove
         * \~chinese @brief 安装用户回调以接收事件批次通知
         * \~chinese @param cb  接收 EventBatch 迭代器的可调用对象
         * \~chinese @return 不透明回调句柄；传给 unregisterEventBatchCallback() 以移除
         * \~english @note Multiple callbacks may be installed; they fire in installation order.
         * \~chinese @note 可安装多个回调；按安装顺序依次触发。
         */
        virtual uint32_t registerEventBatchCallback(EventBatchHandleCallback cb) = 0;

        /**
         * \~english @brief Uninstall a previously installed event-batch callback
         * \~english @param callback_id  Handle returned by registerEventBatchCallback()
         * \~english @return true  — callback found and removed
         * \~english @return false — no such callback
         * \~chinese @brief 卸载之前安装的事件批次回调
         * \~chinese @param callback_id  registerEventBatchCallback() 返回的句柄
         * \~chinese @return true  — 找到并移除
         * \~chinese @return false — 无此回调
         */
        virtual bool unregisterEventBatchCallback(uint32_t callback_id) = 0;

        /**
         * \~english @brief Install a callback for external trigger-input events
         * \~english @param cb  Callable that receives an EventTriggerIn
         * \~english @return Opaque callback handle for later removal
         * \~chinese @brief 安装外部触发输入事件的回调
         * \~chinese @param cb  接收 EventTriggerIn 的可调用对象
         * \~chinese @return 不透明回调句柄，供后续移除
         */
        virtual uint32_t registerTriggerInCallback(EvTriggerInCallback cb) = 0;

        /**
         * \~english @brief Uninstall a trigger-input callback
         * \~english @param callback_id  Handle returned by registerTriggerInCallback()
         * \~english @return true  — callback found and removed
         * \~english @return false — no such callback
         * \~chinese @brief 卸载触发输入回调
         * \~chinese @param callback_id  registerTriggerInCallback() 返回的句柄
         * \~chinese @return true  — 找到并移除
         * \~chinese @return false — 无此回调
         */
        virtual bool unregisterTriggerInCallback(uint32_t callback_id) = 0;

        /**
         * \~english @brief Fetch the next event batch (blocking)
         * \~english @details The batch boundary is determined by the mode set via
         *                  setBatchEventsNum() (count-based) or setBatchEventsTime() (time-based).
         *                  One of these must be called before the first getNextBatch().
         * \~english @param event_batch  [out] Receives the decoded events
         * \~english @return true  — batch populated successfully
         * \~english @return false — no data available (device not started, EOF, etc.)
         * \~chinese @brief 获取下一个事件批次（阻塞）
         * \~chinese @details 批次边界由 setBatchEventsNum()（按数量）或
         *                  setBatchEventsTime()（按时间）设定的模式决定。
         *                  首次调用 getNextBatch() 前必须先设定其中一种模式。
         * \~chinese @param event_batch  [out] 接收解码后的事件
         * \~chinese @return true  — 成功填充批次
         * \~chinese @return false — 无可用数据（设备未启动、EOF 等）
         */
        virtual bool getNextBatch(EventBatch &event_batch) = 0;

        /**
         * \~english @brief Switch batch mode to count-based and set the count threshold
         * \~english @param n  Number of events to accumulate before getNextBatch() returns
         * \~chinese @brief 切换批次模式为按数量，并设置数量阈值
         * \~chinese @param n  getNextBatch() 返回前需累积的事件数量
         * \~english @note Activates N_EVENTS batch condition
         * \~chinese @note 激活 N_EVENTS 批次条件
         */
        virtual void setBatchEventsNum(uint64_t n) = 0;

        /**
         * \~english @brief Switch batch mode to time-based and set the interval
         * \~english @param n  Collection window in microseconds
         * \~chinese @brief 切换批次模式为按时间，并设置时间窗口
         * \~chinese @param n  采集窗口（微秒）
         * \~english @note Activates N_US batch condition
         * \~chinese @note 激活 N_US 批次条件
         */
        virtual void setBatchEventsTime(Timestamp n) = 0;

        /**
         * \~english @brief Begin streaming events from the sensor
         * \~english @return true  — streaming started
         * \~english @return false — failed (device disconnected, already streaming, etc.)
         * \~chinese @brief 开始从传感器流式传输事件
         * \~chinese @return true  — 流式传输已开始
         * \~chinese @return false — 失败（设备断连、已在传输等）
         */
        virtual bool start() = 0;

        /**
         * \~english @brief Stop streaming; sensor enters idle state
         * \~english @return true  — streaming stopped
         * \~english @return false — failed (already idle, etc.)
         * \~chinese @brief 停止流式传输；传感器进入空闲状态
         * \~chinese @return true  — 已停止
         * \~chinese @return false — 失败（已空闲等）
         */
        virtual bool stop() = 0;

        /**
         * \~english @brief Shut down the device and free all OS resources
         * \~english @details After close() the object is no longer usable. Memory, file handles
         *                  and USB interfaces are released.
         * \~english @return true  — device closed cleanly
         * \~english @return false — close failed
         * \~chinese @brief 关闭设备并释放所有操作系统资源
         * \~chinese @details close() 后对象不再可用。内存、文件句柄和 USB 接口被释放。
         * \~chinese @return true  — 设备正常关闭
         * \~chinese @return false — 关闭失败
         */
        virtual bool close() = 0;

        /**
         * \~english @brief Start writing incoming events to a RAW file
         * \~english @param file_path  Destination file (created or overwritten)
         * \~english @return true  — recording started
         * \~english @return false — failed (cannot open file, already recording, etc.)
         * \~chinese @brief 开始将到达的事件写入 RAW 文件
         * \~chinese @param file_path  目标文件（创建或覆盖）
         * \~chinese @return true  — 录制已开始
         * \~chinese @return false — 失败（无法打开文件、已在录制等）
         */
        virtual bool startRecording(const std::string& file_path) = 0;

        /**
         * \~english @brief Configure the file-caching window for long recordings
         * \~english @param sec  Cache duration in seconds
         * \~english @param file_path  Directory for cache files (default: current working directory)
         * \~chinese @brief 配置长时间录制的文件缓存窗口
         * \~chinese @param sec  缓存持续时间（秒）
         * \~chinese @param file_path  缓存文件目录（默认：当前工作目录）
         * \~english @note Caching improves I/O throughput for sustained recording sessions
         * \~chinese @note 缓存可提升持续录制会话的 I/O 吞吐
         */
        virtual void setFileCacheTime(uint16_t sec, const std::string& file_path = ".") {}

        /**
         * \~english @brief Stop the active recording
         * \~english @return true  — recording stopped, file finalized
         * \~english @return false — failed (was not recording)
         * \~chinese @brief 停止当前录制
         * \~chinese @return true  — 录制已停止，文件已收尾
         * \~chinese @return false — 失败（未在录制）
         */
        virtual bool stopRecording() = 0;

        /**
         * \~english @brief Sensor width in pixels
         * \~english @return Horizontal resolution (e.g. 1280 for IMX636)
         * \~chinese @brief 传感器宽度（像素）
         * \~chinese @return 水平分辨率（如 IMX636 为 1280）
         */
        virtual uint16_t getWidth() = 0;

        /**
         * \~english @brief Sensor height in pixels
         * \~english @return Vertical resolution (e.g. 720 for IMX636)
         * \~chinese @brief 传感器高度（像素）
         * \~chinese @return 垂直分辨率（如 IMX636 为 720）
         */
        virtual uint16_t getHeight() = 0;

        /**
         * \~english @brief Install a callback for decode-bandwidth / event-count statistics
         * \~english @param cb  Callable that receives EvCameraStatisticInfo
         * \~chinese @brief 安装解码带宽/事件计数统计回调
         * \~chinese @param cb  接收 EvCameraStatisticInfo 的可调用对象
         * \~english @note Payload includes bandwidth (bytes) and event count
         * \~chinese @note 载荷包含带宽（字节）和事件计数
         */
        virtual void setStatisticsCallback(EvCameraStatisticInfoCallback cb) {}

        /**
         * \~english @brief Enumerate metadata for every tool exposed by this camera
         * \~english @return Vector of ToolInfo entries
         * \~chinese @brief 枚举此相机暴露的所有工具元数据
         * \~chinese @return ToolInfo 条目向量
         * \~english @see ToolInfo, ToolType
         * \~chinese @see ToolInfo, ToolType
         */
        std::vector<ToolInfo> getToolsInfo() const;

        /**
         * \~english @brief Look up tool metadata by its type enum
         * \~english @param type  The requested ToolType
         * \~english @return ToolInfo for that tool
         * \~chinese @brief 按类型枚举查找工具元数据
         * \~chinese @param type  请求的 ToolType
         * \~chinese @return 该工具的 ToolInfo
         * \~english @throws std::runtime_error if the camera does not support this tool type
         * \~chinese @throws std::runtime_error 如果相机不支持此工具类型
         */
        ToolInfo getToolInfo(ToolType type) const;

        /**
         * \~english @brief Obtain a live tool handle by type
         * \~english @param type  The requested ToolType
         * \~english @return Shared pointer to the tool implementation
         * \~chinese @brief 按类型获取工具的活动句柄
         * \~chinese @param type  请求的 ToolType
         * \~chinese @return 指向工具实现的共享指针
         * \~english @throws std::runtime_error if the tool type is unavailable
         * \~chinese @throws std::runtime_error 如果工具类型不可用
         * \~english @note Tools allow runtime reconfiguration (bias, ROI, trigger, etc.)
         * \~chinese @note 工具支持运行时重配（偏置、ROI、触发等）
         */
        std::shared_ptr<CameraTool> getTool(ToolType type) const;

        /**
         * \~english @brief Obtain a live tool handle by its string name
         * \~english @param tool_name  Case-sensitive tool name
         * \~english @return Shared pointer to the tool implementation
         * \~chinese @brief 按字符串名称获取工具的活动句柄
         * \~chinese @param tool_name  区分大小写的工具名称
         * \~chinese @return 指向工具实现的共享指针
         * \~english @throws std::runtime_error if no tool matches the name
         * \~chinese @throws std::runtime_error 如果名称无匹配工具
         */
        std::shared_ptr<CameraTool> getTool(const std::string& tool_name) const;

        /**
         * \~english @brief Write all tool parameters to a JSON file
         * \~english @param json_file_path  Destination file path
         * \~english @return true on success, false on I/O or serialization error
         * \~chinese @brief 将所有工具参数写入 JSON 文件
         * \~chinese @param json_file_path  目标文件路径
         * \~chinese @return 成功返回 true，I/O 或序列化错误返回 false
         * \~english @note Includes bias, ROI, trigger, anti-flicker, etc.
         * \~chinese @note 包含偏置、ROI、触发、抗闪烁等
         */
        bool exportCameraConfig(const std::string& json_file_path);

        /**
         * \~english @brief Load tool parameters from a JSON file
         * \~english @param json_file_path  Source file path
         * \~english @return true on success, false on I/O or parse error
         * \~chinese @brief 从 JSON 文件加载工具参数
         * \~chinese @param json_file_path  源文件路径
         * \~chinese @return 成功返回 true，I/O 或解析错误返回 false
         * \~english @note Applies bias, ROI, trigger, anti-flicker, etc.
         * \~chinese @note 应用偏置、ROI、触发、抗闪烁等
         */
        bool importCameraConfig(const std::string& json_file_path);

        /**
         * \~english @brief Perform over-the-air firmware upgrade from an .img file
         * \~chinese @brief 通过 .img 文件执行 OTA 固件升级
         * @param image_path Path to the .img firmware file.
         * @return true on success, false on failure.
         */
        virtual bool firmwareUpgrade(const std::string &image_path) { return false; }

    protected:
        /**
         * \~english @brief Stored camera descriptor
         * \~chinese @brief 存储的相机描述符
         */
        CameraDescription camera_desc_;

        /**
         * \~english @brief Hardware initialization hook called after the device is opened
         * \~english @details Subclasses perform sensor bring-up, register writes and any
         *                  one-time setup required before event streaming can begin.
         * \~english @return true  — init succeeded
         * \~english @return false — init failed
         * \~chinese @brief 设备打开后的硬件初始化钩子
         * \~chinese @details 子类执行传感器启动、寄存器写入以及事件流式传输前
         *                  所需的任何一次性设置。
         * \~chinese @return true  — 初始化成功
         * \~chinese @return false — 初始化失败
         */
        virtual bool init() = 0;

        /// \~english @brief Tool-type → tool-instance lookup table
        /// \~chinese @brief 工具类型→工具实例查找表
        std::map<ToolType, std::shared_ptr<CameraTool>> tools_;
    };

} // namespace fluxeem

#endif // I_CAMERA_HPP
