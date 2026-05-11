#ifndef __CAMERA_TOOL_PRIVATE_H__
#define __CAMERA_TOOL_PRIVATE_H__

#include <fluxeem/hal/tools/tool_info.h>
#include <fluxeem/hal/tools/tool_info_private.h>
#include <fluxeem/hal/tools/camera_tool.h>
#include <fluxeem/hal/registers/register_controller.h>
#include <fluxeem/base/logging/logger.h>
#include <string>
#include <map>
#include <memory>
#include <variant>

namespace fluxeem {

	class FLUXEEM_API CameraToolBasePrivate : public CameraTool
	{
	public:
		CameraToolBasePrivate(std::string prefix) : CameraTool(prefix) {}

		std::map<std::string, BasicParameterInfo> getAllParamInfo() override {
			std::map<std::string, BasicParameterInfo> result;
			for (auto& [name, info] : parameter_info_map_) {
				result[info.basic_info.name] = info.basic_info;
			}
			return result;
		}

		virtual bool getParamInfo(const std::string name, IntParameterInfo &info) override {
			std::string full_name = prefix_ + name;
			if (!find_param(full_name, ToolParameterType::INT)) {
				return false;
			}
			info = std::get<IntParameterInfo>(parameter_info_map_[full_name].info);
			return true;
		}
		virtual bool getParamInfo(const std::string name, FloatParameterInfo &info)  override {
			std::string full_name = prefix_ + name;
			if (!find_param(full_name, ToolParameterType::FLOAT)) {
				return false;
			}
			info = std::get<FloatParameterInfo>(parameter_info_map_[full_name].info);
			return true;
		}
		virtual bool getParamInfo(const std::string name, BoolParameterInfo &info)  override {
			std::string full_name = prefix_ + name;
			if (!find_param(full_name, ToolParameterType::BOOL)) {
				return false;
			}
			info = std::get<BoolParameterInfo>(parameter_info_map_[full_name].info);
			return true;
		}
		virtual bool getParamInfo(const std::string name, EnumParameterInfo &info)  override {
			std::string full_name = prefix_ + name;
			if (!find_param(full_name, ToolParameterType::ENUM)) {
				return false;
			}
			info = std::get<EnumParameterInfo>(parameter_info_map_[full_name].info);
			return true;
		}

		virtual bool getParamInfo(const std::string name, StringParameterInfo &info) override
		{
			std::string full_name = prefix_ + name;
			if (!find_param(full_name, ToolParameterType::STRING))
			{
				return false;
			}
			info = std::get<StringParameterInfo>(parameter_info_map_[full_name].info);
			return true;
		}

	protected:
		bool find_param(const std::string name, const ToolParameterType parameter_type) {
			if (parameter_info_map_.find(name) == parameter_info_map_.end())
			{
				LOG_ERROR("Parameter %s not found", name.data());
				return false;
			}
			if (parameter_info_map_[name].basic_info.type != parameter_type) {
				LOG_ERROR("Parameter *s is not of type %s", name.data(), ToolParameterTypeToString(parameter_type).data());
				return false;
			}
			return true;
		}
		std::map<std::string, FullParameterInfo> parameter_info_map_;
	};

	class FLUXEEM_API CameraToolRegister : public CameraToolBasePrivate
	{
	public:
		CameraToolRegister(std::shared_ptr<RegisterController> register_controller, std::string prefix) : 
			CameraToolBasePrivate(prefix), register_controller_(register_controller){}

	protected:
		std::shared_ptr<RegisterController> register_controller_;
	};

	class FLUXEEM_API CameraToolRegisterWithCallback : public CameraToolRegister
	{
	public:
		CameraToolRegisterWithCallback(std::shared_ptr<RegisterController> register_controller, std::string prefix) :
			CameraToolRegister(register_controller, prefix) {}


		virtual bool getParamInfo(const std::string name, IntParameterInfo& info) override;

		virtual bool getParamInfo(const std::string name, FloatParameterInfo& info) override;

		virtual bool getParamInfo(const std::string name, BoolParameterInfo& info) override;

		virtual bool getParamInfo(const std::string name, EnumParameterInfo& info) override;

		// rewrite for types parameter
		bool getParam(const std::string name, int& value) override;
		bool setParam(const std::string name, const int& value) override;
		bool getParam(const std::string name, bool& value) override;
		bool setParam(const std::string name, const bool& value) override;
		bool getParam(const std::string name, float& value) override;
		bool setParam(const std::string name, const float& value) override;
		bool getParam(const std::string name, std::string& value) override;
		bool setParam(const std::string name, const char* value) override;
		bool setParam(const std::string name, const std::string& value) override;
	};
}	// namespace fluxeem

#endif
