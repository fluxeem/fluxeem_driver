// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#include <fluxeem/base/utility/func_utils.h>
#include <fluxeem/base/logging/logger.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <devguid.h>
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")
#endif

namespace
{
    std::string formatInteger(int value, int width, std::ios_base &(*base)(std::ios_base &), bool uppercase = false)
    {
        std::ostringstream stream;
        if (uppercase)
        {
            stream << std::uppercase;
        }
        stream << base << std::setw(width) << std::setfill('0') << value;
        return stream.str();
    }

    std::array<uint8_t, 4> splitBigEndian(uint32_t value)
    {
        return {
            static_cast<uint8_t>((value >> 24) & 0xFF),
            static_cast<uint8_t>((value >> 16) & 0xFF),
            static_cast<uint8_t>((value >> 8) & 0xFF),
            static_cast<uint8_t>(value & 0xFF),
        };
    }

    bool readStrictOctet(const std::string &token, int &value)
    {
        if (token.empty() || (token.size() > 1 && token.front() == '0'))
        {
            return false;
        }
        if (!std::all_of(token.begin(), token.end(), [](unsigned char c) { return std::isdigit(c) != 0; }))
        {
            return false;
        }
        value = std::stoi(token);
        return value >= 0 && value <= 255;
    }

    std::tm makeLocalTm(std::time_t time)
    {
        std::tm local_tm{};
#ifdef _WIN32
        localtime_s(&local_tm, &time);
#else
        localtime_r(&time, &local_tm);
#endif
        return local_tm;
    }

    std::string formatLocalTimestamp(const std::chrono::system_clock::time_point &time_point)
    {
        using namespace std::chrono;

        const auto us = duration_cast<microseconds>(time_point.time_since_epoch()) % 1'000'000;
        const std::time_t time = system_clock::to_time_t(time_point);
        const std::tm local_tm = makeLocalTm(time);

        std::ostringstream stream;
        stream << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S")
               << '.' << std::setfill('0') << std::setw(6) << us.count();
        return stream.str();
    }

    long parseMicroseconds(std::istream &stream)
    {
        if (stream.peek() != '.')
        {
            return 0;
        }

        stream.get();
        std::string fraction;
        std::getline(stream, fraction);
        if (fraction.empty())
        {
            return 0;
        }

        if (fraction.size() > 6)
        {
            fraction.resize(6);
        }
        while (fraction.size() < 6)
        {
            fraction.push_back('0');
        }

        try
        {
            return std::stol(fraction);
        }
        catch (...)
        {
            throw std::runtime_error("Invalid microsecond part");
        }
    }
}

namespace fluxeem
{
    void convertEndian(std::vector<uint8_t> &buffer)
    {
        for (size_t offset = 0; offset + 1 < buffer.size(); offset += 2)
        {
            std::swap(buffer[offset], buffer[offset + 1]);
        }
    }

    std::string hexToString(int number, int n_digits)
    {
        return formatInteger(number, n_digits, std::hex, true);
    }

    std::string intToString(int number, int n_digits)
    {
        return formatInteger(number, n_digits, std::dec);
    }

    std::string ipToString(const uint8_t addr[4])
    {
        std::ostringstream oss;
        oss << static_cast<int>(addr[0]) << '.'
            << static_cast<int>(addr[1]) << '.'
            << static_cast<int>(addr[2]) << '.'
            << static_cast<int>(addr[3]);
        return oss.str();
    }

    std::string ipToString(const uint32_t addr)
    {
        const auto bytes = splitBigEndian(addr);
        return ipToString(bytes.data());
    }

    uint32_t bufToU32BE(const uint8_t *buf)
    {
        return (static_cast<uint32_t>(buf[0]) << 24) | (static_cast<uint32_t>(buf[1]) << 16) |
               (static_cast<uint32_t>(buf[2]) << 8) | static_cast<uint32_t>(buf[3]);
    }

    int stringToIP(const std::string &ip)
    {
        std::array<uint8_t, 4> bytes{};
        std::istringstream iss(ip);
        std::string token;
        size_t offset = 0;
        while (std::getline(iss, token, '.') && offset < bytes.size())
        {
            bytes[offset++] = static_cast<uint8_t>(std::stoi(token));
        }
        return static_cast<int>(bufToU32BE(bytes.data()));
    }

