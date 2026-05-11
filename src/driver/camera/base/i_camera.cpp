#include <fluxeem/driver/camera/base/i_camera.hpp>
#include <fluxeem/hal/tools/camera_tool.h>
#include <fluxeem/hal/tools/tool_info.h>
#include <fluxeem/base/logging/logger.h>
#include <fstream>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace fluxeem
{

	ICamera::ICamera(CameraDescription camera_desc)
		: camera_desc_(std::move(camera_desc))
	{
	}

	std::shared_ptr<CameraTool> ICamera::getTool(ToolType type) const
	{
		const auto it = tools_.find(type);
		if (it == tools_.end())
		{
			LOG_ERROR("Tool not found");
			return nullptr;
		}
		return it->second;
	}

	std::shared_ptr<CameraTool> ICamera::getTool(const std::string& tool_name) const
	{
		const std::string &tool_name_ref = tool_name;

		for (const auto &pair : tools_)
		{
			const auto &tool = pair.second;
			if (tool && tool->getToolInfo().tool_name == tool_name_ref)
			{
				return tool;
			}
		}
		return nullptr;
	}

	std::vector<ToolInfo> ICamera::getToolsInfo() const
	{
		std::vector<ToolInfo> tools_info;
		tools_info.reserve(tools_.size());

		for (const auto &tool_pair : tools_)
		{
			const auto &tool = tool_pair.second;
			if (tool)
			{
				tools_info.push_back(tool->getToolInfo());
			}
		}

		return tools_info;
	}

	ToolInfo ICamera::getToolInfo(ToolType type) const
	{
		const auto it = tools_.find(type);
		if (it == tools_.end() || !it->second)
		{
			LOG_ERROR("Tool not found");
			throw std::runtime_error("Tool not found");
		}

		return it->second->getToolInfo();
	}

	bool ICamera::exportCameraConfig(const std::string& json_file_path)
	{
		nlohmann::json config_params;

		for (const auto &tool_info : getToolsInfo())
		{
			const auto tool = getTool(tool_info.tool_type);
			if (!tool)
			{
				LOG_WARN("Tool not found: %s, skipping...", tool_info.tool_name.c_str());
				continue;
			}

			const auto infos = tool->getAllParamInfo();
			config_params[tool_info.tool_name] = nlohmann::json::object();

			for (const auto &info : infos)
			{
				const auto &param_name = info.first;
				const auto &param_info = info.second;

				if (param_info.type == fluxeem::ToolParameterType::INT)
				{
					int value = 0;
					tool->getParam(param_name, value);
					config_params[tool_info.tool_name][param_name] = value;
				}
				else if (param_info.type == fluxeem::ToolParameterType::FLOAT)
				{
					float value = 0.f;
					tool->getParam(param_name, value);
					config_params[tool_info.tool_name][param_name] = value;
				}
				else if (param_info.type == fluxeem::ToolParameterType::BOOL)
				{
					bool value = false;
					tool->getParam(param_name, value);
					config_params[tool_info.tool_name][param_name] = value;
				}
				else if (param_info.type == fluxeem::ToolParameterType::ENUM)
				{
					std::string value;
					tool->getParam(param_name, value);
					config_params[tool_info.tool_name][param_name] = value;
				}
			}
		}

		std::ofstream file(json_file_path);
		if (!file.is_open())
		{
			LOG_ERROR("Failed to open %s file for writing.", json_file_path.c_str());
			return false;
		}

		file << config_params.dump(4);
		file.close();

		LOG_INFO("Save param path %s", json_file_path.c_str());
		return true;
	}

	bool ICamera::importCameraConfig(const std::string& json_file_path)
	{
		std::ifstream file(json_file_path);
		if (!file.is_open())
		{
			LOG_ERROR("Failed to open config file: %s", json_file_path.c_str());
			return false;
		}

		nlohmann::json config;
		file >> config;
		file.close();

		for (auto &tool_item : config.items())
		{
			const std::string tool_name = tool_item.key();
			const nlohmann::json &tool_params = tool_item.value();

			const auto tool = getTool(tool_name);
			if (!tool)
			{
				LOG_WARN("Tool not found: %s, skipping...", tool_name.c_str());
				continue;
			}

			for (auto &param_item : tool_params.items())
			{
				const std::string param_name = param_item.key();
				const nlohmann::json &param_value = param_item.value();

				try
				{
					if (param_value.is_boolean())
					{
						tool->setParam(param_name, param_value.get<bool>());
					}
					else if (param_value.is_number_float())
					{
						tool->setParam(param_name, static_cast<float>(param_value.get<double>()));
					}
					else if (param_value.is_number_integer())
					{
						tool->setParam(param_name, param_value.get<int>());
					}
					else if (param_value.is_string())
					{
						tool->setParam(param_name, param_value.get<std::string>());
					}
					else
					{
						LOG_WARN("Unsupported parameter type for %s.%s, skipped.", tool_name.c_str(), param_name.c_str());
						continue;
					}

					LOG_INFO("Set %s.%s = %s", tool_name.c_str(), param_name.c_str(), param_value.dump().c_str());
				}
				catch (const std::exception &e)
				{
					LOG_ERROR("Failed to set %s.%s: %s", tool_name.c_str(), param_name.c_str(), e.what());
				}
			}
		}

		LOG_INFO("Load param path %s", json_file_path.c_str());
		return true;
	}

} // namespace fluxeem
