#include <fluxeem/hal/tools/tool_info.h>
#include <fluxeem/hal/tools/tool_info_private.h>
#include <fluxeem/hal/tools/camera_tool.h>
#include <fluxeem/hal/tools/camera_tool_private.h>
#include <fluxeem/hal/registers/register_controller.h>
#include <fluxeem/hal/tools/trigger/imx636_trigger_in.hpp>
#include <fluxeem/base/logging/logger.h>
#include <string>
#include <memory>
#include <cstdint>

namespace fluxeem
{

	Imx636TriggerIn::Imx636TriggerIn(std::shared_ptr<RegisterController> register_ctrl)
		: CameraToolRegisterWithCallback(register_ctrl, "edf/trigger_in")
	{
		for (auto& info : imx636_infos_)
		{
			addRegister2Map(info);
		}
	}

	bool Imx636TriggerIn::addRegister2Map(const FullParameterInfo& info)
	{
		parameter_info_map_.emplace(info.basic_info.name, info);
		bool en = false;
		getParam(info.basic_info.name, en);
		std::get<BoolParameterInfo>(parameter_info_map_[info.basic_info.name].info).default_value = en;

		return true;
	}

	bool Imx636TriggerIn::setEnable(bool en)
	{
		LOG_INFO("Trigger in set enable: %d", en);
		register_controller_->writeRegisterField(prefix_, "enable", en);

		return true;
	}

	bool Imx636TriggerIn::getEnable(bool& en)
	{
		uint32_t get_en = 0;
		register_controller_->readRegisterField(prefix_, "enable", get_en);
		if (get_en)
		{
			en = true;
		}
		else
		{
			en = false;
		}
		LOG_DEBUG("Trigger in get enable: %d", en);
		return true;
	}
}