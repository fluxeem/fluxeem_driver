// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#include <fluxeem/hal/common/usb_interface.h>
#include <fluxeem/base/logging/logger.h>
#include <fluxeem/driver/camera/device/prophesee_rdk3.hpp>
#include <fluxeem/driver/camera/device/fluxeem_apex_vision_s1.hpp>
#include <fstream>
#include <cstring>
#include <chrono>
#include <thread>
#include <algorithm>

namespace fluxeem
{
	UsbInterface::UsbInterface(CameraDescription camera_description) : Interface(camera_description)
	{
		camera_description_ = camera_description;
	}
	UsbInterface::UsbInterface(std::shared_ptr<UsbDevice> usb_handle) : Interface(usb_handle)
	{
		usb_handle_ = usb_handle;
	}

	UsbInterface::~UsbInterface()
	{
		close();
	}

	bool UsbInterface::open()
	{
		if (!usb_handle_)
		{
			LOG_ERROR("UsbInterface: usb_handle_ is null");
			return false;
		}
		if (usb_handle_->isOpen())
		{
			return true;
		}
		const int r = usb_handle_->open();
		if (r != LIBUSB_SUCCESS)
		{
			LOG_ERROR("UsbInterface: open failed: %s", libusb_error_name(r));
			return false;
		}
		// Best-effort: initialize endpoint info (failure is non-fatal; some devices may not be ready yet).
		try
		{
			usb_handle_->initInterface();
		}
		catch (const std::exception &e)
		{
			LOG_WARN("UsbInterface: initInterface failed: %s", e.what());
		}
		return true;
	}

	void UsbInterface::close() noexcept
	{
		if (usb_handle_)
		{
			usb_handle_->close();
		}
	}

	bool UsbInterface::isOpen() const noexcept
	{
		return usb_handle_ && usb_handle_->isOpen();
	}

	bool UsbInterface::isConnected() const
	{
		if (!usb_handle_ || !usb_handle_->isOpen())
		{
			return false;
		}
		return usb_handle_->checkConnectionStatus() >= 0;
	}

	// uint32_t UsbInterface::getFirmwareVersion()
	// {
	// 	CtrlFrame req(PROP_RELEASE_VERSION);
	// 	transferFrame(req);
	// 	return req.get32(0);
	// }

	// uint32_t UsbInterface::getBuildDate()
	// {
	// 	CtrlFrame req(PROP_BUILD_DATE);
	// 	transferFrame(req);
	// 	return req.get32(0);
	// }

	// uint32_t UsbInterface::getDeviceCount()
	// {
	// 	CtrlFrame req(PROP_DEVICES);
	// 	transferFrame(req);
	// 	return req.get32(0);
	// }

	const std::vector<CameraDescription> UsbInterface::findUsbCamera(const uint16_t vendor_id, const uint16_t product_id)
	{
		return findUsbCameras({{vendor_id, product_id}});
	}