    bool isValidIP(const std::string &ip)
    {
        std::istringstream iss(ip);
        std::string octet;
        int count = 0;
        while (std::getline(iss, octet, '.'))
        {
            int value = 0;
            if (!readStrictOctet(octet, value))
            {
                return false;
            }
            ++count;
        }
        return count == 4;
    }

    std::string getSysTime()
    {
        return formatLocalTimestamp(std::chrono::system_clock::now());
    }

    std::chrono::system_clock::time_point parseLocalTimeString(const std::string &time_str)
    {
        if (time_str.empty())
        {
            throw std::invalid_argument("Time string is empty");
        }

        std::istringstream ss(time_str);
        std::tm tm = {};
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

        if (ss.fail())
        {
            throw std::runtime_error("Failed to parse time string: invalid format");
        }

        const long microsec_value = parseMicroseconds(ss);

        std::time_t tt = std::mktime(&tm);
        if (tt == -1)
        {
            throw std::runtime_error("mktime failed");
        }

        using namespace std::chrono;
        auto tp = system_clock::from_time_t(tt) + microseconds{microsec_value};

        return tp;
    }

    std::string timePointToString(const std::chrono::system_clock::time_point &tp)
    {
        return formatLocalTimestamp(tp);
    }

    std::string generateEvFileHeader(EvFileInfo &file_info)
    {
        std::stringstream dst;
        dst << "% camera_integrator_name ev_camera" << std::endl;
        dst << "% date " << getSysTime() << std::endl;
        dst << "% evt 3.0" << std::endl;
        dst << "% format EVT3; height = 720; width = 1280" << std::endl;
        dst << "% generation 4.2" << std::endl;
        dst << "% geometry 1280x720" << std::endl;
        dst << "% integrator_name ev_camera" << std::endl;
        dst << "% plugin_integrator_name ev_camera" << std::endl;
        dst << "% plugin_name hal_plugin_fluxeem" << std::endl;
        dst << "% sensor_generation 4.2" << std::endl;
        dst << "% sensor_name IMX646" << std::endl;
        dst << "% serial_number " << file_info.serial_number << std::endl;
        dst << "% system_ID 53" << std::endl;
        dst << "% start_timestamp " << file_info.start_timestamp << std::endl;
        dst << "% end" << std::endl;
        return dst.str();
    }

#ifdef _WIN32
    std::string utf8ToLocal8Bit(const std::string &utf8)
    {
        int len_w = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
        std::wstring wpath(len_w, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wpath[0], len_w);

        int len_a = WideCharToMultiByte(CP_ACP, 0, wpath.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string acp_path(len_a, 0);
        WideCharToMultiByte(CP_ACP, 0, wpath.c_str(), -1, &acp_path[0], len_a, nullptr, nullptr);
        return acp_path;
    }

    std::string local8bitToUtf8(const std::string &localStr)
    {
        if (localStr.empty())
            return std::string();

        int wlen = MultiByteToWideChar(CP_ACP, 0, localStr.c_str(), -1, nullptr, 0);
        if (wlen == 0)
            throw std::runtime_error("MultiByteToWideChar failed.");

        std::wstring wstr(wlen, L'\0');
        if (MultiByteToWideChar(CP_ACP, 0, localStr.c_str(), -1, &wstr[0], wlen) == 0)
            throw std::runtime_error("MultiByteToWideChar failed.");

        int u8len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (u8len == 0)
            throw std::runtime_error("WideCharToMultiByte failed.");

        std::string utf8Str(u8len, '\0');
        if (WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8Str[0], u8len, nullptr, nullptr) == 0)
            throw std::runtime_error("WideCharToMultiByte failed.");

        if (!utf8Str.empty() && utf8Str.back() == '\0')
            utf8Str.pop_back();

        return utf8Str;
    }
#endif

    EventIterator_t binarySearchTimestamp(Timestamp target_ts, EventIterator_t begin, EventIterator_t end)
    {
        return std::lower_bound(begin, end, target_ts,
                                [](const Event2D &event, Timestamp ts)
                                {
                                    return event.timestamp < ts;
                                });
    }

    std::string interfaceTypetoString(InterfaceType interface_type)
    {
        switch (interface_type)
        {
        case InterfaceType::USB:
            return "USB";
        default:
            return "UNDEFINED";
        }
    }
}