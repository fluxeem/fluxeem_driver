#include <fluxeem/driver/sensors/imx6x6_sensor.h>
#include <fluxeem/hal/registers/register_operation/imx636_register_operation_sequence.h>
#include <fluxeem/hal/registers/register_controller.h>
#include <fluxeem/base/logging/logger.h>

#include <thread>
#include <chrono>

namespace fluxeem
{
    namespace
    {
        // Sensor register name prefix (empty for IMX636)
        constexpr const char* SENSOR_PREFIX = "";
    }

    Imx6x6Sensor::Imx6x6Sensor() : Sensor()
    {
    }

    Imx6x6Sensor::~Imx6x6Sensor()
    {
    }

    int Imx6x6Sensor::init(std::shared_ptr<Interface> interface)
    {
        if (!register_access_)
        {
            LOG_ERROR("Imx6x6Sensor init failed! No register controller provided!");
            return -1;
        }
        applyRegisterOperationSequence(issd_evk3_imx636_stop);
        applyRegisterOperationSequence(issd_evk3_imx636_destroy);
        applyRegisterOperationSequence(issd_evk3_imx636_init);
        temperatureInit();
        controlIphMirror(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        controlLifo(true, true, true);
        timeBaseConfig(false, true);
        inited_ = true;
        return 0;
    }

    int Imx6x6Sensor::startStreaming()
    {
        if (!inited_)
        {
            LOG_ERROR("Please initialize the Imx6x6Sensor first!");
            return -1;
        }
        if (!register_access_)
        {
            LOG_ERROR("Imx6x6Sensor startStreaming failed! No register controller provided!");
            return -1;
        }
        applyRegisterOperationSequence(issd_evk3_imx636_start);
        return 0;
    }

    int Imx6x6Sensor::stopStreaming()
    {
        if (!register_access_)
        {
            LOG_ERROR("Imx6x6Sensor stopStreaming failed! No register controller provided!");
            return -1;
        }
        applyRegisterOperationSequence(issd_evk3_imx636_stop);
        return 0;
    }

    void Imx6x6Sensor::temperatureInit()
    {
        // Temperature ADC init
        writeRegisterField(std::string(SENSOR_PREFIX) + "adc_control", "adc_en", 1);
        writeRegisterField(std::string(SENSOR_PREFIX) + "adc_control", "adc_clk_en", 1);
        writeRegisterField(std::string(SENSOR_PREFIX) + "adc_misc_ctrl", "adc_buf_cal_en", 1);
        std::this_thread::sleep_for(std::chrono::microseconds(100));

        // Temperature sensor init
        writeRegisterField(std::string(SENSOR_PREFIX) + "temp_ctrl", "temp_buf_en", 1);
        writeRegisterField(std::string(SENSOR_PREFIX) + "temp_ctrl", "temp_buf_cal_en", 1);
        std::this_thread::sleep_for(std::chrono::microseconds(100));

        writeRegisterField(std::string(SENSOR_PREFIX) + "adc_control", "adc_clk_en", 0);
    }

    void Imx6x6Sensor::controlLifo(bool enable, bool out_en, bool cnt_en)
    {
        // TODO: reinplement the logic here
        if (enable && out_en)
        {
            writeRegisterField(std::string(SENSOR_PREFIX) + "lifo_ctrl", "lifo_en", enable);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            writeRegisterField(std::string(SENSOR_PREFIX) + "lifo_ctrl", "lifo_out_en", out_en);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        else if (enable && !out_en)
        {
            writeRegisterField(std::string(SENSOR_PREFIX) + "lifo_ctrl", "lifo_en", enable);
        }
        else if (!enable && out_en)
        {
            writeRegisterField(std::string(SENSOR_PREFIX) + "lifo_ctrl", "lifo_out_en", out_en);
        }
        else if (!enable && !out_en)
        {
            writeRegisterField(std::string(SENSOR_PREFIX) + "lifo_ctrl", "lifo_en", enable);
            writeRegisterField(std::string(SENSOR_PREFIX) + "lifo_ctrl", "lifo_out_en", out_en);
        }
        writeRegisterField(std::string(SENSOR_PREFIX) + "lifo_ctrl", "lifo_cnt_en", cnt_en);
    }

    void Imx6x6Sensor::controlIphMirror(bool enable)
    {
        writeRegisterField(std::string(SENSOR_PREFIX) + "iph_mirr_ctrl", "iph_mirr_en", enable);
        std::this_thread::sleep_for(std::chrono::microseconds(20));
        writeRegisterField(std::string(SENSOR_PREFIX) + "iph_mirr_ctrl", "iph_mirr_amp_en", enable);
        std::this_thread::sleep_for(std::chrono::microseconds(20));
    }

    /**
     * @brief Configure sensor time base settings. By default, the sensor is in monocular mode
     *
     * @param external if true external time base, otherwise, use internal
     * @param master if true, use master mode, else slave mode
     */
    void Imx6x6Sensor::timeBaseConfig(bool external, bool master)
    {
        writeRegisterField(std::string(SENSOR_PREFIX) + "ro/time_base_ctrl", "time_base_mode", external);
        writeRegisterField(std::string(SENSOR_PREFIX) + "ro/time_base_ctrl", "external_mode", master);
        writeRegisterField(std::string(SENSOR_PREFIX) + "ro/time_base_ctrl", "external_mode_enable", external);
        writeRegisterField(std::string(SENSOR_PREFIX) + "ro/time_base_ctrl", "Reserved_10_4", 100);
        if (external)
        {
            if (master)
            {
                // set SYNCHRO IO to output mode
                writeRegisterField(std::string(SENSOR_PREFIX) + "dig_pad2_ctrl", "pad_sync", 0b1100);
            }
            else
            {
                // set SYNCHRO IO to input mode
                writeRegisterField(std::string(SENSOR_PREFIX) + "dig_pad2_ctrl", "pad_sync", 0b1111);
            }
        }
    }

} // namespace fluxeem