	const std::vector<CameraDescription> UsbInterface::findUsbCameras(const std::vector<std::pair<uint16_t, uint16_t>> &vid_pid_list)
	{
		std::vector<CameraDescription> descs;
		if (vid_pid_list.empty())
		{
			return descs;
		}

		libusb_device **devices;
		std::shared_ptr<LibUsbContext> ctx = std::make_shared<LibUsbContext>();
		ssize_t device_count = libusb_get_device_list(ctx->ctx(), &devices);
		if (device_count < 0)
		{
			LOG_ERROR("Failed to get USB device list");
			return descs;
		}

		for (ssize_t i = 0; i < device_count; i++)
		{
			struct libusb_device_descriptor desc;
			int r = libusb_get_device_descriptor(devices[i], &desc);
			if (r < 0)
			{
				// TODO: log error
				LOG_ERROR("Failed to get USB device descriptor");
				continue;
			}

			std::shared_ptr<UsbDevice> usb_handle;
			bool matched = false;
			for (const auto &vid_pid : vid_pid_list)
			{
				if (desc.idVendor == vid_pid.first && desc.idProduct == vid_pid.second)
				{
					matched = true;
					break;
				}
			}

			if (!matched)
			{
				continue;
			}

			usb_handle = std::make_shared<UsbDevice>(ctx, devices[i], desc);

			// Open device to check availability and read descriptor/serial.
			if (usb_handle->open() != LIBUSB_SUCCESS)
			{
				// Device is busy or cannot be opened
				usb_handle.reset();
				continue;
			}

			// Initialize endpoint info so that bulk transfers (used by getSerial) work.
			try
			{
				usb_handle->initInterface();
			}
			catch (const std::exception &e)
			{
				LOG_WARN("findUsbCamera: initInterface failed: %s", e.what());
				usb_handle->close();
				usb_handle.reset();
				continue;
			}

			std::string serial = UsbInterface::getSerial(usb_handle, desc.idVendor, desc.idProduct);
			if (serial == "")
			{
				descs.clear();
				usb_handle->close();
				usb_handle.reset();
				return descs;
			}
			std::string fw_version;
			// Read string descriptors before creating temp_iface: the UsbInterface
			// destructor calls close(), which nulls the underlying libusb handle and
			// would cause "USB not open" errors on subsequent getStringDescriptor calls.
			std::string product_str      = usb_handle->getStringDescriptor(desc.iProduct);
			std::string manufacturer_str = usb_handle->getStringDescriptor(desc.iManufacturer);
			if ((desc.idVendor == PropheseeRdk3Camera::vendor_id && desc.idProduct == PropheseeRdk3Camera::product_id) ||
			    (desc.idVendor == FluxeemApexVisionS1Camera::vendor_id && desc.idProduct == FluxeemApexVisionS1Camera::product_id))
			{
				UsbInterface temp_iface(usb_handle);
				fw_version = temp_iface.fwGetVersion();
			}
			
			CameraDescription retrun_desc{
				serial,
				product_str,
				manufacturer_str,
				desc.idVendor,
				desc.idProduct,
				InterfaceType::USB,
				fw_version};
			descs.emplace_back(retrun_desc);
			usb_handle->close();
			usb_handle.reset();
		}

		libusb_free_device_list(devices, 1);

		return descs;
	}

	const std::shared_ptr<UsbInterface> UsbInterface::createUsbInterface(CameraDescription camera_desc)
	{
		libusb_device **devices;
		std::shared_ptr<LibUsbContext> ctx = std::make_shared<LibUsbContext>();
		ssize_t device_count = libusb_get_device_list(ctx->ctx(), &devices);
		if (device_count < 0)
		{
			LOG_ERROR("Failed to get USB device list");
			return nullptr;
		}

		std::shared_ptr<UsbDevice> usb_handle;
		for (ssize_t i = 0; i < device_count; i++)
		{
			struct libusb_device_descriptor desc;
			int r = libusb_get_device_descriptor(devices[i], &desc);
			if (r < 0)
			{
				continue;
			}

			if (desc.idVendor != camera_desc.vid || desc.idProduct != camera_desc.pid)
			{
				continue;
			}

			usb_handle = std::make_shared<UsbDevice>(ctx, devices[i], desc);

			// Open device to read descriptor/serial and verify match.
			if (usb_handle->open() != LIBUSB_SUCCESS)
			{
				usb_handle.reset();
				continue;
			}

			// Initialize endpoint info so that bulk transfers (used by getSerial) work.
			try
			{
				usb_handle->initInterface();
			}
			catch (const std::exception &e)
			{
				LOG_WARN("createUsbInterface: initInterface failed: %s", e.what());
				usb_handle->close();
				usb_handle.reset();
				continue;
			}

			std::string serial = UsbInterface::getSerial(usb_handle, camera_desc.vid, camera_desc.pid);
			if (serial == camera_desc.serial)
			{
				// Build a CameraDescription with the info we gathered while the handle is still open.
				CameraDescription found_desc{
					serial,
					usb_handle->getStringDescriptor(desc.iProduct),
					usb_handle->getStringDescriptor(desc.iManufacturer),
					camera_desc.vid,
					camera_desc.pid,
					InterfaceType::USB};

				// Close the probing session so that the caller can re-open via UsbInterface::open().
				usb_handle->close();
				libusb_free_device_list(devices, 1);

				auto iface = std::make_shared<UsbInterface>(usb_handle);
				iface->camera_description_ = found_desc;
				return iface;
			}

			usb_handle->close();
			usb_handle.reset();
		}

		libusb_free_device_list(devices, 1);
		return nullptr;
	}

