#ifndef __DECODER_FACTORY_HPP__
#define __DECODER_FACTORY_HPP__

#include <fluxeem/hal/data_stream/raw_event_stream_encoding_type.hpp>
#include <fluxeem/hal/data_stream/decoder.hpp>

namespace fluxeem
{

	class FLUXEEM_API DecoderFactory
	{
	private:
		/* data */
	public:
		static std::unique_ptr<Decoder> createUniqueDecoder(const RawEventStreamFormat& format);
	};


} // namespace fluxeem

#endif // __DECODER_FACTORY_HPP__
