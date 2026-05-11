#include <fluxeem/hal/tools/roi/imx636_roi.h>
#include <fluxeem/base/logging/logger.h>
#include <sstream>
#include <iomanip>

namespace fluxeem
{
    Imx636ROI::Imx636ROI(std::shared_ptr<RegisterController> register_ctrl) : CameraToolRegister(register_ctrl, "roi/")
    {
        using IR = ParamConstraint::IntRange;
        using BD = ParamConstraint::BoolDef;
        using ED = ParamConstraint::EnumDef;

        registerParam({"enable", ParamType::Bool, "Set whether ROI is enabled", "", {BD{false}}});
        registerParam({"mode", ParamType::Enum, "Set ROI mode.", "", {ED{{"ROI", "RONI"}, "ROI"}}});
        registerParam({"x", ParamType::Int, "Set ROI x.", "pixel", {IR{0, IMX636_SENSOR_WIDTH, 0}}});
        registerParam({"y", ParamType::Int, "Set ROI y.", "pixel", {IR{0, IMX636_SENSOR_HEIGHT, 0}}});
        registerParam({"width", ParamType::Int, "Set ROI width.", "pixel", {IR{0, IMX636_SENSOR_WIDTH, IMX636_SENSOR_WIDTH}}});
        registerParam({"height", ParamType::Int, "Set ROI height.", "pixel", {IR{0, IMX636_SENSOR_HEIGHT, IMX636_SENSOR_HEIGHT}}});

        resetToFullRoi();
    }

    bool Imx636ROI::onParamChanged(uint8_t idx, const ParamValue& old_val, const ParamValue& new_val)
    {
        switch (idx)
        {
        case IDX_ENABLE:
            writeEnableRegisters(std::get<bool>(new_val));
            return true;
        case IDX_MODE:
            return std::get<std::string>(new_val) == "ROI" || std::get<std::string>(new_val) == "RONI";
        case IDX_X:
        case IDX_Y:
        case IDX_WIDTH:
        case IDX_HEIGHT:
        {
            int x = std::get<int>(*get("x"));
            int y = std::get<int>(*get("y"));
            int width = std::get<int>(*get("width"));
            int height = std::get<int>(*get("height"));

            if (!isRoiFeasible(x, y, width, height))
            {
                return false;
            }
            writeROI();

            if (std::get<bool>(*get("enable")))
            {
                writeEnableRegisters(true);
            }
            return true;
        }
        default:
            break;
        }
        (void)old_val;
        return false;
    }

    void Imx636ROI::writeEnableRegisters(bool state)
    {
        const std::string mode = std::get<std::string>(*get("mode"));

        register_controller_->writeRegisterField("roi_ctrl", "roi_td_en", state);
        register_controller_->writeRegisterField("roi_ctrl", "td_roi_roni_n_en", mode == "ROI" ? 1 : 0);
        register_controller_->writeRegisterField("roi_ctrl", "px_td_rstn", 1);
        register_controller_->writeRegisterField("roi_ctrl", "roi_td_shadow_trigger", 1);
        register_controller_->writeRegisterField("roi_win_ctrl", "roi_master_en", 0);
        register_controller_->writeRegisterField("roi_win_ctrl", "roi_win_done", 0);
    }

    bool Imx636ROI::writeROI()
    {
        const std::string mode = std::get<std::string>(*get("mode"));
        if (mode == "ROI")
        {
            int x = std::get<int>(*get("x"));
            int y = std::get<int>(*get("y"));
            int width = std::get<int>(*get("width"));
            int height = std::get<int>(*get("height"));

            register_controller_->writeRegisterField("roi_win_start_addr", "roi_win_start_x", x);
            register_controller_->writeRegisterField("roi_win_start_addr", "roi_win_start_y", y);
            register_controller_->writeRegisterField("roi_win_end_addr", "roi_win_end_x", x + width);
            register_controller_->writeRegisterField("roi_win_end_addr", "roi_win_end_y", y + height);

            register_controller_->writeRegisterField("roi_win_ctrl", "roi_master_en", 1);

            uint32_t roi_win_done = 0;
            do
            {
                register_controller_->readRegisterField("roi_win_ctrl", "roi_win_done", roi_win_done);
            } while (roi_win_done != 1);
        }
        return true;
    }

