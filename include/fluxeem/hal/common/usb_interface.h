// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef __USB_INTERFACE_H__
#define __USB_INTERFACE_H__

#include <string>
#include <vector>
#include <functional>
#include <utility>
#include <memory>
#include <chrono>
#include <stdexcept>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <cstring>
#include <fluxeem/hal/common/interface.hpp>
#include <fluxeem/hal/common/usb_device.h>
#include <fluxeem/base/define/usb_data_structure.h>
#include <fluxeem/hal/common/async_bulk_transfer_handle.h>

#define USB_MAX_ANSWER_SIZE 1024

#define CMD_REQUEST_READ_REGISTER (0xA0)
#define CMD_REQUEST_WRITE_REGISTER (0xA1)
#define CMD_REQUEST_FILE_READ (0xA2)
#define CMD_REQUEST_FILE_WRITE (0xA3)
#define CMD_REQUEST_FILE_INFO (0xA4)

#define CMD_INDEX_CALIBRATION (0x00)

namespace fluxeem
{

    class FLUXEEM_API UsbInterface : public Interface
    {
    public:
        UsbInterface(CameraDescription camera_description);

        UsbInterface(std::shared_ptr<UsbDevice> usb_handle);

        ~UsbInterface();

        virtual bool isConnected() const override;

        // Lifecycle: explicitly open/close the underlying UsbDevice.
        // open() calls usb_handle_->open() and attempts initInterface().
        bool open() override;

        void close() noexcept override;

        bool isOpen() const noexcept override;

        static const std::shared_ptr<UsbInterface> createUsbInterface(CameraDescription cameraDesc);

        static const std::shared_ptr<UsbInterface> createUsbFusionInterface(CameraDescription cameraDesc);

        // ============ High-level Async Bulk Transfer API ============

        /**
         * @brief Create a new async bulk transfer handle
         */
        std::unique_ptr<AsyncBulkTransferHandle> createAsyncBulkHandle();

        /**
         * @brief Wait for any pending events with timeout
         */
        void handleEvents();

        // ============ Low-level API (kept for compatibility) ============

        static libusb_transfer *createAsyncBulkTransfer();

        std::vector<uint8_t> getEpCommAddress();

        int clearHalt(uint8_t endpoint);

        static const std::vector<CameraDescription> findUsbCamera(uint16_t vendor_id, uint16_t product_id);

        static const std::vector<CameraDescription> findUsbCameras(const std::vector<std::pair<uint16_t, uint16_t>> &vid_pid_list);

        static std::string getSerial(std::shared_ptr<UsbDevice> usb_handle, const uint16_t vid, const uint16_t pid);

        bool readRegister(uint32_t address, uint32_t &val) override;

        bool readRegister(uint32_t address, uint32_t *val, int nval);

        bool writeRegister(uint32_t address, uint32_t value) override;

        bool transferFrame(CtrlFrame &frame);

        bool readRegisterWithControlTransfer(uint32_t address, uint32_t &val) override;

        bool writeRegisterWithControlTransfer(uint32_t address, uint32_t val) override;

        bool readRegisterWithControlTransfer(uint32_t address, uint8_t* data, uint16_t read_length) override;

        bool writeRegisterWithControlTransfer(uint32_t address, uint8_t* data, uint16_t write_length) override;

        bool writeCalibrationFile(std::string file_path) override;
        
        bool readCalibrationData(std::vector<uint8_t> &buffer) override;

        void flushEndpointBuffer(uint8_t endpoint);

        void fillDataBulkTransfer(libusb_transfer *transfer, uint8_t *buf, int packet_size,
                                  libusb_transfer_cb_fn async_bulk_cb, void *user_data,
                                  uint32_t timeout);

        int submitTransfer(libusb_transfer *transfer);

        void handleSubmittedTransferTimeout(timeval *tv);

        libusb_context *ctx();

        // ============ OTA Firmware Upgrade API ============

        /**
         * \~english @brief Progress callback for firmware upgrade.
         * \~chinese @brief 固件升级进度回调。
         * @param phase   "erase", "write", or "verify"
         * @param current Current progress (bytes or sector index)
         * @param total   Total (bytes or sectors)
         */
        using FwUpgradeProgressCb = std::function<void(const std::string &phase, uint32_t current, uint32_t total)>;

        /**
         * \~english @brief Check device OTA identity (expects "PSEEUPD").
         * \~chinese @brief 检查设备OTA身份（期望返回"PSEEUPD"）。
         */
        bool fwCheckIdentity();

        /**
         * \~english @brief Read SDK API version string from device.
         * \~chinese @brief 从设备读取SDK API版本字符串。
         */
        std::string fwGetVersion();

        /**
         * \~english @brief Full OTA upgrade: erase → write → optional verify → optional reset.
         * \~chinese @brief 完整OTA升级：擦除 → 写入 → 可选校验 → 可选复位。
         *
         * @param image_path Path to .img firmware file.
         * @param verify     Read-back and compare after write.
         * @param reset      Reset device after successful upgrade.
         * @param cb         Optional progress callback.
         * @return true on success.
         */
        bool fwUpgrade(const std::string &image_path, bool verify = true, bool reset = true,
                       FwUpgradeProgressCb cb = nullptr);

        /**
         * \~english @brief Full OTA upgrade from in-memory image data.
         * \~chinese @brief 从内存中的镜像数据执行完整OTA升级。
         */
        bool fwUpgrade(const std::vector<uint8_t> &image_data, bool verify = true, bool reset = true,
                       FwUpgradeProgressCb cb = nullptr);

        /**
         * \~english @brief Reset (reboot) the device.
         * \~chinese @brief 复位（重启）设备。
         */
        void fwReset();

        /**
         * \~english @brief Get the last firmware-upgrade error message.
         * \~chinese @brief 获取最后一条固件升级错误信息。
         */
        const std::string &fwLastError() const { return fw_last_error_; }

    private:
        std::mutex control_mutex_;

        std::shared_ptr<UsbDevice> usb_handle_;

        // OTA internals (called with control_mutex_ held)
        std::string fw_last_error_;
        bool fwCheckIdentityLocked();
        std::string fwGetVersionLocked();
        bool fwValidateImage(const std::vector<uint8_t> &data);
        bool fwEraseSectors(uint32_t num_sectors, FwUpgradeProgressCb cb);
        bool fwWriteFlash(const std::vector<uint8_t> &data, FwUpgradeProgressCb cb);
        bool fwVerifyFlash(const std::vector<uint8_t> &data, FwUpgradeProgressCb cb);
        bool fwPollWipDone(unsigned int timeout_ms = 30000);

        static bool isBusy(std::shared_ptr<UsbDevice> usb_handle);
    };

} // namespace fluxeem

#endif // __USB_INTERFACE_H__
