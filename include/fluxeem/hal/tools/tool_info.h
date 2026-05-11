#ifndef __TOOLINFO_H__
#define __TOOLINFO_H__

#include <vector>
#include <string>
#include <functional>
#include <fluxeem/base/define/base_define.h>

namespace fluxeem
{
    /**
     * \~english @brief The type of the tool.
     * \~chinese @brief 工具的类型。
     * @ingroup camera_tools
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

    std::string FLUXEEM_API to_string(ToolType type);

    /**
     * \~english @brief The information of the tool, including the type, the parameter names and the description.
     * \~chinese @brief 工具的信息，包括类型、可以更改的参数名称和描述。
     * @ingroup camera_tools
     */
    struct FLUXEEM_API ToolInfo
    {
        ToolType tool_type;
        std::string tool_name;
        std::vector<std::string> parameter_names;
        std::string description;
    };

    /**
     * \~english @brief The type of the parameter.
     * \~chinese @brief 参数的类型。
     * @ingroup camera_tools
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
     * \~english @brief Convert the ToolParameterType to string.
     * \~chinese @brief 将 ToolParameterType 转换为字符串。
     * 
     * \~english @param type The type of the parameter. 
     * \~chinese @param type 参数的类型。
     * \~english @return \~english The string of the parameter type. 
     * \~chinese @return 参数类型的字符串。
     */
    std::string FLUXEEM_API ToolParameterTypeToString(ToolParameterType type);

    /**
     * \~english @brief The basic information of the parameter, including the name, the description and the type. \
     *      To get the detailed information, please refer to @ref CameraTool::getParamInfo .
     * \~chinese @brief 参数的基本信息，包括名称、描述和类型。\
     *      要获取详细信息，请参考 @ref CameraTool::getParamInfo 。
     * @ingroup camera_tools
     */
    struct FLUXEEM_API BasicParameterInfo {
        std::string name;
        std::string description;
        ToolParameterType type;
        std::string toString() {
			return "Name: " + name + " Description: " + description + " Type: " + ToolParameterTypeToString(type);
        }
    };

    /**
     * \~english @brief The detailed information of the integer parameter.
     * \~chinese @brief 整数参数的详细信息。
     * @ingroup camera_tools
     */
    struct FLUXEEM_API IntParameterInfo {
        int min;
        int max;
        int default_value;
        std::string unit;
        std::function<bool(int&)> readValue;
        std::function<bool(int)> writeValue;
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
     * \~english @brief The detailed information of the float parameter.
     * \~chinese @brief 浮点数参数的详细信息。
     * @ingroup camera_tools
     */
    struct FLUXEEM_API FloatParameterInfo {
        float min;
        float max;
        float default_value;
        std::string unit;
        std::function<bool(float&)> readValue;
        std::function<bool(float)> writeValue;
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
     * \~english @brief The detailed information of the boolean parameter.
     * \~chinese @brief 布尔参数的详细信息。
     * @ingroup camera_tools
     */
    struct FLUXEEM_API BoolParameterInfo {
        bool default_value;
        std::function<bool(bool&)> readValue;
        std::function<bool(bool)> writeValue;
        std::string toString() {
			return "Default: " + std::to_string(default_value);
        }
    };

    /**
     * \~english @brief The detailed information of the enumeration parameter.
     * \~chinese @brief 枚举参数的详细信息。
     * @ingroup camera_tools
     */
    struct FLUXEEM_API EnumParameterInfo {
        std::vector<std::string> options;
        std::string default_value;
        std::function<bool(std::string&)> readValue;
        std::function<bool(std::string)> writeValue;
        std::string toString() {
			return "Options: " + std::to_string(options.size()) + " Default: " + default_value;
        }
    };  

    struct FLUXEEM_API StringParameterInfo {
        std::string default_value;
        std::function<bool(std::string&)> readValue;
        std::function<bool(std::string)> writeValue;
        std::string toString() {
			return " Default: " + default_value;
        }
    };  
    
} // namespace fluxeem


#endif