#ifndef PROPHESEE_EVK4
#define PROPHESEE_EVK4

#include <fluxeem/driver/camera/base/imx636_camera.hpp>
#include <fluxeem/base/define/base_define.h>

namespace fluxeem
{
    /**
     * @ingroup camera_interface
     * @brief Prophesee EVK4 相机实现类
     */
    class FLUXEEM_API PropheseeEvk4Camera : public Imx636Camera{
    public:
        const static InterfaceType interface_type;
        const static uint16_t vendor_id;
        const static uint16_t product_id;
        const static std::string getManufacturer() {return "Prophesee";}
        const static std::string getProductName() {return "EVK4";}

        PropheseeEvk4Camera(CameraDescription cameraDesc) : Imx636Camera(cameraDesc) {}
        ~PropheseeEvk4Camera() {}
    };

} // namespace fluxeem

#endif
