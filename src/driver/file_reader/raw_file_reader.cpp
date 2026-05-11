// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#include <fluxeem/driver/file_reader/raw_file_reader.h>
#include <fluxeem/hal/data_stream/decode_factory.hpp>
#include <fluxeem/base/utility/func_utils.h>
#include <fluxeem/base/logging/logger.h>

namespace fluxeem
{
	RawFileReader::RawFileReader(const std::string& filepath)
		: input_file_path_(filepath)
	{
		RawEventStreamFormat rawEventStreamFormat = { "EVT3;height=720;width=1280" };
		decoder_ = DecoderFactory::createUniqueDecoder(rawEventStreamFormat);
		decoded_events_buffer_ = std::make_shared<EventBatch>();
		cached_events_buffer_ = std::make_shared<EventBatch>();
		read_events_buffer_ = std::make_shared<EventBatch>();
	}

	RawFileReader::~RawFileReader()
	{
		if (input_stream_.is_open())
		{
			input_stream_.close();
		}
	}

	bool RawFileReader::open()
	{
		input_stream_.open(input_file_path_, std::ios::in | std::ios::binary);
		index_file_path_ = input_file_path_;
		size_t pos = index_file_path_.rfind(".raw");
		if (pos != std::string::npos) {
			index_file_path_.replace(pos, 4, ".flxidx");
		}
		if (!input_stream_.is_open())
		{
			LOG_ERROR("Failed to open raw file: %s", input_file_path_.c_str());
			return false;
		}

		input_stream_.seekg(0, std::ios::end);
		file_size_bytes_ = input_stream_.tellg();
		input_stream_.seekg(0, std::ios::beg);

		if(!readFileHeader(input_stream_, file_info_))
		{
			LOG_ERROR("Failed to read file header from: %s", input_file_path_.c_str());
			return false;
		}

		// Validate that file header contained resolution
		if (file_info_.width == 0 || file_info_.height == 0)
		{
			LOG_ERROR("File header missing sensor resolution (width=%d, height=%d)",
					  file_info_.width, file_info_.height);
			return false;
		}

		data_start_offset_ = input_stream_.tellg();

		LOG_INFO("Loading raw file...");

		if (std::filesystem::exists(index_file_path_)) {
			if (checkBookmarksFromFile(index_file_path_)) {
				loadBookmarksFromFile(index_file_path_);
			}
			else {
				LOG_WARN("Bookmark file check failed, re-initialize bookmarks.");
				initBookmark();
			}
		}
		else {
			initBookmark();		
		}
		decoder_->setEventBatchHandleCallback([this](const fluxeem::Event2D* begin, const fluxeem::Event2D* end) {
			decoded_events_buffer_->insert(decoded_events_buffer_->end(), begin, end);
		});
		if (bookmarks_by_timestamp_.size() > 0)
		{
			// init parameter
			start_timestamp_ = bookmarks_by_timestamp_.begin()->first;
        	end_timestamp_ = std::prev(bookmarks_by_timestamp_.end())->first;
        	total_event_count_ = std::prev(bookmarks_by_timestamp_.end())->second.event_count;
			seekToTimestamp(start_timestamp_);
			LOG_INFO("Loading raw file succeeded.");
			decoder_->setTriggerInCallback([this](const EventTriggerIn trigger)
			{
				std::unique_lock<std::mutex> lock(trigger_in_callbacks_mutex_);
				for (auto it = trigger_in_callbacks_.begin(), it_end = trigger_in_callbacks_.end(); it != it_end; ++it)
				{
					it->second(trigger);
				}
			});
			return true;
		}
		else
		{
			LOG_ERROR("Loading raw file failed. There is no data in the file.");
			return false;
		}
	}

	bool RawFileReader::getStartTime(Timestamp &start_timestamp)
	{
		start_timestamp = start_timestamp_;
		return true;
	}

	bool RawFileReader::getEndTime(Timestamp &end_timestamp)
	{
		end_timestamp = end_timestamp_;
		return true;
	}

	bool RawFileReader::getEventCount(uint64_t &num)
	{
		num = total_event_count_;
		return true;
	}

	bool RawFileReader::isEndReached() 
	{
		if(current_timestamp_ > end_timestamp_)
		{
			return true;
		}else
		{
			return false;
		}
	}

