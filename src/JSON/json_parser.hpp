#ifndef __COMPACT_JSON_PARSER__
#define __COMPACT_JSON_PARSER__

#include <memory>
#include <string>
#include <vector>

#include "../util.hpp"

class JSON
{
  public:
    // Parses the JSON `string`.
    static std::unique_ptr<JSON> parse(char const*& string);

    virtual ~JSON(void) = default;

    // Methods for operating on JSON null values.
    virtual bool is_null(void) const;

    // Get the string representation of the type of this JSON node.
    virtual std::string get_type(void) const = 0;

    // Print the parsed JSON with a base `indentation`.
    virtual void print(size_t indention) const = 0;

    // Convenience method. Equivalent to `print(0)`.
    virtual void print(void) const;

    // Get the `string` representation of this node. Valid on all types.
    virtual std::string to_string(void) const = 0;

    // Methods for operating on JSON arrays.
    // Get the `i`-th node immutably from the array. Returns a null-node
    // reference
    // if the index is out of bounds.
    virtual JSON const& get(size_t i) const;
    // Same as `get(size_t i)`, but failing if the index is OOB.
    virtual JSON const& operator[](size_t) const;
    // Get the size of the array.
    virtual size_t size(void) const;
    virtual std::vector<std::unique_ptr<JSON>> const& as_vector(void) const;

    // Methods for accessing JSON objects.
    // Check whether the `key` is in the JSON.
    virtual bool has(char const* key) const;
    virtual bool has(std::string const& key) const;
    // Get an immutable reference the the node with the key `key`.
    // Returns a null-node reference if the index is out of bounds.
    virtual JSON const& get(char const* key) const;
    virtual JSON const& get(std::string const& key) const;
    // Same as `get(char const* key)` and `get(std::string const& key)`, but
    // failing if the `key` is not present.
    virtual JSON const& operator[](char const* key) const;
    virtual JSON const& operator[](std::string const& key) const;

    // The following are conversion methods for different types. The regular
    // conversion operators fail if a conversion is not possible due to any
    // reason. The `as_*_with_default(* default_value)` methods return
    // `default_value` instead of failing.

    // Methods for operating on JSON strings.
    virtual operator std::string const&() const;
    virtual std::string const&
    as_string_with_default(std::string const& default_value) const;

    // Methods for operating on JSON numbers.
    virtual operator uint8_t() const;
    virtual uint8_t as_uint8_t_with_default(uint8_t default_value) const;
    virtual operator int() const;
    virtual int as_int_with_default(int default_value) const;
    virtual operator unsigned int() const;
    virtual unsigned as_unsigned_with_default(unsigned default_value) const;
    virtual operator long() const;
    virtual long as_long_with_default(long default_value) const;
    virtual operator double() const;
    virtual double as_double_with_default(double default_value) const;

    // Methods for operating on JSON booleans.
    virtual operator bool() const;
    virtual bool as_bool_with_default(bool default_value) const;
};

// A Basic test.
void test_json(void);

#endif
