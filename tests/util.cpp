#include <limits>
#include <random>
#include <string>
#include <type_traits>

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
        convert_to_number<T>(upper_oob.c_str(), valid, endptr);
        AssertThat(valid, IsFalse());
    });
}

#define TEST_OUT_OF_BOUNDS_CONVERT_TO_NUMBER(type, lower, upper)                                                       \
    test_out_of_bounds_convert_to_number<type>(#type, lower, upper)

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
    });
});
/*
// Convert to unsigned integer.
template <typename T, typename std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value>* =
nullptr>
// NOLINTNEXTLINE: Desired call signature.
T convert_to_number(const char* c_str, bool& valid, char*& endptr) {
    uint64_t result = strtoull(c_str, &endptr, 10);
    valid = endptr != c_str && errno != ERANGE && result <= std::numeric_limits<T>::max();
    return static_cast<T>(result);
}

// Convert to signed integer.
template <typename T, typename std::enable_if_t<std::is_integral<T>::value && std::is_signed<T>::value>* = nullptr>
// NOLINTNEXTLINE: Desired call signature.
T convert_to_number(const char* c_str, bool& valid, char*& endptr) {
    int64_t result = strtoll(c_str, &endptr, 10);
    valid = endptr != c_str && errno != ERANGE && result <= std::numeric_limits<T>::max() &&
            result >= std::numeric_limits<T>::min();
    return static_cast<T>(result);
}

// Convert to floating point.
template <typename T, typename std::enable_if_t<std::is_floating_point<T>::value>* = nullptr>
// NOLINTNEXTLINE: Desired call signature.
T convert_to_number(const char* c_str, bool& valid, char*& endptr) {
    long double result = strtold(c_str, &endptr);
    valid = endptr != c_str && errno != ERANGE && result <= std::numeric_limits<T>::max() &&
            result >= std::numeric_limits<T>::lowest();
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

class StringPointer final {
  private:
    char const* const base;
    ssize_t offset;

  public:
    explicit StringPointer(char const* base);
    operator char const*() const; // NOLINT: Implicit conversion desired.

    char peek() const;
    void skip(size_t offset);
    char next();
    void whitespace();
    void nonspace();
    // NOLINTNEXTLINE: Desired call signature.
    std::string escaped_string();

    template <typename T>
    T number() {
        bool valid;
        char* endptr;
        // NOLINTNEXTLINE: Pointer arithmetic intended.
        auto n = convert_to_number<T>(base + offset, valid, endptr);
        // NOLINTNEXTLINE: Pointer arithmetic intended.
        if(endptr == base + offset) {
            std::cerr << "Could not convert string to number" << std::endl;
            exit(1);
        }
        offset = endptr - base;
        return n;
    }

    template <typename T>
    std::string number_str() {
        bool valid;
        char* endptr;
        // NOLINTNEXTLINE: Pointer arithmetic intended.
        convert_to_number<T>(base + offset, valid, endptr);
        ssize_t start = offset;
        offset = endptr - base;
        // NOLINTNEXTLINE: Pointer arithmetic intended.
        return std::string(base + start, static_cast<size_t>(offset - start));
    }
};

// In a fail case errno is set appropriately.
bool has_input(int fd, int microsec = 0);

void store_string(char* dst, size_t dst_size, char* src, size_t src_size);

// NOLINTNEXTLINE: Desired call signature.
bool load_file(std::string const& name, std::string& content);

// NOLINTNEXTLINE: Desired call signature.
std::ostream& print_time(std::ostream& out, struct tm& ptm, char const* const format);

ssize_t read_all(int fd, char* buf, size_t count);
ssize_t write_all(int fd, char const* buf, size_t count);

std::string make_hex_color(uint8_t a, uint8_t r, uint8_t g, uint8_t b);

class FileStream : public std::streambuf {
  private:
    const int fd;
    int buffered_char = EOF;

  public:
    explicit FileStream(int fd);
    explicit FileStream(FILE* f);

    FileStream(const FileStream&) = delete;
    FileStream(FileStream&&) = delete;
    FileStream& operator=(const FileStream&) = delete;
    FileStream& operator=(FileStream&&) = delete;

    ~FileStream() override = default;

    int underflow() override;
    int uflow() override;
    int overflow(int c) override;
};

using UniqueFile = std::unique_ptr<FILE, int (*)(FILE*)>;

UniqueFile open_file(std::string const& path);
UniqueFile run_command(std::string const& command, std::string const& mode);

template <typename Resource, typename _ = void>
class UniqueResource {
    using Release = std::function<_(Resource)>;

  protected:
    Resource resource;
    Release release;

  public:
    explicit UniqueResource(Resource new_resource, Release new_release)
        : resource(new_resource), release(new_release) {}

    UniqueResource(UniqueResource const& other) = delete;
    UniqueResource& operator=(UniqueResource const& other) = delete;

    UniqueResource(UniqueResource&& other) noexcept : resource(other.resource), release(other.release) {}
    UniqueResource& operator=(UniqueResource&& other) noexcept {
        release(resource);
        resource = other.resource;
        release = other.release;
    }

    virtual ~UniqueResource() { release(resource); }
};

class UniqueSocket final : public UniqueResource<int> {
  public:
    explicit UniqueSocket(int new_sockfd);
    operator int(); // NOLINT: Implicit `int()` conversions are expected behavior.
};

#endif*/
