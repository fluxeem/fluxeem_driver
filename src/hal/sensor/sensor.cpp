#include <fluxeem/driver/sensors/sensor.hpp>
#include <fluxeem/base/logging/logger.h>

namespace fluxeem
{

    void Sensor::writeRegisterField(const std::string& reg_name, const std::string& field_name, uint32_t value)
    {
        if (register_access_)
        {
            register_access_->writeRegisterField(reg_name, field_name, value);
        }
        else
        {
            LOG_ERROR("Sensor::writeRegisterField failed: no register access interface set");
        }
    }

    void Sensor::applyRegisterOperationSequence(const std::vector<RegisterOperation>& operations)
    {
        if (register_access_)
        {
            register_access_->applyRegisterOperationSequence(operations);
        }
        else
        {
            LOG_ERROR("Sensor::applyRegisterOperationSequence failed: no register access interface set");
        }
    }

} // namespace fluxeem
