#include <fluxeem/hal/tools/tool_info.h>
#include <fluxeem/hal/tools/tool_info_private.h>
#include <fluxeem/hal/tools/camera_tool.h>
#include <fluxeem/hal/tools/camera_tool_private.h>
#include <fluxeem/hal/registers/register_controller.h>
#include <fluxeem/base/logging/logger.h>
#include <string>
#include <memory>

namespace fluxeem
{
	bool CameraToolRegisterWithCallback::getParamInfo(const std::string name, IntParameterInfo& info)
	{

		if (!find_param(name, ToolParameterType::INT)) {
			return false;
		}

		info = std::get<IntParameterInfo>(parameter_info_map_[name].info);
		return true;
	}

	bool CameraToolRegisterWithCallback::getParamInfo(const std::string name, FloatParameterInfo& info) 
	{

		if (!find_param(name, ToolParameterType::FLOAT)) {
			return false;
		}

		info = std::get<FloatParameterInfo>(parameter_info_map_[name].info);
		return true;
	}

	bool CameraToolRegisterWithCallback::getParamInfo(const std::string name, BoolParameterInfo& info) 
	{

		if (!find_param(name, ToolParameterType::BOOL)) {
			return false;
		}

		info = std::get<BoolParameterInfo>(parameter_info_map_[name].info);
		return true;
	}

	bool CameraToolRegisterWithCallback::getParamInfo(const std::string name, EnumParameterInfo& info)
	{
		if (!find_param(name, ToolParameterType::ENUM))
		{
			return false;
		}

		info = std::get<EnumParameterInfo>(parameter_info_map_[name].info);
		return true;
	}

	bool CameraToolRegisterWithCallback::getParam(const std::string name, int& value)
	{
		if (!find_param(name, ToolParameterType::INT)) {
			return false;
		}
		int read_value;

		IntParameterInfo param_info = std::get<IntParameterInfo>(parameter_info_map_[name].info);
		if (!param_info.readValue(read_value))
		{
			return false;
		}
		value = read_value;
		return true;
	}

	bool CameraToolRegisterWithCallback::setParam(const std::string name, const char* value)
	{
		return setParam(name, std::string(value));
	}

	bool CameraToolRegisterWithCallback::setParam(const std::string name, const int& value)
	{
		if (!find_param(name, ToolParameterType::INT)) {
			return false;
		}

		IntParameterInfo param_info = std::get<IntParameterInfo>(parameter_info_map_[name].info);
		if (!param_info.writeValue(value))
		{
			return false;
		}
		return true;
	}

	bool CameraToolRegisterWithCallback::getParam(const std::string name, bool& value)
	{
		if (!find_param(name, ToolParameterType::BOOL)) {
			return false;
		}
		bool read_value;

		BoolParameterInfo param_info = std::get<BoolParameterInfo>(parameter_info_map_[name].info);
		if (!param_info.readValue(read_value))
		{
			return false;
		}
		value = read_value;
		return true;
	}

	bool CameraToolRegisterWithCallback::setParam(const std::string name, const bool& value)
	{
		if (!find_param(name, ToolParameterType::BOOL)) {
			return false;
		}

		BoolParameterInfo param_info = std::get<BoolParameterInfo>(parameter_info_map_[name].info);
		if (!param_info.writeValue(value))
		{
			return false;
		}
		return true;
	}

	bool CameraToolRegisterWithCallback::getParam(const std::string name, float& value)
	{
		if (!find_param(name, ToolParameterType::FLOAT)) {
			return false;
		}
		float read_value;

		FloatParameterInfo param_info = std::get<FloatParameterInfo>(parameter_info_map_[name].info);
		if (!param_info.readValue(read_value))
		{
			return false;
		}
		value = read_value;
		return true;
	}

	bool CameraToolRegisterWithCallback::setParam(const std::string name, const float& value)
	{
		if (!find_param(name, ToolParameterType::FLOAT)) {
			return false;
		}

		FloatParameterInfo param_info = std::get<FloatParameterInfo>(parameter_info_map_[name].info);
		if (!param_info.writeValue(value))
		{
			return false;
		}
		return true;
	}

	bool CameraToolRegisterWithCallback::getParam(const std::string name, std::string& value)
	{
		if (find_param(name, ToolParameterType::ENUM))
		{
			EnumParameterInfo param_info = std::get<EnumParameterInfo>(parameter_info_map_[name].info);
			if (!param_info.readValue(value))
			{

				return false;
			}
			return true;
		}
		else if (find_param(name, ToolParameterType::STRING))
		{
			StringParameterInfo param_info = std::get<StringParameterInfo>(parameter_info_map_[name].info);
			if (!param_info.readValue(value))
			{

				return false;
			}
			return true;
		}

		return false;
	}

	bool CameraToolRegisterWithCallback::setParam(const std::string name, const std::string &value)
	{
		if (find_param(name, ToolParameterType::ENUM))
		{
			EnumParameterInfo param_info = std::get<EnumParameterInfo>(parameter_info_map_[name].info);
			if (!param_info.writeValue(value))
			{
				return false;
			}
			return true;
		}
		else if (find_param(name, ToolParameterType::STRING))
		{
			StringParameterInfo param_info = std::get<StringParameterInfo>(parameter_info_map_[name].info);
			if (!param_info.writeValue(value))
			{
				return false;
			}

			return true;
		}
		return false;
	}
}