	bool RawFileReader::seekToTimestamp(Timestamp t)
	{
		if(t > end_timestamp_)
		{
			current_timestamp_ = end_timestamp_ + 1;
			return false;
		}
		
		if(t < start_timestamp_)
		{
			current_timestamp_ = start_timestamp_ - 1;
			return false;
		}
		Bookmark get_bookmark;
		Bookmark set_bookmark;
		if (!getRawFileBookmark(t, get_bookmark))
		{
			return false;
		}

		if (get_bookmark.timestamp == t)
		{
			set_bookmark = get_bookmark;
		}
		else
		{
			input_stream_.clear();
			input_stream_.seekg(get_bookmark.byte_offset, std::ios::beg);
			evt3Decoder().setDecodeStatus(get_bookmark.status);
			decoder_->setLastTimestamp(get_bookmark.timestamp);
			bool find_timestamp_flag = false;
			std::streamsize cur_pos_before_read = 0;
			while (!input_stream_.eof())
			{
				io_buffer_.resize(kReadChunkSizeBytes);
				cur_pos_before_read = input_stream_.tellg();
				input_stream_.read(reinterpret_cast<char*>(io_buffer_.data()), io_buffer_.size());
				int cur_read_size = static_cast<int>(input_stream_.gcount());
				io_buffer_.resize(cur_read_size);

				uint8_t raw_event_bytes = decoder_->getRawEventSizeBytes();

				for (int i = 0; (i + raw_event_bytes) <= static_cast<int>(io_buffer_.size()); i += raw_event_bytes) {
					decoder_->decodeTimestamp(io_buffer_.data() + i, io_buffer_.data() + i + raw_event_bytes);
					Timestamp new_ts = decoder_->getLastTimestamp();
					if (new_ts >= t)
					{
						set_bookmark.byte_offset = cur_pos_before_read + i;
						set_bookmark.timestamp = new_ts;
						find_timestamp_flag = true;
						break;
					}
				}
				if (find_timestamp_flag)
				{
					break;
				}
			}
			if (find_timestamp_flag)
			{
				uint32_t event_num = 0;
				input_stream_.clear();
				input_stream_.seekg(get_bookmark.byte_offset);
				while (!input_stream_.eof() && input_stream_.tellg() < set_bookmark.byte_offset)
				{
					io_buffer_.resize(kReadChunkSizeBytes);
					cur_pos_before_read = input_stream_.tellg();
					input_stream_.read(reinterpret_cast<char*>(io_buffer_.data()), io_buffer_.size());
					int cur_read_size = static_cast<int>(input_stream_.gcount());
					io_buffer_.resize(cur_read_size);
					event_num += decoder_->decode(io_buffer_);
					decoded_events_buffer_->clear();
				}
				set_bookmark.event_count = get_bookmark.event_count + event_num;
				set_bookmark.status = evt3Decoder().getDecodeStatus();
			}
			else
			{
				return false;
			}
		}

		input_stream_.clear();
		input_stream_.seekg(set_bookmark.byte_offset, std::ios::beg);
		evt3Decoder().setDecodeStatus(set_bookmark.status);
		decoder_->setLastTimestamp(set_bookmark.timestamp);
		current_timestamp_ = t;
		current_event_index_ = set_bookmark.event_count;
		cached_events_buffer_->clear();
		decoded_events_buffer_->clear();
		// Seek causes a natural timestamp discontinuity; suppress the first warning.
		decoder_->suppressNextTsJumpWarning(1);
		return true;
	}

	bool RawFileReader::seekToEventIndex(uint64_t n_event)
	{
		if(n_event > total_event_count_)
		{
			current_event_index_ = total_event_count_ + 1;
			current_timestamp_ = end_timestamp_ + 1;
			return false;
		}

		Bookmark get_bookmark;
		Bookmark set_bookmark;
		if (!getRawFileBookmarkByNEvents(n_event, get_bookmark))
		{
			return false;
		}
		
		// No mutex needed — class is single-threaded
		if (get_bookmark.event_count == n_event)
		{
			set_bookmark = get_bookmark;
		}
		else
		{
			bool find_n_event_flag = false;
			input_stream_.clear();
			input_stream_.seekg(get_bookmark.byte_offset, std::ios::beg);
			evt3Decoder().setDecodeStatus(get_bookmark.status);
			decoder_->setLastTimestamp(get_bookmark.timestamp);

			std::streamsize cur_pos_before_read = 0;
			uint64_t decode_count = 0;
			while (!input_stream_.eof())
			{
				io_buffer_.resize(kReadChunkSizeBytes);
				cur_pos_before_read = input_stream_.tellg();
				input_stream_.read(reinterpret_cast<char*>(io_buffer_.data()), io_buffer_.size());
				int cur_read_size = static_cast<int>(input_stream_.gcount());
				io_buffer_.resize(cur_read_size);

				uint8_t raw_event_bytes = decoder_->getRawEventSizeBytes();

				for (int i = 0; i < static_cast<int>(io_buffer_.size()); i += raw_event_bytes) {
					decode_count += decoder_->decode(io_buffer_.data() + i, io_buffer_.data() + i + raw_event_bytes);
					if (get_bookmark.event_count + decode_count >= n_event)
					{
						find_n_event_flag = true;
						set_bookmark.event_count = static_cast<uint32_t>(get_bookmark.event_count + decode_count);
						set_bookmark.status = evt3Decoder().getDecodeStatus();
						set_bookmark.timestamp = decoder_->getLastTimestamp();
						set_bookmark.byte_offset = cur_pos_before_read + i;
						break;
					}
				}
				if (find_n_event_flag)
				{
					break;
				}
			}
			if (!find_n_event_flag)
			{
				return false;
			}
		}
		
		input_stream_.clear();
		input_stream_.seekg(set_bookmark.byte_offset, std::ios::beg);
		evt3Decoder().setDecodeStatus(set_bookmark.status);
		decoder_->setLastTimestamp(set_bookmark.timestamp);
		current_timestamp_ = set_bookmark.timestamp;
		current_event_index_ = set_bookmark.event_count;
		cached_events_buffer_->clear();
		decoded_events_buffer_->clear();
		// Seek causes a natural timestamp discontinuity; suppress the first warning.
		decoder_->suppressNextTsJumpWarning(1);
		return true;
	}

