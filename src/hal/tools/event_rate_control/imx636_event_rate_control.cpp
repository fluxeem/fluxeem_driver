#include <fluxeem/hal/tools/tool_info.h>
#include <fluxeem/hal/tools/tool_info_private.h>
#include <fluxeem/hal/tools/camera_tool.h>
#include <fluxeem/hal/tools/camera_tool_private.h>
#include <fluxeem/hal/registers/register_controller.h>
#include <fluxeem/hal/tools/event_rate_control/imx636_event_rate_control.hpp>
#include <fluxeem/base/utility/func_utils.h>
#include <fluxeem/base/logging/logger.h>
#include <string>
#include <memory>
#include <cstdint>
#include <map>
#include <tuple>

namespace fluxeem
{
    Imx636EventRateControl::Imx636EventRateControl(std::shared_ptr<RegisterController> register_ctrl)
        : CameraToolRegisterWithCallback(register_ctrl, "erc/")
    {
        // Set Default Configuration
        for (auto i = 0; i < 230; ++i)
        {
            lut_configs_["Reserved"][i] = std::make_tuple(0x8, 0x8, 0x8, 0x8);
        }
        for (auto i = 0; i < 256; ++i)
        {
            lut_configs_["t_drop_lut"][i] = std::make_tuple((i * 2) + 0, (i * 2) + 1, 0, 0);
        }
        for (auto& info : imx636_infos_)
        {
            addRegister2Map(info);
        }
        
    }

    bool Imx636EventRateControl::setEnable(bool en)
    {
        if (!inited_)
        {
            initialize();
        }
        register_controller_->writeRegisterField(prefix_ + "t_dropping_control", "t_dropping_en", en);

        if (en)
        {
            setMaxEventRate(event_rate_);
        }

        return true;
    }

    bool Imx636EventRateControl::getEnable(bool &en)
    {
        uint32_t read_val = 0;
        register_controller_->readRegisterField(prefix_ + "Reserved_6000", "Reserved_1_0", read_val);
        bool res = (read_val == 1) ? true : false;
        register_controller_->readRegisterField(prefix_ + "t_dropping_control", "t_dropping_en", read_val);
        bool t_dropping_en = (read_val == 1) ? true : false;
        en = t_dropping_en && res;
        return true;
    }

    bool Imx636EventRateControl::setMaxEventRate(int rate)
    {
        if(rate > max_event_rate_)
        {
            LOG_ERROR("Current event rate %d MEv/s exceed the limit %d MEv/s", rate, max_event_rate_);
            return false;
        }
        event_rate_ = rate;
        uint32_t count_period = getCountPeriod();
        rate *= 1000 * 1000;//Convert Ev/s to MEv/s
        register_controller_->writeRegister(prefix_ + "td_target_event_rate", static_cast<uint32_t>(static_cast<uint64_t>(rate) * count_period / 1000000));
        
        LOG_DEBUG("set event rate(Ev/s): %d", rate);
        return true;
    }

    bool Imx636EventRateControl::getMaxEventRate(int &rate)
    {
        uint32_t count_period = getCountPeriod();
        uint32_t target_rate = 0;
        register_controller_->readRegister(prefix_ + "td_target_event_rate", target_rate);
        rate = static_cast<uint32_t>(static_cast<uint64_t>(target_rate) * 1000000 / count_period);
        rate /= 1000000;
        return true;
    }

    void Imx636EventRateControl::initialize()
    {
        LOG_INFO("Imx636 ERC Init");

        register_controller_->writeRegisterField(prefix_ + "Reserved_6000", "Reserved_1_0", 0);

        register_controller_->writeRegisterField(prefix_ + "in_drop_rate_control", "cfg_event_delay_fifo_en", 1);
        register_controller_->writeRegisterField(prefix_ + "reference_period", "erc_reference_period", 200);

        register_controller_->writeRegisterField(prefix_ + "td_target_event_rate", "target_event_rate", 10000);
        register_controller_->writeRegisterField(prefix_ + "erc_enable", "erc_en", 1);
        register_controller_->writeRegisterField(prefix_ + "erc_enable", "Reserved_1", 1);
        register_controller_->writeRegisterField(prefix_ + "erc_enable", "Reserved_2", 0);

        register_controller_->writeRegisterField(prefix_ + "Reserved_602C", "Reserved_0", 1);
        for (auto i = 0; i < 230; ++i)
        {
            register_controller_->writeRegisterField(prefix_ + "Reserved_" + hexToString((26624 + 4 * i), 4), "Reserved_5_0", std::get<0>(lut_configs_["Reserved"][i]));
            register_controller_->writeRegisterField(prefix_ + "Reserved_" + hexToString((26624 + 4 * i), 4), "Reserved_13_8", std::get<1>(lut_configs_["Reserved"][i]));
            register_controller_->writeRegisterField(prefix_ + "Reserved_" + hexToString((26624 + 4 * i), 4), "Reserved_21_16", std::get<2>(lut_configs_["Reserved"][i]));
            register_controller_->writeRegisterField(prefix_ + "Reserved_" + hexToString((26624 + 4 * i), 4), "Reserved_29_24", std::get<3>(lut_configs_["Reserved"][i]));
        }
        register_controller_->writeRegisterField(prefix_ + "Reserved_602C", "Reserved_0", 0);

        int j = 0;

        for (auto i = 0; i < 256; ++i)
        {
            std::string lut_index;
            std::string lut_field_index_0;
            std::string lut_field_index_1;

            lut_index = intToString(i, 2);
            lut_field_index_0 = intToString(j, 3);
            lut_field_index_1 = intToString(j + 1, 3);

            register_controller_->writeRegisterField(prefix_ + "t_drop_lut_" + lut_index, "tlut" + lut_field_index_0, std::get<0>(lut_configs_["t_drop_lut"][i]));
            register_controller_->writeRegisterField(prefix_ + "t_drop_lut_" + lut_index, "tlut" + lut_field_index_1, std::get<1>(lut_configs_["t_drop_lut"][i]));
            j += 2;
        }
        register_controller_->writeRegisterField(prefix_ + "t_dropping_control", "t_dropping_en", 0);
        register_controller_->writeRegisterField(prefix_ + "h_dropping_control", "h_dropping_en", 0);
        register_controller_->writeRegisterField(prefix_ + "v_dropping_control", "v_dropping_en", 0);
        register_controller_->writeRegisterField(prefix_ + "Reserved_6000", "Reserved_1_0", 1);
    }
    
    uint32_t Imx636EventRateControl::getCountPeriod() const {
        uint32_t val = 0;
        register_controller_->readRegister(prefix_ + "reference_period", val);
        return val;
    }

    bool Imx636EventRateControl::addRegister2Map(const FullParameterInfo& info)
    {
        parameter_info_map_.emplace(info.basic_info.name, info);

        return true;
    }
}