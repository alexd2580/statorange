/**
 * ONLY FOR PRIVATE USE! UNREFINED AND FAILING EVERYWHERE!
 * ESPECIALLY FOR NON-VALID JSON!
 * IF YOU FIND BUGS YOU CAN KEEP 'EM!
 * I WILL EVENTUALLY (MAYBE) FIX THIS!
 */

#include<cstdlib>
#include<cstring>
#include<iostream>

#include"jsonParser.hpp"
#include"util.hpp"

using namespace std;

/******************************************************************************/

JSONArray::JSONArray(cchar*& str)
{
  if(*str != '[')
    throw JSONException("Array not starting at the given position");
  
  char c = '\0';
  while(true)
  {
    c = *str;
    if(elems.size() == 0 && c== '[')
    {
      str++;
      skipWhitespaces(str);
      if(*str == ']')
        break;
      elems.push_back(JSON::parseJSON(str));
      skipWhitespaces(str);
    }
    else if(elems.size() > 0 && c == ',')
    {
      str++; //skip komma
      skipWhitespaces(str);
      elems.push_back(JSON::parseJSON(str));
      skipWhitespaces(str);
    }
    else if(c == ']')
      break;
    else if(c == '\0')
      throw JSONException("Unexpected end of str");
    else
      throw JSONException(std::string("Unexpected symbol: ") + c);
  }
  
  str++;
}

JSONArray::~JSONArray()
{
  for(auto i=elems.begin(); i!=elems.end(); i++)
    delete *i;
}

void JSONArray::print(size_t indention)
{
  cout << '[' << endl << std::string(indention+INDENT_WIDTH, ' ');
  if(elems.size() > 0)
    elems[0]->print(indention+INDENT_WIDTH);
  for(size_t i=1; i<elems.size(); i++)
  {
    cout << ',' << endl << std::string(indention+INDENT_WIDTH, ' ');
    elems[i]->print(indention+INDENT_WIDTH);
  }
  cout << endl << std::string(indention, ' ') << ']';
}

JSON& JSONArray::get(size_t i)
{
  if(i > elems.size())
    throw JSONException("Array index out of bounds");
  return *elems[i];
}

__attribute__((pure)) size_t JSONArray::length(void)
{
  return elems.size();
}

/******************************************************************************/

void JSONObject::parseNamed(cchar*& str)
{
  if(*str != '"')
    throw JSONException("Named not starting at the given position");
  
  str++;
  cchar* start = str;
  bool escaped = false;
  char c = *str;
  
  while((c != '"' || escaped) && c != '\0')
  {
    escaped = !escaped && c == '\\';
    str++;
    c = *str;
  }
  
  if(c == '\0')
    throw JSONException("Unexpected end of str");

  std::string key(start, (size_t)(str-start));
  
  //checking " not required here
  str++; //skip closing "
  skipWhitespaces(str);
  if(*str != ':')
  {
    std::string errmsg("Unexpected symbol: ");
    throw JSONException(errmsg + *str);
  }
  str++; // skip :
  skipWhitespaces(str);
  JSON* something = JSON::parseJSON(str);

  fields.insert(std::pair<std::string,JSON*>(key, something));
}

/******************************************************************************/

void printNamed(map<std::string,JSON*>::iterator i, size_t indention)
{
  cout << '"' << i->first << "\" : ";
  i->second->print(indention);
}

JSONObject::JSONObject(cchar*& str)
{
  if(*str != '{')
    throw JSONException("Object not starting at the given position");
  
  char c = '\0';
  while(true)
  {
    c = *str;
    if(fields.size() == 0 && c ==  '{')
    {
      str++;
      skipWhitespaces(str);
      if(*str == '}')
        break;
      parseNamed(str);
      skipWhitespaces(str);
    }
    else if(fields.size() > 0 && c ==  ',')
    {
      str++;
      skipWhitespaces(str);
      parseNamed(str);
      skipWhitespaces(str);
    }
    else if(c == '}')
      break;
    else if(c == '\0')
      throw JSONException("Unexpected end of string");
    else
      throw JSONException(std::string("Unexpected symbol: ") + c);
  }
  
  str++;
  return;
}

JSONObject::~JSONObject()
{
  for(auto i=fields.begin(); i!=fields.end(); i++)
    delete i->second;
}

