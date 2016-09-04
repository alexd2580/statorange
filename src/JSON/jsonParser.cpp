/**
 * ONLY FOR PRIVATE USE! UNREFINED AND FAILING EVERYWHERE!
 * ESPECIALLY AT NON-VALID JSON!
 * IF YOU FIND BUGS YOU CAN KEEP 'EM!
 * I WILL EVENTUALLY (MAYBE) FIX THIS!
 */

#include "jsonParser.hpp"
#include "../util.hpp"
#include "JSONException.hpp"

#include <map>
#include <vector>

#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace std;

/******************************************************************************/

class JSONArray : public JSON
{
private:
  std::vector<std::unique_ptr<JSON>> elems;

public:
  JSONArray(TextPos& pos)
  {
    try
    {
      char c = *pos;
      if(c != '[')
        throw JSONException("Array not starting at the given position");

      while(true)
      {
        c = *pos;
        size_t esize = elems.size();
        if((esize == 0 && c == '[') || (esize > 0 && c == ','))
        {
          (void)pos.next();
          pos.skip_whitespace();
          if(*pos == ']' && esize == 0)
            break;
          try
          {
            elems.push_back(JSON::parseJSON(pos));
          }
          catch(TraceCeption& e)
          {
            e.push_stack("While parsing array element #" +
                         std::to_string(elems.size()));
            throw e;
          }
          pos.skip_whitespace();
        }
        else if(c == ']')
          break;
        else if(c == '\0')
          throw JSONException(pos, "Unexpected end of str");
        else
          throw JSONException(pos,
                              std::string("Unexpected symbol: ") + (char)c);
      }
      (void)pos.next(); // no need to check closing ] or \0
    }
    catch(TraceCeption& e)
    {
      e.push_stack("While parsing array");
      throw e;
    }
  }

  virtual ~JSONArray(void) = default;
  std::string get_type(void) const { return "array"; }

  virtual void print(size_t indention) const
  {
    cout << '[' << endl << std::string(indention + INDENT_WIDTH, ' ');
    if(elems.size() > 0)
      elems[0]->print(indention + INDENT_WIDTH);
    for(size_t i = 1; i < elems.size(); i++)
    {
      cout << ',' << endl << std::string(indention + INDENT_WIDTH, ' ');
      elems[i]->print(indention + INDENT_WIDTH);
    }
    cout << endl << std::string(indention, ' ') << ']';
  }

  JSON& get(size_t i) const
  {
    if(i > elems.size())
      throw JSONException("Array index out of bounds: " + ::to_string(i));
    return *elems[i];
  }

  JSON& operator[](size_t i) const { return get(i); }

  size_t size(void) const { return elems.size(); }

  string to_string(void) const { return "JSONArray"; }
};

/******************************************************************************/

void printNamed(map<std::string, std::unique_ptr<JSON>>::const_iterator i,
                size_t indention)
{
  cout << '"' << i->first << "\" : ";
  i->second->print(indention);
}

class JSONObject : public JSON
{
private:
  std::map<std::string, std::unique_ptr<JSON>> fields;

  void parseNamed(TextPos& pos)
  {
    std::string key;
    try
    {
      key.assign(pos.parse_escaped_string());
      pos.skip_whitespace();
      char c = *pos;
      if(c != ':')
        throw JSONException(pos, std::string("Expected ':', got: ") + (char)c);
      (void)pos.next(); // skip :
    }
    catch(TraceCeption& e)
    {
      e.push_stack("While parsing a field name of an object");
      throw e;
    }

    try
    {
      pos.skip_whitespace();
      std::unique_ptr<JSON> something = JSON::parseJSON(pos);
      std::pair<std::string, std::unique_ptr<JSON>> kvpair(
          std::move(key), std::move(something));
      fields.insert(std::move(kvpair));
    }
    catch(TraceCeption& e)
    {
      e.push_stack("While trying to parse the field with the key \"" + key +
                   "\".");
      throw e;
    }
  }

public:
  JSONObject(TextPos& pos)
  {
    try
    {
      char c = *pos;
      if(c != '{')
        throw JSONException(pos, std::string("Expected '{', got: ") + (char)c);

      while(true)
      {
        c = *pos;
        size_t field_size = fields.size();
        if((field_size == 0 && c == '{') || (field_size > 0 && c == ','))
        {
          pos.next();
          pos.skip_whitespace();
          if(*pos == '}' && field_size == 0)
            break;
          parseNamed(pos);
          pos.skip_whitespace();
        }
        else if(c == '}')
          break;
        else if(c == '\0')
          throw JSONException(pos, "Unexpected end of string");
        else
          throw JSONException(pos,
                              std::string("Unexpected symbol: ") + (char)c);
      }

      (void)pos.next(); // skip closing brace
    }
    catch(TraceCeption& e)
    {
      e.push_stack("While parsing JSONObject");
      throw e;
    }
  }

