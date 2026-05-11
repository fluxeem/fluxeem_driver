#include <fluxeem/base/utility/func_utils.h>
#include <fluxeem/base/logging/logger.h>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <devguid.h>
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")
#endif

namespace fluxeem
{
    // 高效转换buffer中的所有16位数据的大小端
    void convertEndian(std::vector<uint8_t> &buffer)
    {
        size_t size = buffer.size();
        for (size_t i = 0; i < size; i += 2)
        {
            swap16(&buffer[i]); // 直接交换16位数据的字节顺序
        }
    }

    std::string hexToString(int number, int n_digits)
    {
        std::stringstream ss;
        ss << std::uppercase << std::hex << std::setw(n_digits) << std::setfill('0') << number;
        return ss.str();
    }

    std::string intToString(int number, int n_digits)
    {
        std::stringstream ss;
        ss << std::dec << std::setw(n_digits) << std::setfill('0') << number;
        return ss.str();
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
        uint8_t buf[4];
        buf[0] = (addr >> 24) & 0xFF;
        buf[1] = (addr >> 16) & 0xFF;
        buf[2] = (addr >> 8) & 0xFF;
        buf[3] = addr & 0xFF;
        return ipToString(buf);
    }

    uint32_t bufToU32BE(const uint8_t *buf)
    {
        return (static_cast<uint32_t>(buf[0]) << 24) |
               (static_cast<uint32_t>(buf[1]) << 16) |
               (static_cast<uint32_t>(buf[2]) << 8) |
               static_cast<uint32_t>(buf[3]);
    }

    int stringToIP(const std::string &ip)
    {
        uint8_t buf[4] = {0};
        std::istringstream iss(ip);
        std::string token;
        int i = 0;
        while (std::getline(iss, token, '.') && i < 4)
        {
            buf[i++] = static_cast<uint8_t>(std::stoi(token));
        }
        return bufToU32BE(buf);
    }

    bool isValidIP(const std::string &ip)
    {
        std::istringstream iss(ip);
        std::string octet;
        int count = 0;
        while (std::getline(iss, octet, '.'))
        {
            if (octet.empty() || (octet.size() > 1 && octet[0] == '0'))
                return false; // 不允许前导零
            for (char c : octet)
                if (!isdigit(c))
                    return false;
            int val = std::stoi(octet);
            if (val < 0 || val > 255)
                return false;
            ++count;
        }
        return count == 4;
    }

    std::string getSysTime()
    {
        using namespace std::chrono;

        auto now = system_clock::now();
        auto us = duration_cast<microseconds>(now.time_since_epoch()) % 1'000'000;

        std::time_t tt = system_clock::to_time_t(now);
        std::tm tm{};

#ifdef _WIN32
        localtime_s(&tm, &tt);
#else
        localtime_r(&tt, &tm);
#endif

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
            << '.' << std::setfill('0') << std::setw(6) << us.count();

        return oss.str();
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

        long microsec_value = 0;
        if (ss.peek() == '.')
        {
            ss.get(); // skip '.'
            std::string frac;
            std::getline(ss, frac);
            if (!frac.empty())
            {
                if (frac.size() > 6)
                    frac.resize(6);
                while (frac.size() < 6)
                    frac += '0';
                try
                {
                    microsec_value = std::stol(frac);
                }
                catch (...)
                {
                    throw std::runtime_error("Invalid microsecond part");
                }
            }
        }

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
        using namespace std::chrono;

        auto us = duration_cast<microseconds>(tp.time_since_epoch()) % 1'000'000;

        std::time_t tt = system_clock::to_time_t(tp);
        std::tm tm{};

#ifdef _WIN32
        localtime_s(&tm, &tt);
#else
        localtime_r(&tt, &tm);
#endif

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
            << '.' << std::setfill('0') << std::setw(6) << us.count();

        return oss.str();
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

    bool restartUSBDevice(uint16_t vid, uint16_t pid)
    {
        LOG_INFO("Restart USB device: %04x:%04x", vid, pid);
        bool removed = false;
#ifdef _WIN32
        HDEVINFO devInfo = SetupDiGetClassDevs(nullptr, TEXT("USB"), nullptr, DIGCF_PRESENT | DIGCF_ALLCLASSES);
        if (devInfo == INVALID_HANDLE_VALUE)
        {
            LOG_ERROR("SetupDiGetClassDevs failed");
            return false;
        }

        SP_DEVINFO_DATA devInfoData{};
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        DWORD i = 0;
        while (SetupDiEnumDeviceInfo(devInfo, i, &devInfoData))
        {
            TCHAR hardwareId[256]{};
            DWORD requiredSize = 0;

            if (SetupDiGetDeviceRegistryProperty(devInfo, &devInfoData, SPDRP_HARDWAREID,
                                                 nullptr, reinterpret_cast<BYTE *>(hardwareId),
                                                 sizeof(hardwareId), &requiredSize))
            {
                char vid_str[10], pid_str[10];
                sprintf_s(vid_str, "VID_%04X", vid);
                sprintf_s(pid_str, "PID_%04X", pid);

                // 转换为 ANSI 字符串以便 strstr（注意：实际应处理宽字符，但多数硬件ID是ASCII）
                char ansiHardwareId[256];
#ifdef UNICODE
                WideCharToMultiByte(CP_ACP, 0, hardwareId, -1, ansiHardwareId, sizeof(ansiHardwareId), nullptr, nullptr);
#else
                strcpy_s(ansiHardwareId, hardwareId);
#endif

                if (strstr(ansiHardwareId, vid_str) != nullptr && strstr(ansiHardwareId, pid_str) != nullptr)
                {
                    LOG_INFO("Found USB device: %s", ansiHardwareId);

                    LONG disableResult = CM_Disable_DevNode(devInfoData.DevInst, 0);
                    if (disableResult != CR_SUCCESS)
                    {
                        LOG_ERROR("Disable USB device error: %ld", disableResult);
                        // 尝试强制重装（可选）
                        SP_DEVINSTALL_PARAMS params{};
                        params.cbSize = sizeof(params);
                        if (SetupDiGetDeviceInstallParams(devInfo, &devInfoData, &params))
                        {
                            params.Flags |= 0x00000040; // DI_FORCE_REMOVE
                            SetupDiSetDeviceInstallParams(devInfo, &devInfoData, &params);
                            SetupDiCallClassInstaller(DIF_INSTALLDEVICE, devInfo, &devInfoData);
                        }
                    }

                    Sleep(100);

                    LONG enableResult = CM_Enable_DevNode(devInfoData.DevInst, 0);
                    if (enableResult != CR_SUCCESS)
                    {
                        LOG_ERROR("Enable USB device error: %ld", enableResult);
                    }

                    LOG_INFO("Restart USB device success.");
                    removed = true;
                    break; // 通常只重启第一个匹配设备
                }
            }
            ++i;
        }

        SetupDiDestroyDeviceInfoList(devInfo);
#endif
        return removed;
    }

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