	Timestamp RawFileReader::getCurrentTimestamp()
	{
		return current_timestamp_;
	}

	uint64_t RawFileReader::getCurrentEventIndex()
	{
		return current_event_index_;
	}

	std::shared_ptr<EventBatch> RawFileReader::readEvents(uint64_t n)
	{
		if(current_event_index_ + n > total_event_count_)
		{
			LOG_WARN("Requested number of events exceeds total number of events.");
			n = total_event_count_ - current_event_index_;
		}
		read_events_buffer_->clear();

		// If the reserved buffer contains data, it directly retrieves the data from the buffer
		if (cached_events_buffer_->size() > 0)
		{
			if (cached_events_buffer_->size() > n)
			{
				read_events_buffer_->insert(read_events_buffer_->end(), cached_events_buffer_->begin(), cached_events_buffer_->begin() + n);
				cached_events_buffer_->erase(cached_events_buffer_->begin(), cached_events_buffer_->begin() + n);
				if (read_events_buffer_->size() > 0)
				{
					auto last_event = std::prev(read_events_buffer_->end());
					current_timestamp_ = last_event->timestamp + 1;
				}
				current_event_index_ += read_events_buffer_->size();
				return read_events_buffer_;
			}
			else
			{
				read_events_buffer_->insert(read_events_buffer_->end(), cached_events_buffer_->begin(), cached_events_buffer_->end());
				cached_events_buffer_->clear();
			}
		}

		auto stop_condition = [this, n]() {
			uint64_t cur_events = read_events_buffer_->size();
			if (decoded_events_buffer_->size() + cur_events >= n)
			{
				read_events_buffer_->insert(read_events_buffer_->end(),
					decoded_events_buffer_->begin(),
					decoded_events_buffer_->begin() + (n - cur_events));
				cached_events_buffer_->clear();
				cached_events_buffer_->insert(cached_events_buffer_->end(),
					decoded_events_buffer_->begin() + (n - cur_events),
					decoded_events_buffer_->end());
				decoded_events_buffer_->clear();
				return true;
			}
			else
			{
				read_events_buffer_->insert(read_events_buffer_->end(),
					decoded_events_buffer_->begin(),
					decoded_events_buffer_->end());
				decoded_events_buffer_->clear();
				return false;
			}
			};

		if (readAndDecodeEvents(stop_condition))
		{
			if (read_events_buffer_->size() > 0)
			{
				auto last_event = std::prev(read_events_buffer_->end());
				current_timestamp_ = last_event->timestamp + 1;
			}
			current_event_index_ += read_events_buffer_->size();
			return read_events_buffer_;
		}
		else
		{
            return read_events_buffer_;
		}
	}

	std::shared_ptr<EventBatch> RawFileReader::readEventsFromTimestamp(Timestamp start, uint64_t n)
	{
		if (seekToTimestamp(start))
		{
			return readEvents(n);
		}
		else
		{
			return std::make_shared<EventBatch>();
		}
	}

	std::shared_ptr<EventBatch> RawFileReader::readEventsFromEventIndex(uint64_t start_event_num, uint64_t n)
	{
		if (seekToEventIndex(start_event_num))
		{
			return readEvents(n);
		}
		else
		{
			return std::make_shared<EventBatch>();
		}
	}

