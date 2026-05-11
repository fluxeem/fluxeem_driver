// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef __TOOLINFO_H__
#define __TOOLINFO_H__

#include <vector>
#include <string>
#include <functional>
#include <fluxeem/base/define/base_define.h>

namespace fluxeem
{
    /**
     * \~english @brief Enumeration of camera tool categories.
     * \~chinese @brief 相机工具类别枚举。
     * @ingroup fluxeem_camera_tools
     */
    enum class FLUXEEM_API ToolType
    {
        TOOL_BIAS,
        TOOL_TRIGGER_IN,
        TOOL_SYNC,
        TOOL_ANTI_FLICKER,
        TOOL_EVENT_TRAIL_FILTER,
        TOOL_EVENT_RATE_CONTROL,
        TOOL_ROI
    };

    /**
     * \~english @brief Convert a ToolType value to its string representation.
     * \~chinese @brief 将 ToolType 值转换为其字符串表示。
     */
    std::string FLUXEEM_API to_string(ToolType type);

    /**
     * \~english @brief Static metadata for a camera tool: its type, name, parameter list and description.
     * \~chinese @brief 相机工具的静态元数据：类型、名称、参数列表和描述。
     * @ingroup fluxeem_camera_tools
     */
    struct FLUXEEM_API ToolInfo
    {
        ToolType tool_type;                       ///< \~english Tool category \~chinese 工具类别
        std::string tool_name;                    ///< \~english Human-readable tool name \~chinese 工具的可读名称
        std::vector<std::string> parameter_names; ///< \~english Names of all exposed parameters \~chinese 所有暴露参数的名称
        std::string description;                  ///< \~english Free-form description \~chinese 自由格式描述
    };

    /**
     * \~english @brief Enumeration of parameter value types supported by camera tools.
     * \~chinese @brief 相机工具支持的参数值类型枚举。
     * @ingroup fluxeem_camera_tools
     */
    enum class FLUXEEM_API ToolParameterType
    {
        INT,
        FLOAT,
        BOOL,
        STRING,
        ENUM
    };

    /**
     * \~english @brief Convert a ToolParameterType value to its string representation.
     * \~chinese @brief 将 ToolParameterType 值转换为其字符串表示。
     *
     * \~english @param type The parameter type enum value.
     * \~chinese @param type 参数类型枚举值。
     * \~english @return String name of the parameter type.
     * \~chinese @return 参数类型的字符串名称。
     */
    std::string FLUXEEM_API ToolParameterTypeToString(ToolParameterType type);

    /**
     * \~english @brief Lightweight summary of a parameter: its name, description and value type.
     *                  For the full parameter descriptor (range, default, etc.) see @ref CameraTool::getParamInfo.
     * \~chinese @brief 参数的轻量概要：名称、描述和值类型。
     *                  完整描述（范围、默认值等）参见 @ref CameraTool::getParamInfo。
     * @ingroup fluxeem_camera_tools
     */
    struct FLUXEEM_API BasicParameterInfo {
        std::string name;           ///< \~english Parameter name \~chinese 参数名称
        std::string description;    ///< \~english Human-readable description \~chinese 可读描述
        ToolParameterType type;     ///< \~english Value type \~chinese 值类型
        std::string toString() {
			return "Name: " + name + " Description: " + description + " Type: " + ToolParameterTypeToString(type);
        }
    };

