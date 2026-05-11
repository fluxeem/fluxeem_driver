#include <fluxeem/hal/common/usb_device.h>
#include <fluxeem/base/logging/logger.h>

namespace fluxeem
{
    LibUsbContext::LibUsbContext()
    {
        int ret = libusb_init(&ctx_);
        if (ret)
        {
            throw std::runtime_error("libusb_init failed");
        }
    }

    LibUsbContext::~LibUsbContext()
    {
        libusb_exit(ctx_);
    }

    libusb_context *LibUsbContext::ctx()
    {
        return ctx_;
    }

    UsbDevice::UsbDevice(
        std::shared_ptr<LibUsbContext> ctx,
        libusb_device *usb_device,
        libusb_device_descriptor usb_desc)
        : ctx_(std::move(ctx)), usb_desc_(usb_desc), usb_device_(usb_device)
    {
        // Device pointers returned by libusb_get_device_list must be ref'd to keep them valid after free_device_list.
        if (usb_device_)
        {
            libusb_ref_device(usb_device_);
        }
    }

    int UsbDevice::open()
    {
        if (isOpen())
        {
            return LIBUSB_SUCCESS;
        }

        int ret = libusb_open(usb_device_, &usb_handle_);
        if (ret != LIBUSB_SUCCESS)
        {
            LOG_ERROR("Error opening device. Error code: %s.", libusb_error_name(ret));
            usb_handle_ = nullptr;
            return ret;
        }

        // Claim interface 0 (consistent with existing call logic).
        ret = libusb_claim_interface(usb_handle_, 0);
        if (ret != LIBUSB_SUCCESS)
        {
            LOG_ERROR("libusb_claim_interface failed: %s", libusb_error_name(ret));
            close();
            return ret;
        }

        ret = libusb_get_config_descriptor(usb_device_, 0, &usb_config_);
        if (ret != LIBUSB_SUCCESS)
        {
            LOG_ERROR("Error getting config descriptor. Error code: %s.", libusb_error_name(ret));
            close();
            return ret;
        }

        if (checkConnectionStatus() < 0)
        {
            LOG_INFO("Check connection status failed.");
        }

        return LIBUSB_SUCCESS;
    }
    UsbDevice::~UsbDevice() noexcept
    {
        close();
        if (usb_device_)
        {
            libusb_unref_device(usb_device_);
            usb_device_ = nullptr;
        }
    }

    void UsbDevice::close() noexcept
    {
        if (usb_handle_)
        {
            // Best-effort: release all claimed interfaces (ignore errors).
            (void)libusb_release_interface(usb_handle_, 0);
            if (usb_device_info_.interface_id != 0)
            {
                (void)libusb_release_interface(usb_handle_, usb_device_info_.interface_id);
            }
            libusb_close(usb_handle_);
            usb_handle_ = nullptr;
        }
        if (usb_config_)
        {
            libusb_free_config_descriptor(usb_config_);
            usb_config_ = nullptr;
        }
        // Reset endpoint info so a subsequent open()+initInterface() starts clean.
        usb_device_info_ = UsbDeviceInfo{};
    }

    bool UsbDevice::isOpen() const noexcept
    {
        return usb_handle_ != nullptr;
    }

    libusb_device_handle *UsbDevice::handle() const noexcept
    {
        return usb_handle_;
    }

    libusb_device_descriptor UsbDevice::getDescription() const
    {
        return usb_desc_;
    }

    const UsbDeviceInfo &UsbDevice::getInfo() const noexcept
    {
        return usb_device_info_;
    }

    int UsbDevice::clearHalt(uint8_t endpoint)
    {
        return libusb_clear_halt(usb_handle_, endpoint);
    }

    int UsbDevice::checkConnectionStatus()
    {
        uint8_t data[1];
        int result = libusb_control_transfer(usb_handle_,
                                             LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_ENDPOINT_IN,
                                             LIBUSB_REQUEST_GET_DESCRIPTOR,
                                             (LIBUSB_DT_DEVICE << 8),
                                             0,
                                             data,
                                             sizeof(data),
                                             1000);
        return result;
    }