void JSONObject::print(size_t indention)
{
  cout << '{' << endl << std::string(indention+INDENT_WIDTH, ' ');

  auto i=fields.begin();
  if(fields.size() > 0)
    printNamed(i, indention+INDENT_WIDTH);
  for(; i!=fields.end(); i++)
  {
    cout << ',' << endl << std::string(indention+INDENT_WIDTH, ' ');
    printNamed(i, indention+INDENT_WIDTH);
  }
  cout << endl << std::string(indention, ' ') << '}';
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

JSON& JSONObject::operator[](std::string& key)
{
  return get(key);
}

/******************************************************************************/

JSONString::JSONString(cchar*& str)
{
  if(*str != '"')
    throw JSONException("String not starting at the given position");
  
  str++;
  cchar* const start = str;
  bool escaped = 0;
  char c = *str;
  
  while((c != '"' || escaped) && c != '\0')
  {
    escaped = !escaped && c == '\\';
    str++;
    c = *str;
  }
  
  if(c == '\0')
    throw JSONException("Unexpected end of string");

  string.assign(start, str-start);
  
  //checking " not required here
  str++;
  
}

JSONString::~JSONString()
{
}

void JSONString::print(size_t)
{
  cout << string;
}

string JSONString::get(void)
{
  return string;
}

__attribute__((const)) JSONString::operator std::string&()
{
  return string;
}

/******************************************************************************/

JSONNumber::JSONNumber(cchar*& str)
{
  char* endptr;
  n = strtod(str, &endptr);
  
  if(endptr == str)
    throw JSONException("Could not convert string to number");
  
  str = endptr;
}

void JSONNumber::print(size_t)
{
  cout << n;
}

__attribute__((pure)) JSONNumber::operator uint8_t()
{
  return (uint8_t)n;
}

__attribute__((pure)) JSONNumber::operator int()
{
  return (int)n;
}

__attribute__((pure)) JSONNumber::operator long()
{
  return (long)n;
}

__attribute__((pure)) JSONNumber::operator double()
{
  return n;
}

/******************************************************************************/

JSONBool::JSONBool(cchar*& str)
{
  if(strncmp(str, "false", 5) == 0)
  {
    b = false;
    str += 5;
  }
  else if(strncmp(str, "true", 4) == 0)
  {
    b = true;
    str += 4;
  }
  else
  {
    std::string errmsg("Could not detect neither true nor false: ");
    throw JSONException(errmsg + std::string(str, 5));
  }
}

void JSONBool::print(size_t)
{
  cout << b;
}

__attribute__((pure)) JSONBool::operator bool()
{
  return b;
}

/******************************************************************************/

JSONNull::JSONNull(cchar*& str)
{
  if(strncmp(str, "null", 4) == 0)
    str += 4;
  else
    throw JSONException(std::string("Could not detect null: ") + std::string(str, 4));
}

void JSONNull::print(size_t)
{
  cout << "null";
}

/******************************************************************************/

/**
 * Parses the first JSONSomthing it encounters,
 * string is set to the next (untouched) character.
 */
JSON* JSON::parse(cchar* str)
{
  return parseJSON(str);
}

JSON* JSON::parseJSON(cchar*& str)
{
  skipWhitespaces(str);
  char c = *str;
  switch(c)
  {
  case '\0':
    throw JSONException("Unexpected end of string");
  case '"':
    return new JSONString(str);
  case '{':
    return new JSONObject(str);
  case '[':
    return new JSONArray(str);
  default:
    if((c >= '0' && c <= '9') || c == '.' || c == '+' || c == '-')
      return new JSONNumber(str);
    else if(c == 't' || c == 'f')
      return new JSONBool(str);
    else if(c == 'n')
      return new JSONNull(str);
    break;
  }
  
  std::string errmsg("No valid JSON detected: ");
  throw JSONException(errmsg + c);
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
    throw JSONException("Requested thing is not a JSONArray");
  return *ptr;
}

JSONObject& JSON::object(void)
{
  JSONObject* ptr = dynamic_cast<JSONObject*>(this);
  if(ptr == nullptr)
    throw JSONException("Requested thing is not a JSONObject");
  return *ptr;
}

JSONString& JSON::string(void)
{
  JSONString* ptr = dynamic_cast<JSONString*>(this);
  if(ptr == nullptr)
    throw JSONException("Requested thing is not a JSONString");
  return *ptr;
}

JSONNumber& JSON::number(void)
{
  JSONNumber* ptr = dynamic_cast<JSONNumber*>(this);
  if(ptr == nullptr)
    throw JSONException("Requested thing is not a JSONNumber");
  return *ptr;
}

JSONBool& JSON::boolean(void)
{
  JSONBool* ptr = dynamic_cast<JSONBool*>(this);
  if(ptr == nullptr)
    throw JSONException("Requested thing is not a JSONBool");
  return *ptr;
}

JSONNull& JSON::null(void)
{
  JSONNull* ptr = dynamic_cast<JSONNull*>(this);
  if(ptr == nullptr)
    throw JSONException("Requested thing is not a JSONNull");
  return *ptr;
}


/******************************************************************************/

void test_json(void)
{
  char buffer[300] = {0};
  char* str = buffer;
  strncpy(buffer, "    {\"lolwut\"             : [\"ROFL\", [], {}, [{}]] }", 290);
  JSON* json = JSON::parse(str);
  json->object().get("lolwut").print();
  delete json;
}

