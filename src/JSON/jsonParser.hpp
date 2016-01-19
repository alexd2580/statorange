#ifndef __COMPACT_JSON_PARSER__
#define __COMPACT_JSON_PARSER__

#include <string>
#include <vector>
#include <map>
#include <iostream>

#include "../util.hpp"
#include "JSONException.hpp"

class JSONArray;
class JSONObject;
class JSONString;
class JSONNumber;
class JSONBool;
class JSONNull;

class JSON
{
protected:
  static JSON* parseJSON(TextPos& string);

public:
  virtual ~JSON(){};
  virtual std::string get_type(void) = 0;

  virtual void print(size_t indention) = 0;
  virtual void print(void);

  static JSON* parse(char const* string);

  JSONArray& array(void);
  JSONObject& object(void);
  JSONString& string(void);
  JSONNumber& number(void);
  JSONBool& boolean(void);
  JSONNull& null(void);
};

class JSONArray : public JSON
{
private:
  std::vector<JSON*> elems;

public:
  JSONArray(TextPos&);
  virtual ~JSONArray();
  std::string get_type(void);

  virtual void print(size_t indention);
  JSON& get(size_t i);
  JSON& operator[](size_t);
  size_t size(void);
};

class JSONObject : public JSON
{
private:
  std::map<std::string, JSON*> fields;

  void parseNamed(TextPos&);

public:
  JSONObject(TextPos&);
  virtual ~JSONObject();
  std::string get_type(void);

  virtual void print(size_t indention);
  JSON* has(cchar*);
  JSON* has(std::string&);
  JSON& get(cchar*);
  JSON& operator[](cchar*);
  JSON& get(std::string&);
  JSON& operator[](std::string&);
};

class JSONString : public JSON
{
private:
  std::string string;

public:
  JSONString(TextPos&);
  virtual ~JSONString();
  std::string get_type(void);

  virtual void print(size_t);
  std::string get(void);
  operator std::string&();
};

class JSONNumber : public JSON
{
private:
  double n;

public:
  JSONNumber(TextPos&);
  virtual ~JSONNumber(){};
  std::string get_type(void);

  virtual void print(size_t);
  operator uint8_t();
  operator int();
  operator unsigned int();
  operator long();
  operator double();
};

class JSONBool : public JSON
{
private:
  bool b;

public:
  JSONBool(TextPos&);
  virtual ~JSONBool(){};
  std::string get_type(void);

  virtual void print(size_t);
  operator bool();
};

class JSONNull : public JSON
{
public:
  JSONNull(TextPos&);
  virtual ~JSONNull(){};
  std::string get_type(void);

  virtual void print(size_t);
};

#define INDENT_WIDTH 2

void test_json(void);

#endif
