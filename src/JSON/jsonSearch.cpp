/**
 * ONLY FOR PRIVATE USE! UNREFINED AND FAILING EVERYWHERE!
 * ESPECIALLY FOR NON-VALID JSON!
 * IF YOU FIND BUGS YOU CAN KEEP 'EM!
 * I WILL EVENTUALLY (MAYBE) FIX THIS!
 */


#include<cstdlib>
#include<cstring>
#include<iostream>
#include<string>
#include"jsonSearch.hpp"
#include"../util.hpp"

using namespace std;

/******************************************************************************/

/*
JSONArray* parseJSONArray(char** string)
{
  if(**string != '[')
  {
    fprintf(stderr, "Array not starting at the given position\n");
    return NULL;
  }
  
  JSONArray* array = (JSONArray*)malloc(sizeof(JSONArray));
  array->type = JSONArrayType;
  
  int elem_count = 0;
  //assuming less than 100 elems
  JSONSomething** elems = (JSONSomething**)malloc(100*sizeof(JSONSomething*));
  JSONSomething* elem = NULL;

  char c = '\0';
  while(1)
  {
    c = **string;
    if(elem_count == 0 && c== '[')
    {
      *string = *string+1;
      skipWhitespaces(string);
      if(**string == ']')
        break;
      elem = parseJSONSomething(string);
      if(elem == NULL) goto abort_init_array;
      elems[elem_count] = elem;
      elem_count++;
      skipWhitespaces(string);
    }
    else if(elem_count > 0 && c == ',')
    {
      *string = *string+1; //skip komma
      skipWhitespaces(string);
      elem = parseJSONSomething(string);
      if(elem == NULL) goto abort_init_array;
      elems[elem_count] = elem;
      elem_count++;
      skipWhitespaces(string);
    }
    else if(c == ']')
      break;
    else if(c == '\0')
    {
      fprintf(stderr, "Unexpected end of string\n");
      goto abort_init_array;
    }
    else
    {
      fprintf(stderr, "Unexpected symbol: %c\n", c);
      goto abort_init_array;
    }
  }
  
  *string = *string+1;
  
  array->elems = elems;
  array->elem_count = elem_count;
  return array;
  
abort_init_array:
  while(elem_count > 0)
  {
    elem_count--;
    freeJSON(elems[elem_count]);
  }
  free(elems);
  free(array);
  return NULL;
}

JSONSomething* getElem(JSONArray* jsonArray, int i)
{
  if(jsonArray->type != JSONArrayType)
  {
    fprintf(stderr, "Invalid JSON query, getElem only possible with JSONArrays\n");
    return NULL;
  }

  if(i > jsonArray->elem_count)
  {
    fprintf(stderr, "Array index out of bounds\n");
    return NULL;
  }
  return jsonArray->elems[i];
}

int getArrayLength(JSONArray* jsonArray)
{
  if(jsonArray->type != JSONArrayType)
  {
    fprintf(stderr, "Invalid JSON query, getArrayLength only possible with JSONArrays\n");
    return -1;
  }
  
  return jsonArray->elem_count;
}*/

/******************************************************************************/

/**
 * Returns a pointer to the first character of the field.
 * Stores a ptr to the field name to fn if it's != NULL.
 * Stores the length of the field name to fl if it's != NULL.
 * Returns NULL on error.
 */
char const* getJSONNamed(char const* string, char const** fn, size_t* fl)
{
  if(*string != '"')
  {
    cerr << "Named not starting at the given position" << endl;
    return nullptr;
  }
  
  string++;
  char const* start = string;
  bool escaped = false;
  char c = *string;
  
  while((c != '"' || escaped) && c != '\0')
  {
    escaped = !escaped && c == '\\';
    string++;
    c = *string;
  }
  
  if(c == '\0')
  {
    cerr << "Unexpected end of string" << endl;
    return nullptr;
  }
  //checking " not required here
  
  if(fn) *fn = start;
  if(fl) *fl = (size_t)(string-start);
  
  string++; //skip closing "
  
  skipWhitespaces(string);
  if(*string != ':')
  {
    cerr << "Unexpected symbol: " << *string << '(' << (int)*string << ')' << endl;
    return nullptr;
  }
  
  string++; // skip :
  skipWhitespaces(string);
  
  return string;
}


/******************************************************************************/

/**
 * Returns a pointer to the first character after the object.
 * Stores the number of fields to l if it's != NULL.
 * Returns NULL on error.
 */
char const* getJSONObject(char const* string, size_t* l)
{
  if(*string != '{')
  {
    cerr << "Object not starting at the given position" << endl;
    return nullptr;
  }
  
  size_t field_count = 0;
  char c = '\0';
  while(true)
  {
    c = *string;
    if(field_count == 0 && c ==  '{')
    {
      string++;
      skipWhitespaces(string);
      if(*string == '}')
        break;
      string = getJSONNamed(string, nullptr, nullptr);
      string = skipJSONSomething(string);
      if(string == nullptr)
        return nullptr;
      
      field_count++;
      skipWhitespaces(string);
    }
    else if(field_count > 0 && c ==  ',')
    {
      string++;
      skipWhitespaces(string);
      string = getJSONNamed(string, nullptr, nullptr);
      string = skipJSONSomething(string);
      if(string == nullptr)
        return nullptr;
      field_count++;
      skipWhitespaces(string);
    }
    else if(c == '}')
      break;
    else if(c == '\0')
    {
      cerr << "Unexpected end of string" << endl;
      return nullptr;
    }
    else
    {
      cerr << "Unexpected symbol: " << c << '(' << (int)c << ')' << endl;
      return nullptr;
    }
  }

  if(l) *l = field_count;
  return string+1;
}

