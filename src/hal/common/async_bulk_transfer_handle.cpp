#include <fluxeem/hal/common/async_bulk_transfer_handle.h>
#include <fluxeem/hal/common/usb_interface.h>
#include <fluxeem/base/logging/logger.h>

#include <stdexcept>

namespace fluxeem
{

    // ==================== AsyncBulkTransferHandle ====================

    AsyncBulkTransferHandle::AsyncBulkTransferHandle()
    {
        transfer_ = libusb_alloc_transfer(0);
        if (!transfer_)
        {
            throw std::runtime_error("Failed to allocate libusb transfer");
        }
        completion_ = 1;  // Not queued = completed
    }

    AsyncBulkTransferHandle::~AsyncBulkTransferHandle()
    {
        if (transfer_)
        {
            try
            {
                cancel();
            }
            catch (...)
            {
            }
            libusb_free_transfer(transfer_);
            transfer_ = nullptr;
        }
    }

    AsyncBulkTransferHandle::AsyncBulkTransferHandle(AsyncBulkTransferHandle&& other) noexcept
        : transfer_(other.transfer_),
          buffer_(std::move(other.buffer_)),
          completion_(other.completion_)
    {
        other.transfer_ = nullptr;
        other.completion_ = 1;
    }

    AsyncBulkTransferHandle& AsyncBulkTransferHandle::operator=(AsyncBulkTransferHandle&& other) noexcept
    {
        if (this != &other)
        {
            if (transfer_)
            {
                libusb_free_transfer(transfer_);
            }
            transfer_ = other.transfer_;
            buffer_ = std::move(other.buffer_);
            completion_ = other.completion_;
            other.transfer_ = nullptr;
            other.completion_ = 1;
        }
        return *this;
    }

    void AsyncBulkTransferHandle::prepare(
        UsbInterface* usb_interface,
        BufferPtr buffer,
        uint8_t endpoint,
        uint32_t timeout)
    {
        buffer_ = buffer;
        transfer_->endpoint = endpoint;
        usb_interface->fillDataBulkTransfer(
            transfer_,
            buffer_->data(),
            static_cast<int>(buffer_->size()),
            asyncBulkCallback,
            this,
            timeout);
    }

    bool AsyncBulkTransferHandle::submit(UsbInterface* usb_interface)
    {
        completion_ = 0;
        int r = usb_interface->submitTransfer(transfer_);
        if (r < 0)
        {
            if (r != LIBUSB_ERROR_BUSY)
            {
                completion_ = 1;  // Avoid wait on non-submitted transfers
            }
            LOG_ERROR("Submit transfer error: %s", libusb_error_name(r));
            return false;
        }
        return true;
    }

    void AsyncBulkTransferHandle::cancel()
    {
        if (transfer_ && !isCompleted())
        {
            int err = libusb_cancel_transfer(transfer_);
            if (err && err != LIBUSB_ERROR_NOT_FOUND)
            {
                LOG_ERROR("Cancel transfer error: %s", libusb_error_name(err));
            }
        }
    }

    void AsyncBulkTransferHandle::waitCompletion(UsbInterface* usb_interface)
    {
        while (!isCompleted())
        {
            int err = libusb_handle_events_completed(usb_interface->ctx(), &completion_);
            if (err)
            {
                LOG_ERROR("Error in handle events completed: %s", libusb_error_name(err));
                break;
            }
        }
    }

    TransferStatus AsyncBulkTransferHandle::status() const
    {
        if (!isCompleted())
        {
            return TransferStatus::PENDING;
        }

        switch (transfer_->status)
        {
        case LIBUSB_TRANSFER_COMPLETED:
            return TransferStatus::COMPLETED;
        case LIBUSB_TRANSFER_TIMED_OUT:
            // If timed out but has data, treat as completed
            if (transfer_->actual_length > 0)
            {
                return TransferStatus::COMPLETED;
            }
            return TransferStatus::TIMED_OUT;
        case LIBUSB_TRANSFER_CANCELLED:
            return TransferStatus::CANCELLED;
        default:
            return TransferStatus::TRANSFER_ERROR;
        }
    }

    BufferPtr AsyncBulkTransferHandle::getBuffer()
    {
        buffer_->resize(static_cast<size_t>(transfer_->actual_length));
        return buffer_;
    }

    size_t AsyncBulkTransferHandle::actualLength() const
    {
        return static_cast<size_t>(transfer_->actual_length);
    }

    void AsyncBulkTransferHandle::asyncBulkCallback(struct libusb_transfer* transfer)
    {
        if (!transfer->user_data)
            return;

        auto* handle = static_cast<AsyncBulkTransferHandle*>(transfer->user_data);
        handle->completion_ = 1;
    }

} // namespace fluxeem
