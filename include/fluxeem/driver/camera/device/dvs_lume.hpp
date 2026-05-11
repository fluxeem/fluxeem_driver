// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef __DVS_LUME_HPP__
#define __DVS_LUME_HPP__

#include <fluxeem/driver/camera/base/imx636_camera.hpp>
#include <fluxeem/base/define/base_define.h>
namespace fluxeem
{
    /**
     * @ingroup fluxeem_camera_api
     * @brief DVSense DvsLume 相机实现类
     */
    class FLUXEEM_API DvsenseLumeCamera : public Imx636Camera
    {
    public:
        const static InterfaceType interface_type;
        const static uint16_t vendor_id;
        const static uint16_t product_id;
        const static std::string getManufacturer() { return "DVSense"; }
        const static std::string getProductName() { return "DvsLume"; }

        DvsenseLumeCamera(const CameraDescription& cameraDesc) : Imx636Camera(cameraDesc) {}
        ~DvsenseLumeCamera() override = default;
    };

} // namespace fluxeem

#endif // __DVS_LUME_HPP__
