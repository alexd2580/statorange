/**
 * ONLY FOR PRIVATE USE! UNREFINED AND FAILING EVERYWHERE!
 * ESPECIALLY AT NON-VALID JSON!
 * IF YOU FIND BUGS YOU CAN KEEP 'EM!
 * I WILL EVENTUALLY (MAYBE) FIX THIS!
 */

#include <cstdlib>
#include <cstring>
#include <iostream>

#include "jsonParser.hpp"
#include "../util.hpp"

using namespace std;

/******************************************************************************/

JSONArray::JSONArray(TextPos& pos)
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
        throw JSONException(pos, std::string("Unexpected symbol: ") + (char)c);
    }
    (void)pos.next(); // no need to check closing ] or \0
  }
  catch(TraceCeption& e)
  {
    e.push_stack("While parsing array");
    throw e;
  }
}

JSONArray::~JSONArray()
{
  for(auto i = elems.begin(); i != elems.end(); i++)
    delete *i;
}

std::string JSONArray::get_type(void) { return "array"; }

void JSONArray::print(size_t indention)
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

JSON& JSONArray::get(size_t i)
{
  if(i > elems.size())
    throw JSONException("Array index out of bounds: " + to_string(i));
  return *elems[i];
}

JSON& JSONArray::operator[](size_t i) { return get(i); }

__attribute__((pure)) size_t JSONArray::size(void) { return elems.size(); }

/******************************************************************************/

void JSONObject::parseNamed(TextPos& pos)
{
  std::string key;
  try
  {
    key.assign(parse_escaped_string(pos));
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
    JSON* something = JSON::parseJSON(pos);
    fields.insert(std::pair<std::string, JSON*>(key, something));
  }
  catch(TraceCeption& e)
  {
    e.push_stack("While trying to parse the field with the key \"" + key +
                 "\".");
    throw e;
  }
}

/******************************************************************************/

void printNamed(map<std::string, JSON*>::iterator i, size_t indention)
{
  cout << '"' << i->first << "\" : ";
  i->second->print(indention);
}

JSONObject::JSONObject(TextPos& pos)
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
        throw JSONException(pos, std::string("Unexpected symbol: ") + (char)c);
    }

    (void)pos.next(); // skip closing brace
  }
  catch(TraceCeption& e)
  {
    e.push_stack("While parsing JSONObject");
    throw e;
  }
}

JSONObject::~JSONObject()
{
  for(auto i = fields.begin(); i != fields.end(); i++)
    delete i->second;
}

std::string JSONObject::get_type(void) { return "object"; }

void JSONObject::print(size_t indention)
{
  cout << '{' << endl << std::string(indention + INDENT_WIDTH, ' ');

  auto i = fields.begin();
  size_t field_size = fields.size();
  if(field_size > 0)
    printNamed(i, indention + INDENT_WIDTH);
  for(; i != fields.end(); i++)
  {
    cout << ',' << endl << std::string(indention + INDENT_WIDTH, ' ');
    printNamed(i, indention + INDENT_WIDTH);
  }
  cout << endl << std::string(indention, ' ') << '}';
}

JSON* JSONObject::has(cchar* key)
{
  std::string skey(key);
  return has(skey);
}

JSON* JSONObject::has(std::string& key)
{
  auto it = fields.find(key);
  return (it == fields.end() ? nullptr : it->second);
}

JSON& JSONObject::get(cchar* key)
{
  std::string skey(key);
  return get(skey);
}

JSON& JSONObject::operator[](cchar* key)
{
  std::string skey(key);
  return get(skey);
}

JSON& JSONObject::get(std::string& key)
{
  auto it = fields.find(key);
  if(it == fields.end())
    throw JSONException(std::string("Element ") + key + " not found");
  return *(it->second);
}

JSON& JSONObject::operator[](std::string& key) { return get(key); }

/******************************************************************************/

JSONString::JSONString(TextPos& pos)
{
  try
  {
    string.assign(parse_escaped_string(pos));
  }
  catch(TraceCeption& e)
  {
    e.push_stack("While parsing JSONString");
    throw e;
  }
}

JSONString::~JSONString() {}

std::string JSONString::get_type(void) { return "string"; }

void JSONString::print(size_t) { cout << string; }

string JSONString::get(void) { return string; }

__attribute__((const)) JSONString::operator std::string&() { return string; }

/******************************************************************************/

JSONNumber::JSONNumber(TextPos& pos)
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

std::string JSONNumber::get_type(void) { return "num"; }

void JSONNumber::print(size_t) { cout << n; }

