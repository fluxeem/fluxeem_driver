// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
/**
 * @file event_type.h
 * @author Fluxeem
 * @since 1.0.0
 * @brief Event type definitions for Event cameras
 * @brief 事件相机的事件类型定义
 * @details Defines base event classes and specific event types (2D events, trigger events, statistics)
 * @details 定义基础事件类和具体事件类型（2D 事件、触发事件、统计信息）
 * @ingroup fluxeem_event_types
 */

#ifndef __EVENT_TYPES_HPP__
#define __EVENT_TYPES_HPP__

#include <cstddef>
#include <cstdint>
#include <vector>
#include <functional>
#include <fluxeem/base/define/base_define.h>

namespace fluxeem
{
    /**
     * \~english @brief Base class for all event types
     * \~chinese @brief 所有事件类型的基类
     * @ingroup fluxeem_event_types
     */
    class FLUXEEM_API EventBase
    {
    };

    /**
     * \~english @brief 2D event class containing x, y coordinates, polarity and timestamp
     * \~chinese @brief 包含 x、y 坐标、极性和时间戳的 2D 事件类
     * @ingroup fluxeem_event_types
     */
    class FLUXEEM_API Event2D : public EventBase
    {
    public:
        /**
         * \~english @brief Constructor with parameters
         * \~english @param x X coordinate (0-2047)
         * \~english @param y Y coordinate (0-2047)
         * \~english @param polarity Event polarity (0: decrease, 1: increase)
         * \~english @param timestamp Event timestamp in microseconds
         * \~chinese @brief 带参数的构造函数
         * \~chinese @param x X 坐标（0-2047）
         * \~chinese @param y Y 坐标（0-2047）
         * \~chinese @param polarity 事件极性（0：下降沿，1：上升沿）
         * \~chinese @param timestamp 事件时间戳（微秒）
         */
        Event2D(
            uint16_t x,
            uint16_t y,
            uint8_t polarity,
            Timestamp timestamp) : timestamp(timestamp), x(x), y(y), polarity(polarity) {}
        
        /**
         * \~english @brief Default constructor
         * \~chinese @brief 默认构造函数
         */
        Event2D() : timestamp(0), x(0), y(0), polarity(0) {}
        
        /// \~english @brief Timestamp in microseconds
        /// \~chinese @brief 时间戳（微秒）
        Timestamp timestamp;
        
        /// \~english @brief X coordinate
        /// \~chinese @brief X 坐标
        uint16_t x;
        
        /// \~english @brief Y coordinate
        /// \~chinese @brief Y 坐标
        uint16_t y;
        
        /// \~english @brief Event polarity (0: decrease, 1: increase)
        /// \~chinese @brief 事件极性（0：下降沿，1：上升沿）
        uint8_t polarity;
    };

    /**
     * \~english @brief Trigger input event class
     * \~chinese @brief 触发输入事件类
     * @ingroup fluxeem_event_types
     */
    class FLUXEEM_API EventTriggerIn : public EventBase
    {
    public:
        /**
         * \~english @brief Constructor with parameters
         * \~english @param id Trigger ID
         * \~english @param polarity Trigger polarity
         * \~english @param timestamp Trigger timestamp
         * \~chinese @brief 带参数的构造函数
         * \~chinese @param id 触发器 ID
         * \~chinese @param polarity 触发极性
         * \~chinese @param timestamp 触发时间戳
         */
        EventTriggerIn(
            short id,
            short polarity,
            Timestamp timestamp) : id(id), polarity(polarity), timestamp(timestamp) {}
        
        /**
         * \~english @brief Default constructor
         * \~chinese @brief 默认构造函数
         */
        EventTriggerIn() : id(0), polarity(0), timestamp(0) {}
        
        /// \~english @brief Trigger polarity
        /// \~chinese @brief 触发极性
        short polarity;
        
        /// \~english @brief Trigger timestamp
        /// \~chinese @brief 触发时间戳
        Timestamp timestamp;
        
        /// \~english @brief Trigger ID
        /// \~chinese @brief 触发器 ID
        short id;
    };

    /**
     * \~english @brief Statistics information class for data stream
     * \~chinese @brief 数据流的统计信息类
     * @ingroup fluxeem_event_types
     */
    class FLUXEEM_API EvCameraStatisticInfo : public EventBase
    {
    public:
        /**
         * \~english @brief Constructor with parameters
         * \~english @param bandwidth_byte Bandwidth in bytes
         * \~english @param events_count Number of events
         * \~chinese @brief 带参数的构造函数
         * \~chinese @param bandwidth_byte 带宽（字节）
         * \~chinese @param events_count 事件数量
         */
        EvCameraStatisticInfo(
            uint64_t bandwidth_byte,
            uint64_t events_count) : bandwidth_byte(bandwidth_byte), events_count(events_count) {}
        
        /**
         * \~english @brief Default constructor
         * \~chinese @brief 默认构造函数
         */
        EvCameraStatisticInfo() : bandwidth_byte(0), events_count(0){}
        
        /// \~english @brief Bandwidth in bytes
        /// \~chinese @brief 带宽（字节）
        uint64_t bandwidth_byte;
        
        /// \~english @brief Number of events
        /// \~chinese @brief 事件数量
        uint64_t events_count;
    };

    /**
     * \~english @brief Vector of 2D events (event batch)
     * \~chinese @brief 2D 事件向量（事件批次）
     * @ingroup fluxeem_event_types
     */
    using EventBatch = std::vector<Event2D>;

    /**
     * \~english @brief Event iterator type
     * \~chinese @brief 事件迭代器类型
     * @ingroup fluxeem_event_types
     */
    using EventIterator_t = Event2D*;

    /**
     * \~english @brief Callback function type for event batch processing
     * \~english @param begin Iterator to first event in batch
     * \~english @param end Iterator past last event in batch
     * \~chinese @brief 事件批次处理的回调函数类型
     * \~chinese @param begin 批次中第一个事件的迭代器
     * \~chinese @param end 批次最后一个事件之后的迭代器
     * @ingroup fluxeem_event_types
     */
    using EventBatchHandleCallback = std::function<void(EventIterator_t begin, EventIterator_t end)>;

    /**
     * \~english @brief Callback function type for trigger input events
     * \~chinese @brief 触发输入事件的回调函数类型
     * @ingroup fluxeem_event_types
     */
    using EvTriggerInCallback = std::function<void(const EventTriggerIn&)>;

    /**
     * \~english @brief Callback function type for statistics information
     * \~chinese @brief 统计信息的回调函数类型
     * @ingroup fluxeem_event_types
     */
    using EvCameraStatisticInfoCallback = std::function<void(const EvCameraStatisticInfo&)>; 
} // namespace fluxeem

#endif // __EVENT_TYPES_HPP__
