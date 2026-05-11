// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
/**
 * @file ev_file_reader.h
 * @author Fluxeem
 * @since 1.0.0
 * \~english @brief Abstract interface for reading event data from files
 * \~chinese @brief 从文件读取事件数据的抽象接口
 * \~english @details This file defines the EvFileReader abstract base class that provides
 *                  a unified interface for reading event data from various file formats.
 *                  It supports:
 *                  - Loading and parsing event files
 *                  - Seeking to specific timestamps or event positions
 *                  - Retrieving events by count or time interval
 *                  - Extracting event data to new files
 *                  - Trigger event callback registration
 *                  
 *                  The class is NOT thread-safe. All methods must be called from
 *                  the same thread. Use factory method createFileReader() to create
 *                  instances of concrete implementations.
 * \~chinese @details 此文件定义了 EvFileReader 抽象基类，为从各种文件格式读取事件数据
 *                  提供统一接口。支持：
 *                  - 加载和解析事件文件
 *                  - 跳转到特定时间戳或事件位置
 *                  - 按数量或时间间隔检索事件
 *                  - 提取事件数据到新文件
 *                  - 触发事件回调注册
 *                  
 *                  此类**不**是线程安全的。所有方法必须由同一线程调用。
 *                  使用工厂方法 createFileReader() 创建具体实现的实例。
 * @ingroup fluxeem_file_reader
 */

#ifndef EV_FILE_READER_H
#define EV_FILE_READER_H

#include <memory>
#include <string>
#include <vector>
#include <fluxeem/base/define/event_type.h>

namespace fluxeem
{
    /**
     * \~english @brief Abstract base class for event file readers
     * \~chinese @brief 事件文件读取器的抽象基类
     * \~english @details EvFileReader provides a unified interface for reading event data
     *                  from files. Concrete implementations (e.g., RawFileReader) handle
     *                  specific file formats. The class uses the Template Method pattern,
     *                  with pure virtual methods that must be implemented by subclasses.
     *                  
     *                  Key features:
     *                  - Sequential and random access to event data
     *                  - Time-based and count-based event retrieval
     *                  - File position management (seek operations)
     *                  - Metadata access (timestamps, event counts, sensor dimensions)
     *                  
     *                  @warning This class is NOT thread-safe. All methods must be called
     *                         from the same thread.
     * \~chinese @details EvFileReader 为从文件读取事件数据提供统一接口。具体实现
     *                  （如 RawFileReader）处理特定文件格式。此类使用模板方法模式，
     *                  具有必须由子类实现的纯虚方法。
     *                  
     *                  主要功能：
     *                  - 事件数据的顺序和随机访问
     *                  - 基于时间和计数的事件检索
     *                  - 文件位置管理（跳转操作）
     *                  - 元数据访问（时间戳、事件计数、传感器尺寸）
     *                  
     *                  @warning 此类**不**是线程安全的。所有方法必须由同一线程调用。
     * @ingroup fluxeem_file_reader
     */
    class FLUXEEM_API EvFileReader
    {
    public:
        /**
         * \~english @brief Default constructor
         * \~chinese @brief 无参构造
         */
        EvFileReader() = default;
        
        /// \~english @brief Copy constructor deleted (non-copyable)
        /// \~chinese @brief 禁用拷贝构造函数（不可拷贝）
        EvFileReader(const EvFileReader&) = delete;
        
        /// \~english @brief Copy assignment operator deleted (non-copyable)
        /// \~chinese @brief 禁用拷贝赋值运算符（不可拷贝）
        EvFileReader& operator=(const EvFileReader&) = delete;
        
        /// \~english @brief Move constructor deleted (non-movable)
        /// \~chinese @brief 禁用移动构造函数（不可移动）
        EvFileReader(EvFileReader&&) = delete;
        
        /// \~english @brief Move assignment operator deleted (non-movable)
        /// \~chinese @brief 禁用移动赋值运算符（不可移动）
        EvFileReader& operator=(EvFileReader&&) = delete;

