// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef REGISTER_HPP_
#define REGISTER_HPP_

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <stdexcept>
#include <fluxeem/base/define/base_define.h>

namespace fluxeem
{

    class FLUXEEM_API Register
    {
    public:
        Register(/* args */) = default;
        Register(std::string name, uint32_t address) : name_(name), address_(address) {};

        ~Register();

        uint32_t getAddress() const { return address_; }
        uint32_t getValue() const { return 0; }

        const std::string &getName() const { return name_; }

        class FLUXEEM_API Field
        {
        private:
            std::string name_;
            uint8_t start_;
            uint8_t len_;
            uint32_t mask_ = 0;
            uint32_t default_value_ = 0;
            std::map<std::string, uint32_t> aliases_;

            void initMask();

        public:
            Field() = default;
            Field(const Field &) = default;
            Field(std::string name,
                  uint8_t start, uint8_t len, uint32_t default_value = 0,
                  const std::map<std::string, uint32_t> &aliases = {}) : name_(name), start_(start), len_(len), aliases_(aliases), default_value_(default_value)
            {
                initMask();
            }

            void addAlias(const std::string &name, uint32_t value)
            {
                aliases_[name] = value;
            }
            const uint32_t operator[](const std::string &name) const;
            const uint8_t getStart() const { return start_; };
            const uint8_t getLen() const { return len_; };
            const uint32_t getDefaultValue() const { return default_value_; };
            void setStart(uint8_t start)
            {
                start_ = start;
                initMask();
            }
            void setLen(uint8_t len)
            {
                len_ = len;
                initMask();
            }
            const std::string &getName() const { return name_; }

            uint32_t setBitfieldInValue(uint32_t v, uint32_t register_value) const;
            uint32_t getBitfieldInValue(uint32_t register_value) const;
            uint32_t getAliasValue(const std::string &alias) const;
        };

        void addField(const Field &&field);

        const Field &operator[](const std::string &name) const
        {
            auto it = fields_.find(name);
            if (it == fields_.end())
            {
                throw std::runtime_error("Register: Field not found");
            }
            return it->second;
        }

    private:
        std::string name_;
        uint32_t address_;

        std::map<std::string, Field> fields_;
    };

} // namespace fluxeem

#endif /* REGISTER_HPP_ */