    void Imx636ROI::resetToFullRoi()
    {
        for (uint32_t col_td_ind = 0; col_td_ind <= 39; col_td_ind += 1)
        {
            std::ostringstream stream;
            stream << std::setw(2) << std::setfill('0') << col_td_ind;
            std::string index_str = stream.str();
            register_controller_->writeRegisterFieldByAlias(prefix_ + "td_roi_x" + index_str, "effective", "enable");
        }
        for (uint32_t row_td_ind = 0; row_td_ind <= 22; row_td_ind += 1)
        {
            std::ostringstream stream;
            stream << std::setw(2) << std::setfill('0') << row_td_ind;
            std::string index_str = stream.str();
            register_controller_->writeRegisterFieldByAlias(prefix_ + "td_roi_y" + index_str, "effective", "enable");
        }
    }

    bool Imx636ROI::isRoiFeasible(int x, int y, int width, int height)
    {
        if (x < 0 || y < 0)
        {
            LOG_ERROR("Error: x and y must be non-negative.");
            return false;
        }

        if (width <= 0 || height <= 0)
        {
            LOG_ERROR("Error: width and height must be positive.");
            return false;
        }

        if (x + width > IMX636_SENSOR_WIDTH || y + height > IMX636_SENSOR_HEIGHT)
        {
            LOG_ERROR("Error: ROI window exceeds sensor boundaries.");
            return false;
        }

        return true;
    }

    std::map<std::string, BasicParameterInfo> Imx636ROI::getAllParamInfo()
    {
        std::map<std::string, BasicParameterInfo> result;
        for (const auto& desc : descriptors())
        {
            ToolParameterType old_type = ToolParameterType::STRING;
            switch (desc.type)
            {
            case ParamType::Int: old_type = ToolParameterType::INT; break;
            case ParamType::Float: old_type = ToolParameterType::FLOAT; break;
            case ParamType::Bool: old_type = ToolParameterType::BOOL; break;
            case ParamType::String: old_type = ToolParameterType::STRING; break;
            case ParamType::Enum: old_type = ToolParameterType::ENUM; break;
            }
            result[desc.name] = BasicParameterInfo{desc.name, desc.description, old_type};
        }
        return result;
    }

    bool Imx636ROI::getParamInfo(const std::string name, IntParameterInfo& info)
    {
        auto idx = findParamIndex(name);
        if (!idx || descriptors()[*idx].type != ParamType::Int)
        {
            return false;
        }
        auto range = std::get<ParamConstraint::IntRange>(descriptors()[*idx].constraint.data);
        info.min = range.min;
        info.max = range.max;
        info.default_value = range.default_val;
        info.unit = descriptors()[*idx].unit;
        return true;
    }

    bool Imx636ROI::getParamInfo(const std::string name, FloatParameterInfo& info)
    {
        auto idx = findParamIndex(name);
        if (!idx || descriptors()[*idx].type != ParamType::Float)
        {
            return false;
        }
        auto range = std::get<ParamConstraint::FloatRange>(descriptors()[*idx].constraint.data);
        info.min = range.min;
        info.max = range.max;
        info.default_value = range.default_val;
        info.unit = descriptors()[*idx].unit;
        return true;
    }

    bool Imx636ROI::getParamInfo(const std::string name, BoolParameterInfo& info)
    {
        auto idx = findParamIndex(name);
        if (!idx || descriptors()[*idx].type != ParamType::Bool)
        {
            return false;
        }
        info.default_value = std::get<ParamConstraint::BoolDef>(descriptors()[*idx].constraint.data).default_val;
        return true;
    }

    bool Imx636ROI::getParamInfo(const std::string name, EnumParameterInfo& info)
    {
        auto idx = findParamIndex(name);
        if (!idx || descriptors()[*idx].type != ParamType::Enum)
        {
            return false;
        }
        auto def = std::get<ParamConstraint::EnumDef>(descriptors()[*idx].constraint.data);
        info.options = def.options;
        info.default_value = def.default_val;
        return true;
    }

    bool Imx636ROI::getParamInfo(const std::string name, StringParameterInfo& info)
    {
        auto idx = findParamIndex(name);
        if (!idx || descriptors()[*idx].type != ParamType::String)
        {
            return false;
        }
        info.default_value = std::get<ParamConstraint::StringDef>(descriptors()[*idx].constraint.data).default_val;
        return true;
    }
} // namespace fluxeem
