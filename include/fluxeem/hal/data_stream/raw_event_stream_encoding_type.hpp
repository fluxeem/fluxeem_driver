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

    /** \~english @brief Enum for the different types of raw event stream encoding
     * \~chinese @brief 原始事件流编码类型的枚举
     */
    #ifdef _WIN32
    enum class FLUXEEM_API RawEventStreamEncodingType
    #else
    enum class RawEventStreamEncodingType
    #endif // _WIN32
    {
        /** \~english @brief Encoding type for EVT3
         * \~chinese @brief EVT3 编码类型
         */
        EVT3 = 0,

        /** \~english @brief Unknown encoding type
         * \~chinese @brief 未知编码类型
         */
        UNKNOWN = -1
    };

    /** \~english @brief A class to describe the format and geometry of raw event stream directly from camera before decoding
     * \~chinese @brief 描述直接从相机读取的原始事件流格式和几何形状的类，在解码之前
     */
    class FLUXEEM_API RawEventStreamFormat
    {
    public:
        /** \~english @brief Construct RawEventStreamFormat instance from string read from camera
         * \~english @param format, expected to look like "EVT3;height=720;width=1280"
         * \~chinese @brief 从相机读取的字符串构造 RawEventStreamFormat 实例
         * \~chinese @param format 预期看起来像 "EVT3;height=720;width=1280"
         */
        RawEventStreamFormat(std::string format);

        /** \~english @brief Get the encoding type as a string
         * \~english @return std::string Encoding type as a string
         * \~chinese @brief 获取编码类型为字符串形式
         * \~chinese @return std::string 编码类型为字符串形式
         */
        std::string getEncodingTypeStr() const;

        /** \~english @brief Get the encoding type
         * \~english @return RawEventStreamEncodingType Encoding type
         * \~chinese @brief 获取编码类型
         * \~chinese @return RawEventStreamEncodingType 编码类型
         */
        RawEventStreamEncodingType getEncodingType() const;

        /** \~english @brief Check if the format contains a specific option
         * \~english @param name Name of the option to check
         * \~english @return bool True if the option is contained, false otherwise
         * \~chinese @brief 检查格式是否包含特定选项
         * \~chinese @param name 要检查的选项名称
         * \~chinese @return bool 如果包含该选项则返回true，否则返回false
         */
        bool contains(const std::string &name) const;

        /** \~english @brief Accessor to format options, such as width, height, etc.
         * \~english @param name Option name
         * \~english @return const std::string & Value of the option
         * \~chinese @brief 访问格式选项，如宽度、高度等
         * \~chinese @param name 选项名称
         * \~chinese @return const std::string & 选项值
         */
        const std::string &operator[](const std::string_view name) const;

    private:
        /** \~english @brief Parses the encoding type from a string
         * \~english @param encodingTypeStr String representation of the encoding type
         * \~chinese @brief 从字符串解析编码类型
         * \~chinese @param encodingTypeStr 编码类型的字符串表示
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