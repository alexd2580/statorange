#ifndef __COMPACT_JSON_PARSER__
#define __COMPACT_JSON_PARSER__

#include<string>
#include<vector>
#include<map>

class JSONException
{
private:
  std::string msg;
  
public:
  JSONException(std::string errormsg) : msg(errormsg)
  {
  }
  
  virtual char const* what()
  {
    return msg.c_str();
  }
};

class JSONArray;
class JSONObject;
class JSONString;
class JSONNumber;
class JSONBool;
class JSONNull;

class JSON
{
protected:
  static JSON* parseJSON(char const*& string);
public:
  virtual ~JSON() {};
  
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
  JSONArray(char const*&);
  virtual ~JSONArray();
  
  virtual void print(size_t indention);
  JSON& get(size_t i);
  size_t length(void);
};

class JSONObject : public JSON
{
private:
  std::map<std::string, JSON*> fields;

  void parseNamed(char const*&);
public:
  JSONObject(char const*&);
  virtual ~JSONObject();

  virtual void print(size_t indention);
  JSON& get(char const*);
  JSON& operator[](char const*);  
  JSON& get(std::string&);
  JSON& operator[](std::string&);
};

class JSONString : public JSON
{
private:
  std::string string;
public:
  JSONString(char const*&);
  virtual ~JSONString();
  
  virtual void print(size_t);
  std::string get(void);
  operator std::string&();
};

class JSONNumber : public JSON
{
private:
  double n;
public:
  JSONNumber(char const*&);
  virtual ~JSONNumber() {};
  
  virtual void print(size_t);
  operator uint8_t();
  operator int();
  operator long();
  operator double();
};

class JSONBool : public JSON
{
private:
  bool b;
public:
  JSONBool(char const*&);
  virtual ~JSONBool() {};
  
  virtual void print(size_t);
  operator bool();
};

class JSONNull : public JSON
{
public:
  JSONNull(char const*&);
  virtual ~JSONNull() {};

  virtual void print(size_t);
};

#define INDENT_WIDTH 2

void test_main(void);

#endif
