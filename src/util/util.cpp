#include "util/enum_traits.h"

enum class ExampleEnum { A, B, C };

template<>
struct enum_traits<ExampleEnum> {
    using enum ExampleEnum;
    static constexpr std::array<ExampleEnum, 3> elements{A, B, C};
    static constexpr std::array<std::string_view, elements.size()> names{"A", "B", "C"};
};
