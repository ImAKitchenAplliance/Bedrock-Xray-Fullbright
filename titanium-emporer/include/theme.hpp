#pragma once
#include <cstdint>
namespace titanium::theme {
struct Color { std::uint8_t r, g, b; };
constexpr Color orange{255, 111, 30};
constexpr Color black{9, 10, 13};
constexpr Color blend(Color a, Color b, unsigned position, unsigned maximum) {
    if (!maximum) return a;
    if (position > maximum) position = maximum;
    return {static_cast<std::uint8_t>((a.r * (maximum-position) + b.r * position) / maximum),
            static_cast<std::uint8_t>((a.g * (maximum-position) + b.g * position) / maximum),
            static_cast<std::uint8_t>((a.b * (maximum-position) + b.b * position) / maximum)};
}
}
