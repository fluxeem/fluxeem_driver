#ifndef IMX636EVENTRATECONTROL_HPP_
#define IMX636EVENTRATECONTROL_HPP_

#include <variant>
#include <cstdint>
#include <map>
#include <tuple>
#include <fluxeem/hal/tools/camera_tool_private.h>

#define IMX636_EVENT_RATE_MEV_MAX 320  //320 MEv/s max
#define IMX636_EVENT_RATE_MEV_MIN 0
#define IMX636_EVENT_RATE_MEV_DEFAULT 320

namespace fluxeem
{
    /**
     * @ingroup tools_filter
     * @brief IMX636 事件速率控制工具
     */
    class FLUXEEM_API Imx636EventRateControl : public CameraToolRegisterWithCallback
    {
    public:
        Imx636EventRateControl(std::shared_ptr<RegisterController> register_ctrl);

        // warring: Remove the default copy constructor so that the compiler knows it is not there to avoid errors
        Imx636EventRateControl(const Imx636EventRateControl &) = delete;
        Imx636EventRateControl &operator=(const Imx636EventRateControl &) = delete;

        ~Imx636EventRateControl() {}

        const ToolInfo getToolInfo() override final
        {
            return ToolInfo{
                ToolType::TOOL_EVENT_RATE_CONTROL,
                "EventRateControl",
                {"enable", "max_event_rate"},
                "ERC of IMX636/646 sensor"};
        }

        void setMaxLimitEventRate(uint32_t rate)
        {
            max_event_rate_ = rate;
        }

    private:
        bool inited_ = false;
        bool enabled_ = false;
        uint32_t event_rate_ = IMX636_EVENT_RATE_MEV_DEFAULT;
        uint32_t max_event_rate_ = IMX636_EVENT_RATE_MEV_MAX;
        std::map<std::string, std::map<uint32_t, std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>>> lut_configs_;

        bool addRegister2Map(const FullParameterInfo &info);

        bool setEnable(bool en);

        bool getEnable(bool &en);

        bool setMaxEventRate(int rate);

        bool getMaxEventRate(int &rate);

        void initialize();

        uint32_t getCountPeriod() const;

        // name, min, max, unit, description
        const std::vector<FullParameterInfo> imx636_infos_{
            {"enable", "Set whether the Event Rate Control function is enabled", ToolParameterType::BOOL,
             BoolParameterInfo{false,
                               std::bind(&Imx636EventRateControl::getEnable, this, std::placeholders::_1), std::bind(&Imx636EventRateControl::setEnable, this, std::placeholders::_1)}},
            {"max_event_rate", "Set threshold", ToolParameterType::INT,
             IntParameterInfo{IMX636_EVENT_RATE_MEV_MIN, IMX636_EVENT_RATE_MEV_MAX, IMX636_EVENT_RATE_MEV_DEFAULT, "MEv/s",
                              std::bind(&Imx636EventRateControl::getMaxEventRate, this, std::placeholders::_1), std::bind(&Imx636EventRateControl::setMaxEventRate, this, std::placeholders::_1)}},
        };
    };

} // namespace fluxeem

#endif // IMX636BIASES_HPP_