	const std::shared_ptr<UsbInterface> UsbInterface::createUsbFusionInterface(CameraDescription camera_desc)
	{
		libusb_device **devices;
		std::shared_ptr<LibUsbContext> ctx = std::make_shared<LibUsbContext>();
		ssize_t deviceCnt = libusb_get_device_list(ctx->ctx(), &devices);
		if (deviceCnt < 0)
		{
			LOG_ERROR("Failed to get USB device list");
			return nullptr;
		}

		std::shared_ptr<UsbDevice> usb_handle;
		for (ssize_t i = 0; i < deviceCnt; i++)
		{
			struct libusb_device_descriptor desc;
			int r = libusb_get_device_descriptor(devices[i], &desc);

			if (r < 0)
			{
				continue;
			}
			if (desc.idVendor != camera_desc.vid || desc.idProduct != camera_desc.pid)
			{
				continue;
			}

			usb_handle = std::make_shared<UsbDevice>(ctx, devices[i], desc);
			if (usb_handle->open() != LIBUSB_SUCCESS)
			{
				usb_handle.reset();
				continue;
			}

			try
			{
				usb_handle->initInterface();
			}
			catch (const std::exception &e)
			{
				LOG_WARN("createUsbFusionInterface: initInterface failed: %s", e.what());
				usb_handle->close();
				usb_handle.reset();
				continue;
			}

			std::string serial = UsbInterface::getSerial(usb_handle, camera_desc.vid, camera_desc.pid);
			if (serial == camera_desc.serial)
			{
				CameraDescription found_desc{
					serial,
					usb_handle->getStringDescriptor(desc.iProduct),
					usb_handle->getStringDescriptor(desc.iManufacturer),
					camera_desc.vid,
					camera_desc.pid,
					InterfaceType::USB};

				usb_handle->close();
				libusb_free_device_list(devices, 1);

				auto iface = std::make_shared<UsbInterface>(usb_handle);
				iface->camera_description_ = found_desc;
				return iface;
			}

			usb_handle->close();
			usb_handle.reset();
		}

		libusb_free_device_list(devices, 1);
		return nullptr;
	}

	bool UsbInterface::isBusy(std::shared_ptr<UsbDevice> usb_handle)
	{
		int r = usb_handle->claimInterface(0);
		if (r != LIBUSB_SUCCESS)
		{
			LOG_INFO("Camera interface is busy");
			return true;
		}
		usb_handle->releaseInterface(0);
		return false;
	}

	libusb_transfer *UsbInterface::createAsyncBulkTransfer()
	{
		libusb_transfer *transfer = libusb_alloc_transfer(0);
		if (!transfer)
		{
			LOG_ERROR("libusb_alloc_transfer Failed");
			return transfer;
		}
		return transfer;
	}

	std::string UsbInterface::getSerial(std::shared_ptr<UsbDevice> usb_handle, const uint16_t vid, const uint16_t pid)
	{
		(void)vid;
		(void)pid;

		// Read serial via bulk CtrlFrame(PROPERTY).
		// IMPORTANT: we must NOT create a temporary UsbInterface here because its
		// destructor would close the shared usb_handle, breaking the caller's
		// subsequent operations.  Instead, do the bulk transfer directly.
		if (!usb_handle || !usb_handle->isOpen())
		{
			return "";
		}

		CtrlFrame req(PROP_SERIAL_NUMBER);
		int sent = 0;
		std::vector<uint8_t> answer(USB_MAX_ANSWER_SIZE);

		// send
		int r = usb_handle->bulkTransfer(usb_handle->getInfo().endpoint_control_out,
										 req.frame(), req.frameSize(), &sent, 1000);
		if (r != LIBUSB_SUCCESS)
		{
			LOG_ERROR("getSerial: bulk send failed: %s", libusb_error_name(r));
			return "";
		}

		// recv
		sent = 0;
		r = usb_handle->bulkTransfer(usb_handle->getInfo().endpoint_control_in,
									 answer.data(), answer.size(), &sent, 10000);
		if (r != LIBUSB_SUCCESS || sent <= 0)
		{
			LOG_ERROR("getSerial: bulk recv failed: %s (sent=%d)", libusb_error_name(r), sent);
			return "";
		}

		answer.resize(static_cast<size_t>(sent));
		if (req.swapReqAndAnswer(answer))
		{
			const auto payload_size = req.payloadSize();
			if (payload_size > sizeof(uint64_t))
			{
				const auto *payload = req.payload();
				const bool printable = std::all_of(payload, payload + payload_size, [](uint8_t value) {
					return value >= 0x20 && value <= 0x7E;
				});
				if (printable)
				{
					return std::string(reinterpret_cast<const char *>(payload), payload_size);
				}
			}

			std::ostringstream ostr;
			ostr << std::internal << std::setfill('0') << std::setw(8) << std::hex << req.get64(0);
			ostr << std::dec;
			return ostr.str();
		}
		LOG_ERROR("Get serial number failed.");
		return "";
	}

