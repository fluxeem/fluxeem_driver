#ifndef USB_HANDLE_H
#define USB_HANDLE_H

#include <vector>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <cstdint>

#ifdef _WIN32
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#define SUBCLASS_TYPE (0x19)

#define CMD_READ_VERSION_FX3 (0x70)
#define CMD_CHECK_FPGA_BOOT_STATE (0x71)
#define CMD_READ_SYSTEM_ID (0x72)

#define CMD_READ (0x50)  /*Read Transaction*/
#define CMD_WRITE (0x51) /*Write Transaction*/

namespace fluxeem
{
    class LibUsbContext
    {
    public:
        LibUsbContext();
        ~LibUsbContext();

        libusb_context *ctx();

    private:
        libusb_context *ctx_ = nullptr;
    };

    // Interface of USB device. Not the abstract interface of the camera
    struct UsbDeviceInfo
    {
        uint8_t interface_id = 0;
        uint8_t endpoint_control_in = 0;
        uint8_t endpoint_control_out = 0;
        std::vector<uint8_t> endpoint_comm_address;
    };

    class UsbDevice
    {
    public:
        UsbDevice(std::shared_ptr<LibUsbContext> ctx,
                  libusb_device *usb_dev,
                  libusb_device_descriptor usb_desc);

        ~UsbDevice() noexcept;

        UsbDevice(const UsbDevice &) = delete;
        UsbDevice &operator=(const UsbDevice &) = delete;
        UsbDevice(UsbDevice &&) = delete;
        UsbDevice &operator=(UsbDevice &&) = delete;

        // Open the device, claim interface 0, and cache the config descriptor.
        // Returns LIBUSB_SUCCESS(0) or a libusb error code.
        int open();

        // Release the claimed interface, free the config descriptor, and close the handle.
        void close() noexcept;

        bool isOpen() const noexcept;

        libusb_device_handle *handle() const noexcept;

        int clearHalt(uint8_t endpoint);

        libusb_device_descriptor getDescription() const;

        // Returns the result of libusb_control_transfer (< 0 indicates failure).
        int checkConnectionStatus();

        // Parse endpoint / interface info and perform claim / alt-setting.
        void initInterface();

        int kernelDriverActive(int interface_number);
        int detachKernelDriver(int interface_number);
        int claimInterface(int interface_number);
        int setInterfaceAlt(int interface_number, int alternate_setting);
        int releaseInterface(int interface_number);

        uint16_t controlReadRegister16bits(uint8_t usbvendorcmd, uint32_t address);
        uint32_t controlReadRegister32bits(uint8_t usbvendorcmd, uint32_t address, bool big_endian = true);

        std::string getStringDescriptor(uint8_t desc_index);

        int controlTransfer(uint8_t request_type, uint8_t request, uint16_t w_value, uint16_t w_index,
                            unsigned char *data, uint16_t length, unsigned int timeout);

        int bulkTransfer(uint8_t endpoint, uint8_t *data, int length, int *transferred, uint32_t timeout);

        void fillBulkTransfer(libusb_transfer *transfer, uint8_t *buffer, int length,
                              libusb_transfer_cb_fn async_bulk_cb, void *user_data, uint32_t timeout);

        int submitTransfer(libusb_transfer *transfer);

        void handleSubmittedTransferTimeout(timeval *tv);

        // Compatible with the old interface. Returns the error code from libusb_handle_events_completed.
        int waitCompletedTransfer(int *completed);

        const UsbDeviceInfo &getInfo() const noexcept;

        libusb_context *ctx()
        {
            return ctx_->ctx();
        }

    private:
        std::shared_ptr<LibUsbContext> ctx_;
        libusb_device_descriptor usb_desc_{};
        libusb_device_handle *usb_handle_ = nullptr;
        libusb_config_descriptor *usb_config_ = nullptr;
        libusb_device *usb_device_ = nullptr;
        int completion_ = 0;

        UsbDeviceInfo usb_device_info_{};
    };
} // namespace fluxeem

#endif // USB_HANDLE_H
