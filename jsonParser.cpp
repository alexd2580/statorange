/**
 * ONLY FOR PRIVATE USE! UNREFINED AND FAILING EVERYWHERE!
 * ESPECIALLY FOR NON-VALID JSON!
 * IF YOU FIND BUGS YOU CAN KEEP 'EM!
 * I WILL EVENTUALLY (MAYBE) FIX THIS!
 */


#include<cstdlib>
#include<cstring>
#include<iostream>

#include"jsonParser.h"
#include"strUtil.h"

using namespace std;

/******************************************************************************/

JSONArray::JSONArray(char const*& string)
{
  if(*string != '[')
    throw JSONException("Array not starting at the given position");
  
  char c = '\0';
  while(true)
  {
    c = *string;
    if(elems.size() == 0 && c== '[')
    {
      string++;
      skipWhitespaces(string);
      if(*string == ']')
        break;
      elems.push_pack(JSON::parse(string));
      skipWhitespaces(string);
    }
    else if(elems.size() > 0 && c == ',')
    {
      string++; //skip komma
      skipWhitespaces(string);
      elems.push_back(JSON::parse(string));
      skipWhitespaces(string);
    }
    else if(c == ']')
      break;
    else if(c == '\0')
      throw JSONException("Unexpected end of string");
    else
    {
      fprintf(stderr, "Unexpected symbol: %c\n", c); //TODO
      throw JSONException("Unexpected symbol");
    }
  }
  
  string++;
}

JSONArray::~JSONArray()
{
}

void JSONArray::print(size_t indention)
{
  cout << '[' << endl << string(indention+INDENT_WIDTH, ' ');
  if(elems.size() > 0)
    elems[0].print(indention+INDENT_WIDTH);
  for(size_t i=1; i<elems.size(); i++)
  {
    cout << ',' << endl << string(indention+INDENT_WIDTH, ' ');
    elems[i].print(indention+INDENT_WIDTH);
  }
  cout << endl << string(indention, ' ') << ']';
}

JSON& JSONArray::get(size_t i)
{
  if(i > elems.size())
    throw JSONException("Array index out of bounds");
  return elems[i];
}

size_t JSONArray::length(void)
{
  return elems.size();
}

/******************************************************************************/

struct JSONNamed_
{
  char* name;
  size_t name_length;
  JSONSomething* something;
};

JSONNamed* parseJSONNamed(char** string)
{
  if(**string != '"')
  {
    fprintf(stderr, "Named not starting at the given position\n");
    return NULL;
  }
  
  char* start = *string+1;
  char* i = start;
  char escaped = 0;
  
  while((*i != '"' || escaped) && *i != '\0')
  {
    escaped = !escaped && *i == '\\';
    i++;
  }
  
  if(*i == '\0')
  {
    fprintf(stderr, "Unexpected end of string\n");
    return NULL;
  }
  //checking " not required here
  *string = i+1; //skip closing "
  
  skipWhitespaces(string);
  if(**string != ':')
  {
    fprintf(stderr, "Unexpected symbol: %c", **string);
    fprintf(stderr, " (%d)\n", **string);
    return NULL;
  }
  *string = *string+1; // skip :
  skipWhitespaces(string);
  JSONSomething* something = parseJSONSomething(string);
  if(something == NULL)
    return NULL;
  
  JSONNamed* named = (JSONNamed*)malloc(sizeof(JSONNamed));
  named->name = start;
  named->name_length = (size_t)(i - start);
  named->something = something;
  return named;
}

void freeJSONNamed(JSONNamed* jsonNamed)
{
  freeJSON(jsonNamed->something);
  free(jsonNamed);
}

void printJSONNamed(JSONNamed* jsonNamed, int indention)
{
  printf("\"%.*s\" : ", (int)jsonNamed->name_length, jsonNamed->name);
  printJSONSomething(jsonNamed->something, indention);
}

/******************************************************************************/

