// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#include <fluxeem/hal/tools/tool_info.h>
#include <fluxeem/hal/tools/tool_info_private.h>
#include <fluxeem/hal/tools/camera_tool.h>
#include <fluxeem/hal/tools/camera_tool_private.h>
#include <fluxeem/hal/registers/register_controller.h>
#include <fluxeem/hal/tools/anti_flicker/imx636_anti_flicker.hpp>
#include <fluxeem/base/logging/logger.h>
#include <string>
#include <memory>
#include <cstdint>
#include <utility>
#include <cmath>

namespace
{
	constexpr uint32_t kPipelineBypass = 0b101;
	constexpr uint32_t kPipelineAfkEnabled = 0b001;
	constexpr uint32_t kMinInvertedDutyCycle = 15;

	bool valueInRange(int value, int min_value, int max_value)
	{
		return value >= min_value && value <= max_value;
	}

	uint32_t invertedDutyCycleFromPercent(float duty_cycle)
	{
		const auto value = static_cast<uint32_t>(std::round((16.0f * (100.0f - duty_cycle)) / 100.0f));
		return value >= 16 ? kMinInvertedDutyCycle : value;
	}

	float dutyCycleFromInverted(uint32_t inverted_duty_cycle)
	{
		return 100.0f - (static_cast<float>(inverted_duty_cycle) * 100.0f) / 16.0f;
	}

	uint32_t modeToInvertBit(const std::string &mode)
	{
		return mode == "Band cut" ? 0u : 1u;
	}

	void writeAfkField(const std::shared_ptr<fluxeem::RegisterController> &controller,
					   const std::string &prefix,
					   const std::string &register_suffix,
					   const std::string &field,
					   uint32_t value)
	{
		controller->writeRegisterField(prefix + register_suffix, field, value);
	}
}

namespace fluxeem
{

	Imx636AntiFlicker::Imx636AntiFlicker(std::shared_ptr<RegisterController> register_ctrl)
		: CameraToolRegisterWithCallback(register_ctrl, "afk/")
	{
		for (auto &info : imx636_infos_)
		{
			addRegisterToMap(info);
		}
	}

	bool Imx636AntiFlicker::addRegisterToMap(const FullParameterInfo &info)
	{
		parameter_info_map_.emplace(info.basic_info.name, info);
		return true;
	}

	bool Imx636AntiFlicker::setEnableStatus(bool b)
	{
		register_controller_->writeRegister(prefix_ + "pipeline_control", kPipelineBypass);
		if (!b)
		{
			return true;
		}

		uint32_t init_done = 0;
		register_controller_->readRegisterField(prefix_ + "initialization", "afk_flag_init_done", init_done);
		if (init_done == 1)
		{
			LOG_INFO("afk init success!");
		}
		else
		{
			LOG_ERROR("afk init failed!");
		}

		const uint32_t min_cutoff_period = freqToPeriod(high_freq_);
		const uint32_t max_cutoff_period = freqToPeriod(low_freq_);

		writeAfkField(register_controller_, prefix_, "invalidation", "dt_fifo_wait_time", 1630);

		register_controller_->writeRegisterField("afk/filter_period", "min_cutoff_period", min_cutoff_period);
		register_controller_->writeRegisterField("afk/filter_period", "max_cutoff_period", max_cutoff_period);
		register_controller_->writeRegisterField("afk/filter_period", "inverted_duty_cycle", inverted_duty_cycle_);

		writeAfkField(register_controller_, prefix_, "param", "invert", modeToInvertBit(mode_));
		writeAfkField(register_controller_, prefix_, "param", "counter_high", start_threshold_);
		writeAfkField(register_controller_, prefix_, "param", "counter_low", stop_threshold_);

		register_controller_->writeRegister(prefix_ + "pipeline_control", kPipelineAfkEnabled);
		return true;
	}

	bool Imx636AntiFlicker::readEnabled(bool& en) const
	{
		uint32_t read_value = 0;
		register_controller_->readRegister(prefix_ + "pipeline_control", read_value);
		en = read_value == kPipelineAfkEnabled;
		return true;
	}

	bool Imx636AntiFlicker::reset()
	{
		// 滤波启用时尝试复位，否则直接返回 true。
		bool is_enable = false;
		readEnabled(is_enable);
		if (is_enable)
		{
			if (!setEnableStatus(false))
			{
				return false;
			}

			if (!setEnableStatus(true))
			{
				return false;
			}
		}
		return true;
	}

	bool Imx636AntiFlicker::getLowFrequency(int& low_freq)
	{
		low_freq = low_freq_;
		return true;
	}

	bool Imx636AntiFlicker::setLowFrequency(int low_freq)
	{
		if (!valueInRange(low_freq, ANTI_FLICKER_MIN_FREQ, static_cast<int>(high_freq_)))
		{
			return false;
		}
		low_freq_ = low_freq;
		return reset();
	}

	bool Imx636AntiFlicker::getHighFrequency(int& high_freq)
	{
		high_freq = high_freq_;
		return true;
	}

	bool Imx636AntiFlicker::setHighFrequency(int high_freq)
	{
		if (!valueInRange(high_freq, static_cast<int>(low_freq_), ANTI_FLICKER_MAX_FREQ))
		{
			LOG_ERROR("Set high freq value error.");
			return false;
		}
		high_freq_ = high_freq;
		return reset();
	}

	bool Imx636AntiFlicker::setFilteringMode(std::string mode)
	{
		mode_ = mode;
		return reset();
	}

	bool Imx636AntiFlicker::getFilteringMode(std::string& mode)
	{
		mode = mode_;
		return true;
	}

	bool Imx636AntiFlicker::getDutyCycle(float& duty_cycle) const
	{
		duty_cycle = dutyCycleFromInverted(inverted_duty_cycle_);
		return true;
	}

	bool Imx636AntiFlicker::setDutyCycle(float duty_cycle)
	{
		if (duty_cycle <= 0.0f || duty_cycle > 100.0f)
		{
			LOG_ERROR("The duty cycle value is incorrect");
			return false;
		}
		inverted_duty_cycle_ = invertedDutyCycleFromPercent(duty_cycle);
		return reset();
	}

	bool Imx636AntiFlicker::getStartThreshold(int& start_th)
	{
		start_th = start_threshold_;
		return true;
	}

	bool Imx636AntiFlicker::setStartThreshold(int threshold)
	{
		if (!valueInRange(threshold, ANTI_FLICKER_MIN_START_TH, ANTI_FLICKER_MAX_START_TH))
		{
			LOG_ERROR("The start threshold  is incorrect.");
			return false;
		}

		start_threshold_ = threshold;
		return reset();
	}

	bool Imx636AntiFlicker::setStopThreshold(int threshold)
	{
		if (!valueInRange(threshold, ANTI_FLICKER_MIN_STOP_TH, ANTI_FLICKER_MAX_STOP_TH))
		{
			return false;
		}

		stop_threshold_ = threshold;
		return reset();
	}

	bool Imx636AntiFlicker::getStopThreshold(int& stop_th)
	{
		stop_th = stop_threshold_;
		return true;
	}

	uint32_t Imx636AntiFlicker::freqToPeriod(const uint32_t& freq) {
		constexpr uint32_t kMicrosecondsPerSecond = 1000000;
		return (kMicrosecondsPerSecond / freq) >> 7;
	}
}