	bool UsbInterface::readRegisterWithControlTransfer(uint32_t address, uint32_t &val)
	{
		uint8_t bm_request_type = LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE;
		uint8_t custom_request_code = CMD_REQUEST_READ_REGISTER;
		uint16_t index = 0;
		const int r = usb_handle_->controlTransfer(bm_request_type, custom_request_code, static_cast<uint16_t>(address & 0xFFFF),
										  index, reinterpret_cast<unsigned char *>(&val), sizeof(val), 1000);
		return r >= 0;
	}

	bool UsbInterface::readRegisterWithControlTransfer(uint32_t address, uint8_t *data, uint16_t read_length)
	{
		uint8_t bm_request_type = LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE;
		uint8_t custom_request_code = CMD_REQUEST_READ_REGISTER;
		uint16_t index = 0;
		const int r = usb_handle_->controlTransfer(bm_request_type, custom_request_code, static_cast<uint16_t>(address & 0xFFFF),
										  index, reinterpret_cast<unsigned char *>(data), read_length, 1000);
		return r >= 0;
	}

	bool UsbInterface::writeRegisterWithControlTransfer(uint32_t address, uint8_t *data, uint16_t write_length)
	{
		uint8_t bm_request_type = LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE;
		uint8_t custom_request_code = CMD_REQUEST_WRITE_REGISTER;
		uint16_t index = 0;
		const int r = usb_handle_->controlTransfer(bm_request_type, custom_request_code, static_cast<uint16_t>(address & 0xFFFF),
										  index, reinterpret_cast<unsigned char *>(data), write_length, 1000);
		return r >= 0;
	}

	bool UsbInterface::writeRegisterWithControlTransfer(uint32_t address, uint32_t val)
	{
		uint8_t bm_request_type = LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE;
		uint8_t custom_request_code = CMD_REQUEST_WRITE_REGISTER;
		uint16_t index = 0;
		const int r = usb_handle_->controlTransfer(bm_request_type, custom_request_code, static_cast<uint16_t>(address & 0xFFFF),
										  index, reinterpret_cast<unsigned char *>(&val), sizeof(val), 1000);
		return r >= 0;
	}

	bool UsbInterface::readRegister(uint32_t address, uint32_t &val)
	{
		CtrlFrame req(PROP_DEVICE_REG32);
		const int nval = 1;
		req.pushBack32(0); // 未使用：设备 ID 在运行时由 openeb 定义，非固件属性
		req.pushBack32(address);
		req.pushBack32(nval);

		// TODO[2]: try and catch here
		transferFrame(req);

		// 完整性校验
		if (req.get32(1) != address)
		{
			LOG_ERROR("Address mismatch");
			return false;
		}
		if (req.payloadSize() < ((nval + 2) * sizeof(uint32_t)))
		{
			LOG_ERROR("Not enough data in answer");
			return false;
		}
		val = req.get32(2);
		return true;
	}

	bool UsbInterface::readRegister(uint32_t address, uint32_t *val, int nval)
	{
		CtrlFrame req({PROP_DEVICE_REG32});

		req.pushBack32(0);
		req.pushBack32(address);
		req.pushBack32(nval);

		try
		{
			transferFrame(req);
		}
		catch (std::system_error &e)
		{
			LOG_ERROR("Error in control transfer");
			return false;
		}
		if (req.get32(0) != 0)
		{
			LOG_ERROR("Device mismatch");
			return false;
		}
		if (req.get32(1) != address)
		{
			LOG_ERROR("Address mismatch");
			return false;
		}
		if (req.payloadSize() < ((nval + 2) * sizeof(uint32_t)))
		{
			LOG_ERROR("Not enough data in answer");
			return false;
		}

		memcpy(val, req.payload() + (2 * sizeof(uint32_t)), nval * sizeof(uint32_t));

		return true;
	}

