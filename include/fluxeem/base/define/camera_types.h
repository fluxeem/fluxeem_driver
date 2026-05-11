// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
/**
 * @file camera_types.h
 * @author Fluxeem
 * @since 1.0.0
 * @brief Camera type definitions and descriptions
 * @brief 相机类型定义和描述
 * @details Defines camera types, stream types, batch conditions and camera description structure
 * @details 定义相机类型、流类型、批处理条件和相机描述结构体
 * @ingroup fluxeem_camera_mgmt
 */

#ifndef __CAMERA_TYPES_H__
#define __CAMERA_TYPES_H__

#include <string>
#include <cstdint>
#include <fluxeem/base/define/base_define.h>

namespace fluxeem   
{    
    /**
     * \~english @brief Strategy for bounding an event batch: by count or by time window
     * \~chinese @brief 事件批次的边界策略：按事件计数或按时间窗口
     * @ingroup fluxeem_camera_api
     */
    enum FLUXEEM_API BatchConditionType
    {
        NO_CONDITION = 0,  ///< \~english No batching constraint \~chinese 无批次约束
        N_EVENTS,          ///< \~english Bound by event count \~chinese 按事件计数截断
        N_US               ///< \~english Bound by time window (µs) \~chinese 按时间窗口截断（微秒）
    };

    /**
     * \~english @brief Device descriptor for a connected camera
     * \~chinese @brief 已连接相机的设备描述符
     * @ingroup fluxeem_camera_mgmt
     */
    struct FLUXEEM_API CameraDescription
    {
        std::string serial;           ///< \~english Device serial string \~chinese 设备序列号
        std::string product;          ///< \~english Product model name \~chinese 产品型号名称
        std::string manufacturer;     ///< \~english Vendor name \~chinese 厂商名称
        uint16_t vid;                 ///< \~english USB Vendor ID \~chinese USB 厂商 ID
        uint16_t pid;                 ///< \~english USB Product ID \~chinese USB 产品 ID
        InterfaceType interface_type; ///< \~english Bus type (USB/MIPI) \~chinese 总线类型（USB/MIPI）
        std::string firmware_version; ///< \~english Firmware revision \~chinese 固件版本号
    };

} // namespace fluxeem

#endif // __CAMERA_TYPES_H__