        /** \~english @brief Virtual destructor
         * \~chinese @brief 多态析构
         */
        virtual ~EvFileReader() = default;

        /**
         * \~english @brief Factory method to create a file reader instance
         * \~english @details This method examines the file extension and creates
         *                  the appropriate file reader implementation. For example,
         *                  .dat files create a RawFileReader instance.
         * \~english @param filepath Path to the event file
         * \~english @return std::unique_ptr<EvFileReader> Unique pointer to the created reader
         * \~english @throws std::runtime_error if file format is not supported
         * \~chinese @brief 创建文件读取器实例的工厂方法
         * \~chinese @details 此方法检查文件扩展名并创建适当的文件读取器实现。
         *                  例如，.dat 文件创建 RawFileReader 实例。
         * \~chinese @param filepath 事件文件路径
         * \~chinese @return std::unique_ptr<EvFileReader> 指向已创建读取器的唯一指针
         * \~chinese @throws std::runtime_error 如果不支持文件格式
         */
        static std::unique_ptr<EvFileReader> createFileReader(const std::string& filepath);

        /**
         * \~english @brief Load an event file for reading
         * \~english @details Opens and parses the specified file, building internal
         *                  data structures for efficient event retrieval. This method
         *                  must be called before any other file operations.
         * \~english @return true if file was loaded successfully
         * \~english @return false if file cannot be opened or is invalid
         * \~english @note This operation may take significant time for large files
         *                as it needs to parse the entire file structure.
         * \~chinese @brief 加载事件文件以进行读取
         * \~chinese @details 打开并解析指定文件，构建用于高效事件检索的内部
         *                  数据结构。必须在任何其他文件操作之前调用此方法。
         * \~chinese @return 如果文件加载成功返回 true
         * \~chinese @return 如果无法打开文件或文件无效返回 false
         * \~chinese @note 对于大文件，此操作可能需要较长时间，
         *                因为需要解析整个文件结构。
         */
        virtual bool open() = 0;

        /**
         * \~english @brief Get the start timestamp of the event stream
         * \~english @details Retrieves the timestamp of the first event in the file.
         *                  This represents the temporal beginning of the recording.
         * \~english @param start_timestamp Reference to store the start timestamp (microseconds)
         * \~english @return true if start timestamp was retrieved successfully
         * \~english @return false if file has no timestamp information
         * \~chinese @brief 获取事件流的起始时间戳
         * \~chinese @details 检索文件中第一个事件的时间戳。
         *                  这表示录制的时间起点。
         * \~chinese @param start_timestamp 引用参数，用于存储起始时间戳（微秒）
         * \~chinese @return 如果成功获取起始时间戳返回 true
         * \~chinese @return 如果文件没有时间戳信息返回 false
         */
        virtual bool getStartTime(Timestamp& start_timestamp) = 0;

        /**
         * \~english @brief Get the end timestamp of the event stream
         * \~english @details Retrieves the timestamp of the last event in the file.
         *                  This represents the temporal end of the recording.
         * \~english @param end_timestamp Reference to store the end timestamp (microseconds)
         * \~english @return true if end timestamp was retrieved successfully
         * \~english @return false if file has no timestamp information
         * \~chinese @brief 获取事件流的结束时间戳
         * \~chinese @details 检索文件中最后一个事件的时间戳。
         *                  这表示录制的时间终点。
         * \~chinese @param end_timestamp 引用参数，用于存储结束时间戳（微秒）
         * \~chinese @return 如果成功获取结束时间戳返回 true
         * \~chinese @return 如果文件没有时间戳信息返回 false
         */
        virtual bool getEndTime(Timestamp& end_timestamp) = 0;

