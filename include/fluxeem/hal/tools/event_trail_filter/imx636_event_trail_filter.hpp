// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef IMX636EVENTFILTER_HPP_
#define IMX636EVENTFILTER_HPP_

#include <variant>
#include <cstdint>
#include <fluxeem/hal/tools/camera_tool_private.h>

#define IMX636_EVENT_TRAIL_FILTER_THRESHOLD_MAX 100
#define IMX636_EVENT_TRAIL_FILTER_THRESHOLD_MIN 1
#define IMX636_EVENT_TRAIL_FILTER_THRESHOLD_DEFAULT 10

namespace fluxeem
{
    /**
     * @ingroup tools_filter
     * @brief IMX636 事件拖尾过滤器配置工具
     */
    class FLUXEEM_API Imx636EventTrailFilter : public CameraToolRegisterWithCallback
    {
    public:
        Imx636EventTrailFilter(std::shared_ptr<RegisterController> register_ctrl);

        // warring: Remove the default copy constructor so that the compiler knows it is not there to avoid errors
        Imx636EventTrailFilter(const Imx636EventTrailFilter &) = delete;
        Imx636EventTrailFilter &operator=(const Imx636EventTrailFilter &) = delete;

        ~Imx636EventTrailFilter() {}

        const ToolInfo getToolInfo() override final
        {
            return ToolInfo{
                ToolType::TOOL_EVENT_TRAIL_FILTER,
                "EventTrailFilter",
                {"enable", "threshold", "type"},
                "Event Trail Filter of IMX636/646 sensor"};
        }

    private:
        bool addRegister2Map(const FullParameterInfo &info);

        bool setEnable(bool en);

        bool getEnable(bool &en);

        bool setThreshold(int th);

        bool getThreshold(int &th);

        bool getType(std::string &type);

        bool setType(std::string type);

        bool enabled_ = false;
        std::string filtering_type_ = "TRAIL";
        uint32_t threshold_ms_ = IMX636_EVENT_TRAIL_FILTER_THRESHOLD_DEFAULT;
        std::vector<std::string> event_trail_filter_options_ = {"TRAIL", "STC_CUT_TRAIL", "STC_KEEP_TRAIL"};
        // name, min, max, unit, description
        const std::vector<FullParameterInfo> imx636_infos_{
            {"enable", "Set whether the Event Trail Filter function is enabled", ToolParameterType::BOOL,
             BoolParameterInfo{false,
                               std::bind(&Imx636EventTrailFilter::getEnable, this, std::placeholders::_1), std::bind(&Imx636EventTrailFilter::setEnable, this, std::placeholders::_1)}},
            {"threshold", "Set threshold", ToolParameterType::INT,
             IntParameterInfo{IMX636_EVENT_TRAIL_FILTER_THRESHOLD_MIN, IMX636_EVENT_TRAIL_FILTER_THRESHOLD_MAX, IMX636_EVENT_TRAIL_FILTER_THRESHOLD_DEFAULT, "ms",
                              std::bind(&Imx636EventTrailFilter::getThreshold, this, std::placeholders::_1), std::bind(&Imx636EventTrailFilter::setThreshold, this, std::placeholders::_1)}},
            {"type", "Set event trail filter type", ToolParameterType::ENUM,
             EnumParameterInfo{event_trail_filter_options_, "TRAIL", std::bind(&Imx636EventTrailFilter::getType, this, std::placeholders::_1), std::bind(&Imx636EventTrailFilter::setType, this, std::placeholders::_1)}},
        };
    };

} // namespace fluxeem

#endif // IMX636EVENTTRAILFILTER_HPP_
