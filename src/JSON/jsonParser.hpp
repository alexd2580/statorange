#ifndef __COMPACT_JSON_PARSER__
#define __COMPACT_JSON_PARSER__

#include <string>
#include <memory>

#include "../util.hpp"

class JSON
{
protected:
  static std::unique_ptr<JSON> parseJSON(TextPos& string);

public:
  static std::unique_ptr<JSON> parse(char const* string);

  virtual ~JSON(void) = default;
  virtual std::string get_type(void) const = 0;

  virtual void print(size_t indention) const = 0;
  virtual void print(void) const;
  // return string representation. valid on all types.
  virtual std::string to_string(void) const = 0;

  // JSONArray
  virtual JSON& get(size_t i) const;
  virtual JSON& operator[](size_t) const;
  virtual size_t size(void) const;

  // JSONObject
  virtual bool has(cchar*) const;
  virtual bool has(std::string const&) const;
  virtual JSON& get(cchar*) const;
  virtual JSON& operator[](cchar*) const;
  virtual JSON& get(std::string const&) const;
  virtual JSON& operator[](std::string const&) const;

  // JSONString
  virtual operator std::string&();

  // JSONNumber
  virtual operator uint8_t();
  virtual operator int();
  virtual operator unsigned int();
  virtual operator long();
  virtual operator double();

  // JSONBool
  virtual operator bool();

  // JSONNull
};

#define INDENT_WIDTH 2

void test_json(void);

#endif