        /**
         * \~english @brief Get the total number of events in the file
         * \~english @details Returns the total count of events from the beginning
         *                  to the end of the file.
         * \~english @param num Reference to store the maximum event count
         * \~english @return true if event count was retrieved successfully
         * \~english @return false if event count cannot be determined
         * \~chinese @brief 获取文件中的事件总数
         * \~chinese @details 返回从文件开始到结束的事件总数。
         * \~chinese @param num 引用参数，用于存储最大事件数量
         * \~chinese @return 如果成功获取事件数量返回 true
         * \~chinese @return 如果无法确定事件数量返回 false
         */
        virtual bool getEventCount(uint64_t &num) = 0;

        /**
         * \~english @brief Check if the end of the event stream has been reached
         * \~english @details Determines whether all events have been read and no
         *                  more data is available for retrieval.
         * \~english @return true if all events have been read
         * \~english @return false if there are still unread events
         * \~chinese @brief 检查是否已到达事件流末尾
         * \~chinese @details 确定是否已读取所有事件且没有更多数据可供检索。
         * \~chinese @return 如果所有事件都已读取返回 true
         * \~chinese @return 如果仍有未读取的事件返回 false
         */
        virtual bool isEndReached() = 0;


        /**
         * \~english @brief Seek to a specific timestamp in the event stream
         * \~english @details Moves the current read position to the event at the
         *                  specified timestamp. If the timestamp is before the start,
         *                  position is set to the beginning. If after the end,
         *                  position is set to the end.
         * \~english @param t Target timestamp to seek to (microseconds)
         * \~english @return true if seek operation was successful
         * \~english @return false if timestamp is invalid or file error occurred
         * \~chinese @brief 跳转到事件流中的特定时间戳
         * \~chinese @details 将当前读取位置移动到指定时间戳的事件。
         *                  如果时间戳在开始之前，位置设置为起点。
         *                  如果在结束之后，位置设置为终点。
         * \~chinese @param t 要跳转的目标时间戳（微秒）
         * \~chinese @return 如果跳转操作成功返回 true
         * \~chinese @return 如果时间戳无效或发生文件错误返回 false
         */
        virtual bool seekToTimestamp(Timestamp t) = 0;

        /**
         * \~english @brief Seek to the Nth event in the file
         * \~english @details Moves the current read position to the event at the
         *                  specified index (0-based). If the index is out of bounds,
         *                  position is set to the beginning or end accordingly.
         * \~english @param n_event Event index to seek to (0 = first event)
         * \~english @return true if seek operation was successful
         * \~english @return false if index is invalid or file error occurred
         * \~chinese @brief 跳转到文件中的第 N 个事件
         * \~chinese @details 将当前读取位置移动到指定索引的事件（从 0 开始）。
         *                  如果索引超出范围，位置会相应设置为起点或终点。
         * \~chinese @param n_event 要跳转的事件索引（0 = 第一个事件）
         * \~chinese @return 如果跳转操作成功返回 true
         * \~chinese @return 如果索引无效或发生文件错误返回 false
         */
        virtual bool seekToEventIndex(uint64_t n_event) = 0;

        /**
         * \~english @brief Get the timestamp at the current read position
         * \~english @details Returns the timestamp of the next event to be read
         *                  from the current file position.
         * \~english @return Timestamp of current position (microseconds)
         * \~english @return 0 if file is at end or position is invalid
         * \~english @note When reading with fixed event count or time interval,
         *                the current file position timestamp may not exactly match
         *                the timestamp of the first event in the returned batch.
         * \~chinese @brief 获取当前读取位置的时间戳
         * \~chinese @details 返回从当前文件位置将要读取的下一个事件的时间戳。
         * \~chinese @return 当前位置的时间戳（微秒）
         * \~chinese @return 如果文件在末尾或位置无效返回 0
         * \~chinese @note 当以固定事件数量或时间间隔读取时，
         *                当前文件位置的时间戳可能与返回批次中第一个事件的时间戳不完全匹配。
         */
        virtual Timestamp getCurrentTimestamp() = 0;

