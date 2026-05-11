#include <fluxeem/hal/data_stream/decode_factory.hpp>
#include <fluxeem/hal/data_stream/evt3_decoder.h>

namespace fluxeem
{
    std::unique_ptr<Decoder> DecoderFactory::createUniqueDecoder(const RawEventStreamFormat &format)
    {
        switch (format.getEncodingType())
        {
        case RawEventStreamEncodingType::EVT3:
            return std::make_unique<EVT3Decoder>(std::stoi(format["width"]), std::stoi(format["height"]));
            break;

        default:
            return nullptr;
            break;
        }
    }
} // namespace fluxeem