	bool UsbInterface::writeRegister(uint32_t address, uint32_t value)
	{
		CtrlFrame req(PROP_DEVICE_REG32 | USB_WRITE_FLAG);

		req.pushBack32(0); // 未使用：设备 ID 在运行时由 openeb 定义，非固件属性
		req.pushBack32(address);
		req.pushBack32(value);

		transferFrame(req);

		if (req.get32(1) != address)
		{
			return false;
		}

		return true;
	}

	bool UsbInterface::transferFrame(CtrlFrame &frame)
	{
		int sent = 0;
		std::vector<uint8_t> answer(USB_MAX_ANSWER_SIZE);
		uint16_t retry_count = 0;
		bool result = false;

		std::lock_guard<std::mutex> guard(control_mutex_);

		/* 发送指令 */
		int r = usb_handle_->bulkTransfer(usb_handle_->getInfo().endpoint_control_out, frame.frame(), frame.frameSize(), &sent, 1000);
		if (r != LIBUSB_SUCCESS)
		{
			LOG_ERROR("transferFrame: bulk send failed: %s", libusb_error_name(r));
			return false;
		}

		/* 读取响应 */
		sent = 0;
		r = usb_handle_->bulkTransfer(usb_handle_->getInfo().endpoint_control_in, answer.data(), answer.size(), &sent, 10000);
		if (r != LIBUSB_SUCCESS || sent <= 0)
		{
			LOG_ERROR("transferFrame: bulk recv failed: %s (sent=%d)", libusb_error_name(r), sent);
			return false;
		}

		answer.resize(static_cast<size_t>(sent));
		result = frame.swapReqAndAnswer(answer);
		return result;
	}

	std::vector<uint8_t> UsbInterface::getEpCommAddress()
	{
		return usb_handle_->getInfo().endpoint_comm_address;
	}

	void UsbInterface::flushEndpointBuffer(uint8_t ep)
	{
		long total_flush = 0;
		constexpr int flush_max_data = static_cast<int>(3840 * 2160 * 1.5); // ~12MB

		LOG_DEBUG("Data Transfer: Try to flush");

		try
		{
			int bytes_cnt;
			int result;
			do
			{
				uint8_t buf[16 << 10];
				result = usb_handle_->bulkTransfer(ep,
												   buf, 16 << 10, &bytes_cnt, 1);
				if (result == 0 && bytes_cnt > 0)
				{
					LOG_DEBUG("Flushed %d bytes from endpoint: %02x\n", bytes_cnt, ep);
				}
				total_flush += bytes_cnt;
				if (total_flush >= flush_max_data)
				{
					break;
				}
			} while (result == 0 || result == LIBUSB_ERROR_OVERFLOW);
		}
		catch (const std::system_error &e)
		{
		}

		LOG_DEBUG("Total of {%d} bytes flushed", total_flush);
	}

	void UsbInterface::fillDataBulkTransfer(libusb_transfer *transfer, uint8_t *buf, int packet_size,
											libusb_transfer_cb_fn async_bulk_cb, void *user_data, uint32_t timeout)
	{
		usb_handle_->fillBulkTransfer(transfer, buf, packet_size,
									  async_bulk_cb, user_data, timeout);
		transfer->flags &= ~LIBUSB_TRANSFER_FREE_BUFFER;
		transfer->flags &= ~LIBUSB_TRANSFER_FREE_TRANSFER;
	}

	int UsbInterface::submitTransfer(libusb_transfer *transfer)
	{
		return usb_handle_->submitTransfer(transfer);
	}

	void UsbInterface::handleSubmittedTransferTimeout(timeval *tv)
	{
		usb_handle_->handleSubmittedTransferTimeout(tv);
	}

	bool UsbInterface::writeCalibrationFile(std::string file_path)
	{
		LOG_INFO("Write calibration file: %s", file_path.c_str());
		std::ifstream file(file_path, std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			LOG_ERROR("Can't open file: %s", file_path.c_str());
			return false;
		}

		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<uint8_t> buffer(size);
		if (!file.read(reinterpret_cast<char *>(buffer.data()), size))
		{
			file.close();
			return false;
		}
		file.close();

		uint8_t bmRequestType = LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE;
		uint8_t custom_request_code = CMD_REQUEST_FILE_WRITE;
		uint16_t wValue = 0;
		uint16_t wIndex = CMD_INDEX_CALIBRATION;

		const int r = usb_handle_->controlTransfer(
			bmRequestType,
			custom_request_code,
			wValue,
			wIndex,
			buffer.data(),
			buffer.size(),
			1000);
		return r >= 0;
	}

