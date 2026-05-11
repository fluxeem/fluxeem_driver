// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#include <fluxeem/hal/tools/tool_info.h>
#include <fluxeem/hal/tools/tool_info_private.h>
#include <fluxeem/hal/tools/camera_tool.h>
#include <fluxeem/hal/tools/camera_tool_private.h>
#include <fluxeem/hal/registers/register_controller.h>
#include <fluxeem/hal/tools/event_trail_filter/imx636_event_trail_filter.hpp>
#include <fluxeem/base/logging/logger.h>
#include <string>
#include <memory>
#include <cstdint>
#include <map>
#include <algorithm>

namespace
{
    struct TrailTimingPreset
    {
        uint32_t prescaler;
        uint32_t multiplier;
        uint32_t fifo_timeout;
    };

    enum class TrailFilterMode
    {
        Trail,
        StcCutTrail,
        StcKeepTrail,
        Unknown,
    };

    TrailFilterMode parseTrailMode(const std::string &mode)
    {
        if (mode == "TRAIL")
        {
            return TrailFilterMode::Trail;
        }
        if (mode == "STC_CUT_TRAIL")
        {
            return TrailFilterMode::StcCutTrail;
        }
        if (mode == "STC_KEEP_TRAIL")
        {
            return TrailFilterMode::StcKeepTrail;
        }
        return TrailFilterMode::Unknown;
    }
}

namespace fluxeem
{
    const std::map<int, TrailTimingPreset> stc_threshold_params = {
        {1, {12, 15, 90}}, {2, {10, 3, 90}}, {3, {11, 5, 95}}, {4, {9, 1, 102}},
        {5, {13, 15, 90}}, {6, {11, 3, 114}}, {7, {11, 3, 90}}, {8, {12, 5, 109}},
        {9, {13, 9, 122}}, {10, {13, 9, 90}}, {11, {10, 1, 102}}, {12, {14, 15, 109}},
        {13, {13, 7, 117}}, {14, {14, 13, 127}}, {15, {12, 3, 138}}, {16, {12, 3, 90}},
        {17, {12, 3, 90}}, {18, {14, 11, 99}}, {19, {13, 5, 109}}, {20, {13, 5, 109}},
        {21, {14, 9, 122}}, {22, {14, 9, 122}}, {23, {11, 1, 209}}, {24, {11, 1, 138}},
        {25, {11, 1, 138}}, {26, {15, 15, 147}}, {27, {15, 15, 147}}, {28, {14, 7, 158}},
        {29, {14, 7, 158}}, {30, {15, 13, 170}}, {31, {15, 13, 170}}, {32, {13, 3, 185}},
        {33, {13, 3, 185}}, {34, {13, 3, 185}}, {35, {13, 3, 90}}, {36, {13, 3, 90}},
        {37, {15, 11, 202}}, {38, {15, 11, 99}}, {39, {15, 11, 99}}, {40, {15, 11, 99}},
        {41, {14, 5, 109}}, {42, {14, 5, 109}}, {43, {14, 5, 109}}, {44, {14, 5, 109}},
        {45, {15, 9, 248}}, {46, {15, 9, 122}}, {47, {15, 9, 122}}, {48, {15, 9, 122}},
        {49, {15, 9, 122}}, {50, {12, 1, 280}}, {51, {12, 1, 280}}, {52, {12, 1, 138}},
        {53, {12, 1, 138}}, {54, {12, 1, 138}}, {55, {12, 1, 138}}, {56, {16, 15, 147}},
        {57, {16, 15, 147}}, {58, {16, 15, 147}}, {59, {15, 7, 158}}, {60, {15, 7, 158}},
        {61, {15, 7, 158}}, {62, {15, 7, 158}}, {63, {15, 7, 158}}, {64, {16, 13, 171}},
        {65, {16, 13, 171}}, {66, {16, 13, 171}}, {67, {16, 13, 171}}, {68, {16, 13, 171}},
        {69, {14, 3, 185}}, {70, {14, 3, 185}}, {71, {14, 3, 185}}, {72, {14, 3, 185}},
        {73, {14, 3, 185}}, {74, {16, 11, 409}}, {75, {16, 11, 202}}, {76, {16, 11, 202}},
        {77, {16, 11, 202}}, {78, {16, 11, 202}}, {79, {16, 11, 202}}, {80, {16, 11, 202}},
        {81, {15, 5, 451}}, {82, {15, 5, 223}}, {83, {15, 5, 223}}, {84, {15, 5, 223}},
        {85, {15, 5, 223}}, {86, {15, 5, 223}}, {87, {15, 5, 223}}, {88, {15, 5, 223}},
        {89, {16, 9, 501}}, {90, {16, 9, 501}}, {91, {16, 9, 501}}, {92, {16, 9, 248}},
        {93, {16, 9, 248}}, {94, {16, 9, 248}}, {95, {16, 9, 248}}, {96, {16, 9, 248}},
        {97, {16, 9, 248}}, {98, {16, 9, 248}}, {99, {13, 1, 564}}, {100, {13, 1, 564}},
    };
    Imx636EventTrailFilter::Imx636EventTrailFilter(std::shared_ptr<RegisterController> register_ctrl)
        : CameraToolRegisterWithCallback(register_ctrl, "stc/")
    {
        for (auto& info : imx636_infos_)
        {
            addRegister2Map(info);
        }
    }

