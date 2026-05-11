// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef PARAM_DESCRIPTOR_H_
#define PARAM_DESCRIPTOR_H_

#include <string>
#include <variant>
#include <vector>
#include <fluxeem/base/define/base_define.h>

namespace fluxeem {

/// \~english @brief Variant type that can hold any parameter value (int, float, bool, or string).
/// \~chinese @brief 可持有任意参数值（int、float、bool 或 string）的变体类型。
using ParamValue = std::variant<int, float, bool, std::string>;

/**
 * \~english @brief Enumeration of parameter value types in the descriptor-based interface.
 * \~chinese @brief 描述符接口中的参数值类型枚举。
 * @ingroup fluxeem_camera_tools
 */
enum class ParamType : uint8_t {
    Int,    ///< \~english Integer parameter \~chinese 整数参数
    Float,  ///< \~english Floating-point parameter \~chinese 浮点参数
    Bool,   ///< \~english Boolean parameter \~chinese 布尔参数
    String, ///< \~english String parameter \~chinese 字符串参数
    Enum    ///< \~english Enumeration parameter \~chinese 枚举参数
};

/**
 * \~english @brief Constraint specification for a parameter (range, default, or allowed options).
 * \~chinese @brief 参数约束规格（范围、默认值或允许选项）。
 * @ingroup fluxeem_camera_tools
 */
struct FLUXEEM_API ParamConstraint {
    /// \~english @brief Integer range constraint: min / max / default.
    /// \~chinese @brief 整数范围约束：最小值 / 最大值 / 默认值。
    struct IntRange { int min; int max; int default_val; };
    /// \~english @brief Float range constraint: min / max / default.
    /// \~chinese @brief 浮点范围约束：最小值 / 最大值 / 默认值。
    struct FloatRange { float min; float max; float default_val; };
    /// \~english @brief Boolean default value.
    /// \~chinese @brief 布尔默认值。
    struct BoolDef { bool default_val; };
    /// \~english @brief Enumeration constraint: allowed options and default.
    /// \~chinese @brief 枚举约束：允许选项和默认值。
    struct EnumDef { std::vector<std::string> options; std::string default_val; };
    /// \~english @brief String default value.
    /// \~chinese @brief 字符串默认值。
    struct StringDef { std::string default_val; };

    /// \~english @brief The actual constraint payload — one of the above alternatives.
    /// \~chinese @brief 实际约束载荷——上述替代类型之一。
    std::variant<IntRange, FloatRange, BoolDef, EnumDef, StringDef> data;
};

/**
 * \~english @brief Full descriptor for a single parameter: its name, type, description, unit and constraint.
 * \~chinese @brief 单个参数的完整描述符：名称、类型、描述、单位和约束。
 * @ingroup fluxeem_camera_tools
 */
struct FLUXEEM_API ParamDescriptor {
    std::string name;          ///< \~english Parameter name \~chinese 参数名称
    ParamType type;            ///< \~english Value type \~chinese 值类型
    std::string description;   ///< \~english Human-readable description \~chinese 可读描述
    std::string unit;          ///< \~english Unit string (e.g. "mV", "Hz") \~chinese 单位字符串（如 "mV"、"Hz"）
    ParamConstraint constraint; ///< \~english Range/default constraint \~chinese 范围/默认值约束
};

} // namespace fluxeem

#endif // PARAM_DESCRIPTOR_H_