	bool UsbInterface::readCalibrationData(std::vector<uint8_t> &buffer)
	{
		uint8_t bm_request_type = LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE;
		uint8_t custom_request_code = CMD_REQUEST_FILE_READ;
		uint16_t w_value = 0;
		uint16_t w_index = CMD_INDEX_CALIBRATION;
		buffer.clear();
		uint8_t chunk_data[4 * 1024] = {0};

		const int read_data_size = usb_handle_->controlTransfer(
			bm_request_type,
			custom_request_code,
			w_value,
			w_index,
			chunk_data,
			4 * 1024,
			1000);

		if (read_data_size <= 0)
		{
			return false;
		}
		buffer.insert(buffer.end(), chunk_data, chunk_data + read_data_size);
		return !buffer.empty();
	}

	libusb_context *UsbInterface::ctx()
	{
		return usb_handle_->ctx();
	}

	int UsbInterface::clearHalt(uint8_t endpoint)
	{
		return usb_handle_->clearHalt(endpoint);
	}

	// ==================== UsbInterface Async API ====================

	std::unique_ptr<AsyncBulkTransferHandle> UsbInterface::createAsyncBulkHandle()
	{
		return std::make_unique<AsyncBulkTransferHandle>();
	}

	void UsbInterface::handleEvents()
	{
		struct timeval tv = {0, 0};
		libusb_handle_events_timeout(usb_handle_->ctx(), &tv);
	}

	// ============ OTA Firmware Upgrade ============

	static constexpr uint8_t  FW_CMD_ID_CHECK  = 0xB0;
	static constexpr uint8_t  FW_CMD_SPI_WRITE = 0xC2;
	static constexpr uint8_t  FW_CMD_SPI_READ  = 0xC3;
	static constexpr uint8_t  FW_CMD_SPI_ERASE = 0xC4;
	static constexpr uint8_t  FW_CMD_RESET     = 0xE0;
	static constexpr uint8_t  FW_VENDOR_OUT    = 0x40;
	static constexpr uint8_t  FW_VENDOR_IN     = 0xC0;
	static constexpr uint16_t FW_SPI_PAGE_SIZE = 256;
	static constexpr uint32_t FW_SPI_SECTOR_SZ = 0x10000; // 64 KB
	static constexpr uint16_t FW_EP0_MAX_DATA  = 4096;

	bool UsbInterface::fwCheckIdentity()
	{
		std::lock_guard<std::mutex> lock(control_mutex_);
		return fwCheckIdentityLocked();
	}

	std::string UsbInterface::fwGetVersion()
	{
		// No pre-lock here: transferFrame() acquires control_mutex_ internally
		return fwGetVersionLocked();
	}

	bool UsbInterface::fwCheckIdentityLocked()
	{
		uint8_t buf[8] = {};
		int r = usb_handle_->controlTransfer(FW_VENDOR_IN, FW_CMD_ID_CHECK, 0, 0, buf, 8, 5000);
		if (r < 0)
		{
			fw_last_error_ = "ID check control transfer failed";
			LOG_ERROR("fwCheckIdentity: %s", fw_last_error_.c_str());
			return false;
		}
		std::string id(reinterpret_cast<char *>(buf),
					   strnlen(reinterpret_cast<char *>(buf), 8));
		if (id != "PSEEUPD")
		{
			fw_last_error_ = "Unexpected device identity: " + id;
			LOG_ERROR("fwCheckIdentity: %s", fw_last_error_.c_str());
			return false;
		}
		return true;
	}

