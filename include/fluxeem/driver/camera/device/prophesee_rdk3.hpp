// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef PROPHESEE_RDK3_HPP
#define PROPHESEE_RDK3_HPP

#include <fluxeem/driver/camera/base/imx636_camera.hpp>
#include <fluxeem/base/define/base_define.h>

namespace fluxeem
{
    /**
     * @ingroup fluxeem_camera_api
     * @brief RDK3 相机实现类
     */
    class FLUXEEM_API PropheseeRdk3Camera : public Imx636Camera{
    public:
        const static InterfaceType interface_type;
        const static uint16_t vendor_id;
        const static uint16_t product_id;
        const static std::string getManufacturer() {return "Prophesee";}
        const static std::string getProductName() {return "RDK3";}

        PropheseeRdk3Camera(const CameraDescription& cameraDesc) : Imx636Camera(cameraDesc) {}
        ~PropheseeRdk3Camera() override = default;
    };

} // namespace fluxeem

#endif
