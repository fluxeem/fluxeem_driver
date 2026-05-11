// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef FLUXEEM_IMX6X6_SENSOR_HPP
#define FLUXEEM_IMX6X6_SENSOR_HPP

#include <fluxeem/driver/sensors/sensor.hpp>
#include <fluxeem/base/define/base_define.h>

namespace fluxeem
{
    class FLUXEEM_API Imx6x6Sensor : public Sensor
    {
    public:

        Imx6x6Sensor();

        ~Imx6x6Sensor();

        int init(std::shared_ptr<Interface> interface) override;

        int startStreaming() override;

        int stopStreaming() override;

        uint32_t getRawEventSizeBytes() const override { return 2; }

    private:
        void temperatureInit();
        void controlLifo(bool enable, bool out_en, bool cnt_en);
        void controlIphMirror(bool enable);
        void timeBaseConfig(bool external, bool master);
    };
} // namespace fluxeem


#endif // FLUXEEM_IMX6X6_SENSOR_HPP