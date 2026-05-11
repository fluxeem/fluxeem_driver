#ifndef IMX636ROI_HPP_
#define IMX636ROI_HPP_

#include <fluxeem/hal/tools/camera_tool_private.h>
#include <fluxeem/hal/tools/param_descriptor.h>
#include <cstdint>

#define IMX636_SENSOR_HEIGHT 720
#define IMX636_SENSOR_WIDTH 1280
namespace fluxeem
{

    class FLUXEEM_API Imx636ROI : public CameraToolRegister
    {
    public:
        Imx636ROI(std::shared_ptr<RegisterController> register_ctrl);

        // warring: Remove the default copy constructor so that the compiler knows it is not there to avoid errors
        Imx636ROI(const Imx636ROI &) = delete;
        Imx636ROI &operator=(const Imx636ROI &) = delete;

        ~Imx636ROI() {}

        const ToolInfo getToolInfo() override final
        {
            return ToolInfo{
                ToolType::TOOL_ROI,
                "ROI",
                {"enable", "mode", "x", "y", "width", "height"},
                "ROI of IMX636/646 sensor"};
        }

        std::map<std::string, BasicParameterInfo> getAllParamInfo() override final;
        bool getParamInfo(const std::string name, IntParameterInfo& info) override final;
        bool getParamInfo(const std::string name, FloatParameterInfo& info) override final;
        bool getParamInfo(const std::string name, BoolParameterInfo& info) override final;
        bool getParamInfo(const std::string name, EnumParameterInfo& info) override final;
        bool getParamInfo(const std::string name, StringParameterInfo& info) override final;

    protected:
        bool onParamChanged(uint8_t idx, const ParamValue& old_val, const ParamValue& new_val) override;

    private:
        void writeEnableRegisters(bool state);

        bool writeROI();

        void resetToFullRoi();

        bool isRoiFeasible(int x, int y, int width, int height);

        static constexpr uint8_t IDX_ENABLE = 0;
        static constexpr uint8_t IDX_MODE = 1;
        static constexpr uint8_t IDX_X = 2;
        static constexpr uint8_t IDX_Y = 3;
        static constexpr uint8_t IDX_WIDTH = 4;
        static constexpr uint8_t IDX_HEIGHT = 5;
    };

} // namespace fluxeem

#endif // IMX636BIASES_HPP_