	std::string UsbInterface::fwGetVersionLocked()
	{
		// PROP_RELEASE_VERSION (0x79) is a treuzell bulk property.
		// transferFrame() acquires control_mutex_ internally.
		CtrlFrame req(PROP_RELEASE_VERSION);
		if (!transferFrame(req))
		{
			fw_last_error_ = "Get app version transfer failed";
			return "unknown";
		}
		// Firmware response payload: uint32 little-endian = 0x00_MAJOR_MINOR_MICRO
		uint32_t val = req.get32(0);
		uint8_t major = static_cast<uint8_t>((val >> 16) & 0xFF);
		uint8_t minor = static_cast<uint8_t>((val >> 8) & 0xFF);
		uint8_t micro = static_cast<uint8_t>(val & 0xFF);
		return std::to_string(major) + "." + std::to_string(minor) + "." +
			   std::to_string(micro);
	}

	bool UsbInterface::fwValidateImage(const std::vector<uint8_t> &data)
	{
		if (data.size() < 16)
		{
			fw_last_error_ = "Image too small";
			return false;
		}
		if (data[0] != 0x43 || data[1] != 0x59) // "CY"
		{
			fw_last_error_ = "Bad magic (expected 'CY')";
			return false;
		}
		if (data[3] != 0xB0)
		{
			fw_last_error_ = "Unexpected image type (expected 0xB0)";
			return false;
		}
		return true;
	}

