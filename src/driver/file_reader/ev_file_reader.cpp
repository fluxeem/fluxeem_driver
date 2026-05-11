#include <fluxeem/driver/file_reader/ev_file_reader.h>
#include <fluxeem/driver/file_reader/raw_file_reader.h>

namespace fluxeem
{
	std::unique_ptr<EvFileReader> EvFileReader::createFileReader(const std::string& filepath)
	{
		std::string format_name;
		size_t dotPos = filepath.rfind('.');
		if (dotPos != std::string::npos) {
			format_name = filepath.substr(dotPos);
		}
		else
		{
			return nullptr;
		}

		if (format_name == ".raw")
		{
			return std::make_unique<RawFileReader>(filepath);
		}
		else
		{
			return nullptr;
		}
	}
}//namespace fluxeem