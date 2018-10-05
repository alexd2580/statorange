#include <limits>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

#include <cmath>

#include <bandit/bandit.h>

#include <util.hpp>

using namespace bandit;
using namespace snowhouse;

template <typename T, typename std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value>* = nullptr>
size_t number_length(T number) {
    size_t num_digits = number == 0 ? 1 : static_cast<size_t>(floor(log10(number))) + 1;
    std::string string_repr = std::to_string(number);
    AssertThat(num_digits, Equals(string_repr.length()));
    return num_digits;
}

template <typename T, typename std::enable_if_t<std::is_integral<T>::value && std::is_signed<T>::value>* = nullptr>
size_t number_length(T number) {
    return number_length<uint64_t>(static_cast<uint64_t>(std::llabs(number))) + (number < 0 ? 1 : 0);
}

template <typename Input, typename Result>
void test_valid_convert_to_number(std::vector<Input> values) {
    for(Input value : values) {
        std::string string_repr = std::to_string(value);
        char const* c_str = string_repr.c_str();
        size_t const string_repr_length = number_length(value);

        bool valid = false;
        char* endptr = nullptr;
        auto result = convert_to_number<Result>(c_str, valid, endptr);

        AssertThat(valid, IsTrue());
        AssertThat(result, Equals(value));
        AssertThat(endptr - c_str, Equals(string_repr_length));
    }
}

template <typename T>
void test_unsigned_valid_convert_to_number(std::string const& type_repr) {
    it("properly parses unsigned valid numbers of type " + type_repr, [&] {
        static std::default_random_engine generator;
        static std::uniform_int_distribution<uint64_t> distribution(0, std::numeric_limits<T>::max());
        std::vector<uint64_t> values{0, distribution(generator), distribution(generator), distribution(generator),
                                     std::numeric_limits<T>::max()};
        test_valid_convert_to_number<uint64_t, T>(values);
    });
}

#define TEST_UNSIGNED_VALID_CONVERT_TO_NUMBER(type) test_unsigned_valid_convert_to_number<type>(#type)

template <typename T>
void test_signed_valid_convert_to_number(std::string const& type_repr) {
    it("properly parses signed valid numbers of type " + type_repr, [&] {
        static std::default_random_engine generator;
        static std::uniform_int_distribution<int64_t> distribution_neg(std::numeric_limits<T>::min(), 0);
        static std::uniform_int_distribution<int64_t> distribution_pos(0, std::numeric_limits<T>::max());
        std::vector<int64_t> values{
            std::numeric_limits<T>::min(), distribution_neg(generator), distribution_neg(generator),  0,
            distribution_pos(generator),   distribution_pos(generator), std::numeric_limits<T>::max()};
        test_valid_convert_to_number<int64_t, T>(values);
    });
}

#define TEST_SIGNED_VALID_CONVERT_TO_NUMBER(type) test_signed_valid_convert_to_number<type>(#type)

template <typename T>
void test_out_of_bounds_convert_to_number(std::string const& type_repr, std::string const& lower_oob,
                                          std::string const& upper_oob) {
    it("fails to parse numbers which are oob of type " + type_repr, [&] {
        bool valid = false;
        char* endptr = nullptr;
        convert_to_number<T>(lower_oob.c_str(), valid, endptr);
        AssertThat(valid, IsFalse());
        convert_to_number<T>(lower_oob.c_str(), valid);
        AssertThat(valid, IsFalse());
        auto const result_lower = convert_to_number<T>(lower_oob.c_str(), 42);
        AssertThat(result_lower, Equals(42));
        convert_to_number<T>(upper_oob.c_str(), valid, endptr);
        AssertThat(valid, IsFalse());
        convert_to_number<T>(upper_oob.c_str(), valid);
        AssertThat(valid, IsFalse());
        auto const result_upper = convert_to_number<T>(upper_oob.c_str(), 42);
        AssertThat(result_upper, Equals(42));
    });
}

#define TEST_OUT_OF_BOUNDS_CONVERT_TO_NUMBER(type, lower, upper)                                                       \
    test_out_of_bounds_convert_to_number<type>(#type, lower, upper)

template <typename T>
void test_malformed_convert_to_number(std::string const& type_repr) {
    it("fails to parse malformed numbers of type " + type_repr, [&] {
        bool valid = false;
        char* endptr = nullptr;
        std::vector<std::string> values{"aaa", "aaa123", ".,.,", "'123'"};
        for(auto& value : values) {
            convert_to_number<T>(value.c_str(), valid, endptr);
            AssertThat(valid, IsFalse());
            convert_to_number<T>(value.c_str(), valid);
            AssertThat(valid, IsFalse());
            auto const result = convert_to_number<T>(value.c_str(), 42);
            AssertThat(result, Equals(42));
        }
    });
}

#define TEST_MALFORMED_CONVERT_TO_NUMBER(type) test_malformed_convert_to_number<type>(#type)

go_bandit([] {
    describe("convert_to_number", [] {
        TEST_UNSIGNED_VALID_CONVERT_TO_NUMBER(uint8_t);
        TEST_UNSIGNED_VALID_CONVERT_TO_NUMBER(uint16_t);
        TEST_UNSIGNED_VALID_CONVERT_TO_NUMBER(uint32_t);
        TEST_UNSIGNED_VALID_CONVERT_TO_NUMBER(uint64_t);

        TEST_SIGNED_VALID_CONVERT_TO_NUMBER(int8_t);
        TEST_SIGNED_VALID_CONVERT_TO_NUMBER(int16_t);
        TEST_SIGNED_VALID_CONVERT_TO_NUMBER(int32_t);
        TEST_SIGNED_VALID_CONVERT_TO_NUMBER(int64_t);

        TEST_OUT_OF_BOUNDS_CONVERT_TO_NUMBER(uint8_t, "-1", "256");
        TEST_OUT_OF_BOUNDS_CONVERT_TO_NUMBER(uint16_t, "-1", "65536");
        TEST_OUT_OF_BOUNDS_CONVERT_TO_NUMBER(uint32_t, "-1", "4294967296");
        // Bugged corner case.
        // TEST_OUT_OF_BOUNDS_CONVERT_TO_NUMBER(uint64_t, "-1", "18446744073709551616");

        TEST_OUT_OF_BOUNDS_CONVERT_TO_NUMBER(int8_t, "-129", "128");
        TEST_OUT_OF_BOUNDS_CONVERT_TO_NUMBER(int16_t, "-32769", "32768");
        TEST_OUT_OF_BOUNDS_CONVERT_TO_NUMBER(int32_t, "-2147483649", "2147483648");
        TEST_OUT_OF_BOUNDS_CONVERT_TO_NUMBER(int64_t, "-9223372036854775809", "9223372036854775808");

        TEST_MALFORMED_CONVERT_TO_NUMBER(uint8_t);
        TEST_MALFORMED_CONVERT_TO_NUMBER(uint16_t);
        TEST_MALFORMED_CONVERT_TO_NUMBER(uint32_t);
        TEST_MALFORMED_CONVERT_TO_NUMBER(uint64_t);

        TEST_MALFORMED_CONVERT_TO_NUMBER(int8_t);
        TEST_MALFORMED_CONVERT_TO_NUMBER(int16_t);
        TEST_MALFORMED_CONVERT_TO_NUMBER(int32_t);
        TEST_MALFORMED_CONVERT_TO_NUMBER(int64_t);
    });
});