    void UsbDevice::initInterface()
    {
        uint8_t interface_id = -1;
        for (int i = 0; i < usb_config_->bNumInterfaces; i++)
        {
            const libusb_interface *interface = &usb_config_->interface[i];
            // check if the interface is the one we want
            for (int index = 0; index < interface->num_altsetting; index++)
            {
                const libusb_interface_descriptor *alt_setting = &interface->altsetting[index];
                if (alt_setting->bInterfaceClass == LIBUSB_CLASS_VENDOR_SPEC &&
                    alt_setting->bInterfaceSubClass == SUBCLASS_TYPE &&
                    ((alt_setting->endpoint[0]).bmAttributes == LIBUSB_TRANSFER_TYPE_BULK) &&
                    (((alt_setting->endpoint[0]).bEndpointAddress & 0x80) == LIBUSB_ENDPOINT_IN) &&
                    ((alt_setting->endpoint[1]).bmAttributes == LIBUSB_TRANSFER_TYPE_BULK) &&
                    (((alt_setting->endpoint[1]).bEndpointAddress & 0x80) == LIBUSB_ENDPOINT_OUT) &&
                    ((alt_setting->endpoint[2]).bmAttributes == LIBUSB_TRANSFER_TYPE_BULK) &&
                    (((alt_setting->endpoint[2]).bEndpointAddress & 0x80) == LIBUSB_ENDPOINT_IN))
                {
                    interface_id = alt_setting->bInterfaceNumber;
                    usb_device_info_.interface_id = interface_id;
                    usb_device_info_.endpoint_control_in = alt_setting->endpoint[0].bEndpointAddress;
                    usb_device_info_.endpoint_control_out = alt_setting->endpoint[1].bEndpointAddress;
                    usb_device_info_.endpoint_comm_address.push_back(alt_setting->endpoint[2].bEndpointAddress);
                    if (interface->altsetting->bNumEndpoints == 4)
                    {
                        usb_device_info_.endpoint_comm_address.push_back(alt_setting->endpoint[3].bEndpointAddress);
                    }
                    break;
                }
            }
        }
        if (kernelDriverActive(usb_device_info_.interface_id) == 1)
        { // find out if kernel driver is attached
            LOG_FATAL("Kernel Driver Active on interface %d of %d", usb_device_info_.interface_id, usb_desc_.idProduct);
            if (detachKernelDriver(usb_device_info_.interface_id) == 0)
            { // detach it
                LOG_FATAL("Kernel Driver Detached from interface %d of %d", usb_device_info_.interface_id, usb_desc_.idProduct);
            }
        }
        // Only claim if this is a different interface than the one already claimed in open() (interface 0).
        if (usb_device_info_.interface_id != 0)
        {
            int ret = claimInterface(usb_device_info_.interface_id);
            if (ret < 0)
            {
                throw std::runtime_error("Error claiming interface");
            }
        }

        setInterfaceAlt(usb_device_info_.interface_id, 0);
    }

    int UsbDevice::setInterfaceAlt(int interface_number, int alternate_setting)
    {
        return libusb_set_interface_alt_setting(usb_handle_, interface_number, alternate_setting);
    }

    int UsbDevice::claimInterface(int interface_id)
    {
        if (!usb_handle_)
        {
            LOG_ERROR("USB not open.");
            return -1;
        }
        return libusb_claim_interface(usb_handle_, interface_id);
    }

    int UsbDevice::kernelDriverActive(int interface_id)
    {
        if (!usb_handle_)
        {
            LOG_ERROR("USB not open.");
            return -1;
        }
        return libusb_kernel_driver_active(usb_handle_, interface_id);
    }

    int UsbDevice::detachKernelDriver(int interface_id)
    {
        if (!usb_handle_)
        {
            LOG_ERROR("USB not open.");
            return -1;
        }
        return libusb_detach_kernel_driver(usb_handle_, interface_id);
    }

    int UsbDevice::releaseInterface(int interface_id)
    {
        if (!usb_handle_)
        {
            LOG_ERROR("USB not open.");
            return -1;
        }
        return libusb_release_interface(usb_handle_, interface_id);
    }