        /**
         * \~english @brief Get the event number at the current read position
         * \~english @details Returns the index (0-based) of the next event to be
         *                  read from the current file position.
         * \~english @return Event number at current position (0 = first event)
         * \~chinese @brief 获取当前读取位置的事件编号
         * \~chinese @details 返回从当前文件位置将要读取的下一个事件的索引（从 0 开始）。
         * \~chinese @return 当前位置的事件编号（0 = 第一个事件）
         */
        virtual uint64_t getCurrentEventIndex() = 0;

        /**
         * \~english @brief Retrieve N events from the current position
         * \~english @details Reads the specified number of events starting from
         *                  the current file position and advances the position.
         * \~english @param n Number of events to retrieve
         * \~english @return Shared pointer to EventBatch containing the events
         * \~english @return nullptr if no events available or end of file reached
         * \~english @warning The returned buffer is reused internally. Do not store
         *                   the pointer across multiple calls as it will be overwritten.
         * \~chinese @brief 从当前位置检索 N 个事件
         * \~chinese @details 从当前文件位置开始读取指定数量的事件并推进位置。
         * \~chinese @param n 要检索的事件数量
         * \~chinese @return 包含事件的 EventBatch 的共享指针
         * \~chinese @return 如果没有可用事件或到达文件末尾返回 nullptr
         * \~chinese @warning 返回的缓冲区在内部重复使用。不要跨多次调用存储
         *                   指针，因为它会被覆盖。
         */
        virtual std::shared_ptr<EventBatch> readEvents(uint64_t n) = 0;

        /**
         * \~english @brief Retrieve N events starting from a given timestamp
         * \~english @details Seeks to the specified timestamp and reads N events.
         *                  The file position is updated to after the retrieved events.
         * \~english @param start Start timestamp (microseconds)
         * \~english @param n Number of events to retrieve
         * \~english @return Shared pointer to EventBatch containing the events
         * \~english @return nullptr if timestamp is invalid or no events available
         * \~english @warning The returned buffer is reused internally. Do not store
         *                   the pointer across multiple calls.
         * \~chinese @brief 从给定时间戳开始检索 N 个事件
         * \~chinese @details 跳转到指定时间戳并读取 N 个事件。
         *                  文件位置更新为检索到的事件之后。
         * \~chinese @param start 起始时间戳（微秒）
         * \~chinese @param n 要检索的事件数量
         * \~chinese @return 包含事件的 EventBatch 的共享指针
         * \~chinese @return 如果时间戳无效或没有可用事件返回 nullptr
         * \~chinese @warning 返回的缓冲区在内部重复使用。不要跨多次调用存储指针。
         */
        virtual std::shared_ptr<EventBatch> readEventsFromTimestamp(Timestamp start, uint64_t n) = 0;

        /**
         * \~english @brief Retrieve N events starting from a given event number
         * \~english @details Seeks to the specified event index and reads N events.
         *                  The file position is updated to after the retrieved events.
         * \~english @param event_num Event number to start from (0-based)
         * \~english @param n Number of events to retrieve
         * \~english @return Shared pointer to EventBatch containing the events
         * \~english @return nullptr if event number is invalid or out of range
         * \~english @warning The returned buffer is reused internally. Do not store
         *                   the pointer across multiple calls.
         * \~chinese @brief 从给定事件编号开始检索 N 个事件
         * \~chinese @details 跳转到指定事件索引并读取 N 个事件。
         *                  文件位置更新为检索到的事件之后。
         * \~chinese @param event_num 开始的事件编号（从 0 开始）
         * \~chinese @param n 要检索的事件数量
         * \~chinese @return 包含事件的 EventBatch 的共享指针
         * \~chinese @return 如果事件编号无效或超出范围返回 nullptr
         * \~chinese @warning 返回的缓冲区在内部重复使用。不要跨多次调用存储指针。
         */
        virtual std::shared_ptr<EventBatch> readEventsFromEventIndex(uint64_t event_num, uint64_t n) = 0;

