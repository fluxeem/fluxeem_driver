#ifndef __TOOL_INFO_PRIVATE_H__
#define __TOOL_INFO_PRIVATE_H__

#include <variant>
#include <string>
#include <fluxeem/hal/tools/tool_info.h>


namespace fluxeem {

    using DetailParameterInfo = std::variant<IntParameterInfo, FloatParameterInfo, BoolParameterInfo, EnumParameterInfo, StringParameterInfo>;

	struct FLUXEEM_API FullParameterInfo {
        BasicParameterInfo basic_info;
		DetailParameterInfo info;

        FullParameterInfo() :
            basic_info(BasicParameterInfo{ "NotDefined", "not defined", ToolParameterType::BOOL }),
            info(BoolParameterInfo{ false }) {}

        FullParameterInfo(std::string name, std::string description, ToolParameterType type, IntParameterInfo info)
            : basic_info(BasicParameterInfo{ name, description, type }), info(info) {}

        FullParameterInfo(std::string name, std::string description, ToolParameterType type, FloatParameterInfo info)
            : basic_info(BasicParameterInfo{ name, description, type }), info(info) {}

        FullParameterInfo(std::string name, std::string description, ToolParameterType type, BoolParameterInfo info)
            : basic_info(BasicParameterInfo{ name, description, type }), info(info) {}

        FullParameterInfo(std::string name, std::string description, ToolParameterType type, EnumParameterInfo info)
            : basic_info(BasicParameterInfo{ name, description, type }), info(info) {}

        FullParameterInfo(std::string name, std::string description, ToolParameterType type, StringParameterInfo info)
            : basic_info(BasicParameterInfo{name, description, type}), info(info) {}
    };

}	// namespace fluxeem

#endif