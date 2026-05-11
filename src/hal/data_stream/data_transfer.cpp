// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#include <fluxeem/hal/data_stream/data_transfer.hpp>
#include <fluxeem/base/logging/logger.h>

namespace fluxeem
{

    // ====== DataTransfer ======

    DataTransfer::DataTransfer(uint32_t buffer_pool_object_size, size_t initial_pool_size)
        : buffer_pool_(buffer_pool_.make_unbounded(initial_pool_size, buffer_pool_object_size))
    {
    }

    DataTransfer::~DataTransfer()
    {
        // Subclass should call stop() in their destructor
    }

    DataTransfer::BufferPtr DataTransfer::createBuffer(size_t buffer_size)
    {
        return buffer_pool_.acquire(buffer_size);
    }

    void DataTransfer::setNewBufferCallback(NewBufferCallback cb)
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        new_buffer_cb_ = std::move(cb);
    }

    void DataTransfer::deliverBuffer(BufferPtr buffer)
    {
        if (!buffer || buffer->empty())
        {
            return;
        }

        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (new_buffer_cb_)
        {
            new_buffer_cb_(buffer);
        }
    }

} // namespace fluxeem
