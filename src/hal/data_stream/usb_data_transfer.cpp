// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#include <fluxeem/hal/data_stream/usb_data_transfer.h>
#include <fluxeem/base/logging/logger.h>

#include <cassert>

namespace fluxeem
{

    // ==================== UsbDataTransfer ====================

    UsbDataTransfer::UsbDataTransfer(
        std::shared_ptr<UsbInterface> usb_interface,
        uint32_t buffer_pool_object_size,
        uint8_t transfer_num,
        UsbDataTransferEndpoint endpoint)
        : DataTransfer(buffer_pool_object_size),
          usb_interface_(usb_interface),
          packet_size_(buffer_pool_object_size),
          transfer_num_(transfer_num)
    {
        if (endpoint == UsbDataTransferEndpoint::USB_DATATRANSFER_ENDPOINT_82)
        {
            endpoint_bulk_ = usb_interface->getEpCommAddress()[0];
        }
        else if (endpoint == UsbDataTransferEndpoint::USB_DATATRANSFER_ENDPOINT_83)
        {
            endpoint_bulk_ = usb_interface->getEpCommAddress()[1];
        }
        else
        {
            LOG_ERROR("Invalid endpoint number!");
        }
    }

    UsbDataTransfer::~UsbDataTransfer()
    {
        stop();
    }

    void UsbDataTransfer::start()
    {
        if (status_ == StreamStatus::RUNNING)
        {
            return;
        }

        status_ = StreamStatus::RUNNING;

        // Flush any stale data
        flush();

        // Create transfer handles
        transfer_handles_.clear();
        transfer_handles_.reserve(transfer_num_);

        for (uint8_t i = 0; i < transfer_num_; ++i)
        {
            transfer_handles_.push_back(usb_interface_->createAsyncBulkHandle());
        }

        // Prepare and submit all transfers with staggered timeouts
        uint32_t timeout = timeout_;
        uint32_t timeout_shift = timeout_ / transfer_handles_.size();

        for (auto& handle : transfer_handles_)
        {
            auto buffer = createBuffer();
            buffer->resize(packet_size_);
            handle->prepare(usb_interface_.get(), buffer, endpoint_bulk_, timeout);
            handle->submit(usb_interface_.get());
            timeout += timeout_shift;
        }

        // Start worker thread
        worker_thread_ = std::thread(&UsbDataTransfer::transferLoop, this);
    }

    void UsbDataTransfer::stop()
    {
        status_ = StreamStatus::STOP;

        // Cancel all pending transfers
        for (auto &handle : transfer_handles_)
        {
            if (handle)
            {
                handle->cancel();
            }
        }

        if (worker_thread_.joinable())
        {
            worker_thread_.join();
        }

        for (auto &handle : transfer_handles_)
        {
            if (handle && !handle->isCompleted() && usb_interface_)
            {
                handle->waitCompletion(usb_interface_.get());
            }
        }

        transfer_handles_.clear();
    }

    void UsbDataTransfer::transferLoop()
    {
        while (status_ == StreamStatus::RUNNING)
        {
            for (auto& handle : transfer_handles_)
            {
                handle->waitCompletion(usb_interface_.get());

                if (status_ != StreamStatus::RUNNING)
                {
                    break;
                }

                auto transfer_status = handle->status();

                if (transfer_status == TransferStatus::COMPLETED)
                {
                    // Get buffer from handle (already resized to actual length)
                    BufferPtr buffer = handle->getBuffer();
                    deliverBuffer(buffer);

                    // Get new buffer from pool and resubmit
                    buffer = createBuffer();
                    buffer->resize(packet_size_);
                    handle->prepare(usb_interface_.get(), buffer, endpoint_bulk_, timeout_);
                    handle->submit(usb_interface_.get());
                }
                else if (transfer_status == TransferStatus::TIMED_OUT)
                {
                    // Just resubmit on timeout
                    handle->submit(usb_interface_.get());
                    break;
                }
                else
                {
                    LOG_ERROR("USB transfer error: status=%d", static_cast<int>(transfer_status));
                    status_ = StreamStatus::STOP;
                    break;
                }
            }
        }
    }

    void UsbDataTransfer::flush()
    {
        usb_interface_->flushEndpointBuffer(endpoint_bulk_);
    }

} // namespace fluxeem