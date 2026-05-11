#include <fluxeem/hal/tools/tool_info.h>
#include <fluxeem/hal/tools/tool_info_private.h>
#include <fluxeem/hal/tools/camera_tool.h>
#include <fluxeem/hal/tools/camera_tool_private.h>
#include <fluxeem/hal/registers/register_controller.h>
#include <fluxeem/hal/tools/trigger/imx636_sync.h>
#include <fluxeem/base/logging/logger.h>
#include <string>
#include <memory>
#include <cstdint>

namespace fluxeem
{

    Imx636Sync::Imx636Sync(std::shared_ptr<RegisterController> register_ctrl)
        : CameraToolRegisterWithCallback(register_ctrl, "trigger_out")
    {
        for (auto &info : imx636_infos_)
        {
            addRegister2Map(info);
        }
    }

    bool Imx636Sync::addRegister2Map(const FullParameterInfo &info)
    {
        parameter_info_map_.emplace(info.basic_info.name, info);

        return true;
    }

    bool Imx636Sync::setMode(std::string mode)
    {
        LOG_INFO("Set mode: %s", mode.c_str());
        if(mode == "STANDALONE")
        {
            timeBaseConfig(false, true);
        }else if(mode == "MASTER")
        {
            timeBaseConfig(true, true);
        }else if(mode == "SLAVE")
        {
            timeBaseConfig(true, false);
        }else{
            LOG_ERROR("unknown mode: %s", mode.c_str());
            return false;
        }
        sync_mode_ = mode;
        return true;
    }

    bool Imx636Sync::getMode(std::string &mode)
    {
        mode = sync_mode_;
        return true;
    }

    void Imx636Sync::timeBaseConfig(bool external, bool master)
    {
        register_controller_->writeRegisterField("ro/time_base_ctrl", "time_base_mode", external);
        register_controller_->writeRegisterField("ro/time_base_ctrl", "external_mode", master);
        register_controller_->writeRegisterField("ro/time_base_ctrl", "external_mode_enable", external);
        register_controller_->writeRegisterField("ro/time_base_ctrl", "Reserved_10_4", 100);

        if (external)
        {
            if (master)
            {
                // set SYNCHRO IO to output mode
                register_controller_->writeRegisterField("dig_pad2_ctrl", "pad_sync", 0b1100);
            }
            else
            {
                // set SYNCHRO IO to input mode
                register_controller_->writeRegisterField("dig_pad2_ctrl", "pad_sync", 0b1111);
            }
        }
    }
}