    bool Imx636EventTrailFilter::setEnable(bool en)
    {
        register_controller_->writeRegister(prefix_ + "pipeline_control", 0b101);
        enabled_ = false;
        if (!en)
            return true;
        register_controller_->writeRegisterField(prefix_ + "initialization", "stc_flag_init_done", 1);
        register_controller_->writeRegisterField(prefix_ + "initialization", "stc_req_init", 1);

        switch (parseTrailMode(filtering_type_))
        {
        case TrailFilterMode::StcCutTrail:
            register_controller_->writeRegisterField(prefix_ + "stc_param", "stc_enable", 1);
            register_controller_->writeRegisterField(prefix_ + "stc_param", "stc_threshold", threshold_ms_ * 1000);
            register_controller_->writeRegisterField(prefix_ + "stc_param", "disable_stc_cut_trail", 0);
            register_controller_->writeRegisterField(prefix_ + "trail_param", "trail_enable", 0);
            break;
        case TrailFilterMode::StcKeepTrail:
            register_controller_->writeRegisterField(prefix_ + "stc_param", "stc_enable", 1);
            register_controller_->writeRegisterField(prefix_ + "stc_param", "stc_threshold", threshold_ms_ * 1000);
            register_controller_->writeRegisterField(prefix_ + "stc_param", "disable_stc_cut_trail", 1);
            register_controller_->writeRegisterField(prefix_ + "trail_param", "trail_enable", 0);
            break;
        case TrailFilterMode::Trail:
            register_controller_->writeRegisterField(prefix_ + "stc_param", "stc_enable", 0);
            register_controller_->writeRegisterField(prefix_ + "trail_param", "trail_enable", 1);
            register_controller_->writeRegisterField(prefix_ + "trail_param", "trail_threshold", threshold_ms_ * 1000);
            break;
        case TrailFilterMode::Unknown:
            LOG_ERROR("Unknown event trail filter type: %s", filtering_type_.c_str());
            return false;
        }

        const auto preset_it = stc_threshold_params.find(static_cast<int>(threshold_ms_));
        if (preset_it == stc_threshold_params.end())
        {
            LOG_ERROR("Unsupported STC threshold: %u", threshold_ms_);
            return false;
        }
        const TrailTimingPreset &timing = preset_it->second;
        register_controller_->writeRegisterField(prefix_ + "timestamping", "prescaler", timing.prescaler);
        register_controller_->writeRegisterField(prefix_ + "timestamping", "multiplier", timing.multiplier);
        register_controller_->writeRegisterField(prefix_ + "timestamping", "enable_last_ts_update_at_every_event", 1);

        register_controller_->writeRegisterField(prefix_ + "invalidation", "dt_fifo_timeout", timing.fifo_timeout);

        // 检查 SRAM 初始化完成
        uint32_t init_done = 0;
        for (int i = 0; i < 3; i++)
        {
            register_controller_->readRegisterField(prefix_ + "initialization", "stc_flag_init_done", init_done);

            if (init_done == 1)
            {
                break;
            }
        }
        if (init_done == 0)
        {
            LOG_ERROR("Set enable failed.");
            return false;
        }
        // 启用滤波器
        register_controller_->writeRegister(prefix_ + "pipeline_control", 0b001);
        enabled_ = true;

        return true;
    }

    bool Imx636EventTrailFilter::getEnable(bool &en)
    {
        en = enabled_;
        return true;
    }

    bool Imx636EventTrailFilter::setThreshold(int th)
    {
        threshold_ms_ = th;
        bool en = false;
        getEnable(en);

        if (en)
        {
            setEnable(false);
            setEnable(true);
        }
        
        return true;
    }

    bool Imx636EventTrailFilter::getThreshold(int &th)
    {
        th = threshold_ms_;
        return true;
    }

    bool Imx636EventTrailFilter::getType(std::string &type)
    {
        type = filtering_type_;
        return true;
    }

    bool Imx636EventTrailFilter::setType(std::string type)
    {
        if (std::find(event_trail_filter_options_.begin(), event_trail_filter_options_.end(), type) == event_trail_filter_options_.end())
        {
            LOG_ERROR("The input type is incorrect.");
            return false;
        }
        filtering_type_ = type;

        bool en = false;
        getEnable(en);

        if (en)
        {
            setEnable(false);
            setEnable(true);
        }
        return true;
    }

    bool Imx636EventTrailFilter::addRegister2Map(const FullParameterInfo& info)
    {
        parameter_info_map_.emplace(info.basic_info.name, info);

        return true;
    }
}