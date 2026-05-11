// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#include <fluxeem/hal/tools/camera_tool.h>
#include <fluxeem/hal/tools/tool_info.h>
#include <fluxeem/base/logging/logger.h>
#include <algorithm>
#include <type_traits>

namespace fluxeem
{
    namespace {
        ParamValue defaultValueFromConstraint(const ParamConstraint& constraint) {
            return std::visit([](const auto& c) -> ParamValue {
                using T = std::decay_t<decltype(c)>;
                if constexpr (std::is_same_v<T, ParamConstraint::IntRange>) {
                    return c.default_val;
                } else if constexpr (std::is_same_v<T, ParamConstraint::FloatRange>) {
                    return c.default_val;
                } else if constexpr (std::is_same_v<T, ParamConstraint::BoolDef>) {
                    return c.default_val;
                } else if constexpr (std::is_same_v<T, ParamConstraint::EnumDef>) {
                    return c.default_val;
                } else {
                    return c.default_val;
                }
            }, constraint.data);
        }
    }

    const std::vector<ParamDescriptor>& CameraTool::descriptors() const {
        return descriptors_;
    }

    std::optional<uint8_t> CameraTool::findParamIndex(std::string_view name) const {
        for (size_t i = 0; i < descriptors_.size(); ++i) {
            if (descriptors_[i].name == name) {
                return static_cast<uint8_t>(i);
            }
        }
        return std::nullopt;
    }

    void CameraTool::registerParam(ParamDescriptor desc) {
        values_.push_back(defaultValueFromConstraint(desc.constraint));
        descriptors_.push_back(std::move(desc));
    }

    std::optional<ParamValue> CameraTool::get(std::string_view name) const {
        auto idx = findParamIndex(name);
        if (!idx) {
            return std::nullopt;
        }

        ParamValue fresh;
        if (onParamRead(*idx, fresh)) {
            return fresh;
        }

        return values_[*idx];
    }

    bool CameraTool::set(std::string_view name, const ParamValue& value) {
        auto idx = findParamIndex(name);
        if (!idx) {
            return false;
        }

        const auto& desc = descriptors_[*idx];
        bool type_ok = false;
        switch (desc.type) {
            case ParamType::Int:
                type_ok = std::holds_alternative<int>(value);
                break;
            case ParamType::Float:
                type_ok = std::holds_alternative<float>(value);
                break;
            case ParamType::Bool:
                type_ok = std::holds_alternative<bool>(value);
                break;
            case ParamType::String:
            case ParamType::Enum:
                type_ok = std::holds_alternative<std::string>(value);
                break;
        }
        if (!type_ok) {
            return false;
        }

        ParamValue normalized = value;
        if (desc.type == ParamType::Int) {
            auto range = std::get<ParamConstraint::IntRange>(desc.constraint.data);
            int v = std::get<int>(normalized);
            normalized = std::clamp(v, range.min, range.max);
        } else if (desc.type == ParamType::Float) {
            auto range = std::get<ParamConstraint::FloatRange>(desc.constraint.data);
            float v = std::get<float>(normalized);
            normalized = std::clamp(v, range.min, range.max);
        } else if (desc.type == ParamType::Enum) {
            const auto& enum_def = std::get<ParamConstraint::EnumDef>(desc.constraint.data);
            const std::string& input = std::get<std::string>(normalized);
            if (std::find(enum_def.options.begin(), enum_def.options.end(), input) == enum_def.options.end()) {
                return false;
            }
        }

        ParamValue old = values_[*idx];
        values_[*idx] = normalized;
        if (!onParamChanged(*idx, old, normalized)) {
            values_[*idx] = old;
            return false;
        }
        return true;
    }

    nlohmann::json CameraTool::toJson() const {
        nlohmann::json result = nlohmann::json::object();
        for (size_t i = 0; i < descriptors_.size(); ++i) {
            const std::string& name = descriptors_[i].name;
            auto value = get(name);
            if (!value) {
                continue;
            }
            std::visit([&](const auto& v) { result[name] = v; }, *value);
        }
        return result;
    }

    bool CameraTool::fromJson(const nlohmann::json& j) {
        for (const auto& [key, val] : j.items()) {
            auto idx = findParamIndex(key);
            if (!idx) {
                continue;
            }

            const auto& desc = descriptors_[*idx];
            switch (desc.type) {
                case ParamType::Int:
                    if (val.is_number_integer()) set(key, ParamValue(val.get<int>()));
                    break;
                case ParamType::Float:
                    if (val.is_number()) set(key, ParamValue(val.get<float>()));
                    break;
                case ParamType::Bool:
                    if (val.is_boolean()) set(key, ParamValue(val.get<bool>()));
                    break;
                case ParamType::String:
                case ParamType::Enum:
                    if (val.is_string()) set(key, ParamValue(val.get<std::string>()));
                    break;
            }
        }
        return true;
    }

    bool CameraTool::getParam(const std::string name, int& value) {
        auto result = get(name);
        if (result.has_value() && std::holds_alternative<int>(*result)) {
            value = std::get<int>(*result);
            return true;
        }
        LOG_ERROR("Get int parameter '%s' not implemented", name.c_str());
        return false;
    }
    bool CameraTool::getParam(const std::string name, float& value) {
        auto result = get(name);
        if (result.has_value() && std::holds_alternative<float>(*result)) {
            value = std::get<float>(*result);
            return true;
        }
        LOG_ERROR("Get float parameter '%s' not implemented", name.c_str());
        return false;
    }
    bool CameraTool::getParam(const std::string name, bool& value){
        auto result = get(name);
        if (result.has_value() && std::holds_alternative<bool>(*result)) {
            value = std::get<bool>(*result);
            return true;
        }
        LOG_ERROR("Get bool parameter '%s' not implemented", name.c_str());
        return false;
    }
    bool CameraTool::getParam(const std::string name, std::string& value) {
        auto result = get(name);
        if (result.has_value() && std::holds_alternative<std::string>(*result)) {
            value = std::get<std::string>(*result);
            return true;
        }
        LOG_ERROR("Get string parameter '%s' not implemented", name.c_str());
        return false;
    }

    bool CameraTool::setParam(const std::string name, const int& value) {
        auto idx = findParamIndex(name);
        if (idx) {
            return set(name, ParamValue(value));
        }
        LOG_ERROR("Set int parameter '%s' not implemented", name.c_str());
        return false;
    }
    bool CameraTool::setParam(const std::string name, const float& value) {
        auto idx = findParamIndex(name);
        if (idx) {
            return set(name, ParamValue(value));
        }
        LOG_ERROR("Set float parameter '%s' not implemented", name.c_str());
        return false;
    }
    bool CameraTool::setParam(const std::string name, const double& value) {
        return setParam(name, static_cast<float>(value));
    }
    bool CameraTool::setParam(const std::string name, const bool& value) {
        auto idx = findParamIndex(name);
        if (idx) {
            return set(name, ParamValue(value));
        }
        LOG_ERROR("Set bool parameter '%s' not implemented", name.c_str());
        return false;
    }

    bool CameraTool::setParam(const std::string name, const char* value) {
        return setParam(name, std::string(value));
    }

    bool CameraTool::setParam(const std::string name, const std::string& value) {
        auto idx = findParamIndex(name);
        if (idx) {
            return set(name, ParamValue(value));
        }
        LOG_ERROR("Set string parameter '%s' not implemented", name.c_str());
        return false;
    }
    
} // namespace fluxeem