	std::shared_ptr<EventBatch> RawFileReader::readEventsByTimeIntervalFromTimestamp(Timestamp start, Timestamp interval)
	{
		if (seekToTimestamp(start))
		{
			uint64_t time_exceeded = current_timestamp_ - start;
			if (interval <= time_exceeded)
			{
				LOG_WARN("Requested time range is not valid, return empty result.");
				std::shared_ptr<EventBatch> empty_vector = std::make_shared<EventBatch>();
				return empty_vector;
			}
			return readEventsByTimeInterval(interval - time_exceeded);
		}
		else
		{
			std::shared_ptr<EventBatch> empty_vector = std::make_shared<EventBatch>();
            return empty_vector;
		}

	}

	std::shared_ptr<EventBatch> RawFileReader::readEventsByTimeInterval(Timestamp interval)
	{
		Timestamp ts_target = current_timestamp_ + interval;
		if(ts_target < start_timestamp_)
		{
			LOG_WARN("Requested interval is before start timestamp.");
            current_event_index_ = 0;
            current_timestamp_ = ts_target;
            return std::make_shared<EventBatch>();
		}
		if (current_timestamp_ > end_timestamp_)
        {
            LOG_WARN("Current read pos exceeds end timestamp.");
            current_event_index_ = total_event_count_;
            return std::make_shared<EventBatch>();
        } else if(ts_target > end_timestamp_)
		{
			LOG_WARN("Requested interval exceeds end timestamp.");
            ts_target = end_timestamp_ + 1;
		}
		current_timestamp_ += interval;
		read_events_buffer_->clear();
		if (cached_events_buffer_->size() > 0)
		{
			// If there is enough data in the remain buffer, the data is returned without further access to the file
			Timestamp last_ts = std::prev(cached_events_buffer_->end())->timestamp;
			if (last_ts < ts_target)
			{
				read_events_buffer_->insert(read_events_buffer_->end(), cached_events_buffer_->begin(), cached_events_buffer_->end());
				cached_events_buffer_->clear();
			}
			else
			{
				EventIterator_t target_pos = binarySearchTimestamp(ts_target,
					cached_events_buffer_->data(),
					cached_events_buffer_->data() + cached_events_buffer_->size());

				uint32_t target_offset = static_cast<uint32_t>(target_pos - cached_events_buffer_->data());
				read_events_buffer_->insert(read_events_buffer_->end(), cached_events_buffer_->begin(), cached_events_buffer_->begin() + target_offset);
				cached_events_buffer_->erase(cached_events_buffer_->begin(), cached_events_buffer_->begin() + target_offset);
				current_event_index_ += read_events_buffer_->size();
				return read_events_buffer_;
			}
		}
		auto stop_condition = [this, &ts_target, &interval]() {
			if (!decoded_events_buffer_->empty())
			{
				auto last_event = decoded_events_buffer_->back();
				if (last_event.timestamp < ts_target)
				{
					read_events_buffer_->insert(read_events_buffer_->end(), decoded_events_buffer_->begin(), decoded_events_buffer_->end());
					decoded_events_buffer_->clear();
					return false;
				}
				else
				{
					EventIterator_t target_pos = binarySearchTimestamp(ts_target, decoded_events_buffer_->data(),
						decoded_events_buffer_->data() + decoded_events_buffer_->size());
					uint32_t target_offset = static_cast<uint32_t>(target_pos - decoded_events_buffer_->data());
					read_events_buffer_->insert(read_events_buffer_->end(), decoded_events_buffer_->data(), decoded_events_buffer_->data() + target_offset);

					if (decoded_events_buffer_->size() - target_offset > 0)
					{
						cached_events_buffer_->insert(cached_events_buffer_->end(), decoded_events_buffer_->data() + target_offset,
							decoded_events_buffer_->data() + decoded_events_buffer_->size());
					}
					decoded_events_buffer_->clear();
					
					return true;
				}
			}
			else
			{
				return false;
			}
		};

		if (readAndDecodeEvents(stop_condition))
		{
			current_event_index_ += read_events_buffer_->size();
			return read_events_buffer_;
		}
		else
		{
            return read_events_buffer_;
		}
	}
	