  virtual ~JSONObject(void) = default;
  virtual std::string get_type(void) const { return "object"; }

  virtual void print(size_t indention) const
  {
    cout << '{' << endl << std::string(indention + INDENT_WIDTH, ' ');

    auto i = fields.begin();
    size_t field_size = fields.size();
    if(field_size > 0)
      printNamed(i, indention + INDENT_WIDTH);
    i++;
    for(; i != fields.end(); i++)
    {
      cout << ',' << endl << std::string(indention + INDENT_WIDTH, ' ');
      printNamed(i, indention + INDENT_WIDTH);
    }
    cout << endl << std::string(indention, ' ') << '}';
  }

  bool has(cchar* key) const
  {
    string skey(key);
    return has(skey);
  }

  bool has(string const& key) const
  {
    auto it = fields.find(key);
    return it != fields.end();
  }

  JSON& get(cchar* key) const
  {
    std::string skey(key);
    return get(skey);
  }

  JSON& operator[](cchar* key) const
  {
    std::string skey(key);
    return get(skey);
  }

  JSON& get(std::string const& key) const
  {
    auto it = fields.find(key);
    if(it == fields.end())
      throw JSONException(std::string("Element ") + key + " not found");
    return *it->second;
  }

  JSON& operator[](std::string& key) const { return get(key); }

  string to_string(void) const { return "JSONObject"; }
};

/******************************************************************************/

class JSONString : public JSON
{
private:
  string string_;

public:
  JSONString(TextPos& pos)
  {
    try
    {
      string_.assign(pos.parse_escaped_string());
    }
    catch(TraceCeption& e)
    {
      e.push_stack("While parsing JSONString");
      throw e;
    }
  }

  virtual ~JSONString(void) = default;

  virtual string get_type(void) const { return "string"; }

  virtual void print(size_t) const { cout << '"' << string_ << '"'; }

  __attribute__((const)) operator string&() { return string_; }

  string to_string(void) const { return string_; }
};

/******************************************************************************/

class JSONNumber : public JSON
{
private:
  double n;

public:
  JSONNumber(TextPos& pos)
  {
    try
    {
      n = pos.parse_num();
    }
    catch(TraceCeption& e) // TODO chage to generic exceptions?
    {
      e.push_stack("While parsing JSONNumber");
      throw e;
    }
  }

  virtual ~JSONNumber(void) = default;

  std::string get_type(void) const { return "num"; }

  virtual void print(size_t) const { cout << n; }

  __attribute__((pure)) virtual operator uint8_t() { return (uint8_t)n; }
  __attribute__((pure)) virtual operator int() { return (int)n; }
  __attribute__((pure)) virtual operator unsigned int()
  {
    return (unsigned int)n;
  }
  __attribute__((pure)) virtual operator long() { return (long)n; }
  __attribute__((pure)) virtual operator double() { return n; }

  string to_string(void) const { return ::to_string(n); }
};

/******************************************************************************/

class JSONBool : public JSON
{
private:
  bool b;

public:
  JSONBool(TextPos& pos)
  {
    cchar* str = pos.ptr();
    if(strncmp(str, "false", 5) == 0)
    {
      b = false;
      pos.offset(5);
    }
    else if(strncmp(str, "true", 4) == 0)
    {
      b = true;
      pos.offset(4);
    }
    else
    {
      std::string errmsg("Could not detect neither true nor false: ");
      throw JSONException(pos, errmsg + std::string(str, 5));
    }
  }

  virtual ~JSONBool(void) = default;

  std::string get_type(void) const { return "bool"; }

  virtual void print(size_t) const { cout << (b ? "true" : "false"); }

  __attribute__((pure)) virtual operator bool() { return b; }

  string to_string(void) const { return ::to_string(b); }
};

