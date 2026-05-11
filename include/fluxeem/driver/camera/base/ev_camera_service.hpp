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
     * @brief
     * \~english @brief Shared pointer to manage camera devices
     * \~chinese @brief 用于管理相机设备的共享指针
     * @ingroup camera_management
     */
    typedef FLUXEEM_API std::shared_ptr<ICamera> CameraDevice;

    /** \~english @brief Camera service class
     * \~chinese @brief 相机服务类
     * @ingroup camera_management
     */
    class FLUXEEM_API EvCameraService
    {
    public:
        /**
         * \~english @brief Constructor
         * \~chinese @brief 构造函数
         */
        EvCameraService();

        /** \~english @brief Update the current searchable camera list.
         * \~english @return int Returns the size of the list in the current camera.
         * \~chinese @brief 更新当前可搜索的相机列表。
         * \~chinese @return int 返回当前相机列表的大小。
         */
        int refresh();

        /** \~english @brief Get the Camera Descriptions vector
         * \~english @return const std::vector<CameraDescription> Returns the vector of camera descriptions.
         * \~chinese @brief 获取相机描述符向量
         * \~chinese @return const std::vector<CameraDescription> 返回相机描述符的常量引用。
         */
        std::vector<CameraDescription> listCameras();

        /** \~english @brief Open a camera with the given std::string number.
         * \~english @param std::string std::string number of the camera to open.
         * \~english @return CameraDevice Pointer to the ICamera object.
         * \~chinese @brief 使用给定的序列号打开相机。
         * \~chinese @param std::string 要打开的相机的序列号。
         * \~chinese @return CameraDevice 指向 ICamera 对象的智能指针。
         */
        CameraDevice open(const std::string &serial);

    private:
        /** \~english @brief Map storing connected camera devices.
         * \~chinese @brief 存储已连接相机设备的映射表
         */
        std::map<std::string, CameraDevice> camera_devices_;

        /** \~english @brief Remove unplugged cameras.
         * \~chinese @brief 移除已拔除的相机
         */
        void pruneDisconnectedCameras();

        /** \~english @brief Find and record all connected cameras.
         * \~english @return size_t Returns the number of found cameras.
         * \~chinese @brief 查找并记录所有连接的相机
         * \~chinese @return size_t 返回找到的相机数量
         */
        size_t discoverAll();

        /** \~english @brief Create camera device by descriptor.
         * \~chinese @brief 根据描述创建相机对象
         */
        CameraDevice createCameraDevice(const CameraDescription &camera_desc) const;

        /** \~english @brief Register newly discovered cameras.
         * \~chinese @brief 注册新发现的相机
         */
        size_t registerDiscoveredCameras(const std::vector<CameraDescription> &camera_descs);
    };

} // namespace fluxeem