        /**
         * \~english @brief Retrieve events within a time interval from current position
         * \~english @details Reads all events that fall within the specified time
         *                  interval starting from the current file position.
         * \~english @param interval Time interval in microseconds
         * \~english @return Shared pointer to EventBatch containing the events
         * \~english @return nullptr if no events in interval or file error
         * \~english @warning The returned buffer is reused internally. Do not store
         *                   the pointer across multiple calls.
         * \~chinese @brief 从当前位置检索时间间隔内的事件
         * \~chinese @details 读取从当前文件位置开始在指定时间间隔内的所有事件。
         * \~chinese @param interval 时间间隔（微秒）
         * \~chinese @return 包含事件的 EventBatch 的共享指针
         * \~chinese @return 如果间隔内没有事件或文件错误返回 nullptr
         * \~chinese @warning 返回的缓冲区在内部重复使用。不要跨多次调用存储指针。
         */
        virtual std::shared_ptr<EventBatch> readEventsByTimeInterval(Timestamp interval) = 0;

        /**
         * \~english @brief Retrieve events within a time interval from given timestamp
         * \~english @details Seeks to the specified timestamp and reads all events
         *                  within the given time interval.
         * \~english @param start Start timestamp (microseconds)
         * \~english @param interval Time interval in microseconds
         * \~english @return Shared pointer to EventBatch containing the events
         * \~english @return nullptr if timestamp invalid or no events in interval
         * \~english @warning The returned buffer is reused internally. Do not store
         *                   the pointer across multiple calls.
         * \~chinese @brief 从给定时间戳开始检索时间间隔内的事件
         * \~chinese @details 跳转到指定时间戳并读取给定时间间隔内的所有事件。
         * \~chinese @param start 起始时间戳（微秒）
         * \~chinese @param interval 时间间隔（微秒）
         * \~chinese @return 包含事件的 EventBatch 的共享指针
         * \~chinese @return 如果时间戳无效或间隔内没有事件返回 nullptr
         * \~chinese @warning 返回的缓冲区在内部重复使用。不要跨多次调用存储指针。
         */
        virtual std::shared_ptr<EventBatch> readEventsByTimeIntervalFromTimestamp(Timestamp start, Timestamp interval) = 0;

        /**
         * \~english @brief Retrieve events within a time interval from given event number
         * \~english @details Seeks to the specified event number and reads all events
         *                  within the given time interval.
         * \~english @param event_num Event number to start from (0-based)
         * \~english @param interval Time interval in microseconds
         * \~english @return Shared pointer to EventBatch containing the events
         * \~english @return nullptr if event number invalid or no events in interval
         * \~english @warning The returned buffer is reused internally. Do not store
         *                   the pointer across multiple calls.
         * \~chinese @brief 从给定事件编号开始检索时间间隔内的事件
         * \~chinese @details 跳转到指定事件编号并读取给定时间间隔内的所有事件。
         * \~chinese @param event_num 开始的事件编号（从 0 开始）
         * \~chinese @param interval 时间间隔（微秒）
         * \~chinese @return 包含事件的 EventBatch 的共享指针
         * \~chinese @return 如果事件编号无效或间隔内没有事件返回 nullptr
         * \~chinese @warning 返回的缓冲区在内部重复使用。不要跨多次调用存储指针。
         */
        virtual std::shared_ptr<EventBatch> readEventsByTimeIntervalFromEventIndex(uint64_t event_num, Timestamp interval) = 0;

