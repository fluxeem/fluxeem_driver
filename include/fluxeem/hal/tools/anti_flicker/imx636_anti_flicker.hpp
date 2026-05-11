// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef IMX636ANTIFLICKER_HPP_
#define IMX636ANTIFLICKER_HPP_

#include <variant>
#include <functional>
#include <cstdint>
#include <fluxeem/hal/tools/camera_tool_private.h>


#define ANTI_FLICKER_MIN_FREQ 50
#define ANTI_FLICKER_MAX_FREQ 520
#define ANTI_FLICKER_MIN_START_TH 0
#define ANTI_FLICKER_MAX_START_TH (1 << 3) - 1
#define ANTI_FLICKER_MIN_STOP_TH 0
#define ANTI_FLICKER_MAX_STOP_TH (1 << 3) - 1

namespace fluxeem
{
    enum AntiFlickerMode { BAND_PASS = 0, BAND_STOP };

    class FLUXEEM_API Imx636AntiFlicker : public CameraToolRegisterWithCallback
    {
    public:
        Imx636AntiFlicker(std::shared_ptr<RegisterController> register_ctrl);

        // warring: Remove the default copy constructor so that the compiler knows it is not there to avoid errors
        Imx636AntiFlicker(const Imx636AntiFlicker&) = delete;
        Imx636AntiFlicker& operator=(const Imx636AntiFlicker&) = delete;

        ~Imx636AntiFlicker(){}

        const ToolInfo getToolInfo() override final {
            std::vector<std::string> parameter_names;
            for (auto it = parameter_info_map_.begin(); it != parameter_info_map_.end(); ++it) {
                parameter_names.push_back(it->first);
            }
            return ToolInfo{
                ToolType::TOOL_ANTI_FLICKER,
                "AntiFlicker",
                parameter_names,
                "Anti flicker of IMX636/646 sensor."
            };
        }

    private:

        bool addRegisterToMap(const FullParameterInfo &info);

        // map interface
        bool setEnableStatus(bool en);

        bool readEnabled(bool& en) const;

        bool getLowFrequency(int& low_freq);

        bool setLowFrequency(int low_freq);

        bool getHighFrequency(int& high_freq);

        bool setHighFrequency(int high_freq);

        bool setFilteringMode(std::string mode);

        bool getFilteringMode(std::string& mode);

        bool getDutyCycle(float& duty_cycle) const;

        bool setDutyCycle(float duty_cycle);

        bool getStartThreshold(int& start_th);

        bool setStartThreshold(int threshold);

        bool setStopThreshold(int threshold);

        bool getStopThreshold(int& stop_th);

        bool reset();

        uint32_t freqToPeriod(const uint32_t& freq);

        // params
        uint32_t low_freq_ = 50;
        uint32_t high_freq_ = 520;
        uint32_t inverted_duty_cycle_ = 0x8;
        float duty_cycle_ = 100.0 - (inverted_duty_cycle_ * 100.0) / 16.0;
        std::string mode_{ "Band cut"};
        uint32_t start_threshold_{ 6 };
        uint32_t stop_threshold_{ 4 };

        // name, min, max, unit, description
        const std::vector<FullParameterInfo> imx636_infos_{
            {"enable", "Set whether the anti fliker function is enabled", ToolParameterType::BOOL, 
                BoolParameterInfo{false, 
                    std::bind(&Imx636AntiFlicker::readEnabled, this, std::placeholders::_1), std::bind(&Imx636AntiFlicker::setEnableStatus, this, std::placeholders::_1)}},
            {"low_frequency", "Set the frequency range to low", ToolParameterType::INT, 
                IntParameterInfo{ANTI_FLICKER_MIN_FREQ, ANTI_FLICKER_MAX_FREQ, ANTI_FLICKER_MIN_FREQ, "hz", 
                    std::bind(&Imx636AntiFlicker::getLowFrequency, this, std::placeholders::_1), std::bind(&Imx636AntiFlicker::setLowFrequency, this, std::placeholders::_1)}},
            {"high_frequency", "Set the frequency range to high", ToolParameterType::INT, 
                IntParameterInfo{ANTI_FLICKER_MIN_FREQ, ANTI_FLICKER_MAX_FREQ, ANTI_FLICKER_MAX_FREQ, "hz",
                    std::bind(&Imx636AntiFlicker::getHighFrequency, this, std::placeholders::_1), std::bind(&Imx636AntiFlicker::setHighFrequency, this, std::placeholders::_1)}},
            {"fliter_mode", "Set fliter mode", ToolParameterType::ENUM, 
                EnumParameterInfo{{"Band cut", "Band pass"}, "Band cut", 
                std::bind(&Imx636AntiFlicker::getFilteringMode, this, std::placeholders::_1), std::bind(&Imx636AntiFlicker::setFilteringMode, this, std::placeholders::_1)}},
            {"duty_cycle", "Set duty cycle", ToolParameterType::FLOAT, 
                FloatParameterInfo{0, 100, duty_cycle_, "%", std::bind(&Imx636AntiFlicker::getDutyCycle, this, std::placeholders::_1), std::bind(&Imx636AntiFlicker::setDutyCycle, this, std::placeholders::_1)}},
            {"start_threshold", "Set start threshold", ToolParameterType::INT, 
                IntParameterInfo{ANTI_FLICKER_MIN_START_TH, ANTI_FLICKER_MAX_START_TH, 6, " ",
                std::bind(&Imx636AntiFlicker::getStartThreshold, this, std::placeholders::_1), std::bind(&Imx636AntiFlicker::setStartThreshold, this, std::placeholders::_1)}},
            {"stop_threshold", "Set stop threshold", ToolParameterType::INT, 
                IntParameterInfo{ANTI_FLICKER_MIN_STOP_TH, ANTI_FLICKER_MAX_STOP_TH, 4, " ",
                std::bind(&Imx636AntiFlicker::getStopThreshold, this, std::placeholders::_1), std::bind(&Imx636AntiFlicker::setStopThreshold, this, std::placeholders::_1)}},

        };
    };

} // namespace fluxeem

#endif // IMX636BIASES_HPP_
