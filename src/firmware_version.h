#pragma once

#include <array>
#include <cstddef>
#include <type_traits>

#ifndef BUILD_TIMESTAMP
#define BUILD_TIMESTAMP __DATE__ " " __TIME__
#endif

namespace firmware_version_detail {

constexpr unsigned long long unsigned_magnitude(long long value) {
    return value < 0 ? static_cast<unsigned long long>(-(value + 1)) + 1ULL
                     : static_cast<unsigned long long>(value);
}

constexpr std::size_t count_digits(long long value) {
    unsigned long long magnitude = unsigned_magnitude(value);
    std::size_t digits = 0;

    do {
        ++digits;
        magnitude /= 10ULL;
    } while (magnitude != 0ULL);

    if (value < 0) {
        ++digits;  // Reserve space for minus sign.
    }

    return digits;
}

template <long long Value>
struct integral_string {
    inline static constexpr std::size_t length = count_digits(Value);

    inline static constexpr std::array<char, length + 1> data = []() {
        std::array<char, length + 1> buffer{};
        std::size_t index = length;
        buffer[index] = '\0';

        unsigned long long magnitude = unsigned_magnitude(Value);
        do {
            buffer[--index] = static_cast<char>('0' + (magnitude % 10ULL));
            magnitude /= 10ULL;
        } while (magnitude != 0ULL);

        if constexpr (Value < 0) {
            buffer[--index] = '-';
        }

        return buffer;
    }();

    static constexpr const char *c_str() {
        return data.data();
    }
};

template <auto Timestamp, typename = void>
struct build_info;

template <auto Timestamp>
struct build_info<Timestamp, std::enable_if_t<std::is_convertible_v<decltype(Timestamp), const char *>>> {
    inline static constexpr const char *value = Timestamp;
};

template <auto Timestamp>
struct build_info<Timestamp, std::enable_if_t<std::is_integral_v<decltype(Timestamp)>>> {
    inline static constexpr const char *value =
        integral_string<static_cast<long long>(Timestamp)>::c_str();
};

}  // namespace firmware_version_detail

constexpr const char *FIRMWARE_BUILD_INFO =
    firmware_version_detail::build_info<BUILD_TIMESTAMP>::value;
