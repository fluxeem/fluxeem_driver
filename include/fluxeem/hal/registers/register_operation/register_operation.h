// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef REGISTER_OPERATION_HH
#define REGISTER_OPERATION_HH

#include <cstdint>

namespace fluxeem
{

    enum class RegisterAction
    {
        N_A,
        READ,
        WRITE,
        WRITE_FIELD,
        DELAY
    };

    class RegisterOperation
    {
    public:
        RegisterAction action = RegisterAction::N_A;
        uint32_t address;
        uint32_t data;
        uint32_t mask;
        uint32_t usec;

        static RegisterOperation Write(uint32_t address, uint32_t data)
        {
            RegisterOperation op = RegisterOperation();
            op.action = RegisterAction::WRITE;
            op.address = address;
            op.data = data;

            return op;
        }

        static RegisterOperation WriteField(uint32_t address, uint32_t data, uint32_t mask)
        {
            RegisterOperation op = RegisterOperation();
            op.action = RegisterAction::WRITE_FIELD;
            op.address = address;
            op.data = data;
            op.mask = mask;

            return op;
        }

        static RegisterOperation Delay(uint32_t usec)
        {
            RegisterOperation op = RegisterOperation();

            op.action = RegisterAction::DELAY;
            op.usec = usec;

            return op;
        }

        static RegisterOperation Read(uint32_t address, uint32_t data, uint32_t mask)
        {
            RegisterOperation op = RegisterOperation();

            op.action = RegisterAction::READ;
            op.address = address;
            op.data = data;
            op.mask = mask;

            return op;
        }

        static RegisterOperation Read(uint32_t address, uint32_t data)
        {
            RegisterOperation op = RegisterOperation();

            op.action = RegisterAction::READ;
            op.address = address;
            op.data = data;
            op.mask = 0xFFFFFFFF;

            return op;
        }
    };

} // namespace fluxeem

#endif // REGISTER_OPERATION_HH