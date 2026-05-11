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
#include <math.h>

namespace fluxeem
{

	Imx636AntiFlicker::Imx636AntiFlicker(std::shared_ptr<RegisterController> register_ctrl)
	: CameraToolRegisterWithCallback(register_ctrl, "afk/")
{
	for (auto& info : imx636_infos_)
	{
		addRegisterToMap(info);
	}
}

	bool Imx636AntiFlicker::addRegisterToMap(const FullParameterInfo& info)
	{
	parameter_info_map_.emplace(info.basic_info.name, info);

	return true;
}

	bool Imx636AntiFlicker::setEnableStatus(bool b) {
		// Always disable in order to configure AFK parameters
		register_controller_->writeRegister(prefix_ + "pipeline_control", 0b101);
		if (!b) {
			return true;
		}

		uint32_t read_flag_init_done = 0;
		// May need retries.
		register_controller_->readRegisterField(prefix_ + "initialization", "afk_flag_init_done", read_flag_init_done);
		if (read_flag_init_done == 1)
		{
			LOG_INFO("afk init success!");
		}
		else
		{
			LOG_ERROR("afk init failed!");
		}

		// Set frequency bandwidth
		uint32_t min_cutoff_period = freqToPeriod(high_freq_);
		uint32_t max_cutoff_period = freqToPeriod(low_freq_);

		std::pair<uint32_t, uint32_t> invalidation_cfg;

		register_controller_->writeRegisterField(prefix_ + "invalidation", "dt_fifo_wait_time", 1630);


		// Set duty cycle
		register_controller_->writeRegisterField("afk/filter_period", "min_cutoff_period", min_cutoff_period);
		register_controller_->writeRegisterField("afk/filter_period", "max_cutoff_period", max_cutoff_period);
		register_controller_->writeRegisterField("afk/filter_period", "inverted_duty_cycle", inverted_duty_cycle_);

		// Set filtering mode
		register_controller_->writeRegisterField(prefix_ + "param", "invert", mode_ == "Band cut" ? 0 : 1);

		// Set hysteresis counters
		register_controller_->writeRegisterField(prefix_ + "param", "counter_high", start_threshold_);
		register_controller_->writeRegisterField(prefix_ + "param", "counter_low", stop_threshold_);

		register_controller_->writeRegister(prefix_ + "pipeline_control", 0b001);

		return true;
	}

	bool Imx636AntiFlicker::readEnabled(bool& en) const
	{
		uint32_t read_value = 0;
		register_controller_->readRegister(prefix_ + "pipeline_control", read_value);
		if (read_value == 0b001)
		{
			en = true;
		}
		else
		{
			en = false;
		}
		return true;
	}

	bool Imx636AntiFlicker::reset()
	{
		// If the filter is enabled, enter reset to try, otherwise return true.
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
		if (low_freq < ANTI_FLICKER_MIN_FREQ || low_freq > high_freq_)
		{
			return false;
		}
		low_freq_ = low_freq;
		if (!reset())
		{
			return false;
		}
		return true;
	}

	bool Imx636AntiFlicker::getHighFrequency(int& high_freq)
	{
		high_freq = high_freq_;
		return true;
	}

	bool Imx636AntiFlicker::setHighFrequency(int high_freq)
	{
		if (high_freq > ANTI_FLICKER_MAX_FREQ || high_freq < low_freq_)
		{
			LOG_ERROR("Set high freq value error.");
			return false;
		}
		high_freq_ = high_freq;
		if (!reset())
		{
			return false;
		}
		return true;
	}

	bool Imx636AntiFlicker::setFilteringMode(std::string mode)
	{
		mode_ = mode;

		// Reset if needed
		if (!reset())
		{
			return false;
		}

		return true;
	}

	bool Imx636AntiFlicker::getFilteringMode(std::string& mode)
	{
		mode = mode_;
		return true;
	}

	bool Imx636AntiFlicker::getDutyCycle(float& duty_cycle) const
	{
		duty_cycle = 100.0 - (inverted_duty_cycle_ * 100.0) / 16.0;
		return true;
	}

	bool Imx636AntiFlicker::setDutyCycle(float duty_cycle)
	{
		if (duty_cycle <= 0.0 || duty_cycle > 100)
		{
			LOG_ERROR("The duty cycle value is incorrect");
			return false;
		}
		inverted_duty_cycle_ = std::roundf((16 * (100.0 - duty_cycle)) / 100.0);
		// Lowest duty cycle is 1/16th
		if (inverted_duty_cycle_ >= 16)
		{
			inverted_duty_cycle_ = 15;
		}

		// Reset if needed
		if (!reset())
		{
			return false;
		}

		return true;
	}

	bool Imx636AntiFlicker::getStartThreshold(int& start_th)
	{
		start_th = start_threshold_;
		return true;
	}

	bool Imx636AntiFlicker::setStartThreshold(int threshold)
	{
		if (threshold < ANTI_FLICKER_MIN_START_TH || threshold > ANTI_FLICKER_MAX_START_TH)
		{
			LOG_ERROR("The start threshold  is incorrect.");
			return false;
		}

		start_threshold_ = threshold;

		// Reset if needed
		if (!reset())
		{
			return false;
		}

		return true;
	}

	bool Imx636AntiFlicker::setStopThreshold(int threshold)
	{
		if (threshold < ANTI_FLICKER_MIN_STOP_TH || threshold > ANTI_FLICKER_MAX_STOP_TH)
		{
			return false;
		}

		stop_threshold_ = threshold;

		// Reset if needed
		if (!reset())
		{
			return false;
		}

		return true;
	}

	bool Imx636AntiFlicker::getStopThreshold(int& stop_th)
	{
		stop_th = stop_threshold_;
		return true;
	}

	uint32_t Imx636AntiFlicker::freqToPeriod(const uint32_t& freq) {
		int period = 1e6 / freq;
		return period >> 7;
	}
}