        /**
         * \~english @brief Extract event data within a time range to a file
         * \~english @details Reads all events between the start and end timestamps
         *                  and saves them to the specified output file. The output
         *                  directory must already exist.
         * \~english @param start Start timestamp (microseconds)
         * \~english @param end End timestamp (microseconds)
         * \~english @param out_file_path Path to output file (directory must exist)
         * \~english @return true if extraction was successful
         * \~english @return false if file I/O error or invalid parameters
         * \~chinese @brief 提取时间范围内的事件数据到文件
         * \~chinese @details 读取起始和结束时间戳之间的所有事件并保存到指定输出文件。
         *                  输出目录必须已存在。
         * \~chinese @param start 起始时间戳（微秒）
         * \~chinese @param end 结束时间戳（微秒）
         * \~chinese @param out_file_path 输出文件路径（目录必须存在）
         * \~chinese @return 如果提取成功返回 true
         * \~chinese @return 如果文件 I/O 错误或参数无效返回 false
         */
        virtual bool exportEventsToRaw(Timestamp start, Timestamp end, const std::string& out_file_path) = 0;

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
         * \~english @brief Get the width of the sensor that captured the events
         * \~english @return Sensor width in pixels (e.g., 1280 for IMX636)
         * \~chinese @brief 获取捕获事件的传感器宽度
         * \~chinese @return 传感器宽度（像素），例如 IMX636 为 1280
         */
        virtual uint16_t getWidth() const = 0;

        /**
         * \~english @brief Get the height of the sensor that captured the events
         * \~english @return Sensor height in pixels (e.g., 720 for IMX636)
         * \~chinese @brief 获取捕获事件的传感器高度
         * \~chinese @return 传感器高度（像素），例如 IMX636 为 720
         */
        virtual uint16_t getHeight() const = 0;

        /**
         * \~english @brief Get decode-side statistics for the most recent read/decode window
         * \~english @details Returns bandwidth bytes and decoded event count accumulated
         *                  by the decoder during the latest data read/decode operation.
         *                  Implementations that do not support decode-side statistics
         *                  should return false.
         * \~english @param bandwidth_bytes Output decoded raw bytes in the latest window
         * \~english @param events_count Output decoded event count in the latest window
         * \~english @return true if statistics are available
         * \~english @return false if not supported
         * \~chinese @brief 获取最近一次读取/解码窗口的解码侧统计信息
         * \~chinese @details 返回解码器在最近一次数据读取/解码操作中累计的
         *                  带宽字节数与解码事件数。不支持该能力的实现返回 false。
         * \~chinese @param bandwidth_bytes 输出最近窗口解码的原始字节数
         * \~chinese @param events_count 输出最近窗口解码的事件数量
         * \~chinese @return 若统计可用返回 true
         * \~chinese @return 若不支持返回 false
         */
        virtual bool getDecodeStatistics(uint64_t& bandwidth_bytes, uint64_t& events_count)
        {
            bandwidth_bytes = 0;
            events_count = 0;
            return false;
        }

        /**
         * \~english @brief Get information about the loaded file
         * \~english @param file_info Reference to store file information
         * \~english @return true if file info is available
         * \~english @return false if file is not loaded or info unavailable
         * \~chinese @brief 获取已加载文件的信息
         * \~chinese @param file_info 引用参数，用于存储文件信息
         * \~chinese @return 如果文件信息可用返回 true
         * \~chinese @return 如果文件未加载或信息不可用返回 false
         */
        virtual bool getFileMetadata(EvFileInfo& file_info) { return false; }
    };

    /**
     * \~english @brief Smart pointer type to manage event file reader instances
     * \~english @details This is a unique_ptr to EvFileReader that automatically
     *                  manages the lifetime of file reader objects. When the
     *                  EvFile goes out of scope, the file is automatically closed.
     * \~chinese @brief 用于管理事件文件读取器实例的智能指针类型
     * \~chinese @details 这是指向 EvFileReader 的唯一指针，自动管理文件读取器
     *                  对象的生命周期。当 EvFile 超出作用域时，文件会自动关闭。
     * @ingroup fluxeem_file_reader
     */
    using EvFile = std::unique_ptr<EvFileReader>;

} // namespace fluxeem

#endif // EV_FILE_READER_H