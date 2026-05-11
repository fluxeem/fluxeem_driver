// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#include <fluxeem/hal/tools/tool_info.h>

namespace fluxeem {

    std::string FLUXEEM_API to_string(ToolType type) {
        switch (type)
        {
        case ToolType::TOOL_BIAS:
            return "BIAS";
            break;
        case ToolType::TOOL_TRIGGER_IN:
            return "TRIGGER_IN";
            break;
        case ToolType::TOOL_ANTI_FLICKER:
            return "ANTI_FLICKER";
            break;
        case ToolType::TOOL_EVENT_TRAIL_FILTER:
            return "EVENT_TRAIL_FILTER";
            break;
        case ToolType::TOOL_EVENT_RATE_CONTROL:
            return "EVENT_RATE_CONTROL";
            break;
        case ToolType::TOOL_ROI:
            return "ROI";
            break;      
        default:
            return "UNKNOWN";
        }
    }

    std::string ToolParameterTypeToString(ToolParameterType type) {
        switch (type)
        {
        case ToolParameterType::INT:
            return "INT";
            break;
        case ToolParameterType::FLOAT:
            return "FLOAT";
            break;
        case ToolParameterType::BOOL:
            return "BOOL";
            break;
        case ToolParameterType::STRING:
            return "STRING";
            break;
        case ToolParameterType::ENUM:
            return "ENUM";
            break;
        default:
            return "UNKNOWN";
        }
    }
}