	void RawFileReader::serializeBookmarkMap(const BookmarkMap& bookmarks, const std::string& filename) {
		std::ofstream out(filename, std::ios::binary);
		if (!out.is_open()) {
			LOG_ERROR("Failed to open index file for writing: %s", filename.c_str());
			return;
		}

		// ── Text header (human-readable, viewable in any text editor) ──
		// Uses '% ' prefix to stay consistent with .raw file header style.
		out << "% Fluxeem Index File (.flxidx)\n";
		out << "% index_version " << kIndexFileVersion << "\n";
		out << "% source_file " << input_file_path_ << "\n";
		out << "% source_size " << static_cast<uint64_t>(file_size_bytes_) << "\n";
		out << "% serial_number " << file_info_.serial_number << "\n";
		out << "% sensor_width " << file_info_.width << "\n";
		out << "% sensor_height " << file_info_.height << "\n";
		out << "% start_timestamp " << (bookmarks.empty() ? 0 : bookmarks.begin()->first) << "\n";
		out << "% end_timestamp " << (bookmarks.empty() ? 0 : std::prev(bookmarks.end())->first) << "\n";
		out << "% total_events " << (bookmarks.empty() ? 0 : std::prev(bookmarks.end())->second.event_count) << "\n";
		out << "% bookmark_count " << bookmarks.size() << "\n";
		out << "% bookmark_period_us " << kBookmarkPeriodUs << "\n";
		out << "% end\n";

		// ── Binary payload ──
		// NUL sentinel separates text header from binary data, so the loader
		// can skip the header quickly by scanning for '\0'.
		const char sentinel = '\0';
		out.write(&sentinel, 1);

		uint64_t map_size = bookmarks.size();
		out.write(reinterpret_cast<const char*>(&map_size), sizeof(map_size));

		for (const auto& [ts, bm] : bookmarks) {
			out.write(reinterpret_cast<const char*>(&ts), sizeof(ts));
			out.write(reinterpret_cast<const char*>(&bm.byte_offset), sizeof(bm.byte_offset));
			out.write(reinterpret_cast<const char*>(&bm.status), sizeof(bm.status));
			out.write(reinterpret_cast<const char*>(&bm.timestamp), sizeof(bm.timestamp));
			out.write(reinterpret_cast<const char*>(&bm.event_count), sizeof(bm.event_count));
		}

		out.close();
	}
	
