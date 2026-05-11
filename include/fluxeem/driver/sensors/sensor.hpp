// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef FLUXEEM_SENSOR_HPP
#define FLUXEEM_SENSOR_HPP

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <cstdint>

#include <fluxeem/base/define/base_define.h>
#include <fluxeem/hal/common/interface.hpp>
#include <fluxeem/hal/registers/register_operation/register_operation.h>

namespace fluxeem
{

    /**
     * @brief Abstract interface for register operations
     * 
     * This interface decouples Sensor from RegisterController,
     * allowing different implementations (e.g., direct register access,
     * named register access via RegisterController, etc.)
     */
    class FLUXEEM_API IRegisterAccess
    {
    public:
        virtual ~IRegisterAccess() = default;

        virtual void writeRegisterField(const std::string& reg_name, 
                                        const std::string& field_name, 
                                        uint32_t value) = 0;

        virtual void applyRegisterOperationSequence(const std::vector<RegisterOperation>& operations) = 0;
    };

    /**
     * @brief Sensor base class - responsible ONLY for sensor hardware configuration
     * 
     * This class handles:
     * - Sensor initialization (register configuration)
     * - Sensor start/stop (streaming enable/disable registers)
     * - Sensor-specific settings (bias, ROI, etc.)
     * 
     * This class does NOT handle:
     * - Data transfer (use DataTransfer)
     * - Data decoding (use Decoder)
     * - Pipeline orchestration (use DataPipeline)
     */
    class FLUXEEM_API Sensor
    {
    public:
        Sensor() = default;
        virtual ~Sensor() = default;

        // Non-copyable
        Sensor(const Sensor&) = delete;
        Sensor& operator=(const Sensor&) = delete;

        /**
         * @brief Initialize the sensor hardware
         * @param interface The communication interface
         * @return 0 on success, negative on error
         */
        virtual int init(std::shared_ptr<Interface> interface) = 0;

        /**
         * @brief Start sensor streaming (configure registers to begin data output)
         * @return 0 on success, negative on error
         */
        virtual int startStreaming() = 0;

        /**
         * @brief Stop sensor streaming (configure registers to stop data output)
         * @return 0 on success, negative on error
         */
        virtual int stopStreaming() = 0;

        /**
         * @brief Get the raw event size in bytes for this sensor
         */
        virtual uint32_t getRawEventSizeBytes() const = 0;

        /// @brief Check if sensor is initialized
        bool isInitialized() const { return inited_; }

        /**
         * @brief Set the register access interface
         * @param reg_access Shared pointer to IRegisterAccess implementation
         */
        void setRegisterAccess(std::shared_ptr<IRegisterAccess> reg_access)
        {
            register_access_ = reg_access;
        }

    protected:
        std::shared_ptr<IRegisterAccess> register_access_;
        bool inited_ = false;

        // Helper methods for derived classes (delegate to register_access_)
        void writeRegisterField(const std::string& reg_name, const std::string& field_name, uint32_t value);
        void applyRegisterOperationSequence(const std::vector<RegisterOperation>& operations);
    };

} // namespace fluxeem

#endif // __Sensor_HPP__