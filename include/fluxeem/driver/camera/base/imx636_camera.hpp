// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef IMX636_CAMERA_HPP
#define IMX636_CAMERA_HPP

#include <fluxeem/driver/camera/base/event_camera.hpp>
#include <fluxeem/hal/data_stream/raw_event_stream_encoding_type.hpp>

namespace fluxeem
{
    /**
     * @ingroup fluxeem_camera_api
     * @brief 事件相机基类
     */
    class FLUXEEM_API Imx636Camera : public EventCamera{
    public:
        Imx636Camera(CameraDescription camera_desc) : EventCamera(camera_desc) {}
        ~Imx636Camera() {}

        bool isConnected() const override;

        uint16_t getWidth() override;

        uint16_t getHeight() override;

    private:
        bool init() override;

        RawEventStreamFormat getRawEventStreamFormat() override;

        bool checkSensor();

    };

} // namespace fluxeem

#endif  // IMX636_CAMERA_HPP