/******************************************************************************/

class JSONNull : public JSON
{
public:
  JSONNull(TextPos& pos)
  {
    cchar* str = pos.ptr();
    if(strncmp(str, "null", 4) == 0)
      pos.offset(4);
    else
      throw JSONException(pos, "Could not detect null: " + std::string(str, 4));
  }

  virtual ~JSONNull(void) = default;

  virtual std::string get_type(void) const { return "null"; }

  virtual void print(size_t) const { cout << "null"; }

  string to_string(void) const { return "null"; }
};

/******************************************************************************/

/**
 * Parses the first JSONSomthing it encounters,
 * string is set to the next (untouched) character.
 */
std::unique_ptr<JSON> JSON::parse(cchar* str)
{
  TextPos pos(str);
  return parseJSON(pos);
}

std::unique_ptr<JSON> JSON::parseJSON(TextPos& pos)
{
  pos.skip_whitespace();
  char c = *pos;
  switch(c)
  {
  case '\0':
    throw JSONException("Unexpected end of string");
  case '"':
    return std::make_unique<JSONString>(pos);
  case '{':
    return std::make_unique<JSONObject>(pos);
  case '[':
    return std::make_unique<JSONArray>(pos);
  default:
    if((c >= '0' && c <= '9') || c == '.' || c == '+' || c == '-')
      return std::make_unique<JSONNumber>(pos);
    else if(c == 't' || c == 'f')
      return std::make_unique<JSONBool>(pos);
    else if(c == 'n')
      return std::make_unique<JSONNull>(pos);
    break;
  }

  std::string errmsg("No valid JSON detected: ");
  throw JSONException(pos, errmsg + c);
}

void JSON::print(void) const
{
  print(0);
  cout << endl;
}

// JSONArray
JSON& JSON::get(size_t) const
{
  string errmsg("Cannot access " + get_type() +
                " type using array subscription");
  throw JSONException(errmsg);
}
JSON& JSON::operator[](size_t) const { return get((size_t)0); }

size_t JSON::size(void) const
{
  string errmsg("Cannot access size of " + get_type() + " type");
  throw JSONException(errmsg);
}

// JSONObject
JSON& JSON::get(cchar*) const
{
  string errmsg("Cannot access " + get_type() + " type as a dictionary");
  throw JSONException(errmsg);
}
JSON& JSON::operator[](cchar*) const { return get((cchar*)nullptr); }
JSON& JSON::get(std::string const&) const { return get((cchar*)nullptr); }
JSON& JSON::operator[](std::string const&) const
{
  return get((cchar*)nullptr);
}

bool JSON::has(cchar*) const
{
  string errmsg("Cannot access " + get_type() + " type as a dictionary");
  throw JSONException(errmsg);
}
bool JSON::has(std::string const&) const { return has((cchar*)nullptr); }

// JSONString
JSON::operator std::string&()
{
  string errmsg("Cannot convert " + get_type() + " type to 'string'");
  throw JSONException(errmsg);
}

// JSONNumber
JSON::operator uint8_t()
{
  string errmsg("Cannot convert " + get_type() + " type to 'uint8_t'");
  throw JSONException(errmsg);
}
JSON::operator int()
{
  string errmsg("Cannot convert " + get_type() + " type to 'int'");
  throw JSONException(errmsg);
}
JSON::operator unsigned int()
{
  string errmsg("Cannot convert " + get_type() + " type to 'unsigned int'");
  throw JSONException(errmsg);
}
JSON::operator long()
{
  string errmsg("Cannot convert " + get_type() + " type to 'long'");
  throw JSONException(errmsg);
}
JSON::operator double()
{
  string errmsg("Cannot convert " + get_type() + " type to 'double'");
  throw JSONException(errmsg);
}

// JSONBool
JSON::operator bool()
{
  string errmsg("Cannot convert " + get_type() + " type to 'bool'");
  throw JSONException(errmsg);
}

/******************************************************************************/

void test_json(void)
{
  std::string jsonstring = "{\"asd\":\"asd\", \"qwe\":123}";
  try
  {
    auto json_ptr = JSON::parse(jsonstring.c_str());
    auto& json = *json_ptr;
    std::string asd(json["asd"]);
    std::string qwe(json["qwe"]);
  }
  catch(TraceCeption& e)
  {
    e.printStackTrace(cout);
  }
}
