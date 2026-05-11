// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#include <string>
#include <vector>
#include <stdint.h>
#include <fluxeem/base/define/camera_types.h>
#include <fluxeem/hal/common/usb_device.h>

namespace fluxeem
{
    class FLUXEEM_API Interface
    {
    public:
        Interface(CameraDescription camera_description):interface_type_(camera_description.interface_type)
        {
            camera_description_ = camera_description;
        };

        Interface(std::shared_ptr<UsbDevice> usb_handle):interface_type_(InterfaceType::USB)
        {
        };

        ~Interface() {};

        // Lifecycle management
        virtual bool open() { return true; };
        virtual void close() noexcept {};
        virtual bool isOpen() const noexcept { return false; };

        virtual bool isConnected() const {return false;}

        virtual bool readRegister(uint32_t address, uint32_t &val) { return false; };

        virtual bool readRegister(uint32_t address, int nval, std::vector<uint32_t> &val) { return false; };

        virtual bool writeRegister(uint32_t address, uint32_t value) { return false; };

        virtual bool readRegisterWithControlTransfer(uint32_t address, uint32_t &val) { return false; };

        virtual bool writeRegisterWithControlTransfer(uint32_t address, uint32_t val) { return false; };

        virtual bool readRegisterWithControlTransfer(uint32_t address, uint8_t* data, uint16_t read_length) { return false; };

        virtual bool writeRegisterWithControlTransfer(uint32_t address, uint8_t* data, uint16_t write_length) { return false; };

        virtual bool readCalibrationData(std::vector<uint8_t> &buffer) { return false; };

        virtual bool writeCalibrationFile(std::string file_path) { return false; };

        const InterfaceType &getInterfaceType() const { return interface_type_; }

    protected:
        const InterfaceType interface_type_;

        CameraDescription camera_description_;
    };

} // namespace fluxeem
#endif