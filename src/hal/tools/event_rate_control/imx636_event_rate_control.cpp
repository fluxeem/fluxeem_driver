// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#include <fluxeem/hal/tools/tool_info.h>
#include <fluxeem/hal/tools/tool_info_private.h>
#include <fluxeem/hal/tools/camera_tool.h>
#include <fluxeem/hal/tools/camera_tool_private.h>
#include <fluxeem/hal/registers/register_controller.h>
#include <fluxeem/hal/tools/event_rate_control/imx636_event_rate_control.hpp>
#include <fluxeem/base/utility/func_utils.h>
#include <fluxeem/base/logging/logger.h>
#include <array>
#include <string>
#include <memory>
#include <cstdint>
#include <map>
#include <tuple>

namespace
{
    constexpr uint32_t kReservedLutSize = 230;
    constexpr uint32_t kDropLutSize = 256;
    constexpr uint32_t kReservedBaseAddress = 26624;
    constexpr uint32_t kRegisterStride = 4;
    constexpr const char *kReservedBank = "Reserved";
    constexpr const char *kDropBank = "t_drop_lut";

    using LutEntry = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>;

    LutEntry makeReservedEntry()
    {
        return std::make_tuple(0x8, 0x8, 0x8, 0x8);
    }

    LutEntry makeDropEntry(uint32_t index)
    {
        return std::make_tuple((index * 2) + 0, (index * 2) + 1, 0, 0);
    }

    bool readRegisterFlag(const std::shared_ptr<fluxeem::RegisterController> &controller,
                          const std::string &register_name,
                          const std::string &field_name)
    {
        uint32_t value = 0;
        controller->readRegisterField(register_name, field_name, value);
        return value == 1;
    }

    void writeTupleFields(const std::shared_ptr<fluxeem::RegisterController> &controller,
                          const std::string &register_name,
                          const std::array<const char *, 4> &field_names,
                          const LutEntry &entry)
    {
        controller->writeRegisterField(register_name, field_names[0], std::get<0>(entry));
        controller->writeRegisterField(register_name, field_names[1], std::get<1>(entry));
        controller->writeRegisterField(register_name, field_names[2], std::get<2>(entry));
        controller->writeRegisterField(register_name, field_names[3], std::get<3>(entry));
    }

    uint32_t targetRateRegisterValue(uint32_t rate_mev_per_second, uint32_t count_period)
    {
        constexpr uint32_t kMega = 1000 * 1000;
        const uint64_t rate_events_per_second = static_cast<uint64_t>(rate_mev_per_second) * kMega;
        return static_cast<uint32_t>(rate_events_per_second * count_period / kMega);
    }
}

namespace fluxeem
{
    Imx636EventRateControl::Imx636EventRateControl(std::shared_ptr<RegisterController> register_ctrl)
        : CameraToolRegisterWithCallback(register_ctrl, "erc/")
    {
        // 加载默认配置
        for (uint32_t i = 0; i < kReservedLutSize; ++i)
        {
            lut_configs_[kReservedBank][i] = makeReservedEntry();
        }
        for (uint32_t i = 0; i < kDropLutSize; ++i)
        {
            lut_configs_[kDropBank][i] = makeDropEntry(i);
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
        const bool reserved_ready = readRegisterFlag(register_controller_, prefix_ + "Reserved_6000", "Reserved_1_0");
        const bool time_drop_enabled = readRegisterFlag(register_controller_, prefix_ + "t_dropping_control", "t_dropping_en");
        en = time_drop_enabled && reserved_ready;
        return true;
    }

    bool Imx636EventRateControl::setMaxEventRate(int rate)
    {
        if (rate < IMX636_EVENT_RATE_MEV_MIN || static_cast<uint32_t>(rate) > max_event_rate_)
        {
            LOG_ERROR("Current event rate %d MEv/s exceed the limit %d MEv/s", rate, max_event_rate_);
            return false;
        }
        event_rate_ = rate;
        uint32_t count_period = getCountPeriod();
        const uint32_t target_rate = targetRateRegisterValue(static_cast<uint32_t>(rate), count_period);
        register_controller_->writeRegister(prefix_ + "td_target_event_rate", target_rate);
        
        LOG_DEBUG("set event rate(MEv/s): %d", rate);
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
        constexpr std::array<const char *, 4> kReservedFields{
            "Reserved_5_0",
            "Reserved_13_8",
            "Reserved_21_16",
            "Reserved_29_24",
        };
        for (uint32_t i = 0; i < kReservedLutSize; ++i)
        {
            const auto register_name = prefix_ + "Reserved_" + hexToString(kReservedBaseAddress + kRegisterStride * i, 4);
            writeTupleFields(register_controller_, register_name, kReservedFields, lut_configs_[kReservedBank][i]);
        }
        register_controller_->writeRegisterField(prefix_ + "Reserved_602C", "Reserved_0", 0);

        uint32_t lut_field_offset = 0;
        for (uint32_t i = 0; i < kDropLutSize; ++i)
        {
            const auto register_name = prefix_ + "t_drop_lut_" + intToString(static_cast<int>(i), 2);
            const auto &entry = lut_configs_[kDropBank][i];
            register_controller_->writeRegisterField(register_name, "tlut" + intToString(static_cast<int>(lut_field_offset), 3), std::get<0>(entry));
            register_controller_->writeRegisterField(register_name, "tlut" + intToString(static_cast<int>(lut_field_offset + 1), 3), std::get<1>(entry));
            lut_field_offset += 2;
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