	bool UsbInterface::fwPollWipDone(unsigned int timeout_ms)
	{
		// NOTE: caller must already hold control_mutex_
		auto start = std::chrono::steady_clock::now();
		for (;;)
		{
			uint8_t wip = 0xFF;
			int r = usb_handle_->controlTransfer(FW_VENDOR_IN, FW_CMD_SPI_ERASE, 0, 0, &wip, 1, 5000);
			if (r < 0)
			{
				fw_last_error_ = "WIP poll transfer failed";
				return false;
			}
			if (!(wip & 0x01))
				return true;
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
							   std::chrono::steady_clock::now() - start)
							   .count();
			if (static_cast<unsigned int>(elapsed) >= timeout_ms)
			{
				fw_last_error_ = "WIP poll timeout";
				return false;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}

	bool UsbInterface::fwEraseSectors(uint32_t num_sectors, FwUpgradeProgressCb cb)
	{
		// NOTE: caller must already hold control_mutex_
		for (uint32_t s = 0; s < num_sectors; ++s)
		{
			if (cb)
				cb("erase", s, num_sectors);
			int r = usb_handle_->controlTransfer(FW_VENDOR_OUT, FW_CMD_SPI_ERASE, 1,
												  static_cast<uint16_t>(s), nullptr, 0, 5000);
			if (r < 0)
			{
				fw_last_error_ = "Erase command failed for sector " + std::to_string(s);
				LOG_ERROR("fwEraseSectors: %s", fw_last_error_.c_str());
				return false;
			}
			if (!fwPollWipDone())
			{
				LOG_ERROR("fwEraseSectors: %s (sector %u)", fw_last_error_.c_str(), s);
				return false;
			}
		}
		if (cb)
			cb("erase", num_sectors, num_sectors);
		return true;
	}

	bool UsbInterface::fwWriteFlash(const std::vector<uint8_t> &data, FwUpgradeProgressCb cb)
	{
		// NOTE: caller must already hold control_mutex_
		const uint32_t fw_size = static_cast<uint32_t>(data.size());
		uint32_t offset = 0;
		while (offset < fw_size)
		{
			uint16_t chunk_len = static_cast<uint16_t>(
				std::min<uint32_t>(FW_EP0_MAX_DATA, fw_size - offset));
			uint16_t page_addr = static_cast<uint16_t>(offset / FW_SPI_PAGE_SIZE);

			std::vector<uint8_t> chunk(data.begin() + offset,
									   data.begin() + offset + chunk_len);
			uint16_t rem = chunk.size() % FW_SPI_PAGE_SIZE;
			if (rem)
				chunk.resize(chunk.size() + (FW_SPI_PAGE_SIZE - rem), 0xFF);

			int r = usb_handle_->controlTransfer(FW_VENDOR_OUT, FW_CMD_SPI_WRITE, 0, page_addr,
												  chunk.data(), static_cast<uint16_t>(chunk.size()), 15000);
			if (r < 0)
			{
				fw_last_error_ = "Write failed at offset 0x" + std::to_string(offset);
				LOG_ERROR("fwWriteFlash: %s", fw_last_error_.c_str());
				return false;
			}
			offset += chunk_len;
			if (cb)
				cb("write", offset, fw_size);
		}
		return true;
	}

	bool UsbInterface::fwVerifyFlash(const std::vector<uint8_t> &data, FwUpgradeProgressCb cb)
	{
		// NOTE: caller must already hold control_mutex_
		const uint32_t fw_size = static_cast<uint32_t>(data.size());
		uint32_t offset = 0;
		while (offset < fw_size)
		{
			uint16_t chunk_len = static_cast<uint16_t>(
				std::min<uint32_t>(FW_EP0_MAX_DATA, fw_size - offset));
			uint16_t page_addr = static_cast<uint16_t>(offset / FW_SPI_PAGE_SIZE);

			uint16_t read_len = chunk_len;
			uint16_t rem = read_len % FW_SPI_PAGE_SIZE;
			if (rem)
				read_len += FW_SPI_PAGE_SIZE - rem;

			std::vector<uint8_t> rbuf(read_len, 0);
			int r = usb_handle_->controlTransfer(FW_VENDOR_IN, FW_CMD_SPI_READ, 0, page_addr,
												  rbuf.data(), read_len, 15000);
			if (r < 0)
			{
				fw_last_error_ = "Verify read failed at offset 0x" + std::to_string(offset);
				LOG_ERROR("fwVerifyFlash: %s", fw_last_error_.c_str());
				return false;
			}
			for (uint16_t i = 0; i < chunk_len; ++i)
			{
				if (rbuf[i] != data[offset + i])
				{
					fw_last_error_ = "Verify FAILED at byte 0x" + std::to_string(offset + i);
					LOG_ERROR("fwVerifyFlash: %s", fw_last_error_.c_str());
					return false;
				}
			}
			offset += chunk_len;
			if (cb)
				cb("verify", offset, fw_size);
		}
		return true;
	}

	bool UsbInterface::fwUpgrade(const std::string &image_path, bool verify, bool reset,
								  FwUpgradeProgressCb cb)
	{
		std::ifstream file(image_path, std::ios::binary | std::ios::ate);
		if (!file)
		{
			fw_last_error_ = "Cannot open file: " + image_path;
			LOG_ERROR("fwUpgrade: %s", fw_last_error_.c_str());
			return false;
		}
		auto size = file.tellg();
		file.seekg(0);
		std::vector<uint8_t> data(static_cast<size_t>(size));
		file.read(reinterpret_cast<char *>(data.data()), size);
		if (!file)
		{
			fw_last_error_ = "Read error: " + image_path;
			LOG_ERROR("fwUpgrade: %s", fw_last_error_.c_str());
			return false;
		}
		return fwUpgrade(data, verify, reset, cb);
	}

	bool UsbInterface::fwUpgrade(const std::vector<uint8_t> &image_data, bool verify, bool reset,
								  FwUpgradeProgressCb cb)
	{
		if (!fwValidateImage(image_data))
		{
			LOG_ERROR("fwUpgrade: image validation failed: %s", fw_last_error_.c_str());
			return false;
		}

		const uint32_t fw_size = static_cast<uint32_t>(image_data.size());
		const uint32_t num_sectors = (fw_size + FW_SPI_SECTOR_SZ - 1) / FW_SPI_SECTOR_SZ;
		LOG_INFO("fwUpgrade: %u bytes, %u sectors", fw_size, num_sectors);

		// Hold the lock for the entire erase-write-verify sequence
		std::lock_guard<std::mutex> lock(control_mutex_);

		if (!fwCheckIdentityLocked())
			return false;

		if (!fwEraseSectors(num_sectors, cb))
			return false;
		if (!fwWriteFlash(image_data, cb))
			return false;
		if (verify && !fwVerifyFlash(image_data, cb))
			return false;
		if (reset)
		{
			LOG_INFO("fwUpgrade: resetting device...");
			usb_handle_->controlTransfer(FW_VENDOR_OUT, FW_CMD_RESET, 0, 0, nullptr, 0, 1000);
			// The device disconnects during reboot. Drop the stale libusb handle now
			// so callers do not keep issuing transfers against the pre-reset session.
			usb_handle_->close();
		}
		LOG_INFO("fwUpgrade: complete");
		return true;
	}

	void UsbInterface::fwReset()
	{
		std::lock_guard<std::mutex> lock(control_mutex_);
		usb_handle_->controlTransfer(FW_VENDOR_OUT, FW_CMD_RESET, 0, 0, nullptr, 0, 1000);
	}

} // namespace fluxeem
