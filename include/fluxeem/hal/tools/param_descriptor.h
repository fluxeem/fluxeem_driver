#ifndef PARAM_DESCRIPTOR_H_
#define PARAM_DESCRIPTOR_H_

#include <string>
#include <variant>
#include <vector>
#include <fluxeem/base/define/base_define.h>

namespace fluxeem {

using ParamValue = std::variant<int, float, bool, std::string>;

enum class ParamType : uint8_t {
    Int,
    Float,
    Bool,
    String,
    Enum
};

struct FLUXEEM_API ParamConstraint {
    struct IntRange { int min; int max; int default_val; };
    struct FloatRange { float min; float max; float default_val; };
    struct BoolDef { bool default_val; };
    struct EnumDef { std::vector<std::string> options; std::string default_val; };
    struct StringDef { std::string default_val; };

    std::variant<IntRange, FloatRange, BoolDef, EnumDef, StringDef> data;
};

struct FLUXEEM_API ParamDescriptor {
    std::string name;
    ParamType type;
    std::string description;
    std::string unit;
    ParamConstraint constraint;
};

} // namespace fluxeem

#endif // PARAM_DESCRIPTOR_H_
