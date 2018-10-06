#ifndef UTILS_CONVERT_HPP
#define UTILS_CONVERT_HPP

#include <limits>
#include <type_traits>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

// Convert to unsigned integer.
template <typename T, typename std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value>* = nullptr>
// NOLINTNEXTLINE: Desired call signature.
T convert_to_number(const char* c_str, bool& valid, char*& endptr) {
    errno = 0;
    uint64_t result = strtoull(c_str, &endptr, 10);
    valid = endptr != c_str && errno != ERANGE && result <= std::numeric_limits<T>::max();
    return static_cast<T>(result);
}

// Convert to signed integer.
template <typename T, typename std::enable_if_t<std::is_integral<T>::value && std::is_signed<T>::value>* = nullptr>
// NOLINTNEXTLINE: Desired call signature.
T convert_to_number(const char* c_str, bool& valid, char*& endptr) {
    errno = 0;
    int64_t result = strtoll(c_str, &endptr, 10);
    valid = endptr != c_str && errno != ERANGE && result <= std::numeric_limits<T>::max() &&
            result >= std::numeric_limits<T>::min();
    return static_cast<T>(result);
}

// Convert to floating point.
template <typename T, typename std::enable_if_t<std::is_floating_point<T>::value>* = nullptr>
// NOLINTNEXTLINE: Desired call signature.
T convert_to_number(const char* c_str, bool& valid, char*& endptr) {
    errno = 0;
    long double result = strtold(c_str, &endptr);
    valid = endptr != c_str && errno != ERANGE && result <= static_cast<long double>(std::numeric_limits<T>::max()) &&
            result >= static_cast<long double>(std::numeric_limits<T>::lowest());
    return static_cast<T>(result);
}

template <typename T>
T convert_to_number(const char* c_str, T default_value) {
    char* endptr = nullptr;
    bool valid = false;
    auto result = convert_to_number<T>(c_str, valid, endptr);
    return valid ? result : default_value;
}

template <typename T>
// NOLINTNEXTLINE: Desired call signature.
T convert_to_number(const char* c_str, bool& valid) {
    char* endptr = nullptr;
    return convert_to_number<T>(c_str, valid, endptr);
}

#endif
