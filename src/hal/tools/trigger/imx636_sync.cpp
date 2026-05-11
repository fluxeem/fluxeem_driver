// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#include <fluxeem/hal/tools/tool_info.h>
#include <fluxeem/hal/tools/tool_info_private.h>
#include <fluxeem/hal/tools/camera_tool.h>
#include <fluxeem/hal/tools/camera_tool_private.h>
#include <fluxeem/hal/registers/register_controller.h>
#include <fluxeem/hal/tools/trigger/imx636_sync.h>
#include <fluxeem/base/logging/logger.h>
#include <algorithm>
#include <array>
#include <string>
#include <memory>
#include <cstdint>

namespace
{
    struct SyncModeConfig
    {
        const char *name;
        bool external_timebase;
        bool drive_sync_io;
    };

    constexpr std::array<SyncModeConfig, 3> kSyncModes{{
        {"STANDALONE", false, true},
        {"MASTER", true, true},
        {"SLAVE", true, false},
    }};

    const SyncModeConfig *findSyncMode(const std::string &mode)
    {
        const auto it = std::find_if(kSyncModes.begin(), kSyncModes.end(),
                                     [&mode](const SyncModeConfig &item) { return mode == item.name; });
        return it == kSyncModes.end() ? nullptr : &(*it);
    }
}

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
        const SyncModeConfig *config = findSyncMode(mode);
        if (config == nullptr)
        {
            LOG_ERROR("unknown mode: %s", mode.c_str());
            return false;
        }

        timeBaseConfig(config->external_timebase, config->drive_sync_io);
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

        if (!external)
        {
            return;
        }

        const uint32_t pad_config = master ? 0b1100 : 0b1111;
        register_controller_->writeRegisterField("dig_pad2_ctrl", "pad_sync", pad_config);
    }
}