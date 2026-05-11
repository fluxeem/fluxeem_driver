#include <fluxeem/base/define/base_define.h>
#include <fluxeem/hal/data_stream/raw_event_stream_encoding_type.hpp>
namespace fluxeem
{
    RawEventStreamFormat::RawEventStreamFormat(std::string format)
    {
        // format string should be somethind like "EVT3;height=720;width=1280"
        std::istringstream sf(format);

        // first element is the format name
        std::getline(sf, encoding_type_str_, ';');
        parseEncodingType(encoding_type_str_);

        // then what remains is options
        // we don't try to detect malformed strings
        while (!sf.eof())
        {
            std::string option;
            std::getline(sf, option, ';');
            {
                std::string name, value;
                std::istringstream so{option};
                std::getline(so, name, '=');
                std::getline(so, value);
                options_[name] = value;
            }
        }
    }

    std::string RawEventStreamFormat::getEncodingTypeStr() const
    {
        return encoding_type_str_;
    }

    RawEventStreamEncodingType RawEventStreamFormat::getEncodingType() const
    {
        return encoding_type_;
    }

    bool RawEventStreamFormat::contains(const std::string &option) const
    {
        return options_.find(option) != options_.end();
    }

    const std::string &RawEventStreamFormat::operator[](const std::string_view name) const
    {
        auto it = options_.find(static_cast<std::string>(name));
        if (it != options_.end())
        {
            return it->second;
        }
        else
        {
            throw std::out_of_range("option not found");
        }
    }

    void RawEventStreamFormat::parseEncodingType(const std::string &encodingTypeStr)
    {
        if (encoding_type_str_ == "EVT3")
        {
            encoding_type_ = RawEventStreamEncodingType::EVT3;
        }
        else
        {
            encoding_type_ = RawEventStreamEncodingType::UNKNOWN;
        }
    }

} // namespace fluxeem
