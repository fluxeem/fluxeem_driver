// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef __REGISTER_CONTROLLER_HH__
#define __REGISTER_CONTROLLER_HH__

#include <map>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include <fluxeem/hal/registers/register.h>
#include <fluxeem/hal/registers/init_registers_info/register_info.h>
#include <fluxeem/hal/registers/register_operation/register_operation.h>
#include <fluxeem/hal/common/interface.hpp>
#include <fluxeem/driver/sensors/sensor.hpp>  // For IRegisterAccess interface

namespace fluxeem
{
class FLUXEEM_API RegisterMap
{
private:
    std::map<std::string, Register> registers_;

public:
    RegisterMap(
        RegisterInfo *registersInfo, 
        uint32_t numInfo,
        std::string prefix = ""
    );

    const Register &operator[](const std::string &name) const {
        auto it = registers_.find(name);
        if (it == registers_.end()) {
            throw std::runtime_error("RegisterController: Register not found");
        }
        return it->second;
    }
    
};

class FLUXEEM_API RegisterController : public IRegisterAccess {
public:
    RegisterController(
        std::shared_ptr<Interface> interface,
        RegisterInfo *registersInfo, 
        uint32_t numInfo,
        std::string prefix = ""
    );

    // IRegisterAccess interface implementation
    void writeRegisterField(const std::string &registerName, const std::string &fieldName, uint32_t value) override;
    void applyRegisterOperationSequence(const std::vector<RegisterOperation>& operationSequence) override;

    // Additional methods (not part of IRegisterAccess)
    bool readRegister(const std::string &name, uint32_t &val);
    bool writeRegister(const std::string &name, const uint32_t &value);
    void writeRegisterFieldByAlias(const std::string &registerName, const std::string &fieldName, const std::string &alias);
    void readRegisterField(const std::string& registerName, const std::string& fieldName, uint32_t& value);
    void applyRegisterOperation(const RegisterOperation operation);

    bool readRegisterWithControlTransfer(const std::string &name, uint32_t& read_val);
    bool writeRegisterWithControlTransfer(const std::string &name, uint32_t val);

    bool readRegisterWithControlTransfer(const std::string &name, uint8_t* data, uint16_t read_length);
    bool writeRegisterDataWithControlTransfer(const std::string &name, uint8_t* data, uint16_t write_length);

    std::shared_ptr<Interface> getInterface() const {
        return interface_;
    }
        
private:
    std::shared_ptr<Interface> interface_;
    RegisterMap registerMap_;
};

} // namespace fluxeem  

#endif // __REGISTER_CONTROLLER_HH__