__attribute__((pure)) JSONNumber::operator uint8_t() { return (uint8_t)n; }

__attribute__((pure)) JSONNumber::operator int() { return (int)n; }

__attribute__((pure)) JSONNumber::operator unsigned int()
{
  return (unsigned int)n;
}

__attribute__((pure)) JSONNumber::operator long() { return (long)n; }

__attribute__((pure)) JSONNumber::operator double() { return n; }

/******************************************************************************/

JSONBool::JSONBool(TextPos& pos)
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

std::string JSONBool::get_type(void) { return "bool"; }

void JSONBool::print(size_t) { cout << b; }

__attribute__((pure)) JSONBool::operator bool() { return b; }

/******************************************************************************/

JSONNull::JSONNull(TextPos& pos)
{
  cchar* str = pos.ptr();
  if(strncmp(str, "null", 4) == 0)
    pos.offset(4);
  else
    throw JSONException(pos, "Could not detect null: " + std::string(str, 4));
}

std::string JSONNull::get_type(void) { return "null"; }

void JSONNull::print(size_t) { cout << "null"; }

/******************************************************************************/

/**
 * Parses the first JSONSomthing it encounters,
 * string is set to the next (untouched) character.
 */
JSON* JSON::parse(cchar* str)
{
  TextPos pos(str);
  return parseJSON(pos);
}

JSON* JSON::parseJSON(TextPos& pos)
{
  pos.skip_whitespace();
  char c = *pos;
  switch(c)
  {
  case '\0':
    throw JSONException("Unexpected end of string");
  case '"':
    return new JSONString(pos);
  case '{':
    return new JSONObject(pos);
  case '[':
    return new JSONArray(pos);
  default:
    if((c >= '0' && c <= '9') || c == '.' || c == '+' || c == '-')
      return new JSONNumber(pos);
    else if(c == 't' || c == 'f')
      return new JSONBool(pos);
    else if(c == 'n')
      return new JSONNull(pos);
    break;
  }

  std::string errmsg("No valid JSON detected: ");
  throw JSONException(pos, errmsg + c);
}

void JSON::print(void)
{
  print(0);
  cout << endl;
}

JSONArray& JSON::array(void)
{
  JSONArray* ptr = dynamic_cast<JSONArray*>(this);
  if(ptr == nullptr)
  {
    JSONException e(std::string("This JSON instance is of type ") + get_type());
    e.push_stack("Cannot cast to JSONArray");
    throw e;
  }
  return *ptr;
}

JSONObject& JSON::object(void)
{
  JSONObject* ptr = dynamic_cast<JSONObject*>(this);
  if(ptr == nullptr)
  {
    JSONException e(std::string("This JSON instance is of type ") + get_type());
    e.push_stack("Cannot cast to JSONObject");
    throw e;
  }
  return *ptr;
}

JSONString& JSON::string(void)
{
  JSONString* ptr = dynamic_cast<JSONString*>(this);
  if(ptr == nullptr)
  {
    JSONException e(std::string("This JSON instance is of type ") + get_type());
    e.push_stack("Cannot cast to JSONString");
    throw e;
  }
  return *ptr;
}

JSONNumber& JSON::number(void)
{
  JSONNumber* ptr = dynamic_cast<JSONNumber*>(this);
  if(ptr == nullptr)
  {
    JSONException e(std::string("This JSON instance is of type ") + get_type());
    e.push_stack("Cannot cast to JSONNumber");
    throw e;
  }
  return *ptr;
}

JSONBool& JSON::boolean(void)
{
  JSONBool* ptr = dynamic_cast<JSONBool*>(this);
  if(ptr == nullptr)
  {
    JSONException e(std::string("This JSON instance is of type ") + get_type());
    e.push_stack("Cannot cast to JSONBool");
    throw e;
  }
  return *ptr;
}

JSONNull& JSON::null(void)
{
  JSONNull* ptr = dynamic_cast<JSONNull*>(this);
  if(ptr == nullptr)
  {
    JSONException e(std::string("This JSON instance is of type ") + get_type());
    e.push_stack("Cannot cast to JSONNull");
    throw e;
  }
  return *ptr;
}

/******************************************************************************/

void test_json(void)
{
  std::string jsonstring = "{\"asd\":\"asd\", \"qwe\":123}";
  try
  {
    JSON* json = JSON::parse(jsonstring.c_str());
    (void)json->object()["asd"].string();
    (void)json->object()["qwe"].string();
    delete json;
  }
  catch(TraceCeption& e)
  {
    e.printStackTrace(cout);
  }
}
