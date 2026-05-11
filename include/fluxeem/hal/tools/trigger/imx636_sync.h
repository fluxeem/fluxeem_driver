// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef IMX636_SYNC_HPP_
#define IMX636_SYNC_HPP_

#include <variant>
#include <fluxeem/hal/tools/camera_tool_private.h>

namespace fluxeem
{
        
    class FLUXEEM_API Imx636Sync : public CameraToolRegisterWithCallback
    {
    public:
        Imx636Sync(std::shared_ptr<RegisterController> register_ctrl);

        // warring: Remove the default copy constructor so that the compiler knows it is not there to avoid errors
        Imx636Sync(const Imx636Sync&) = delete;
        Imx636Sync& operator=(const Imx636Sync&) = delete;

        ~Imx636Sync(){}

        const ToolInfo getToolInfo() override final{
            return ToolInfo{
                ToolType::TOOL_SYNC,
                "Sync",
                {"mode"},
                "Sync of IMX636/646 sensors"
            };
        }

    private:
        bool setMode(std::string mode);

        bool getMode(std::string & mode);

        std::string sync_mode_ = "STANDALONE";

        bool addRegister2Map(const FullParameterInfo &info);

        void timeBaseConfig(bool external, bool master);

        std::vector<std::string> mode_options_ = {"STANDALONE", "MASTER", "SLAVE"};
        // name, min, max, unit, description
        const std::vector<FullParameterInfo> imx636_infos_{
            {"mode", "Set sync mode.", ToolParameterType::ENUM,
             EnumParameterInfo{mode_options_, "STANDALONE", std::bind(&Imx636Sync::getMode, this, std::placeholders::_1), std::bind(&Imx636Sync::setMode, this, std::placeholders::_1)}},
        };
    };

} // namespace fluxeem

#endif // IMX636BIASES_HPP_
