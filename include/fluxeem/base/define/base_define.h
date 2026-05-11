// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef __FLUXEEM_BASE_DEFINE_H__
#define __FLUXEEM_BASE_DEFINE_H__

#ifdef _WIN32
    #ifdef FLUXEEM_BUILDING_DLL
        #define FLUXEEM_API __declspec(dllexport)
    #else
        #define FLUXEEM_API __declspec(dllimport)
    #endif
#else
    #define FLUXEEM_API
#endif // _WIN32

#include <cstdint>
#include <chrono>
#include <string>
namespace fluxeem
{
    /**
     * @ingroup fluxeem_base_types
     * @brief 时间戳类型（微秒）
     */
    typedef uint64_t Timestamp;

    /**
     * @ingroup fluxeem_base_types
     * @brief RAW 文件信息结构体
     */
    struct EvFileInfo
    {
        Timestamp start_timestamp;     // Start timestamp of the raw file
        Timestamp end_timestamp;       // End timestamp of the raw file
        uint64_t max_events;           // Maximum number of events in the raw file
        uint16_t width;
        uint16_t height;
        std::string serial_number;
        std::chrono::system_clock::time_point local_time;
    };


    /**
     * @ingroup fluxeem_base_types
     * @brief 流状态枚举
     */
    enum class StreamStatus
    {
        STOP = 0,
        RUNNING = 1,
        SUSPEND = 2
    };

    /**
     * @ingroup fluxeem_base_types
     * @brief 接口类型枚举（USB/MIPI）
     */
    enum class InterfaceType
    {
        USB,
        MIPI
    };
}

#endif // __FLUXEEM_BASE_DEFINE_H__
