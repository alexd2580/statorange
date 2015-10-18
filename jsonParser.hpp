#ifndef __COMPACT_JSON_PARSER__
#define __COMPACT_JSON_PARSER__

#include<string>
#include<vector>
#include<map>

class JSONArray;
class JSONObject;
class JSONString;
class JSONNumber;
class JSONBool;
class JSONNull;

class JSON
{
private:
  virtual void print(int indention) = 0;
  
  static JSON& parseJSON(char const*& string);
public:
  JSON();
  virtual ~JSON();
  
  virtual void print(void);
  
  static JSON& parse(char const* string);
  
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
  std::vector<JSON> elems;

  virtual void print(size_t indention);
public:
  JSONArray(char const*&);
  virtual ~JSONArray();
  
  JSON& get(size_t i);
  size_t length(void);
};

struct JSONNamed;

class JSONObject : public JSON
{
private:
  std::map<std::string, JSON> fields;

  virtual void print(size_t indention);
public:
  JSONObject(char const*&);
  virtual ~JSONObject();

  JSON& get(char const*);
  JSON& operator[](char const*);  
  JSON& get(std::string&);
  JSON& operator[](std::string& key);
};

class JSONString : public JSON
{
private:
  std::string string;
  
  virtual void print(size_t);
public:
  JSONString(char const*&);
  virtual ~JSONString();
  
  std::string get(void);
};

class JSONNumber : public JSON
{
private:
  double n;
  
  virtual void print(size_t);
public:
  JSONNumber(char const*&);
  virtual ~JSONNumber();
  
  double get(void);
};

class JSONBool : public JSON
{
private:
  bool b;
  
  virtual void print(size_t);
public:
  JSONBool(char const*&);
  virtual ~JSONBool();
  
  bool get(void);
};

class JSONNull : public JSON
{
private:
  virtual void print(size_t);
public:
  JSONNull(char const*&);
  virtual ~JSONNull();
};

#define INDENT_WIDTH 2

void test_main(void);

#endif
