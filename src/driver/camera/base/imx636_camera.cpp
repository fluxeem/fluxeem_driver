// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#include <fluxeem/driver/camera/base/imx636_camera.hpp>
#include <fluxeem/hal/common/usb_interface.h>
#include <fluxeem/hal/registers/register_controller.h>
#include <fluxeem/hal/registers/init_registers_info/imx636_registers_info.h>
#include <fluxeem/hal/data_stream/decode_factory.hpp>
#include <fluxeem/driver/sensors/imx6x6_sensor.h>
#include <fluxeem/hal/tools/biases/imx636_biases.hpp>
#include <fluxeem/hal/tools/trigger/imx636_trigger_in.hpp>
#include <fluxeem/hal/tools/anti_flicker/imx636_anti_flicker.hpp>
#include <fluxeem/hal/tools/event_rate_control/imx636_event_rate_control.hpp>
#include <fluxeem/hal/tools/event_trail_filter/imx636_event_trail_filter.hpp>
#include <fluxeem/hal/tools/roi/imx636_roi.h>
#include <fluxeem/hal/tools/trigger/imx636_sync.h>
#include <fluxeem/base/logging/logger.h>
#include <functional>

namespace fluxeem
{
    bool Imx636Camera::isConnected() const
    {
        if (interface_)
        {
            return interface_->isConnected();
        }
        else
        {
            return false;
        }
    }

    bool Imx636Camera::init()
    {
        if (is_initialized_)
        {
            return true;
        }
        // init UsbInterface
        if (camera_desc_.interface_type == InterfaceType::USB)
        {
            interface_ = UsbInterface::createUsbInterface(camera_desc_);
            if (interface_ == nullptr)
            {
                return false;
            }

            if (!interface_->open())
            {
                LOG_ERROR("USB interface open failed.");
                return false;
            }
            if (!interface_->isConnected())
            {
                LOG_ERROR("USB interface is not connected after open.");
                return false;
            }
        }

        // init Registers
        register_controller_ = std::make_shared<RegisterController>(
            interface_,
            Imx636RegisterInfos,
            Imx636RegisterInfoNum,
            IMX636_REGISTER_PREFIX);

        // Create sensor (hardware configuration only)
        sensor_ = std::make_unique<Imx6x6Sensor>();
        sensor_->setRegisterAccess(register_controller_);
        sensor_->init(interface_);

        // Create pipeline
        pipeline_ = std::make_unique<DataPipeline>("event_camera_pipeline");

        // Create and set data transfer
        auto data_transfer = DataTransferFactory::createUsbDataTransfer(interface_);
        pipeline_->setSource(std::move(data_transfer));

        // Create and set decoder
        RawEventStreamFormat raw_event_stream_format = getRawEventStreamFormat();
        auto decoder = DecoderFactory::createUniqueDecoder(raw_event_stream_format);
        pipeline_->setDecoder(std::move(decoder));

        // Create tools
        tools_.emplace(ToolType::TOOL_BIAS, std::make_shared<Imx636Biases>(register_controller_));
        tools_.emplace(ToolType::TOOL_TRIGGER_IN, std::make_shared<Imx636TriggerIn>(register_controller_));
        tools_.emplace(ToolType::TOOL_ANTI_FLICKER, std::make_shared<Imx636AntiFlicker>(register_controller_));
        tools_.emplace(ToolType::TOOL_EVENT_RATE_CONTROL, std::make_shared<Imx636EventRateControl>(register_controller_));
        tools_.emplace(ToolType::TOOL_EVENT_TRAIL_FILTER, std::make_shared<Imx636EventTrailFilter>(register_controller_));
        tools_.emplace(ToolType::TOOL_ROI, std::make_shared<Imx636ROI>(register_controller_));
        tools_.emplace(ToolType::TOOL_SYNC, std::make_shared<Imx636Sync>(register_controller_));

        is_initialized_ = true;
        return true;
    }

    uint16_t Imx636Camera::getWidth()
    {
        return 1280;
    }

    uint16_t Imx636Camera::getHeight()
    {
        return 720;
    }

    RawEventStreamFormat Imx636Camera::getRawEventStreamFormat()
    {
        return RawEventStreamFormat{"EVT3;height=720;width=1280"};
    }

    bool Imx636Camera::checkSensor()
    {
        //check the chip is imx636 or imx646
        uint32_t val1 = 0, val2 = 0;
        interface_->readRegister(0x14, val1);
        bool res = (val1 == 0xA0401806);
        interface_->readRegister(0xF128, val2);
        res = res && ((val2 & 3) == 0); //imx636: 0b00  imx646: 0b01
        return res;
    }
}