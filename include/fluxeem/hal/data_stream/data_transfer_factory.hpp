// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef __DATA_TRANSFER_FACTORY_HPP__
#define __DATA_TRANSFER_FACTORY_HPP__

#include <memory>

#include <fluxeem/base/define/base_define.h>
#include <fluxeem/hal/data_stream/data_transfer.hpp>
#include <fluxeem/hal/data_stream/usb_data_transfer.h>
#include <fluxeem/hal/common/interface.hpp>
#include <fluxeem/hal/common/usb_interface.h>

namespace fluxeem
{

    /**
     * @brief Factory for creating DataTransfer instances
     * 
     * This factory decouples DataTransfer creation from Sensor,
     * allowing flexible configuration and testing.
     */
    class FLUXEEM_API DataTransferFactory
    {
    public:
        /**
         * @brief Create a USB DataTransfer instance
         * @param interface The USB interface
         * @param buffer_pool_object_size Size of each buffer in the pool
         * @param transfer_num Number of async transfers
         * @param endpoint USB endpoint
         * @return Unique pointer to DataTransfer
         */
        static std::unique_ptr<DataTransfer> createUsbDataTransfer(
            std::shared_ptr<Interface> interface,
            uint32_t buffer_pool_object_size = USB_DATATRANSFER_PACKET_SIZE,
            uint8_t transfer_num = DEFAULT_N_ASYNC_TRANFERS_PER_DEVICE,
            UsbDataTransferEndpoint endpoint = UsbDataTransferEndpoint::USB_DATATRANSFER_ENDPOINT_82)
        {
            auto usb_interface = std::dynamic_pointer_cast<UsbInterface>(interface);
            if (!usb_interface)
            {
                return nullptr;
            }
            return std::make_unique<UsbDataTransfer>(
                usb_interface, buffer_pool_object_size, transfer_num, endpoint);
        }
    };

} // namespace fluxeem

#endif // __DATA_TRANSFER_FACTORY_HPP__
