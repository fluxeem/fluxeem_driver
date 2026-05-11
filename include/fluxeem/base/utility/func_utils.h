// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef __FUNC_UTILS_HPP__
#define __FUNC_UTILS_HPP__

#include <vector>
#include <stdint.h>
#include <string>
#include <fluxeem/base/define/base_define.h>
#include <fluxeem/base/define/event_type.h>
namespace fluxeem
{
    inline void swap16(uint8_t *data)
    {
        // 直接交换两个字节
        uint8_t temp = data[0];
        data[0] = data[1];
        data[1] = temp;
    }

    void FLUXEEM_API convertEndian(std::vector<uint8_t> &buffer);

    std::string FLUXEEM_API hexToString(int number, int n_digits);

    std::string FLUXEEM_API intToString(int number, int n_digits);

    std::string FLUXEEM_API getSysTime();

    uint32_t FLUXEEM_API bufToU32BE(const uint8_t *buf);

    int FLUXEEM_API stringToIP(const std::string &ip);

    std::string FLUXEEM_API ipToString(const uint32_t addr);

    std::string FLUXEEM_API ipToString(const uint8_t addr[4]);

    
    bool FLUXEEM_API isValidIP(const std::string &ip);

    std::chrono::system_clock::time_point FLUXEEM_API parseLocalTimeString(const std::string &time_str);

    std::string FLUXEEM_API timePointToString(const std::chrono::system_clock::time_point &tp);

    std::string FLUXEEM_API generateEvFileHeader(EvFileInfo& file_info);

#ifdef _WIN32
    std::string FLUXEEM_API utf8ToLocal8Bit(const std::string &utf8);

    std::string FLUXEEM_API local8bitToUtf8(const std::string &localStr);
#endif

    bool FLUXEEM_API restartUSBDevice(uint16_t vid, uint16_t pid);

    EventIterator_t FLUXEEM_API binarySearchTimestamp(Timestamp target_ts, EventIterator_t begin, EventIterator_t end);
    
    std::string FLUXEEM_API interfaceTypetoString(InterfaceType interface_type);
}

#endif
