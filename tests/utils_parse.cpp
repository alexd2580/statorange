#include <string>
#include <vector>

#include <cmath>

#include <bandit/bandit.h>

#include "utils/parse.hpp"

using namespace bandit;
using namespace snowhouse;

go_bandit([] {
    describe("StringPointer", [] {
        describe("constructor", [] {
            it("throws when supplying `nullptr`", [] {
                AssertThrows(ParseException, StringPointer(nullptr));
                AssertThat(LastException<ParseException>().what(),
                           Equals("Cannot create `StringPointer` to `nullptr`."));
            });
            it("successfully creates new `StringPointer`s", [] {
                std::vector<char const*> values{"", "asdasd", "The lazy dog jumps over the fox"};
                for(auto const& value : values) {
                    StringPointer sp(value);
                }
            });
        });
        describe("operator char const*", [] {
            it("returns a pointer to the start of the supplied string", [] {
                std::vector<char const*> values{"", "asdasd", "The lazy dog jumps over the fox"};
                for(auto const& value : values) {
                    StringPointer sp(value);
                    char const* result(sp);
                    AssertThat(result, Equals(value));
                }
            });
        });
        describe("peek", [] {
            it("returns the first letter of the supplied string", [] {
                std::vector<char const*> values{"", "asdasd", "The lazy dog jumps over the fox"};
                for(auto const& value : values) {
                    StringPointer sp(value);
                    AssertThat(sp.peek(), Equals(value[0]));
                }
            });
        });
        describe("skip", [] {
            it("offsets the internal pointer by the supplied number of characters", [] {
                std::vector<char const*> values{"", "asdasd", "The lazy dog jumps over the fox"};
                for(auto const& value : values) {
                    StringPointer sp(value);
                    sp.skip(4);
                    char const* result(sp);
                    AssertThat(result, Equals(value + 4));
                }
            });
        });
        describe("next", [] {
            it("returns the current character and offsets the internal pointer by 1", [] {
                std::vector<char const*> values{"1", "asdasd", "The lazy dog jumps over the fox"};
                for(auto const& value : values) {
                    StringPointer sp(value);
                    AssertThat(sp.next(), Equals(value[0]));
                    AssertThat(sp.next(), Equals(value[1]));
                    char const* result(sp);
                    AssertThat(result, Equals(value + 2));
                }
            });
        });
        describe("whitespace", [] {
            it("skips whitespace characters", [] {
                StringPointer sp("a \n\t\n b   c\n\n\nd\t\t\tef");
                AssertThat(sp.next(), Equals('a'));
                sp.whitespace();
                AssertThat(sp.next(), Equals('b'));
                sp.whitespace();
                AssertThat(sp.next(), Equals('c'));
                sp.whitespace();
                AssertThat(sp.next(), Equals('d'));
                sp.whitespace();
                AssertThat(sp.next(), Equals('e'));
                sp.whitespace();
                AssertThat(sp.next(), Equals('f'));
                sp.whitespace();
                char const* first_0(sp);
                AssertThat(sp.peek(), Equals('\0'));
                sp.whitespace();
                char const* second_0(sp);
                AssertThat(sp.peek(), Equals('\0'));
                AssertThat(first_0, Equals(second_0));
            });
        });
        describe("nonspace", [] {
            it("skips non-whitespace characters", [] {
                StringPointer sp(" abcdefg\tabc.,:\n123456.654321 \n");
                AssertThat(sp.next(), Equals(' '));
                sp.nonspace();
                AssertThat(sp.next(), Equals('\t'));
                sp.nonspace();
                AssertThat(sp.next(), Equals('\n'));
                sp.nonspace();
                AssertThat(sp.next(), Equals(' '));
                sp.nonspace();
                AssertThat(sp.next(), Equals('\n'));
                sp.nonspace();
                char const* first_0(sp);
                AssertThat(sp.peek(), Equals('\0'));
                sp.nonspace();
                char const* second_0(sp);
                AssertThat(sp.peek(), Equals('\0'));
                AssertThat(first_0, Equals(second_0));
            });
        });
        describe("escaped_string", [] {
            it("parses escaped strings", [] {
                AssertThat(StringPointer(R"("")").escaped_string(), Equals(""));
                AssertThat(StringPointer(R"("abcdef")").escaped_string(), Equals("abcdef"));
                AssertThat(StringPointer(R"("abc\r\ndef")").escaped_string(), Equals("abc\r\ndef"));
                AssertThat(StringPointer(R"("abc\"\"def")").escaped_string(), Equals(R"(abc""def)"));
                StringPointer sp(R"("abc""def")");
                AssertThat(sp.escaped_string(), Equals("abc"));
                AssertThat(sp.escaped_string(), Equals("def"));
            });
            it(R"(throws when the input doesn't start with a ")", [] {
                AssertThrows(ParseException, StringPointer("").escaped_string());
                AssertThat(LastException<ParseException>().what(), Equals(R"(Expected ["], got [)"));
                AssertThrows(ParseException, StringPointer(R"(a "string")").escaped_string());
                AssertThat(LastException<ParseException>().what(), Equals(R"(Expected ["], got [a].)"));
            });
            it(R"(throws when the input doesn't end with a ")", [] {
                AssertThrows(ParseException, StringPointer(R"(")").escaped_string());
                AssertThat(LastException<ParseException>().what(), Equals("Unexpected EOS."));
                AssertThrows(ParseException, StringPointer(R"("a string)").escaped_string());
                AssertThat(LastException<ParseException>().what(), Equals("Unexpected EOS."));
            });
            it("throws when the escaped string contains invalid escape sequences", [] {
                ;
                AssertThrows(ParseException, StringPointer(R"("\q")").escaped_string());
                AssertThat(LastException<ParseException>().what(), Equals(R"(Invalid escape sequence [\q].)"));
                AssertThrows(ParseException, StringPointer(R"("\%")").escaped_string());
                AssertThat(LastException<ParseException>().what(), Equals(R"(Invalid escape sequence [\%].)"));
            });
            it("throws when the escaped string terminates prematurely", [] {
                AssertThrows(ParseException, StringPointer(R"("\)").escaped_string());
                AssertThat(LastException<ParseException>().what(),
                           Equals("Unexpected EOS when parsing escaped character."));
            });
        });
        describe("number", [] {
            it("parses valid numeric strings of proper type", [] {
                StringPointer sp("123456.654321.123123X");
                AssertThat(sp.number<int32_t>(), Equals(123456));
                AssertThat(sp.next(), Equals('.'));
                AssertThat(sp.number<double>(), Equals(654321.123123));
                AssertThat(sp.next(), Equals('X'));
            });
            it("parses a valid numeric string at EOS", [] {
                AssertThat(StringPointer("123456").number<uint64_t>(), Equals(123456));
                float result = StringPointer("123456.654321").number<float>();
                AssertThat(result, EqualsWithDelta(123456.654321f, 0.000001f));
            });
            it("throws when string is not numeric", [] {
                StringPointer sp("a123b");
                AssertThrows(ParseException, sp.number<int16_t>());
                AssertThat(LastException<ParseException>().what(), Equals("Could not convert string to number."));
                AssertThrows(ParseException, sp.number<double>());
                AssertThat(LastException<ParseException>().what(), Equals("Could not convert string to number."));
            });
        });
        describe("number_str", [] {
            it("parses valid numeric strings of proper type", [] {
                StringPointer sp("123456.654321.123123X");
                AssertThat(sp.number_str<int32_t>(), Equals("123456"));
                AssertThat(sp.next(), Equals('.'));
                AssertThat(sp.number_str<double>(), Equals("654321.123123"));
                AssertThat(sp.next(), Equals('X'));
            });
            it("parses a valid numeric string at EOS", [] {
                AssertThat(StringPointer("123456").number_str<uint64_t>(), Equals("123456"));
                AssertThat(StringPointer("123456.654321").number_str<float>(), Equals("123456.654321"));
            });
            it("throws when string is not numeric", [] {
                StringPointer sp("a123b");
                AssertThrows(ParseException, sp.number_str<int16_t>());
                AssertThat(LastException<ParseException>().what(), Equals("Could not convert string to number."));
                AssertThrows(ParseException, sp.number_str<double>());
                AssertThat(LastException<ParseException>().what(), Equals("Could not convert string to number."));
            });
        });
    });
});

/*
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
*/
/*

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
