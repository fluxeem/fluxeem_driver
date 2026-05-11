#include <fluxeem/hal/tools/biases/imx636_biases.hpp>
#include <fluxeem/base/logging/logger.h>

namespace fluxeem
{
    struct BiasParamDef {
        const char* name;
        const char* desc;
        int min_offset;
        int max_offset;
    };

    static constexpr BiasParamDef BIAS_DEFS[] = {
        {"bias_fo", "FO bias", IMX636_BIAS_FO_MIN_OFFSET, IMX636_BIAS_FO_MAX_OFFSET},
        {"bias_hpf", "HPF bias", IMX636_BIAS_HPF_MIN_OFFSET, IMX636_BIAS_HPF_MAX_OFFSET},
        {"bias_diff_on", "DIFF_ON bias", IMX636_BIAS_DIFF_ON_MIN_OFFSET, IMX636_BIAS_DIFF_ON_MAX_OFFSET},
        {"bias_diff", "DIFF bias", IMX636_BIAS_DIFF_MIN_OFFSET, IMX636_BIAS_DIFF_MAX_OFFSET},
        {"bias_diff_off", "DIFF_OFF bias", IMX636_BIAS_DIFF_OFF_MIN_OFFSET, IMX636_BIAS_DIFF_OFF_MAX_OFFSET},
        {"bias_refr", "REFR bias", IMX636_BIAS_REFR_MIN_OFFSET, IMX636_BIAS_REFR_MAX_OFFSET},
    };

    Imx636Biases::Imx636Biases(std::shared_ptr<RegisterController> register_ctrl)
        : CameraToolRegister(register_ctrl, "bias/")
    {
        using IR = ParamConstraint::IntRange;
        for (size_t i = 0; i < NUM_BIASES; ++i)
        {
            const auto& def = BIAS_DEFS[i];
            registerParam({def.name, ParamType::Int, def.desc, "%", {IR{def.min_offset, def.max_offset, 0}}});
            reg_names_[i] = prefix_ + def.name;
        }
        readBaselines();
    }

    void Imx636Biases::readBaselines()
    {
        for (size_t i = 0; i < NUM_BIASES; ++i)
        {
            uint32_t reg_val = 0;
            if (!register_controller_->readRegister(reg_names_[i], reg_val))
            {
                LOG_ERROR("Failed to read baseline register: %s", reg_names_[i].c_str());
            }
            hw_baseline_[i] = static_cast<int>(reg_val & 0xff);
        }
    }

    bool Imx636Biases::onParamChanged(uint8_t idx, const ParamValue& old_val, const ParamValue& new_val)
    {
        (void)old_val;
        if (idx >= NUM_BIASES)
        {
            return false;
        }
        int offset = std::get<int>(new_val);
        uint32_t value_to_write = static_cast<uint32_t>(hw_baseline_[idx] + offset) | BIAS_CONF;
        return register_controller_->writeRegister(reg_names_[idx], value_to_write);
    }

    bool Imx636Biases::onParamRead(uint8_t idx, ParamValue& out_val) const
    {
        if (idx >= NUM_BIASES)
        {
            return false;
        }

        uint32_t reg_val = 0;
        if (!register_controller_->readRegister(reg_names_[idx], reg_val))
        {
            return false;
        }
        out_val = static_cast<int>(reg_val & 0xFF) - hw_baseline_[idx];
        return true;
    }

    std::map<std::string, BasicParameterInfo> Imx636Biases::getAllParamInfo()
    {
        std::map<std::string, BasicParameterInfo> result;
        for (const auto& desc : descriptors())
        {
            result[desc.name] = BasicParameterInfo{desc.name, desc.description, ToolParameterType::INT};
        }
        return result;
    }

    bool Imx636Biases::getParamInfo(const std::string name, IntParameterInfo& info)
    {
        auto idx = findParamIndex(name);
        if (!idx)
        {
            return false;
        }
        auto range = std::get<ParamConstraint::IntRange>(descriptors()[*idx].constraint.data);
        info.min = range.min;
        info.max = range.max;
        info.default_value = hw_baseline_[*idx];
        info.unit = "%";
        return true;
    }
} // namespace fluxeem