/**
 * Returns a pointer to the first character of the object 
 * with the given field name, or NULL 
 *  if the object cannot be found
 *  or the JSON cannot be parsed.
 */
char const* getJSONObjectField(char const* string, char const* nStr, size_t nLen)
{
  if(*string != '{')
  {
    cerr << "Object not starting at the given position" << endl;
    return nullptr;
  }
  
  char const* name;
  size_t nameLen;
  
  size_t field_count = 0;
  char c = '\0';
  while(true)
  {
    c = *string;
    if(field_count == 0 && c ==  '{')
    {
      string++;
      skipWhitespaces(string);
      if(*string == '}')
        break;
      string = getJSONNamed(string, &name, &nameLen);
      if(string == nullptr)
        return nullptr;
        
      if(nLen == nameLen)
        if(strncmp(name, nStr, nLen) == 0)
          return string;

      string = skipJSONSomething(string);
      if(string == nullptr)
        return nullptr;
      field_count++;
      skipWhitespaces(string);
    }
    else if(field_count > 0 && c ==  ',')
    {
      string++;
      skipWhitespaces(string);
      string = getJSONNamed(string, &name, &nameLen);
      if(string == nullptr)
        return nullptr;
        
      if(nLen == nameLen)
        if(strncmp(name, nStr, nLen) == 0)
          return string;

      string = skipJSONSomething(string);
      if(string == nullptr)
        return nullptr;
      field_count++;
      skipWhitespaces(string);
    }
    else if(c == '}')
      return nullptr;
    else if(c == '\0')
    {
      cerr << "Unexpected end of string" << endl;
      return nullptr;
    }
    else
    {
      cerr << "Unexpected symbol: " << c << '(' << (int)c << ')' << endl;
      return nullptr;
    }
  }

  return nullptr;
}

/******************************************************************************/

/**
 * Reads a string at the given position and stores 
 * the start pointer to s and the length to l.
 * If s == NULL then it doesn't store the start pointer.
 * If l == NULL then it doesn't store the length.
 * Returns a pointer to the next character after the read string.
 * Returns NULL on fail. In this case the values of s and l are not changed
 */
char const* getJSONString(char const* string, char const** s, size_t* l)
{
  if(*string != '"')
  {
    cerr << "String not starting at the given position" << endl;
    return nullptr;
  }
  
  string++;
  char const* start = string;
  bool escaped = false;
  char c = *string;
  
  while((c != '"' || escaped) && c != '\0')
  {
    escaped = !escaped && c == '\\';
    string++;
    c = *string;
  }
  
  if(c == '\0')
  {
    cerr << "Unexpected end of string" << endl;
    return nullptr;
  }
  //checking " not required here
  if(s) *s = start;
  if(l) *l = (size_t)(string-start);

  return string+1;  
}

/******************************************************************************/

/**
 * Reads a number at the given position and stores it to n.
 * If n == NULL then it doesn't store the num.
 * Returns a pointer to the next character after the read num.
 * Returns NULL on fail. In this case the value of n is not changed
 */
char const* getJSONNumber(char const* string, double* n)
{
  char* endptr;
  double val = strtod(string, &endptr);
  
  if(endptr == string)
  {
    cerr << "Could not convert string to number" << endl;
    return nullptr;
  }
  
  if(n) *n = val;
  return endptr;
}

/******************************************************************************/

/**
 * Reads a boolean at the given position and stores it to b.
 * If b == NULL then it doesn't store the bool.
 * Returns a pointer to the next character after the read bool.
 * Returns NULL on fail. In this case the value of b is not changed.
 */
char const* getJSONBool(char const* string, bool* b)
{
  if(strncmp(string, "false", 5) == 0)
  {
    if(b) *b = false;
    return string + 5;
  }
  else if(strncmp(string, "true", 4) == 0)
  {
    if(b) *b = true;
    return string + 4;
  }

  cerr << "Could not detect neither true nor false: " << std::string(string, 5) << endl;
  return nullptr;
}

/******************************************************************************/

/**
 * Reads a null at the given position.
 * Offsets string to the next character after the read null.
 * Returns NULL on fail.
 */
char const* getJSONNull(char const* string)
{
  if(strncmp(string, "null", 4) == 0)
    return string+4;
    
  cerr << "Could not detect null: " << std::string(string, 4) << endl;
  return nullptr;
}

/******************************************************************************/

char const* skipJSONSomething(char const* string)
{
  if(string == nullptr)
    return nullptr;

  skipWhitespaces(string);
  char c = *string;
  switch(c)
  {
  case '\0':
    cerr << "Unexpected end of string" << endl;
    return nullptr;
  case '"':
    return getJSONString(string, nullptr, nullptr);
  case '{':
    return getJSONObject(string, nullptr);
  case '[':
    cerr << "arrays currently unsopported" << endl;
    return nullptr;
    //return (JSONSomething*)parseJSONArray(string);
  default:
    if((c >= '0' && c <= '9') || c == '.' || c == '+' || c == '-')
      return getJSONNumber(string, nullptr);
    else if(c == 't' || c == 'f')
      return getJSONBool(string, nullptr);
    else if(c == 'n')
      return getJSONNull(string);
    break;
  }
  cerr << "No valid JSON detected: " << c << '(' << (int)c << ')' << endl;
  return nullptr;
}

/*
#include<stdio.h>

int main(void)
{
  char test[] = "{ \"change\":\"focus\" , \"current\" : { \"id\" : 18837712 , \"qwe\" : false, \"test2\" : \"LOL\" } }";
  
  char* res;
  res = getJSONObjectField(test, "current", 7);
  res = getJSONObjectField(res, "test2", 5);
  char* str;
  size_t len;
  getJSONString(res, &str, &len);
  
  printf("[%.*s]\n", (int)len, str);
  return 0;
}*/






