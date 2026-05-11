// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef REGISTERS_INFO_H
#define REGISTERS_INFO_H

#include <stdint.h>
#include <vector>
#include <string>

enum TypeRegisterInfo
{
    R,
    F,
    A
}; // Register, Field, Alias

struct RegisterInfo
{
    TypeRegisterInfo type;
    struct FieldData
    {
        const char *name;
        uint32_t start;
        uint32_t len;
        uint32_t default_value;
    };
    struct RegisterData
    {
        const char *name;
        uint32_t addr;
    };
    struct AliasData
    {
        const char *name;
        uint32_t value;
    };
    union
    {
        FieldData field_data;
        RegisterData register_data;
        AliasData alias_data;
    };
    // ~RegisterInfo() {}
};

#endif // REGISTERS_INFO_H