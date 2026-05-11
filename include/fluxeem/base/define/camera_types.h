/**
 * @file camera_types.h
 * @brief Camera type definitions and descriptions
 * @brief 相机类型定义和描述
 * @details Defines camera types, stream types, batch conditions and camera description structure
 * @details 定义相机类型、流类型、批处理条件和相机描述结构体
 * @ingroup camera_management
 */

#ifndef __CAMERA_TYPES_H__
#define __CAMERA_TYPES_H__

#include <string>
#include <cstdint>
#include <fluxeem/base/define/base_define.h>

namespace fluxeem   
{    
    /**
     * \~english @brief Condition to cut the batch of events, n_events or n_us
     * \~chinese @brief 事件批次的切分条件：按事件数量或按时间
     * @ingroup camera_interface
     */
    enum FLUXEEM_API BatchConditionType
    {
        NO_CONDITION = 0,  ///< \~english No batching condition \~chinese 无批处理条件
        N_EVENTS,          ///< \~english Batch by number of events \~chinese 按事件数量分批
        N_US               ///< \~english Batch by time interval (microseconds) \~chinese 按时间间隔分批（微秒）
    };

    /**
     * \~english @brief A struct to describe the camera information
     * \~chinese @brief 描述相机信息的结构体
     * @ingroup camera_management
     */
    // TODO: Make Description class abstract according to interfaces
    struct FLUXEEM_API CameraDescription
    {
        std::string serial;           ///< \~english Camera serial number \~chinese 相机序列号
        std::string product;          ///< \~english Product name \~chinese 产品名称
        std::string manufacturer;     ///< \~english Manufacturer name \~chinese 制造商名称
        uint16_t vid;                 ///< \~english USB Vendor ID \~chinese USB 厂商 ID
        uint16_t pid;                 ///< \~english USB Product ID \~chinese USB 产品 ID
        InterfaceType interface_type; ///< \~english Interface type (USB/MIPI) \~chinese 接口类型（USB/MIPI）
        std::string firmware_version; ///< \~english Firmware version string \~chinese 固件版本号
    };

} // namespace fluxeem

#endif // __CAMERA_TYPES_H__
