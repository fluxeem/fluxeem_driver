// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef IMX636TRIGGERIN_HPP_
#define IMX636TRIGGERIN_HPP_

#include <variant>
#include <fluxeem/hal/tools/camera_tool_private.h>

namespace fluxeem
{
    class FLUXEEM_API Imx636TriggerIn : public CameraToolRegisterWithCallback
    {
    public:
        Imx636TriggerIn(std::shared_ptr<RegisterController> register_ctrl);

        // warring: Remove the default copy constructor so that the compiler knows it is not there to avoid errors
        Imx636TriggerIn(const Imx636TriggerIn&) = delete;
        Imx636TriggerIn& operator=(const Imx636TriggerIn&) = delete;

        ~Imx636TriggerIn(){}

        const ToolInfo getToolInfo() override final{
            return ToolInfo{
                ToolType::TOOL_TRIGGER_IN,
                "TriggerIn",
                {"enable"},
                "Trigger in of IMX636/646 sensors"
            };
        }

    private:
        bool setEnable(bool en);

        bool getEnable(bool& en);

        bool addRegister2Map(const FullParameterInfo &info);

        // name, min, max, unit, description
        const std::vector<FullParameterInfo> imx636_infos_{
            {"enable", "Trigger in enable", ToolParameterType::BOOL, 
                BoolParameterInfo{false, std::bind(&Imx636TriggerIn::getEnable, this, std::placeholders::_1), std::bind(&Imx636TriggerIn::setEnable, this, std::placeholders::_1)}},
            
        };


    };

} // namespace fluxeem

#endif // IMX636BIASES_HPP_
