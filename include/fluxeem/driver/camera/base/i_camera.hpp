/**
 * @file i_camera.hpp
 * \~english @brief Interface definition for event cameras
 * \~chinese @brief 事件相机的接口定义
 * \~english @details This file defines the abstract interface class ICamera that provides
 *                  common functionality for all event-based cameras. It includes methods for
 *                  camera control (start/stop/close), event data acquisition, recording,
 *                  and access to camera tools (bias adjustment, ROI, trigger, etc.).
 * \~chinese @details 此文件定义了抽象接口类 ICamera，为所有基于事件的相机提供通用功能。
 *                  包括相机控制（启动/停止/关闭）、事件数据获取、录制以及访问相机工具
 *                  （偏置调整、ROI、触发器等）的方法。
 * @ingroup camera_interface
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
     * \~english @brief Camera operation status
     * \~chinese @brief 相机操作状态
     * \~english @details Indicates whether the camera is stopped or started
     * \~chinese @details 指示相机是已停止还是已启动
     * @ingroup camera_interface
     */
#ifdef _WIN32
    enum FLUXEEM_API CameraStatus
#else
    enum CameraStatus
#endif // _WIN32
    {
        STOPPED = 0,    ///< \~english Camera is stopped (not streaming) \~chinese 相机已停止（未采集数据）
        STARTED = 1     ///< \~english Camera is started (streaming events) \~chinese 相机已启动（正在采集事件）
    };

    /**
     * \~english @brief Abstract interface class for event cameras
     * \~chinese @brief 事件相机的抽象接口类
     * \~english @details This class defines the common interface for all event-based cameras.
     *                  It provides:
     *                  - Camera lifecycle management (start, stop, close)
     *                  - Event data acquisition (synchronous batch retrieval)
     *                  - Event recording to file
     *                  - Access to camera configuration tools
     *                  - Callback registration for event processing
     *                  
     *                  All camera implementations must inherit from this class and implement
     *                  the pure virtual methods.
     * \~chinese @details 此类为所有基于事件的相机定义通用接口。提供：
     *                  - 相机生命周期管理（启动、停止、关闭）
     *                  - 事件数据获取（同步批次检索）
     *                  - 事件录制到文件
     *                  - 访问相机配置工具
     *                  - 事件处理的回调注册
     *                  
     *                  所有相机实现必须继承此类并实现纯虚方法。
     * @ingroup camera_interface
     */
    class FLUXEEM_API ICamera
    {
    private:
        friend EvCameraService;  ///< \~english EvCameraService can access private members \~chinese EvCameraService 可以访问私有成员

    public:
        /**
         * \~english @brief Constructor
         * \~english @param camera_desc Camera description containing serial number, product info, etc.
         * \~chinese @brief 构造函数
         * \~chinese @param camera_desc 相机描述，包含序列号、产品信息等
         */
        ICamera(CameraDescription camera_desc);

        /**
         * \~english @brief Virtual destructor
         * \~chinese @brief 虚析构函数
         */
        virtual ~ICamera() = default;

        /**
         * \~english @brief Check if the camera is physically connected
         * \~english @return true if camera is connected and responsive
         * \~english @return false if camera is disconnected or unresponsive
         * \~chinese @brief 检查相机是否物理连接
         * \~chinese @return 如果相机已连接且响应正常返回 true
         * \~chinese @return 如果相机断开连接或无响应返回 false
         */
        virtual bool isConnected() const = 0;

        /**
         * \~english @brief Get camera description
         * \~english @return Reference to CameraDescription containing camera metadata
         * \~chinese @brief 获取相机描述
         * \~chinese @return CameraDescription 的引用，包含相机元数据
         */
        const CameraDescription& getDescription() const
        {
            return camera_desc_;
        }

        /**
         * \~english @brief Register a callback function for event batch processing
         * \~english @param cb Callback function that receives event batch iterators
         * \~english @return Unique callback ID for later removal via unregisterEventBatchCallback()
         * \~chinese @brief 注册事件批次处理的回调函数
         * \~chinese @param cb 回调函数，接收事件批次迭代器
         * \~chinese @return 唯一的回调 ID，用于后续通过 unregisterEventBatchCallback() 移除
         * \~english @note Multiple callbacks can be registered. They are called in registration order.
         * \~chinese @note 可以注册多个回调。它们按注册顺序调用。
         */
        virtual uint32_t registerEventBatchCallback(EventBatchHandleCallback cb) = 0;

        /**
         * \~english @brief Remove a previously registered event batch callback
         * \~english @param callback_id The callback ID returned by registerEventBatchCallback()
         * \~english @return true if callback was found and removed successfully
         * \~english @return false if no callback with the given ID exists
         * \~chinese @brief 移除之前注册的事件批次回调
         * \~chinese @param callback_id registerEventBatchCallback() 返回的回调 ID
         * \~chinese @return 如果找到并成功移除回调返回 true
         * \~chinese @return 如果不存在给定 ID 的回调返回 false
         */
        virtual bool unregisterEventBatchCallback(uint32_t callback_id) = 0;

        /**
         * \~english @brief Register a callback function for trigger input events
         * \~english @param cb Callback function that receives EventTriggerIn objects
         * \~english @return Unique callback ID for later removal via unregisterTriggerInCallback()
         * \~chinese @brief 注册触发输入事件的回调函数
         * \~chinese @param cb 回调函数，接收 EventTriggerIn 对象
         * \~chinese @return 唯一的回调 ID，用于后续通过 unregisterTriggerInCallback() 移除
         */
        virtual uint32_t registerTriggerInCallback(EvTriggerInCallback cb) = 0;

        /**
         * \~english @brief Remove a previously registered trigger input callback
         * \~english @param callback_id The callback ID returned by registerTriggerInCallback()
         * \~english @return true if callback was found and removed successfully
         * \~english @return false if no callback with the given ID exists
         * \~chinese @brief 移除之前注册的触发输入回调
         * \~chinese @param callback_id registerTriggerInCallback() 返回的回调 ID
         * \~chinese @return 如果找到并成功移除回调返回 true
         * \~chinese @return 如果不存在给定 ID 的回调返回 false
         */
        virtual bool unregisterTriggerInCallback(uint32_t callback_id) = 0;

        /**
         * \~english @brief Synchronously retrieve the next batch of events
         * \~english @details Before calling this method, you must configure the batch condition using either:
         *                  - setBatchEventsNum() to specify number of events per batch, or
         *                  - setBatchEventsTime() to specify time interval per batch
         * \~english @param event_batch Output parameter to store the retrieved events
         * \~english @return true if events were retrieved successfully
         * \~english @return false if no events are available (e.g., camera not started or end of file)
         * \~chinese @brief 同步获取下一批事件
         * \~chinese @details 在调用此方法之前，必须使用以下方法之一配置批次条件：
         *                  - setBatchEventsNum() 指定每批次的事件数量，或
         *                  - setBatchEventsTime() 指定每批次的时间间隔
         * \~chinese @param event_batch 输出参数，用于存储获取的事件
         * \~chinese @return 如果成功获取事件返回 true
         * \~chinese @return 如果没有可用事件（例如相机未启动或文件结束）返回 false
         */
        virtual bool getNextBatch(EventBatch &event_batch) = 0;

        /**
         * \~english @brief Set the number of events per batch for getNextBatch()
         * \~english @param n Number of events to collect before returning from getNextBatch()
         * \~chinese @brief 设置 getNextBatch() 每批次的事件数量
         * \~chinese @param n 在 getNextBatch() 返回之前要收集的事件数量
         * \~english @note This sets the batch condition to N_EVENTS mode
         * \~chinese @note 这会将批次条件设置为 N_EVENTS 模式
         */
        virtual void setBatchEventsNum(uint64_t n) = 0;

        /**
         * \~english @brief Set the time interval per batch for getNextBatch()
         * \~english @param n Time interval in microseconds to collect before returning from getNextBatch()
         * \~chinese @brief 设置 getNextBatch() 每批次的时间间隔
         * \~chinese @param n 在 getNextBatch() 返回之前要收集的时间间隔（微秒）
         * \~english @note This sets the batch condition to N_US mode
         * \~chinese @note 这会将批次条件设置为 N_US 模式
         */
        virtual void setBatchEventsTime(Timestamp n) = 0;

        /**
         * \~english @brief Start the camera and begin event streaming
         * \~english @return true if camera started successfully
         * \~english @return false if start failed (e.g., device not connected, already started)
         * \~chinese @brief 启动相机并开始事件流采集
         * \~chinese @return 如果相机成功启动返回 true
         * \~chinese @return 如果启动失败返回 false（例如设备未连接、已启动）
         */
        virtual bool start() = 0;

        /**
         * \~english @brief Stop the camera and halt event streaming
         * \~english @return true if camera stopped successfully
         * \~english @return false if stop failed (e.g., already stopped)
         * \~chinese @brief 停止相机并停止事件流采集
         * \~chinese @return 如果相机成功停止返回 true
         * \~chinese @return 如果停止失败返回 false（例如已停止）
         */
        virtual bool stop() = 0;

        /**
         * \~english @brief Close the camera and release all resources
         * \~english @details This method permanently closes the camera device and releases
         *                  all associated resources (memory, file handles, etc.).
         *                  The camera object should not be used after calling close().
         * \~english @return true if camera closed successfully
         * \~english @return false if close failed
         * \~chinese @brief 关闭相机并释放所有资源
         * \~chinese @details 此方法永久关闭相机设备并释放所有关联资源（内存、文件句柄等）。
         *                  调用 close() 后不应再使用相机对象。
         * \~chinese @return 如果相机成功关闭返回 true
         * \~chinese @return 如果关闭失败返回 false
         */
        virtual bool close() = 0;

        /**
         * \~english @brief Start recording events to a file
         * \~english @param file_path Path to the output file (will be created or overwritten)
         * \~english @return true if recording started successfully
         * \~english @return false if start failed (e.g., file cannot be created, already recording)
         * \~chinese @brief 开始录制事件到文件
         * \~chinese @param file_path 输出文件路径（将被创建或覆盖）
         * \~chinese @return 如果开始录制成功返回 true
         * \~chinese @return 如果开始失败返回 false（例如无法创建文件、正在录制中）
         */
        virtual bool startRecording(const std::string& file_path) = 0;

        /**
         * \~english @brief Set file cache time for recording
         * \~english @param sec Cache duration in seconds
         * \~english @param file_path Directory path for cache files (default: current directory)
         * \~chinese @brief 设置录制的文件缓存时间
         * \~chinese @param sec 缓存持续时间（秒）
         * \~chinese @param file_path 缓存文件目录路径（默认：当前目录）
         * \~english @note Cache files are used to improve recording performance for long sessions
         * \~chinese @note 缓存文件用于提高长时间录制的性能
         */
        virtual void setFileCacheTime(uint16_t sec, const std::string& file_path = ".") {}

        /**
         * \~english @brief Stop recording events
         * \~english @return true if recording stopped successfully
         * \~english @return false if stop failed (e.g., not recording)
         * \~chinese @brief 停止录制事件
         * \~chinese @return 如果停止录制成功返回 true
         * \~chinese @return 如果停止失败返回 false（例如未录制中）
         */
        virtual bool stopRecording() = 0;

        /**
         * \~english @brief Get the width of the camera sensor in pixels
         * \~english @return Sensor width in pixels (e.g., 1280 for IMX636)
         * \~chinese @brief 获取相机传感器的宽度（像素）
         * \~chinese @return 传感器宽度（像素），例如 IMX636 为 1280
         */
        virtual uint16_t getWidth() = 0;

        /**
         * \~english @brief Get the height of the camera sensor in pixels
         * \~english @return Sensor height in pixels (e.g., 720 for IMX636)
         * \~chinese @brief 获取相机传感器的高度（像素）
         * \~chinese @return 传感器高度（像素），例如 IMX636 为 720
         */
        virtual uint16_t getHeight() = 0;

        /**
         * \~english @brief Set callback function for receiving decode statistics
         * \~english @param cb Callback function that receives EvCameraStatisticInfo objects
         * \~chinese @brief 设置接收解码统计信息的回调函数
         * \~chinese @param cb 回调函数，接收 EvCameraStatisticInfo 对象
         * \~english @note Statistics include bandwidth and event count information
         * \~chinese @note 统计信息包括带宽和事件数量信息
         */
        virtual void setStatisticsCallback(EvCameraStatisticInfoCallback cb) {}

        /**
         * \~english @brief Get information about all available camera tools
         * \~english @return Vector of ToolInfo structures describing each tool
         * \~chinese @brief 获取所有可用相机工具的信息
         * \~chinese @return ToolInfo 结构的向量，描述每个工具
         * \~english @see ToolInfo, ToolType
         * \~chinese @see ToolInfo, ToolType
         */
        std::vector<ToolInfo> getToolsInfo() const;

        /**
         * \~english @brief Get information about a specific tool by type
         * \~english @param type The tool type enum value
         * \~english @return ToolInfo structure describing the tool
         * \~chinese @brief 按类型获取特定工具的信息
         * \~chinese @param type 工具类型枚举值
         * \~chinese @return 描述工具的 ToolInfo 结构
         * \~english @throws std::runtime_error if tool type is not available
         * \~chinese @throws std::runtime_error 如果工具类型不可用
         */
        ToolInfo getToolInfo(ToolType type) const;

        /**
         * \~english @brief Get a tool instance by type
         * \~english @param type The tool type enum value
         * \~english @return Shared pointer to the CameraTool instance
         * \~chinese @brief 按类型获取工具实例
         * \~chinese @param type 工具类型枚举值
         * \~chinese @return 指向 CameraTool 实例的共享指针
         * \~english @throws std::runtime_error if tool type is not available
         * \~chinese @throws std::runtime_error 如果工具类型不可用
         * \~english @note Tools are used to configure camera parameters (bias, ROI, trigger, etc.)
         * \~chinese @note 工具用于配置相机参数（偏置、ROI、触发器等）
         */
        std::shared_ptr<CameraTool> getTool(ToolType type) const;

        /**
         * \~english @brief Get a tool instance by name
         * \~english @param tool_name The tool name string (case-sensitive)
         * \~english @return Shared pointer to the CameraTool instance
         * \~chinese @brief 按名称获取工具实例
         * \~chinese @param tool_name 工具名称字符串（区分大小写）
         * \~chinese @return 指向 CameraTool 实例的共享指针
         * \~english @throws std::runtime_error if tool name is not found
         * \~chinese @throws std::runtime_error 如果找不到工具名称
         */
        std::shared_ptr<CameraTool> getTool(const std::string& tool_name) const;

        /**
         * \~english @brief Export camera configuration parameters to a JSON file
         * \~english @param json_file_path Path to the output JSON file
         * \~english @return true if export was successful
         * \~english @return false if export failed (e.g., file cannot be written)
         * \~chinese @brief 将相机配置参数导出到 JSON 文件
         * \~chinese @param json_file_path 输出 JSON 文件的路径
         * \~chinese @return 如果导出成功返回 true
         * \~chinese @return 如果导出失败返回 false（例如无法写入文件）
         * \~english @note Exports tool parameters, bias settings, ROI configuration, etc.
         * \~chinese @note 导出工具参数、偏置设置、ROI 配置等
         */
        bool exportCameraConfig(const std::string& json_file_path);

        /**
         * \~english @brief Import camera configuration parameters from a JSON file
         * \~english @param json_file_path Path to the input JSON file
         * \~english @return true if import was successful
         * \~english @return false if import failed (e.g., file cannot be read, invalid format)
         * \~chinese @brief 从 JSON 文件导入相机配置参数
         * \~chinese @param json_file_path 输入 JSON 文件的路径
         * \~chinese @return 如果导入成功返回 true
         * \~chinese @return 如果导入失败返回 false（例如无法读取文件、格式无效）
         * \~english @note Imports tool parameters, bias settings, ROI configuration, etc.
         * \~chinese @note 导入工具参数、偏置设置、ROI 配置等
         */
        bool importCameraConfig(const std::string& json_file_path);

        /**
         * \~english @brief OTA firmware upgrade from .img file.
         * \~chinese @brief 通过 .img 文件进行OTA固件升级。
         * @param image_path Path to the .img firmware file.
         * @return true on success, false on failure.
         */
        virtual bool firmwareUpgrade(const std::string &image_path) { return false; }

    protected:
        /**
         * \~english @brief Camera description containing metadata
         * \~chinese @brief 相机描述，包含元数据
         */
        CameraDescription camera_desc_;

        /**
         * \~english @brief Initialize the camera after opening
         * \~english @details This method is called internally when the camera is opened.
         *                  It should perform hardware initialization, register configuration,
         *                  and any other setup required before streaming.
         * \~english @return true if initialization was successful
         * \~english @return false if initialization failed
         * \~chinese @brief 打开相机后进行初始化
         * \~chinese @details 此方法在打开相机时内部调用。
         *                  应执行硬件初始化、寄存器配置以及流式传输之前所需的任何其他设置。
         * \~chinese @return 如果初始化成功返回 true
         * \~chinese @return 如果初始化失败返回 false
         */
        virtual bool init() = 0;

        // tools
        /// \~english @brief Map of tool type to tool instance
        /// \~chinese @brief 工具类型到工具实例的映射
        std::map<ToolType, std::shared_ptr<CameraTool>> tools_;
    };

} // namespace fluxeem

#endif // I_CAMERA_HPP