    /**
     * \~english @brief Full descriptor for an integer-valued parameter, including range and HW access callbacks.
     * \~chinese @brief 整数参数的完整描述符，包含范围和硬件访问回调。
     * @ingroup fluxeem_camera_tools
     */
    struct FLUXEEM_API IntParameterInfo {
        int min;                                    ///< \~english Minimum allowed value \~chinese 允许的最小值
        int max;                                    ///< \~english Maximum allowed value \~chinese 允许的最大值
        int default_value;                          ///< \~english Factory default \~chinese 出厂默认值
        std::string unit;                           ///< \~english Unit string (e.g. "mV") \~chinese 单位字符串（如 "mV"）
        std::function<bool(int&)> readValue;        ///< \~english HW read callback \~chinese 硬件读取回调
        std::function<bool(int)> writeValue;        ///< \~english HW write callback \~chinese 硬件写入回调
		std::string toString() {
			return "Min: " + std::to_string(min) + " Max: " + std::to_string(max) + " Default: " + std::to_string(default_value) + " Unit: " + unit;
		}
		int constraintValue(int value) {
			if (value < min) {
				return min;
			}
			if (value > max) {
				return max;
			}
			return value;
		}
    };

    /**
     * \~english @brief Full descriptor for a floating-point parameter, including range and HW access callbacks.
     * \~chinese @brief 浮点参数的完整描述符，包含范围和硬件访问回调。
     * @ingroup fluxeem_camera_tools
     */
    struct FLUXEEM_API FloatParameterInfo {
        float min;                                  ///< \~english Minimum allowed value \~chinese 允许的最小值
        float max;                                  ///< \~english Maximum allowed value \~chinese 允许的最大值
        float default_value;                        ///< \~english Factory default \~chinese 出厂默认值
        std::string unit;                           ///< \~english Unit string \~chinese 单位字符串
        std::function<bool(float&)> readValue;      ///< \~english HW read callback \~chinese 硬件读取回调
        std::function<bool(float)> writeValue;      ///< \~english HW write callback \~chinese 硬件写入回调
        std::string toString() {
			return "Min: " + std::to_string(min) + " Max: " + std::to_string(max) + " Default: " + std::to_string(default_value) + " Unit: " + unit;
        }
        float constraintValue(float value) {
			if (value < min) {
				return min;
			}
            if (value > max) {
				return max;
            }
            return value;
        }
    };

    /**
     * \~english @brief Descriptor for a boolean parameter with HW access callbacks.
     * \~chinese @brief 布尔参数描述符，含硬件访问回调。
     * @ingroup fluxeem_camera_tools
     */
    struct FLUXEEM_API BoolParameterInfo {
        bool default_value;                         ///< \~english Factory default \~chinese 出厂默认值
        std::function<bool(bool&)> readValue;       ///< \~english HW read callback \~chinese 硬件读取回调
        std::function<bool(bool)> writeValue;       ///< \~english HW write callback \~chinese 硬件写入回调
        std::string toString() {
			return "Default: " + std::to_string(default_value);
        }
    };

    /**
     * \~english @brief Descriptor for an enumeration parameter with HW access callbacks.
     * \~chinese @brief 枚举参数描述符，含硬件访问回调。
     * @ingroup fluxeem_camera_tools
     */
    struct FLUXEEM_API EnumParameterInfo {
        std::vector<std::string> options;           ///< \~english Allowed option strings \~chinese 允许的选项字符串
        std::string default_value;                  ///< \~english Factory default option \~chinese 出厂默认选项
        std::function<bool(std::string&)> readValue;  ///< \~english HW read callback \~chinese 硬件读取回调
        std::function<bool(std::string)> writeValue;  ///< \~english HW write callback \~chinese 硬件写入回调
        std::string toString() {
			return "Options: " + std::to_string(options.size()) + " Default: " + default_value;
        }
    };

    /**
     * \~english @brief Descriptor for a string parameter with HW access callbacks.
     * \~chinese @brief 字符串参数描述符，含硬件访问回调。
     * @ingroup fluxeem_camera_tools
     */
    struct FLUXEEM_API StringParameterInfo {
        std::string default_value;                  ///< \~english Factory default \~chinese 出厂默认值
        std::function<bool(std::string&)> readValue;  ///< \~english HW read callback \~chinese 硬件读取回调
        std::function<bool(std::string)> writeValue;  ///< \~english HW write callback \~chinese 硬件写入回调
        std::string toString() {
			return " Default: " + default_value;
        }
    };
    
} // namespace fluxeem


#endif
