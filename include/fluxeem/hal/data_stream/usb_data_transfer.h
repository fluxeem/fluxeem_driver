// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef __USB_DATA_TRANSFER_HPP__
#define __USB_DATA_TRANSFER_HPP__

#include <fluxeem/hal/data_stream/data_transfer.hpp>
#include <fluxeem/hal/common/usb_interface.h>

#include <thread>
#include <vector>
#include <memory>

#define USB_DATATRANSFER_PACKET_SIZE (1024 * 1024)
#define USB_DATATRANSFER_TIMEOUT 500
#define DEFAULT_N_ASYNC_TRANFERS_PER_DEVICE 10

namespace fluxeem
{
    enum class UsbDataTransferEndpoint : uint8_t
    {
        USB_DATATRANSFER_ENDPOINT_82 = 82,
        USB_DATATRANSFER_ENDPOINT_83 = 83
    };

    /**
     * @brief USB bulk transfer implementation of DataTransfer
     *
     * Uses UsbInterface's async bulk transfer API with multiple handles
     * for high-throughput data acquisition. All libusb details are
     * abstracted by UsbInterface.
     */
    class FLUXEEM_API UsbDataTransfer : public DataTransfer
    {
    public:
        UsbDataTransfer(
            std::shared_ptr<UsbInterface> usb_interface,
            uint32_t buffer_pool_object_size = USB_DATATRANSFER_PACKET_SIZE,
            uint8_t transfer_num = DEFAULT_N_ASYNC_TRANFERS_PER_DEVICE,
            UsbDataTransferEndpoint endpoint = UsbDataTransferEndpoint::USB_DATATRANSFER_ENDPOINT_82);

        ~UsbDataTransfer() override;

        void start() override;
        void stop() override;

    private:
        void transferLoop();
        void flush() override;

        // USB resources
        std::shared_ptr<UsbInterface> usb_interface_;
        std::vector<std::unique_ptr<AsyncBulkTransferHandle>> transfer_handles_;

        // Worker thread
        std::thread worker_thread_;

        // Configuration
        uint32_t packet_size_;
        uint32_t timeout_{USB_DATATRANSFER_TIMEOUT};
        uint8_t endpoint_bulk_;
        uint8_t transfer_num_;
    };

} // namespace fluxeem

#endif // __USB_DATA_TRANSFER_HPP__