JSONObject::JSONObject(char const*& str)
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
      parseJSONNamed(str);
      skipWhitespaces(str);
    }
    else if(fields.size() > 0 && c ==  ',')
    {
      str++;
      skipWhitespaces(str);
      parseJSONNamed(str);
      skipWhitespaces(str);
    }
    else if(c == '}')
      break;
    else if(c == '\0')
      throw JSONException("Unexpected end of string");
    else
      throw JSONException(string("Unexpected symbol: ") + c);
  }
  
  str++;
  return;
}

JSONObject::~JSONObject()
{
}

void JSONObject::print(size_t indention)
{
  cout << '{' << endl << string(indention+INDENT_WIDTH, ' ');

  if(fields.size() > 0)
    fields[0].print(indention+INDENT_WIDTH);
  for(size_t i=1; i<fields.size(); i++)
  {
    cout << ',' << endl << string(indention+INDENT_WIDTH, ' ');
    fields[i].print(indention+INDENT_WIDTH); //TODO
  }
  cout << endl << string(indention, ' ') << '}';
}

JSON& JSONObject::get(char const* key)
{
  return get(string(key));
}

JSON& JSONObject::operator[](char const* key)
{
  return get(string(key));
}

JSON& JSONObject::get(string& key)
{
  auto it = fields.find(key);
  if(it == fields.end())
    throw JSONException(string("Element ") + key + " not found");
  return *it;
}

JSON& JSONObject::operator[](string& key)
{
  return get(key);
}

/******************************************************************************/

JSONString::JSONString(char const*& str)
{
  if(*str != '"')
    throw JSONException("String not starting at the given position");
  
  str++;
  char const* const start = str;
  char escaped = 0;
  char c = *str;
  
  while((c != '"' || escaped) && c != '\0')
  {
    escaped = !escaped && c == '\\';
    str++;
    c = *str;
  }
  
  if(*str == '\0')
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

/******************************************************************************/

JSONNumber::JSONNumber(char const*& string)
{
  char* endptr;
  n = strtod(string, &endptr);
  
  if(endptr == string)
    throw JSONException("Could not convert string to number");
  
  string = endptr;
}

JSONNumber::~JSONNumber()
{
}

void JSONNumber::print(size_t)
{
  cout << n;
}

double JSONNumber::get(void)
{
  return n;
}

/******************************************************************************/

JSONBool::JSONBool(char const*& str)
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
    string errmsg("Could not detect neither true nor false: ");
    throw JSONException(errmsg + string(str, 5));
  }
}

JSONBool::~JSONBool()
{
}

void JSONBool::print(size_t)
{
  cout << b;
}

char JSONBool::get(void)
{
  return b;
}

/******************************************************************************/

JSONNull::JSONNull(char const*& str)
{
  if(strncmp(str, "null", 4) == 0)
    str += 4;
  else
    throw JSONException(string("Could not detect null: ") + string(str, 4));
}

JSONNull::JSONNull()
{
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
JSON& JSON::parse(char const* string)
{
  return parseJSON(string);
}

JSON& JSON::parseJSON(char const*& str)
{
  skipWhitespaces(str);
  char c = *str;
  switch(c)
  {
  case '\0':
    throw JSONException("Unexpected end of string");
  case '"':
    return JSONString(str);
  case '{':
    return JSONObject(str);
  case '[':
    return JSONArray(str);
  default:
    if((c >= '0' && c <= '9') || c == '.' || c == '+' || c == '-')
      return JSONNumber(str);
    else if(c == 't' || c == 'f')
      return JSONBool(str);
    else if(c == 'n')
      return JSONNull(str);
    break;
  }
  
  string errmsg("No valid JSON detected: ");
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
  char* string = buffer;
  strncpy(buffer, "    {\"lolwut\"             : [\"ROFL\", [], {}, [{}]] }", 290);
  JSONSomething* json = parseJSON(string);
  if(json == NULL)
    return;
  printJSON(getField((JSONObject*)json, "lolwut"));
  free(json);
}

