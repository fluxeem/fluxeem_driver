
#include <fluxeem/driver/camera/ev_camera_service.hpp>
#include <fluxeem/driver/camera/base/i_camera.hpp>
#include <fluxeem/driver/camera/device/prophesee_evk4.hpp>
#include <fluxeem/driver/camera/device/prophesee_evk5.hpp>
#include <fluxeem/driver/camera/device/dvs_lume.hpp>
#include <fluxeem/driver/camera/device/prophesee_rdk3.hpp>
#include <fluxeem/driver/camera/device/fluxeem_apex_vision_s1.hpp>
#include <fluxeem/hal/common/usb_interface.h>
#include <fluxeem/base/logging/logger.h>
#include <vector>
#include <memory>
#include <utility>

namespace fluxeem
{

    EvCameraService::EvCameraService()
    {
    }

    int EvCameraService::refresh()
    {
        pruneDisconnectedCameras();
        discoverAll();
        return static_cast<int>(camera_devices_.size());
    }

    void EvCameraService::pruneDisconnectedCameras()
    {
        for (auto it = camera_devices_.begin(); it != camera_devices_.end();)
        {
            if (it->second->isConnected())
            {
                it++;
            }
            else
            {
                LOG_DEBUG("Camera %s disconnected", it->second->getDescription().serial.data());
                it->second->close();
                it = camera_devices_.erase(it);
            }
        }
    }

    std::vector<CameraDescription> EvCameraService::listCameras()
    {
        std::vector<CameraDescription> camera_descs;
        refresh();
        for (auto it = camera_devices_.begin(); it != camera_devices_.end(); ++it)
        {
            camera_descs.push_back(it->second->getDescription());
        }
        return camera_descs;
    }

    size_t EvCameraService::discoverAll()
    {
        const std::vector<std::pair<uint16_t, uint16_t>> supported_vid_pid = {
            {PropheseeEvk4Camera::vendor_id, PropheseeEvk4Camera::product_id},
            {PropheseeEvk5Camera::vendor_id, PropheseeEvk5Camera::product_id},
            {DvsenseLumeCamera::vendor_id, DvsenseLumeCamera::product_id},
            {PropheseeRdk3Camera::vendor_id, PropheseeRdk3Camera::product_id},
            {FluxeemApexVisionS1Camera::vendor_id, FluxeemApexVisionS1Camera::product_id}};

        auto camera_descs = UsbInterface::findUsbCameras(supported_vid_pid);
        if (camera_descs.empty())
        {
            // Retry once to reduce transient USB discovery failures.
            camera_descs = UsbInterface::findUsbCameras(supported_vid_pid);
        }
        return registerDiscoveredCameras(camera_descs);
    }

    CameraDevice EvCameraService::createCameraDevice(const CameraDescription &camera_desc) const
    {
        if (camera_desc.vid == PropheseeEvk4Camera::vendor_id && camera_desc.pid == PropheseeEvk4Camera::product_id)
        {
            return std::make_shared<PropheseeEvk4Camera>(camera_desc);
        }
        if (camera_desc.vid == PropheseeEvk5Camera::vendor_id && camera_desc.pid == PropheseeEvk5Camera::product_id)
        {
            return std::make_shared<PropheseeEvk5Camera>(camera_desc);
        }
        if (camera_desc.vid == DvsenseLumeCamera::vendor_id && camera_desc.pid == DvsenseLumeCamera::product_id)
        {
            return std::make_shared<DvsenseLumeCamera>(camera_desc);
        }
        if (camera_desc.vid == PropheseeRdk3Camera::vendor_id && camera_desc.pid == PropheseeRdk3Camera::product_id)
        {
            return std::make_shared<PropheseeRdk3Camera>(camera_desc);
        }
        if (camera_desc.vid == FluxeemApexVisionS1Camera::vendor_id && camera_desc.pid == FluxeemApexVisionS1Camera::product_id)
        {
            return std::make_shared<FluxeemApexVisionS1Camera>(camera_desc);
        }
        return nullptr;
    }

    size_t EvCameraService::registerDiscoveredCameras(const std::vector<CameraDescription> &camera_descs)
    {
        size_t newly_added = 0;
        for (const auto &desc : camera_descs)
        {
            if (camera_devices_.find(desc.serial) == camera_devices_.end())
            {
                auto camera = createCameraDevice(desc);
                if (camera)
                {
                    camera_devices_[desc.serial] = camera;
                    ++newly_added;
                }
            }
        }
        return newly_added;
    }

    CameraDevice EvCameraService::open(const std::string &serial)
    {
        refresh();
        LOG_DEBUG("Camera serial: %s", serial.data());
        auto it = camera_devices_.find(serial);
        if (it == camera_devices_.end())
        {
            return nullptr;
        }
        if (!it->second->init())
        {
            it->second->close();
            return nullptr;
        }
        return it->second;
    }

} // namespace fluxeem
