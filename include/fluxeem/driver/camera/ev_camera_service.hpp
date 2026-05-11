// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
/**
 * @file ev_camera_service.hpp
 * @author Fluxeem
 * @since 1.0.0
 * \~english @brief Camera service for device discovery and management
 * \~chinese @brief 用于设备发现和管理的相机服务
 * \~english @details This file defines the EvCameraService class which provides
 *                  camera device discovery, enumeration, and lifecycle management.
 *                  It maintains a registry of connected cameras and provides
 *                  methods to refresh the device list, enumerate available cameras,
 *                  and open camera devices by serial number.
 * \~chinese @details 此文件定义了 EvCameraService 类，提供相机设备发现、枚举和生命周期管理。
 *                  它维护已连接相机的注册表，并提供刷新设备列表、枚举可用相机
 *                  以及按序列号打开相机设备的方法。
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
     * \~english @brief Smart pointer type to manage camera device instances
     * \~chinese @brief 用于管理相机设备实例的智能指针类型
     * \~english @details This is a shared_ptr to ICamera that automatically manages
     *                  the lifetime of camera objects. When the last CameraDevice
     *          reference goes out of scope, the camera is automatically closed.
     * \~chinese @details 这是指向 ICamera 的共享指针，自动管理相机对象的生命周期。
     *                  当最后一个 CameraDevice 引用超出作用域时，相机会自动关闭。
     * @ingroup fluxeem_camera_mgmt
     */
    using CameraDevice = std::shared_ptr<ICamera>;

    /**
     * \~english @brief Camera service class for device discovery and management
     * \~chinese @brief 用于设备发现和管理的相机服务类
     * \~english @details EvCameraService provides a centralized interface for:
     *                  - Discovering connected event cameras (USB enumeration)
     *                  - Maintaining a registry of available camera devices
     *                  - Opening camera devices by serial number
     *                  - Automatic cleanup of disconnected cameras
     *                  
     *                  Usage example:
     *                  @code
     *                  EvCameraService service;
     *                  service.refresh();
     *                  auto cameras = service.listCameras();
     *                  auto camera = service.open("CAM001");
     *                  @endcode
     * \~chinese @details EvCameraService 提供集中式接口用于：
     *                  - 发现连接的事件相机（USB 枚举）
     *                  - 维护可用相机设备的注册表
     *                  - 按序列号打开相机设备
     *                  - 自动清理断开的相机
     *                  
     *                  使用示例：
     *                  @code
     *                  EvCameraService service;
     *                  service.refresh();
     *                  auto cameras = service.listCameras();
     *                  auto camera = service.open("CAM001");
     *                  @endcode
     * @ingroup fluxeem_camera_mgmt
     */
    class FLUXEEM_API EvCameraService
    {
    public:
        /**
         * \~english @brief Default constructor
         * \~chinese @brief 默认构造函数
         * \~english @details Initializes the camera service and prepares internal data structures.
         *                  No cameras are opened at this point.
         * \~chinese @details 初始化相机服务并准备内部数据结构。
         *                  此时不会打开任何相机。
         */
        EvCameraService();

        /**
         * \~english @brief Refresh the list of connected cameras
         * \~english @details This method performs USB bus enumeration to discover
         *                  all connected event cameras. It automatically removes
         *                  disconnected cameras from the internal registry and
         *                  adds newly connected cameras.
         * \~english @return int Number of cameras currently available
         * \~chinese @brief 刷新已连接相机的列表
         * \~chinese @details 此方法执行 USB 总线枚举以发现所有连接的事件相机。
         *                  它会自动从内部注册表中移除已断开的相机，
         *                  并添加新连接的相机。
         * \~chinese @return int 当前可用的相机数量
         * \~english @note This operation may take a few hundred milliseconds as it
         *                performs USB device enumeration.
         * \~chinese @note 此操作可能需要几百毫秒，因为它执行 USB 设备枚举。
         */
        int refresh();

        /**
         * \~english @brief Get descriptions of all available cameras
         * \~english @details Returns a vector containing CameraDescription structures
         *                  for each discovered camera. Each description includes
         *                  serial number, product name, manufacturer, and interface type.
         * \~english @return std::vector<CameraDescription> Vector of camera descriptions
         * \~chinese @brief 获取所有可用相机的描述
         * \~chinese @details 返回包含每个已发现相机的 CameraDescription 结构的向量。
         *                  每个描述包括序列号、产品名称、制造商和接口类型。
         * \~chinese @return std::vector<CameraDescription> 相机描述的向量
         * \~english @see CameraDescription
         * \~chinese @see CameraDescription
         */
        std::vector<CameraDescription> listCameras();

        /**
         * \~english @brief Open a camera device by serial number
         * \~english @details Opens the camera with the specified serial number and
         *                  returns a shared_ptr to manage its lifetime. The camera
         *                  must be called before use.
         *                  If the serial number is not found, throws std::runtime_error.
         * \~english @param serial Serial number of the camera to open (case-sensitive)
         * \~english @return CameraDevice Shared pointer to the opened ICamera instance
         * \~english @throws std::runtime_error if serial number is not found or camera is busy
         * \~chinese @brief 按序列号打开相机设备
         * \~chinese @details 打开指定序列号的相机并返回管理其生命周期的共享指针。
         *                  使用前必须调用。
         *                  如果找不到序列号，则抛出 std::runtime_error。
         * \~chinese @param serial 要打开的相机序列号（区分大小写）
         * \~chinese @return CameraDevice 指向已打开 ICamera 实例的共享指针
         * \~chinese @throws std::runtime_error 如果找不到序列号或相机正忙
         * \~english @note The returned CameraDevice uses shared_ptr semantics.
         *                Multiple calls with the same serial will return different
         *                shared_ptr objects managing the same camera instance.
         * \~chinese @note 返回的 CameraDevice 使用 shared_ptr 语义。
         *                使用相同序列号的多次调用将返回管理同一相机实例的不同 shared_ptr 对象。
         */
        CameraDevice open(const std::string &serial);

    private:
        /**
         * \~english @brief Map storing camera devices indexed by serial number
         * \~chinese @brief 按序列号索引存储相机设备的映射
         */
        std::map<std::string, CameraDevice> camera_devices_;

        /**
         * \~english @brief Remove disconnected cameras from the registry
         * \~english @details Iterates through all registered cameras and checks
         *                  their connection status using isConnected().
         *                  Cameras that return false are removed from the map.
         * \~chinese @brief 从注册表中移除已断开的相机
         * \~chinese @details 遍历所有已注册的相机并使用 isConnected() 检查
         *                  它们的连接状态。返回 false 的相机会从映射中移除。
         */
        void pruneDisconnectedCameras();

        /**
         * \~english @brief Discover all connected cameras via USB enumeration
         * \~english @details Performs low-level USB bus enumeration to find all
         *                  connected event cameras. Populates a vector of
         *                  CameraDescription structures with device information.
         * \~english @return size_t Number of cameras discovered
         * \~chinese @brief 通过 USB 枚举发现所有连接的相机
         * \~chinese @details 执行底层 USB 总线枚举以查找所有连接的事件相机。
         *                  用设备信息填充 CameraDescription 结构向量。
         * \~chinese @return size_t 发现的相机数量
         */
        size_t discoverAll();

        /**
         * \~english @brief Create a camera device instance from description
         * \~english @details Factory method that creates the appropriate camera
         *                  implementation based on the CameraDescription.
         *                  Supports EVK4, EVK5, DvsLume, and RDK3 cameras.
         * \~english @param camera_desc Camera description containing device info
         * \~english @return CameraDevice Newly created camera instance
         * \~chinese @brief 从描述创建相机设备实例
         * \~chinese @details 根据 CameraDescription 创建适当的相机实现的工厂方法。
         *                  支持 EVK4、EVK5、DvsLume 和 RDK3 相机。
         * \~chinese @param camera_desc 包含设备信息的相机描述
         * \~chinese @return CameraDevice 新创建的相机实例
         */
        CameraDevice createCameraDevice(const CameraDescription &camera_desc) const;

        /**
         * \~english @brief Register discovered cameras in the internal registry
         * \~english @details Takes a vector of CameraDescription structures and
         *                  creates CameraDevice instances for each one, storing
         *                  them in the camera_devices_ map indexed by serial number.
         * \~english @param camera_descs Vector of camera descriptions to register
         * \~english @return size_t Number of cameras successfully registered
         * \~chinese @brief 在内部注册表中注册已发现的相机
         * \~chinese @details 接受 CameraDescription 结构向量，并为每个创建
         *                  CameraDevice 实例，将它们按序列号索引存储在
         *                  camera_devices_ 映射中。
         * \~chinese @param camera_descs 要注册的相机描述向量
         * \~chinese @return size_t 成功注册的相机数量
         */
        size_t registerDiscoveredCameras(const std::vector<CameraDescription> &camera_descs);
    };

} // namespace fluxeem