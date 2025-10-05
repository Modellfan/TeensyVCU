#pragma once

#include <array>
#include <cstddef>
#include <string_view>
#include <type_traits>

#ifndef BUILD_TIMESTAMP
#define BUILD_TIMESTAMP __DATE__ " " __TIME__
#endif

namespace firmware_version_detail {

constexpr std::size_t count_digits(long long value) {
    if (value == 0) {
        return 1;
    }

    std::size_t digits = 0;
    if (value < 0) {
        digits += 1;  // Reserve space for the minus sign.
        value = -value;
    }

    while (value > 0) {
        digits += 1;
        value /= 10;
    }

    return digits;
}

constexpr auto to_chars(long long value) {
    const bool is_negative = value < 0;
    long long magnitude = is_negative ? -value : value;
    const std::size_t digits = count_digits(value);

    std::array<char, digits + 1> buffer{};
    std::size_t index = digits;
    buffer[index] = '\0';

    if (magnitude == 0) {
        buffer[--index] = '0';
    } else {
        while (magnitude > 0) {
            buffer[--index] = static_cast<char>('0' + (magnitude % 10));
            magnitude /= 10;
        }
    }

    if (is_negative) {
        buffer[--index] = '-';
    }

    return buffer;
}

template <typename T>
constexpr std::string_view build_info() {
    if constexpr (std::is_convertible_v<T, const char *>) {
        return std::string_view{BUILD_TIMESTAMP};
    } else if constexpr (std::is_integral_v<T>) {
        static constexpr auto storage = to_chars(static_cast<long long>(BUILD_TIMESTAMP));
        return std::string_view{storage.data(), storage.size() - 1};
    } else {
        return std::string_view{};
    }
}

}  // namespace firmware_version_detail

constexpr std::string_view FIRMWARE_BUILD_INFO =
    firmware_version_detail::build_info<decltype(BUILD_TIMESTAMP)>();
