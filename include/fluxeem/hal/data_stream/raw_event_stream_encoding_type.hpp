// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef __RAW_EVENT_STREAM_FORMAT_HPP__
#define __RAW_EVENT_STREAM_FORMAT_HPP__

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <sstream>
#include <map>
#include <fluxeem/base/define/base_define.h>

namespace fluxeem
{

    /** \~english @brief Enumeration of raw event stream encoding schemes
     * \~chinese @brief 原始事件流编码方案枚举
     */
    #ifdef _WIN32
    enum class FLUXEEM_API RawEventStreamEncodingType
    #else
    enum class RawEventStreamEncodingType
    #endif // _WIN32
    {
        /** \~english @brief EVT3 encoding scheme
         * \~chinese @brief EVT3 编码方案
         */
        EVT3 = 0,

        /** \~english @brief Unrecognized encoding scheme
         * \~chinese @brief 无法识别的编码方案
         */
        UNKNOWN = -1
    };

    /** \~english @brief Describes the format and geometry of camera's raw event stream prior to decoding
     * \~chinese @brief 描述相机原始事件流在解码前的格式与几何信息
     */
    class FLUXEEM_API RawEventStreamFormat
    {
    public:
        /** \~english @brief Construct from a format string obtained from the camera device
         * \~english @param format Expected format: "EVT3;height=720;width=1280"
         * \~chinese @brief 从相机设备返回的格式字符串构造实例
         * \~chinese @param format 预期格式："EVT3;height=720;width=1280"
         */
        RawEventStreamFormat(std::string format);

        /** \~english @brief Return the encoding type as a human-readable string
         * \~english @return std::string String representation of the encoding type
         * \~chinese @brief 返回编码类型的可读字符串
         * \~chinese @return std::string 编码类型的字符串形式
         */
        std::string getEncodingTypeStr() const;

        /** \~english @brief Return the encoding type as an enum value
         * \~english @return RawEventStreamEncodingType The encoding type enum
         * \~chinese @brief 返回编码类型的枚举值
         * \~chinese @return RawEventStreamEncodingType 编码类型枚举
         */
        RawEventStreamEncodingType getEncodingType() const;

        /** \~english @brief Determine whether a named option exists in the format
         * \~english @param name Option name to look up
         * \~english @return bool true if the option is present
         * \~chinese @brief 判断指定名称的选项是否存在于格式中
         * \~chinese @param name 待查找的选项名称
         * \~chinese @return bool 存在返回 true
         */
        bool contains(const std::string &name) const;

        /** \~english @brief Retrieve the value of a format option (e.g. width, height)
         * \~english @param name Option name
         * \~english @return const std::string & Option value
         * \~chinese @brief 读取格式选项值（如宽度、高度）
         * \~chinese @param name 选项名称
         * \~chinese @return const std::string & 选项值
         */
        const std::string &operator[](const std::string_view name) const;

    private:
        /** \~english @brief Parse the encoding type from its string representation
         * \~english @param encodingTypeStr String form of the encoding type
         * \~chinese @brief 从字符串解析出编码类型
         * \~chinese @param encodingTypeStr 编码类型的字符串形式
         */
        void parseEncodingType(const std::string &encoding_type_str);

        /**
         * \~english @brief String representation of the encoding type
         * \~chinese @brief 编码类型的字符串表示 
        */
        std::string encoding_type_str_; 
        /**
         *  \~english @brief Enum value of the encoding type
         * \~chinese @brief 编码类型的枚举值 
         */
        RawEventStreamEncodingType encoding_type_; 
        /**
         *  \~english @brief Map of format options
         * \~chinese @brief 格式选项的映射 
         */                   
        std::map<std::string, std::string> options_; 
    };

} // namespace fluxeem

#endif // __RAW_EVENT_STREAM_FORMAT_HPP__