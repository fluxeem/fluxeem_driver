// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#include <fluxeem/hal/registers/register_controller.h>

namespace fluxeem
{

RegisterController::RegisterController(
    std::shared_ptr<Interface> interface,
    RegisterInfo *registersInfo, 
    uint32_t numInfo,
    std::string prefix
) : interface_(interface), 
    registerMap_(registersInfo, numInfo, prefix) {}

bool RegisterController::writeRegisterWithControlTransfer(const std::string &name, uint32_t val)
{
    if(!interface_->isConnected())
    {
        return false;
    }
    return interface_->writeRegisterWithControlTransfer(registerMap_[name].getAddress(), val);
}
bool RegisterController::readRegisterWithControlTransfer(const std::string &name, uint32_t &read_val)
{
    if(!interface_->isConnected())
    {
        return false;
    }
    return interface_->readRegisterWithControlTransfer(registerMap_[name].getAddress(), read_val);
}

bool RegisterController::readRegisterWithControlTransfer(const std::string &name, uint8_t* data, uint16_t read_length)
{
    if(!interface_->isConnected())
    {
        return false;
    }
    return interface_->readRegisterWithControlTransfer(registerMap_[name].getAddress(), data, read_length);
}

bool RegisterController::writeRegisterDataWithControlTransfer(const std::string &name, uint8_t* data, uint16_t write_length)
{
    if(!interface_->isConnected())
    {
        return false;
    }
    return interface_->writeRegisterWithControlTransfer(registerMap_[name].getAddress(), data, write_length); 
}

bool RegisterController::readRegister(const std::string &name, uint32_t &val) {
    return interface_->readRegister(registerMap_[name].getAddress(), val);
}

bool RegisterController::writeRegister(const std::string &name, const uint32_t &value) {
    return interface_->writeRegister(registerMap_[name].getAddress(), value);
}

void RegisterController::writeRegisterField(
    const std::string &registerName,
    const std::string &fieldName,
    uint32_t value
){
    uint32_t curValue = 0;
    readRegister(registerName, curValue);
    uint32_t valueToWrite = registerMap_[registerName][fieldName].setBitfieldInValue(
        value, curValue
    );
    writeRegister(registerName, valueToWrite);
}

void RegisterController::writeRegisterFieldByAlias(
    const std::string &registerName,
    const std::string &fieldName,
    const std::string &alias
){
    uint32_t value = registerMap_[registerName][fieldName].getAliasValue(alias);
    uint32_t curValue = 0;
    readRegister(registerName, curValue);
    uint32_t valueToWrite = registerMap_[registerName][fieldName].setBitfieldInValue(
        value, curValue
    );
    writeRegister(registerName, valueToWrite);
}

void RegisterController::readRegisterField(
    const std::string& registerName,
    const std::string& fieldName,
    uint32_t &value
) {
    uint32_t cur_reg_value = 0;
    readRegister(registerName, cur_reg_value);
    value = registerMap_[registerName][fieldName].getBitfieldInValue(cur_reg_value);
}


void RegisterController::applyRegisterOperation(const RegisterOperation operation){
    if(!interface_->isConnected())
    {
        return;
    }
    switch (operation.action) {
    case RegisterAction::READ: {
        uint32_t value = 0;
        interface_->readRegister(operation.address, value);
    } break;
    case RegisterAction::WRITE: {
        interface_->writeRegister(operation.address, operation.data);
        // (*register_map).write(operation.address, operation.data);
    } break;
    case RegisterAction::WRITE_FIELD: {
        uint32_t read_val = 0;
        interface_->readRegister(operation.address, read_val);
        uint32_t saved_fields = read_val & (~operation.mask);
        uint32_t write_field  = operation.data & operation.mask;
        uint32_t write_reg    = saved_fields | write_field;
        interface_->writeRegister(operation.address, write_reg);
    } break;
    case RegisterAction::DELAY: {
        std::this_thread::sleep_for(std::chrono::milliseconds(operation.usec / 1000));
    } break;
    default:
        // FIXME raise error.
        break;
    };
}
    
void RegisterController::applyRegisterOperationSequence(const std::vector<RegisterOperation>& operationSequence) {
    for (const auto& operation : operationSequence) {
        applyRegisterOperation(operation);
    }
}

RegisterMap::RegisterMap(
    RegisterInfo *registersInfo, 
    uint32_t numInfo,
    std::string prefix
) { 
    RegisterInfo *curInfo = registersInfo;
    RegisterInfo *infoEnd = curInfo + numInfo;

    Register *curRegister = nullptr;
    Register::Field *curField = nullptr;
    std::string curFieldName = "";
    for (; curInfo != infoEnd; ++curInfo) {
        switch (curInfo->type){
        case R:
            if (curRegister != nullptr) {
                if (curField != nullptr) {
                    curRegister->addField(std::move(*curField));
                }
                registers_[curRegister->getName()] = std::move(*curRegister);
            }
            curRegister = new Register(
                curInfo->register_data.name, curInfo->register_data.addr
            );
            break;
        case F:
            if (curRegister == nullptr) {
                throw std::runtime_error("RegisterMap: Field without register");
            }
            if (curField != nullptr) {
                curRegister->addField(std::move(*curField));
            }
            curField = new Register::Field(
                curInfo->field_data.name, 
                curInfo->field_data.start, 
                curInfo->field_data.len, 
                curInfo->field_data.default_value
            );
            curFieldName = curInfo->field_data.name;
            break;
        case A:
            if (curRegister == nullptr || curField == nullptr) {
                throw std::runtime_error("RegisterMap: Alias without register");
            }
            curField->addAlias(curInfo->alias_data.name, curInfo->alias_data.value);
            break;
        
        default:
            break;
        }
    }
    if (curField != nullptr) {
        curRegister->addField(std::move(*curField));
    }
    if (curRegister != nullptr) {
        registers_[curRegister->getName()] = std::move(*curRegister);
    }
}

} // namespace fluxeem

