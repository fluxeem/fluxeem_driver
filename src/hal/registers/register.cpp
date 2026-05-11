// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#include <fluxeem/hal/registers/register_operation/register_operation.h>
#include <fluxeem/hal/registers/register.h>

namespace fluxeem
{

    // ====== Register ======

    void Register::addField(const Field &&field)
    {
        fields_[field.getName()] = std::move(field);
    }

    Register::~Register() {}

    // ====== Register::Field ======

    void Register::Field::initMask()
    {
        uint32_t m = 0;
        for (auto i = 0; i < len_; ++i)
        {
            m = (m << 1) | 1;
        }
        m <<= start_;
        mask_ = m;
    }

    /// @brief 将位字段写入寄存器值
    /// @param v 待写入的字段值
    /// @param register_value 待修改的寄存器值
    uint32_t Register::Field::setBitfieldInValue(uint32_t v, uint32_t register_value) const
    {
        return (register_value & ~mask_) | ((v << start_) & mask_);
    }

    uint32_t Register::Field::getBitfieldInValue(uint32_t register_value) const
    {
        return (register_value & mask_) >> start_;
    }

    const uint32_t Register::Field::operator[](const std::string &name) const
    {
        auto it = aliases_.find(name);
        if (it == aliases_.end())
        {
            throw std::runtime_error("Register::Field: Alias not found");
        }
        return it->second;
    }

    uint32_t Register::Field::getAliasValue(const std::string &alias) const
    {
        auto it = aliases_.find(alias);
        if (it == aliases_.end())
            return -1;
        return it->second;
    }

    // uint32_t Register::Field::get_bitfield_in_value(uint32_t register_value) const {
    //     return (register_value & mask_) >> start_;
    // }

} // namespace fluxeem