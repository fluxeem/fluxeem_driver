#ifndef IMX636BIASES_HPP_
#define IMX636BIASES_HPP_

#include <fluxeem/hal/tools/camera_tool_private.h>
#include <fluxeem/hal/tools/param_descriptor.h>
#include <array>
#include <cstdint>

#define IMX636_BIAS_FO_MIN_OFFSET -20
#define IMX636_BIAS_FO_MAX_OFFSET 0
#define IMX636_BIAS_HPF_MIN_OFFSET 0
#define IMX636_BIAS_HPF_MAX_OFFSET 120
#define IMX636_BIAS_DIFF_ON_MIN_OFFSET -80
#define IMX636_BIAS_DIFF_ON_MAX_OFFSET 145
#define IMX636_BIAS_DIFF_MIN_OFFSET -25
#define IMX636_BIAS_DIFF_MAX_OFFSET 23
#define IMX636_BIAS_DIFF_OFF_MIN_OFFSET -30
#define IMX636_BIAS_DIFF_OFF_MAX_OFFSET 200
#define IMX636_BIAS_REFR_MIN_OFFSET -20
#define IMX636_BIAS_REFR_MAX_OFFSET 235

namespace fluxeem
{
    /**
     * @ingroup tools_bias
     * @brief IMX636 偏置电压配置工具
     */
    class FLUXEEM_API Imx636Biases : public CameraToolRegister
    {
    public:
        Imx636Biases(std::shared_ptr<RegisterController> register_ctrl);

        // warring: Remove the default copy constructor so that the compiler knows it is not there to avoid errors
        Imx636Biases(const Imx636Biases&) = delete;
        Imx636Biases& operator=(const Imx636Biases&) = delete;

        ~Imx636Biases(){}

        const ToolInfo getToolInfo() override final{
            return ToolInfo{
                ToolType::TOOL_BIAS,
                "Biases",
                {"bias_fo", "bias_hpf", "bias_diff_on", "bias_diff", "bias_diff_off", "bias_refr"},
                "Biases of IMX636/646 sensor"
            };
        }

        std::map<std::string, BasicParameterInfo> getAllParamInfo() override final;
        bool getParamInfo(const std::string name, IntParameterInfo& info) override final;
        bool getParamInfo(const std::string name, FloatParameterInfo& info) override final { return false; }
        bool getParamInfo(const std::string name, BoolParameterInfo& info) override final { return false; }
        bool getParamInfo(const std::string name, EnumParameterInfo& info) override final { return false; }
        bool getParamInfo(const std::string name, StringParameterInfo& info) override final { return false; }

    protected:
        bool onParamChanged(uint8_t idx, const ParamValue& old_val, const ParamValue& new_val) override;
        bool onParamRead(uint8_t idx, ParamValue& out_val) const override;

    private:
        static constexpr uint32_t BIAS_CONF = 0x11A10000;
        static constexpr size_t NUM_BIASES = 6;

        std::array<int, NUM_BIASES> hw_baseline_{};
        std::array<std::string, NUM_BIASES> reg_names_{};

        void readBaselines();
    };

} // namespace fluxeem

#endif // IMX636BIASES_HPP_