	void RawFileReader::loadBookmarksFromFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::binary);
		if (!file.is_open()) {
			LOG_ERROR("Error opening index file for reading: %s", filename.c_str());
			return;
		}

		// Skip text header — scan past the NUL sentinel
		char ch;
		while (file.get(ch)) {
			if (ch == '\0') break;
		}

		uint64_t map_size = 0;
		file.read(reinterpret_cast<char*>(&map_size), sizeof(map_size));

		BookmarkMap bookmarks;
		for (uint64_t i = 0; i < map_size; ++i) {
			uint64_t timestamp;
			Bookmark bookmark;
			file.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));
			file.read(reinterpret_cast<char*>(&bookmark.byte_offset), sizeof(bookmark.byte_offset));
			file.read(reinterpret_cast<char*>(&bookmark.status), sizeof(bookmark.status));
			file.read(reinterpret_cast<char*>(&bookmark.timestamp), sizeof(bookmark.timestamp));
			file.read(reinterpret_cast<char*>(&bookmark.event_count), sizeof(bookmark.event_count));
			timestamp_by_event_count_.insert({bookmark.event_count, bookmark.timestamp});
			bookmarks[timestamp] = bookmark;
		}
		bookmarks_by_timestamp_ = bookmarks;
		file.close();
	}

	bool RawFileReader::checkBookmarksFromFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::binary);
		if (!file.is_open()) {
			LOG_ERROR("Error opening index file: %s", filename.c_str());
			return false;
		}
		file.seekg(0, std::ios::end);
		std::streamsize fileSize = file.tellg();
		file.seekg(0, std::ios::beg);
		constexpr std::streamsize kMaxIndexFileSize = 1000 * 1024 * 1024;
		if (fileSize > kMaxIndexFileSize) {
			LOG_WARN("Index file is too large (%lld bytes)", static_cast<long long>(fileSize));
			return false;
		}

		// Read entire file for CRC check
		std::vector<uint8_t> file_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();

		if (file_content.size() < 2) {
			LOG_WARN("Index file too small to contain CRC");
			return false;
		}
		uint8_t storedCrc = file_content.back();
		file_content.pop_back();
		uint8_t calculatedCrc = crc8(file_content);
		if (calculatedCrc != storedCrc) {
			LOG_WARN("Index file CRC mismatch — file may be corrupted");
			return false;
		}

		// Parse '% key value' text header (up to NUL sentinel)
		std::string header_text;
		for (uint8_t byte : file_content) {
			if (byte == '\0') break;
			header_text += static_cast<char>(byte);
		}

		std::map<std::string, std::string> header_map;
		std::istringstream iss(header_text);
		std::string line;
		while (std::getline(iss, line)) {
			// Each line: "% key value" or "% key" (no value)
			if (line.size() < 3 || line[0] != '%' || line[1] != ' ') continue;
			std::string rest = line.substr(2);
			size_t sp = rest.find(' ');
			if (sp == std::string::npos) continue;  // skip lines like "% end"
			std::string key = rest.substr(0, sp);
			std::string val = rest.substr(sp + 1);
			header_map[key] = val;
		}

		// Validate index_version
		auto it_ver = header_map.find("index_version");
		if (it_ver == header_map.end() ||
			static_cast<uint32_t>(std::stoul(it_ver->second)) != kIndexFileVersion) {
			LOG_WARN("Index version mismatch (expected %u). Rebuilding.", kIndexFileVersion);
			return false;
		}

		// Validate source file size
		auto it_size = header_map.find("source_size");
		if (it_size == header_map.end() ||
			std::stoull(it_size->second) != static_cast<uint64_t>(file_size_bytes_)) {
			LOG_WARN("Index source_size mismatch — raw file has changed");
			return false;
		}

		// Validate sensor dimensions
		auto it_w = header_map.find("sensor_width");
		if (it_w == header_map.end() ||
			std::stoi(it_w->second) != file_info_.width) {
			LOG_WARN("Index sensor_width mismatch");
			return false;
		}
		auto it_h = header_map.find("sensor_height");
		if (it_h == header_map.end() ||
			std::stoi(it_h->second) != file_info_.height) {
			LOG_WARN("Index sensor_height mismatch");
			return false;
		}

		return true;
	}

	std::shared_ptr<EventBatch> RawFileReader::readEventsByTimeIntervalFromEventIndex(uint64_t event_num, Timestamp interval)
	{
		if (seekToEventIndex(event_num))
		{
			return readEventsByTimeInterval(interval);
		}
		else
		{
			return std::make_shared<EventBatch>();
		}
	}

	bool RawFileReader::exportEventsToRaw(Timestamp start, Timestamp end, const std::string& out_file_path)
	{
		if (!seekToTimestamp(end))
		{
			LOG_WARN("No end timestamp found. Exporting until end of file.");
			input_stream_.clear();
			input_stream_.seekg(0, std::ios::end);
		}
		std::streamsize end_byte_offset = input_stream_.tellg();
		if (!seekToTimestamp(start))
		{
			LOG_WARN("No start timestamp found. Exporting from beginning of file.");
			input_stream_.clear();
			input_stream_.seekg(data_start_offset_);
		}

		std::streamsize start_byte_offset = input_stream_.tellg();

		LOG_INFO("extract event start time: %llu, end time: %llu", start, end);
		
		if (start_byte_offset > end_byte_offset) {
			LOG_ERROR("Start time must be less than or equal to end time.");
			return false;
		}

		std::streamsize data_size = end_byte_offset - start_byte_offset;
		if (data_size <= 0) {
			LOG_ERROR("There is no data in the interval.");
			return false; 
		}

		std::ofstream output_file_stream(out_file_path, std::ios::binary | std::ios::out);
		if (!output_file_stream.is_open()) {
			LOG_ERROR("Failed to open output file: %s.", out_file_path.c_str());
			return false;
		}
		// write file header
		EvFileInfo file_info_tmp = file_info_;
		file_info_tmp.start_timestamp = start;
        std::string file_header_str = generateEvFileHeader(file_info_tmp);
        output_file_stream << file_header_str;
		input_stream_.seekg(start_byte_offset);

		constexpr std::streamsize kExtractBufferSize = 1024 * 1024; // 1MB
		std::vector<uint8_t> buffer;

		while (data_size > 0) 
		{
			std::streamsize bytes_to_read = std::min(data_size, kExtractBufferSize);
			buffer.resize(static_cast<size_t>(bytes_to_read));
			input_stream_.read(reinterpret_cast<char*>(buffer.data()), bytes_to_read);

			if (input_stream_.gcount() != bytes_to_read) {
				LOG_ERROR("Failed to read expected bytes from input file.");
				return false;
			}
			output_file_stream.write(reinterpret_cast<const char*>(buffer.data()), bytes_to_read);

			if (!output_file_stream.good()) {
				LOG_ERROR("Failed to write to output file: %s", out_file_path.c_str());
				return false;
			}

			data_size -= bytes_to_read;
		}

		output_file_stream.close();
		return true; 
	}
    
	uint32_t RawFileReader::registerTriggerInCallback(EvTriggerInCallback cb) {
        std::unique_lock<std::mutex> lock(trigger_in_callbacks_mutex_);
        trigger_in_callbacks_[next_trigger_callback_id_] = cb;
        return next_trigger_callback_id_++;
    }

    bool RawFileReader::unregisterTriggerInCallback(uint32_t callback_id)
    {
        std::unique_lock<std::mutex> lock(trigger_in_callbacks_mutex_);
        return trigger_in_callbacks_.erase(callback_id);
    }

	
	bool RawFileReader::getFileMetadata(EvFileInfo& out_file_info) 
	{
		out_file_info = file_info_;
		return true;
	}

	 bool RawFileReader::getDecodeStatistics(uint64_t& bandwidth_bytes, uint64_t& events_count)
	 {
		 bandwidth_bytes = decode_bandwidth_accum_bytes_;
		 events_count = decode_events_accum_count_;
		 decode_bandwidth_accum_bytes_ = 0;
		 decode_events_accum_count_ = 0;
		 return true;
	 }

	void RawFileReader::initBookmark()
	{
		std::vector<uint8_t> buffer(kReadChunkSizeBytes);
		Bookmark last_book_mark;
		last_book_mark.byte_offset = input_stream_.tellg();
		last_book_mark.status = evt3Decoder().getDecodeStatus();
		last_book_mark.timestamp = file_info_.start_timestamp;
		decoder_->setLastTimestamp(last_book_mark.timestamp);
		last_book_mark.event_count = 0;
		current_event_index_ = 0;
		std::streamsize cur_pos_before_read = 0;
		int cur_read_size = 0;
		while (!input_stream_.eof())
		{
			cur_pos_before_read = input_stream_.tellg();
			input_stream_.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

			cur_read_size = static_cast<int>(input_stream_.gcount());
			buffer.resize(cur_read_size);

			uint8_t raw_event_bytes = decoder_->getRawEventSizeBytes();
			for (int i = 0; i < static_cast<int>(buffer.size()); i += raw_event_bytes)
			{
				decoder_->decodeTimestamp(buffer.data() + i, buffer.data() + i + raw_event_bytes);
				Timestamp new_ts = decoder_->getLastTimestamp();

				if (last_book_mark.timestamp == 0 && new_ts != 0)
				{
					last_book_mark.timestamp = new_ts;
				}
				if (last_book_mark.timestamp != 0 && new_ts != 0)
				{
					if (new_ts >= last_book_mark.timestamp + kBookmarkPeriodUs)
					{
						bookmarks_by_timestamp_.insert(std::pair<Timestamp, Bookmark>(last_book_mark.timestamp, last_book_mark));
						last_book_mark.byte_offset = cur_pos_before_read + i;
						last_book_mark.timestamp = new_ts;
					}
				}
			}
			buffer.resize(kReadChunkSizeBytes);
		}
		last_book_mark.byte_offset = cur_pos_before_read + cur_read_size;
		last_book_mark.timestamp = decoder_->getLastTimestamp();
		bookmarks_by_timestamp_.insert(std::pair<Timestamp, Bookmark>(last_book_mark.timestamp, last_book_mark));
		
		// Second pass: count events per bookmark segment
		uint32_t total_event_num = 0;
		for (auto it = bookmarks_by_timestamp_.begin(); it != bookmarks_by_timestamp_.end(); ++it) {
			auto it_next = std::next(it);
			if (it_next != bookmarks_by_timestamp_.end()) {
				input_stream_.clear();
				input_stream_.seekg(it->second.byte_offset);
				size_t segment_size = static_cast<size_t>(it_next->second.byte_offset - it->second.byte_offset);
				buffer.resize(segment_size);
				input_stream_.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
				cur_read_size = static_cast<int>(input_stream_.gcount());
				buffer.resize(cur_read_size);
				total_event_num += decoder_->decode(buffer);
				it_next->second.event_count = total_event_num;
				it_next->second.status = evt3Decoder().getDecodeStatus();
				timestamp_by_event_count_.insert(std::pair<uint64_t, Timestamp>(it_next->second.event_count, it_next->first));
			}
		}
		timestamp_by_event_count_.insert(std::pair<uint64_t, Timestamp>(bookmarks_by_timestamp_.begin()->second.event_count, bookmarks_by_timestamp_.begin()->second.timestamp));
		serializeBookmarkMap(bookmarks_by_timestamp_, index_file_path_);

		// Append CRC8 to bookmark file
		std::ifstream input_file(index_file_path_, std::ios::binary);
		if (!input_file) {
			LOG_ERROR("Failed to open bookmark file for CRC: %s", index_file_path_.c_str());
			return;
		}
		std::vector<uint8_t> file_content((std::istreambuf_iterator<char>(input_file)), std::istreambuf_iterator<char>());
		input_file.close();
		uint8_t crc_value = crc8(file_content);
		file_content.push_back(crc_value);
		std::ofstream output_file(index_file_path_, std::ios::binary);
		if (!output_file) {
			LOG_ERROR("Failed to write bookmark CRC: %s", index_file_path_.c_str());
			return;
		}
		output_file.write(reinterpret_cast<const char*>(file_content.data()), file_content.size());
		output_file.close();
	}

	bool RawFileReader::getRawFileBookmark(Timestamp ts, Bookmark& bm)
	{
		BookmarkMap::iterator find_pos = bookmarks_by_timestamp_.lower_bound(ts);
		if (find_pos == bookmarks_by_timestamp_.end())
		{
			return false;
		}
		if (find_pos->first == ts)
		{
			//If it can be found directly in the bookmark, return it
			bm = find_pos->second;
			return true;
		}
		BookmarkMap::iterator pre_pos = std::prev(find_pos);
		if (pre_pos == bookmarks_by_timestamp_.end())
		{
			// If there is no previous bookmark, the data does not exist at this timestamp in the file
			return false;
		}
		bm = pre_pos->second;
		return true;
	}

	bool RawFileReader::getRawFileBookmarkByNEvents(uint64_t n_event, Bookmark& bm)
	{
		BookmarkEventsMap::iterator find_event_ts_ = timestamp_by_event_count_.lower_bound(n_event);
		if (find_event_ts_ == timestamp_by_event_count_.end())
		{
			return false;
		}
		if (n_event == 0)
		{
			bm = bookmarks_by_timestamp_.begin()->second;
			return true;
		}

		BookmarkEventsMap::iterator pre_pos = std::prev(find_event_ts_);
		if (pre_pos == timestamp_by_event_count_.end())
		{
			// If there is no previous timestamp, the data does not exist at this n event in the file
			return false;
		}

		Timestamp ts = pre_pos->second;
		
		bm = bookmarks_by_timestamp_[ts];
		return true;
	}

	bool RawFileReader::readAndDecodeEvents(std::function<bool()> stop_condition) {
		// No mutex — class is single-threaded
		// Use member io_buffer_ to avoid repeated allocations on this hot path
		while (!input_stream_.eof()) {
			io_buffer_.resize(kReadChunkSizeBytes);
			input_stream_.read(reinterpret_cast<char*>(io_buffer_.data()), io_buffer_.size());
			int cur_read_size = static_cast<int>(input_stream_.gcount());
			 if (cur_read_size <= 0) {
				 break;
			 }
			io_buffer_.resize(cur_read_size);
			 decode_bandwidth_accum_bytes_ += static_cast<uint64_t>(cur_read_size);
			 decode_events_accum_count_ += decoder_->decode(io_buffer_);
			if (stop_condition()) {
				return true;
			}
		}
		return !read_events_buffer_->empty();
	}

	uint8_t RawFileReader::crc8(const std::vector<uint8_t>& data) {
		uint8_t crc = 0;
		for (uint8_t byte : data) {
			crc ^= byte;
			for (int i = 0; i < 8; ++i) {
				if (crc & 0x80) {
					crc = (crc << 1) ^ 0x07;
				}
				else {
					crc <<= 1;
				}
			}
		}
		return crc;
	}

	bool RawFileReader::readFileHeader(std::ifstream& file_stream, EvFileInfo& file_info)
	{
		while (file_stream.peek() == '%')
		{
			std::string header_line;
			std::getline(file_stream, header_line);
			if (header_line == "% end")
			{
				break;
			}
			else if (header_line.substr(0, 9) == "% format ")
			{
				std::istringstream sf(header_line.substr(9));
				std::string format_name;
				std::getline(sf, format_name, ';');
				if (format_name != "EVT3")
				{
					LOG_ERROR("Detected non-EVT3 input file (format: %s)", format_name.c_str());
					return false;
				}
				while (!sf.eof())
				{
					std::string option;
					std::getline(sf, option, ';');
					{
						std::string name, value;
						std::istringstream so(option);
						std::getline(so, name, '=');
						std::getline(so, value, '=');
						if (name == "width")
						{
							file_info.width = std::stoi(value);
						}
						else if (name == "height")
						{
							file_info.height = std::stoi(value);
						}
					}
				}
			}
			else if (header_line.substr(0, 11) == "% geometry ")
			{
				std::istringstream sg(header_line.substr(11));
				std::string sw, sh;
				std::getline(sg, sw, 'x');
				std::getline(sg, sh);
				file_info.width = std::stoi(sw);
				file_info.height = std::stoi(sh);
			}
			else if (header_line.substr(0, 6) == "% evt ")
			{
				if (header_line.substr(6) != "3.0")
				{
					LOG_ERROR("Detected non-EVT3 input file (evt version: %s)", header_line.substr(6).c_str());
					return false;
				}
			}
			else if (header_line.substr(0, 16) == "% serial_number ")
			{
				file_info.serial_number = header_line.substr(16);
			}
			else if (header_line.substr(0, 18) == "% start_timestamp ")
			{
				file_info.start_timestamp = std::stoull(header_line.substr(18));
			}
			else if (header_line.substr(0, 22) == "% dvs_start_timestamp ")
			{
				file_info.start_timestamp = std::stoull(header_line.substr(22));
			}
			else if (header_line.substr(0, 7) == "% date ")
			{
				std::string local_time_str = header_line.substr(7, 25);
				file_info.local_time = parseLocalTimeString(local_time_str);
			}
		}
		return true;
	}
}
