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

namespace fluxeem
{
    std::map<const int, std::map<const std::string, const int>> stc_threshold_params = {
        {1, {{"presc", 12}, {"mult", 15}, {"dt_fifo_timeout", 90}}},
        {2, {{"presc", 10}, {"mult", 3}, {"dt_fifo_timeout", 90}}},
        {3, {{"presc", 11}, {"mult", 5}, {"dt_fifo_timeout", 95}}},
        {4, {{"presc", 9}, {"mult", 1}, {"dt_fifo_timeout", 102}}},
        {5, {{"presc", 13}, {"mult", 15}, {"dt_fifo_timeout", 90}}},
        {6, {{"presc", 11}, {"mult", 3}, {"dt_fifo_timeout", 114}}},
        {7, {{"presc", 11}, {"mult", 3}, {"dt_fifo_timeout", 90}}},
        {8, {{"presc", 12}, {"mult", 5}, {"dt_fifo_timeout", 109}}},
        {9, {{"presc", 13}, {"mult", 9}, {"dt_fifo_timeout", 122}}},
        {10, {{"presc", 13}, {"mult", 9}, {"dt_fifo_timeout", 90}}},
        {11, {{"presc", 10}, {"mult", 1}, {"dt_fifo_timeout", 102}}},
        {12, {{"presc", 14}, {"mult", 15}, {"dt_fifo_timeout", 109}}},
        {13, {{"presc", 13}, {"mult", 7}, {"dt_fifo_timeout", 117}}},
        {14, {{"presc", 14}, {"mult", 13}, {"dt_fifo_timeout", 127}}},
        {15, {{"presc", 12}, {"mult", 3}, {"dt_fifo_timeout", 138}}},
        {16, {{"presc", 12}, {"mult", 3}, {"dt_fifo_timeout", 90}}},
        {17, {{"presc", 12}, {"mult", 3}, {"dt_fifo_timeout", 90}}},
        {18, {{"presc", 14}, {"mult", 11}, {"dt_fifo_timeout", 99}}},
        {19, {{"presc", 13}, {"mult", 5}, {"dt_fifo_timeout", 109}}},
        {20, {{"presc", 13}, {"mult", 5}, {"dt_fifo_timeout", 109}}},
        {21, {{"presc", 14}, {"mult", 9}, {"dt_fifo_timeout", 122}}},
        {22, {{"presc", 14}, {"mult", 9}, {"dt_fifo_timeout", 122}}},
        {23, {{"presc", 11}, {"mult", 1}, {"dt_fifo_timeout", 209}}},
        {24, {{"presc", 11}, {"mult", 1}, {"dt_fifo_timeout", 138}}},
        {25, {{"presc", 11}, {"mult", 1}, {"dt_fifo_timeout", 138}}},
        {26, {{"presc", 15}, {"mult", 15}, {"dt_fifo_timeout", 147}}},
        {27, {{"presc", 15}, {"mult", 15}, {"dt_fifo_timeout", 147}}},
        {28, {{"presc", 14}, {"mult", 7}, {"dt_fifo_timeout", 158}}},
        {29, {{"presc", 14}, {"mult", 7}, {"dt_fifo_timeout", 158}}},
        {30, {{"presc", 15}, {"mult", 13}, {"dt_fifo_timeout", 170}}},
        {31, {{"presc", 15}, {"mult", 13}, {"dt_fifo_timeout", 170}}},
        {32, {{"presc", 13}, {"mult", 3}, {"dt_fifo_timeout", 185}}},
        {33, {{"presc", 13}, {"mult", 3}, {"dt_fifo_timeout", 185}}},
        {34, {{"presc", 13}, {"mult", 3}, {"dt_fifo_timeout", 185}}},
        {35, {{"presc", 13}, {"mult", 3}, {"dt_fifo_timeout", 90}}},
        {36, {{"presc", 13}, {"mult", 3}, {"dt_fifo_timeout", 90}}},
        {37, {{"presc", 15}, {"mult", 11}, {"dt_fifo_timeout", 202}}},
        {38, {{"presc", 15}, {"mult", 11}, {"dt_fifo_timeout", 99}}},
        {39, {{"presc", 15}, {"mult", 11}, {"dt_fifo_timeout", 99}}},
        {40, {{"presc", 15}, {"mult", 11}, {"dt_fifo_timeout", 99}}},
        {41, {{"presc", 14}, {"mult", 5}, {"dt_fifo_timeout", 109}}},
        {42, {{"presc", 14}, {"mult", 5}, {"dt_fifo_timeout", 109}}},
        {43, {{"presc", 14}, {"mult", 5}, {"dt_fifo_timeout", 109}}},
        {44, {{"presc", 14}, {"mult", 5}, {"dt_fifo_timeout", 109}}},
        {45, {{"presc", 15}, {"mult", 9}, {"dt_fifo_timeout", 248}}},
        {46, {{"presc", 15}, {"mult", 9}, {"dt_fifo_timeout", 122}}},
        {47, {{"presc", 15}, {"mult", 9}, {"dt_fifo_timeout", 122}}},
        {48, {{"presc", 15}, {"mult", 9}, {"dt_fifo_timeout", 122}}},
        {49, {{"presc", 15}, {"mult", 9}, {"dt_fifo_timeout", 122}}},
        {50, {{"presc", 12}, {"mult", 1}, {"dt_fifo_timeout", 280}}},
        {51, {{"presc", 12}, {"mult", 1}, {"dt_fifo_timeout", 280}}},
        {52, {{"presc", 12}, {"mult", 1}, {"dt_fifo_timeout", 138}}},
        {53, {{"presc", 12}, {"mult", 1}, {"dt_fifo_timeout", 138}}},
        {54, {{"presc", 12}, {"mult", 1}, {"dt_fifo_timeout", 138}}},
        {55, {{"presc", 12}, {"mult", 1}, {"dt_fifo_timeout", 138}}},
        {56, {{"presc", 16}, {"mult", 15}, {"dt_fifo_timeout", 147}}},
        {57, {{"presc", 16}, {"mult", 15}, {"dt_fifo_timeout", 147}}},
        {58, {{"presc", 16}, {"mult", 15}, {"dt_fifo_timeout", 147}}},
        {59, {{"presc", 15}, {"mult", 7}, {"dt_fifo_timeout", 158}}},
        {60, {{"presc", 15}, {"mult", 7}, {"dt_fifo_timeout", 158}}},
        {61, {{"presc", 15}, {"mult", 7}, {"dt_fifo_timeout", 158}}},
        {62, {{"presc", 15}, {"mult", 7}, {"dt_fifo_timeout", 158}}},
        {63, {{"presc", 15}, {"mult", 7}, {"dt_fifo_timeout", 158}}},
        {64, {{"presc", 16}, {"mult", 13}, {"dt_fifo_timeout", 171}}},
        {65, {{"presc", 16}, {"mult", 13}, {"dt_fifo_timeout", 171}}},
        {66, {{"presc", 16}, {"mult", 13}, {"dt_fifo_timeout", 171}}},
        {67, {{"presc", 16}, {"mult", 13}, {"dt_fifo_timeout", 171}}},
        {68, {{"presc", 16}, {"mult", 13}, {"dt_fifo_timeout", 171}}},
        {69, {{"presc", 14}, {"mult", 3}, {"dt_fifo_timeout", 185}}},
        {70, {{"presc", 14}, {"mult", 3}, {"dt_fifo_timeout", 185}}},
        {71, {{"presc", 14}, {"mult", 3}, {"dt_fifo_timeout", 185}}},
        {72, {{"presc", 14}, {"mult", 3}, {"dt_fifo_timeout", 185}}},
        {73, {{"presc", 14}, {"mult", 3}, {"dt_fifo_timeout", 185}}},
        {74, {{"presc", 16}, {"mult", 11}, {"dt_fifo_timeout", 409}}},
        {75, {{"presc", 16}, {"mult", 11}, {"dt_fifo_timeout", 202}}},
        {76, {{"presc", 16}, {"mult", 11}, {"dt_fifo_timeout", 202}}},
        {77, {{"presc", 16}, {"mult", 11}, {"dt_fifo_timeout", 202}}},
        {78, {{"presc", 16}, {"mult", 11}, {"dt_fifo_timeout", 202}}},
        {79, {{"presc", 16}, {"mult", 11}, {"dt_fifo_timeout", 202}}},
        {80, {{"presc", 16}, {"mult", 11}, {"dt_fifo_timeout", 202}}},
        {81, {{"presc", 15}, {"mult", 5}, {"dt_fifo_timeout", 451}}},
        {82, {{"presc", 15}, {"mult", 5}, {"dt_fifo_timeout", 223}}},
        {83, {{"presc", 15}, {"mult", 5}, {"dt_fifo_timeout", 223}}},
        {84, {{"presc", 15}, {"mult", 5}, {"dt_fifo_timeout", 223}}},
        {85, {{"presc", 15}, {"mult", 5}, {"dt_fifo_timeout", 223}}},
        {86, {{"presc", 15}, {"mult", 5}, {"dt_fifo_timeout", 223}}},
        {87, {{"presc", 15}, {"mult", 5}, {"dt_fifo_timeout", 223}}},
        {88, {{"presc", 15}, {"mult", 5}, {"dt_fifo_timeout", 223}}},
        {89, {{"presc", 16}, {"mult", 9}, {"dt_fifo_timeout", 501}}},
        {90, {{"presc", 16}, {"mult", 9}, {"dt_fifo_timeout", 501}}},
        {91, {{"presc", 16}, {"mult", 9}, {"dt_fifo_timeout", 501}}},
        {92, {{"presc", 16}, {"mult", 9}, {"dt_fifo_timeout", 248}}},
        {93, {{"presc", 16}, {"mult", 9}, {"dt_fifo_timeout", 248}}},
        {94, {{"presc", 16}, {"mult", 9}, {"dt_fifo_timeout", 248}}},
        {95, {{"presc", 16}, {"mult", 9}, {"dt_fifo_timeout", 248}}},
        {96, {{"presc", 16}, {"mult", 9}, {"dt_fifo_timeout", 248}}},
        {97, {{"presc", 16}, {"mult", 9}, {"dt_fifo_timeout", 248}}},
        {98, {{"presc", 16}, {"mult", 9}, {"dt_fifo_timeout", 248}}},
        {99, {{"presc", 13}, {"mult", 1}, {"dt_fifo_timeout", 564}}},
        {100, {{"presc", 13}, {"mult", 1}, {"dt_fifo_timeout", 564}}}};
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
        if (filtering_type_ == "STC_CUT_TRAIL")
        {
            register_controller_->writeRegisterField(prefix_ + "stc_param", "stc_enable", 1);
            register_controller_->writeRegisterField(prefix_ + "stc_param", "stc_threshold", threshold_ms_ * 1000);
            register_controller_->writeRegisterField(prefix_ + "stc_param", "disable_stc_cut_trail", 0);
            register_controller_->writeRegisterField(prefix_ + "trail_param", "trail_enable", 0);
        }
        else if (filtering_type_ == "STC_KEEP_TRAIL")
        {
            register_controller_->writeRegisterField(prefix_ + "stc_param", "stc_enable", 1);
            register_controller_->writeRegisterField(prefix_ + "stc_param", "stc_threshold", threshold_ms_ * 1000);
            register_controller_->writeRegisterField(prefix_ + "stc_param", "disable_stc_cut_trail", 1);
            register_controller_->writeRegisterField(prefix_ + "trail_param", "trail_enable", 0);
        }else if(filtering_type_ == "TRAIL")
        {
            register_controller_->writeRegisterField(prefix_ + "stc_param", "stc_enable", 0);
            register_controller_->writeRegisterField(prefix_ + "trail_param", "trail_enable", 1);
            register_controller_->writeRegisterField(prefix_ + "trail_param", "trail_threshold", threshold_ms_ * 1000);
        }
        register_controller_->writeRegisterField(prefix_ + "timestamping", "prescaler", stc_threshold_params[threshold_ms_]["presc"]);
        register_controller_->writeRegisterField(prefix_ + "timestamping", "multiplier", stc_threshold_params[threshold_ms_]["mult"]);
        register_controller_->writeRegisterField(prefix_ + "timestamping", "enable_last_ts_update_at_every_event", 1);

        register_controller_->writeRegisterField(prefix_ + "invalidation", "dt_fifo_timeout", stc_threshold_params[threshold_ms_]["dt_fifo_timeout"]);

        // Check sram init done
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
        // Enable filter
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