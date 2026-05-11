// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef FLUXEEM_APEX_VISION_S1
#define FLUXEEM_APEX_VISION_S1

#include <fluxeem/driver/camera/base/imx636_camera.hpp>
#include <fluxeem/base/define/base_define.h>

namespace fluxeem
{
    /**
     * @ingroup fluxeem_camera_api
     * @brief ApexVision S1 相机实现类
     */
    class FLUXEEM_API FluxeemApexVisionS1Camera : public Imx636Camera{
    public:
        const static InterfaceType interface_type;
        const static uint16_t vendor_id;
        const static uint16_t product_id;
        const static std::string getManufacturer() {return "ApexVision";}
        const static std::string getProductName() {return "S1";}

        FluxeemApexVisionS1Camera(CameraDescription cameraDesc) : Imx636Camera(cameraDesc) {}
        ~FluxeemApexVisionS1Camera() {}
    };

} // namespace fluxeem

#endif
