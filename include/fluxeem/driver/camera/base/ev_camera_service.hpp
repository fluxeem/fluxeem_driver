// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
/**
 * @file ev_camera_service.hpp
 * @author Fluxeem
 * @since 1.0.0
 * \~english @brief Device-discovery and lifecycle service for event cameras
 * \~chinese @brief 事件相机设备发现与生命周期服务
 * \~english @details EvCameraService is the entry-point for the Fluxeem SDK.
 *                  It scans the USB bus, maintains an internal registry of reachable
 *                  cameras, and hands out CameraDevice handles on demand.
 * \~chinese @details EvCameraService 是 Fluxeem SDK 的入口。
 *                  它扫描 USB 总线，维护可达相机的内部注册表，并按需发放 CameraDevice 句柄。
 * @ingroup fluxeem_camera_mgmt
 */
#pragma once
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <stdexcept>
#include <fluxeem/driver/camera/base/i_camera.hpp>


namespace fluxeem
{

    /**
     * \~english @brief Shared-pointer alias for an open camera handle
     * \~chinese @brief 打开相机句柄的共享指针别名
     * \~english @details When the last shared_ptr reference is dropped the camera is automatically closed.
     * \~chinese @details 最后一个 shared_ptr 引用释放时，相机自动关闭。
     * @ingroup fluxeem_camera_mgmt
     */
    typedef FLUXEEM_API std::shared_ptr<ICamera> CameraDevice;

    /**
     * \~english @brief Singleton-style service that discovers and manages camera devices
     * \~chinese @brief 发现并管理相机设备的单例式服务
     * \~english @details Typical usage:
     *                  @code
     *                  EvCameraService svc;
     *                  svc.refresh();
     *                  for (auto& desc : svc.listCameras())
     *                      auto cam = svc.open(desc.serial);
     *                  @endcode
     * \~chinese @details 典型用法：
     *                  @code
     *                  EvCameraService svc;
     *                  svc.refresh();
     *                  for (auto& desc : svc.listCameras())
     *                      auto cam = svc.open(desc.serial);
     *                  @endcode
     * @ingroup fluxeem_camera_mgmt
     */
    class FLUXEEM_API EvCameraService
    {
    public:
        /**
         * \~english @brief Default-construct the service (no devices opened yet)
         * \~chinese @brief 默认构造服务（此时未打开任何设备）
         */
        EvCameraService();

        /**
         * \~english @brief Rescan the USB bus and update the internal device registry
         * \~english @return Number of cameras currently reachable
         * \~chinese @brief 重新扫描 USB 总线并更新内部设备注册表
         * \~chinese @return 当前可达的相机数量
         * \~english @note May take a few hundred milliseconds due to USB enumeration
         * \~chinese @note USB 枚举可能耗时数百毫秒
         */
        int refresh();

        /**
         * \~english @brief Return descriptors for every camera in the registry
         * \~english @return Vector of CameraDescription (serial, product, VID/PID, etc.)
         * \~chinese @brief 返回注册表中每台相机的描述符
         * \~chinese @return CameraDescription 向量（序列号、产品名、VID/PID 等）
         */
        std::vector<CameraDescription> listCameras();

        /**
         * \~english @brief Obtain a live camera handle by serial number
         * \~english @param serial  Case-sensitive serial string
         * \~english @return CameraDevice (shared_ptr<ICamera>)
         * \~chinese @brief 按序列号获取相机活动句柄
         * \~chinese @param serial  区分大小写的序列号
         * \~chinese @return CameraDevice（shared_ptr<ICamera>）
         * \~english @throws std::runtime_error if serial not found or device is busy
         * \~chinese @throws std::runtime_error 序列号未找到或设备正忙
         * \~english @note Repeated calls with the same serial return different shared_ptr instances
         *                managing the same underlying ICamera.
         * \~chinese @note 相同序列号的重复调用返回不同 shared_ptr 实例，但管理同一个 ICamera。
         */
        CameraDevice open(const std::string &serial);

    private:
        /** \~english @brief Internal registry: serial → CameraDevice
         *  \~chinese @brief 内部注册表：序列号 → CameraDevice
         */
        std::map<std::string, CameraDevice> camera_devices_;

        /** \~english @brief Purge entries whose devices are no longer reachable
         *  \~chinese @brief 清理已不可达设备的条目
         */
        void pruneDisconnectedCameras();

        /** \~english @brief Enumerate the USB bus and collect camera descriptors
         *  \~english @return Count of cameras found
         *  \~chinese @brief 枚举 USB 总线并收集相机描述符
         *  \~chinese @return 找到的相机数量
         */
        size_t discoverAll();

        /** \~english @brief Factory: instantiate the correct ICamera subclass for a given descriptor
         *  \~chinese @brief 工厂方法：根据描述符实例化正确的 ICamera 子类
         */
        CameraDevice createCameraDevice(const CameraDescription &camera_desc) const;

        /** \~english @brief Insert newly discovered descriptors into the registry
         *  \~chinese @brief 将新发现的描述符插入注册表
         */
        size_t registerDiscoveredCameras(const std::vector<CameraDescription> &camera_descs);
    };

} // namespace fluxeem
