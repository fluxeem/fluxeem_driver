// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef __ASYNC_BULK_TRANSFER_HANDLE_H__
#define __ASYNC_BULK_TRANSFER_HANDLE_H__

#include <vector>
#include <memory>
#include <cstdint>
#include <fluxeem/base/define/base_define.h>
#include "libusb.h"

namespace fluxeem
{
    // Forward declaration
    class UsbInterface;

    /**
     * @brief Buffer type for async bulk transfers (same as DataTransfer::Buffer)
     */
    using Buffer = std::vector<uint8_t>;
    using BufferPtr = std::shared_ptr<Buffer>;

    /**
     * @brief Transfer status enumeration (abstracts libusb status)
     */
    enum class TransferStatus
    {
        COMPLETED,
        TIMED_OUT,
        TRANSFER_ERROR,
        CANCELLED,
        PENDING
    };

    /**
     * @brief Handle for a single async bulk transfer
     * 
     * Encapsulates all libusb async transfer details, providing a clean
     * high-level interface for async USB bulk transfers.
     */
    class FLUXEEM_API AsyncBulkTransferHandle
    {
    public:
        AsyncBulkTransferHandle();
        ~AsyncBulkTransferHandle();

        // Non-copyable, movable
        AsyncBulkTransferHandle(const AsyncBulkTransferHandle&) = delete;
        AsyncBulkTransferHandle& operator=(const AsyncBulkTransferHandle&) = delete;
        AsyncBulkTransferHandle(AsyncBulkTransferHandle&& other) noexcept;
        AsyncBulkTransferHandle& operator=(AsyncBulkTransferHandle&& other) noexcept;

        /**
         * @brief Prepare the transfer with buffer and endpoint
         * @param usb_interface USB interface for configuration
         * @param buffer Buffer from object pool (handle takes ownership during transfer)
         * @param endpoint USB endpoint address
         * @param timeout Transfer timeout in milliseconds
         */
        void prepare(UsbInterface* usb_interface,
                     BufferPtr buffer,
                     uint8_t endpoint,
                     uint32_t timeout);

        /**
         * @brief Submit the transfer for async execution
         */
        bool submit(UsbInterface* usb_interface);

        /**
         * @brief Cancel a pending transfer
         */
        void cancel();

        /**
         * @brief Wait for the transfer to complete
         */
        void waitCompletion(UsbInterface* usb_interface);

        /**
         * @brief Check if transfer has completed
         */
        bool isCompleted() const { return static_cast<bool>(completion_); }

        /**
         * @brief Get the transfer status after completion
         */
        TransferStatus status() const;

        /**
         * @brief Get the buffer (resized to actual transferred length)
         */
        BufferPtr getBuffer();

        /**
         * @brief Get actual bytes transferred
         */
        size_t actualLength() const;

    private:
        static void asyncBulkCallback(struct libusb_transfer* transfer);

        libusb_transfer* transfer_{nullptr};
        BufferPtr buffer_;  // Holds buffer during async transfer
        int completion_{1};  // 1 = completed/not queued, 0 = pending
    };

} // namespace fluxeem

#endif // __ASYNC_BULK_TRANSFER_HANDLE_H__