    uint16_t UsbDevice::controlReadRegister16bits(uint8_t usbvendorcmd, uint32_t address)
    {
        uint16_t val = -1;
        if (!usb_handle_)
        {
            LOG_ERROR("USB not open.");
            return val;
        }
        unsigned char data[4];
        int r = libusb_control_transfer(usb_handle_, //(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN)
                                        0xC0, usbvendorcmd, address, 0, data, 4, 0);
        if (r < 0)
        {
            LOG_ERROR("Error in control transfer");
            return val;
        }

        val = data[2];
        val |= static_cast<uint16_t>(data[3]) << 8;
        return val;
    }

    uint32_t UsbDevice::controlReadRegister32bits(uint8_t usbvendorcmd, uint32_t address, bool big_endian)
    {
        uint32_t val = -1;
        if (!usb_handle_)
        {
            LOG_ERROR("USB not open.");
            return val;
        }
        unsigned char data[8];
        try
        {
            controlTransfer(
                0xC0, usbvendorcmd, uint16_t(address & 0xFFFF), uint16_t((address >> 16) & 0xFFFF), data, 8, 0);
        }
        catch (const std::exception &e)
        {
            LOG_ERROR(e.what());
            return val;
        }

        if (big_endian)
        {
            val = data[7];
            val |= data[6] << 8;
            val |= data[5] << 16;
            val |= data[4] << 24;
        }
        else
        {
            val = data[0];
            val |= data[1] << 8;
            val |= data[2] << 16;
            val |= data[3] << 24;
        }
        return val;
    }

    std::string UsbDevice::getStringDescriptor(uint8_t desc_index)
    {   
        if (!usb_handle_)
        {
            LOG_ERROR("USB not open.");
            return "";
        }
        const int length = 256;
        unsigned char data[length];
        int r = libusb_get_string_descriptor_ascii(usb_handle_, desc_index, data, length);
        if (r < 0)
        {
            LOG_ERROR("Error in getStringDescriptor; %s", libusb_error_name(r));
            return "";
        }
        return std::string(reinterpret_cast<char *>(data));
    }

    int UsbDevice::controlTransfer(uint8_t request_type,
                                   uint8_t request,
                                   uint16_t w_value,
                                   uint16_t w_index,
                                   unsigned char *data,
                                   uint16_t length,
                                   unsigned int timeout)
    {
        int r = libusb_control_transfer(usb_handle_,
                                        request_type,
                                        request,
                                        w_value,
                                        w_index,
                                        data,
                                        length,
                                        timeout);
        if (r < 0)
        {
            LOG_ERROR("Error in control transfer: %s", libusb_error_name(r));
        }
        return r;
    }

    int UsbDevice::bulkTransfer(uint8_t endpoint, uint8_t *data,
                                int length, int *transferred,
                                uint32_t timeout)
    {
        return libusb_bulk_transfer(usb_handle_, endpoint, data,
                                    length, transferred,
                                    timeout);
    }

    void UsbDevice::fillBulkTransfer(libusb_transfer *transfer,
                                     uint8_t *buffer, int length,
                                     libusb_transfer_cb_fn async_bulk_cb,
                                     void *user_data, uint32_t timeout)
    {
        libusb_fill_bulk_transfer(transfer, usb_handle_, transfer->endpoint,
                                  buffer, length, async_bulk_cb, user_data, timeout);
    }

    int UsbDevice::submitTransfer(libusb_transfer *transfer)
    {
        return libusb_submit_transfer(transfer);
    }

    void UsbDevice::handleSubmittedTransferTimeout(timeval *tv)
    {
        libusb_handle_events_timeout(ctx_->ctx(), tv);
    }

    int UsbDevice::waitCompletedTransfer(int *completed)
    {
        // libusb_handle_events_completed 会根据 completed 进行短路；如果上层传入 nullptr，会一直处理事件。
        // 这里保持兼容：如果调用方提供 completed，就用它；否则用内部成员。
        int *completed_ptr = completed ? completed : &completion_;
        auto err = libusb_handle_events_completed(ctx_->ctx(), completed_ptr);
        if (err)
        {
            LOG_ERROR("Error in handle events completed: %s", libusb_error_name(err));
        }
        return err;
    }

} // namespace fluxeem
