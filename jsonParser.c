/**
 * ONLY FOR PRIVATE USE! UNREFINED AND FAILING EVERYWHERE!
 * ESPECIALLY FOR NON-VALID JSON!
 * IF YOU FIND BUGS YOU CAN KEEP 'EM!
 * I WILL EVENTUALLY (MAYBE) FIX THIS!
 */


#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include"jsonParser.h"
#include"strUtil.h"

//used for indention
char indenter = '\0';

enum JSONType_
{
  JSONArrayType = 1,
  JSONObjectType = 2,
  JSONStringType = 3,
  JSONNumberType = 4,
  JSONBoolType = 5,
  JSONNullType = 6
};
typedef enum JSONType_ JSONType;

JSONSomething* parseJSONSomething(char** string);
void printJSONSomething(JSONSomething*, int);

/******************************************************************************/

struct JSONArray_
{
  JSONType type;
  int elem_count;
  JSONSomething** elems;
};

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

void freeJSONArray(JSONArray* jsonArray)
{
  for(int i=0; i<jsonArray->elem_count; i++)
    freeJSON(jsonArray->elems[i]);
  free(jsonArray->elems);
  free(jsonArray);
}

void printJSONArray(JSONArray* jsonArray, int indention)
{
  printf("[\n%*s", indention+INDENT_WIDTH, &indenter);
  if(jsonArray->elem_count > 0)
    printJSONSomething(jsonArray->elems[0], indention+INDENT_WIDTH);
  for(int i=1; i<jsonArray->elem_count; i++)
  {
    printf(",\n%*s", indention+INDENT_WIDTH, &indenter);
    printJSONSomething(jsonArray->elems[i], indention+INDENT_WIDTH);
  }
  printf("\n%*s]", indention, &indenter);
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

struct JSONObject_
{
  JSONType type;
  JSONNamed** fields;
  int field_count;
};

JSONObject* parseJSONObject(char** string)
{
  if(**string != '{')
  {
    fprintf(stderr, "Object not starting at the given position\n");
    return NULL;
  }
  
  JSONObject* object = (JSONObject*)malloc(sizeof(JSONObject));
  object->type = JSONObjectType;
  
  int field_count = 0;
  //assuming less than 50 fields
  JSONNamed** fields = (JSONNamed**)malloc(50*sizeof(JSONNamed*));
  JSONNamed* field = NULL;
  
  char c = '\0';
  while(1)
  {
    c = **string;
    if(field_count == 0 && c ==  '{')
    {
      *string = *string+1;
      skipWhitespaces(string);
      if(**string == '}')
        break;
      field = parseJSONNamed(string);
      if(field == NULL) goto abort_init_object;
      fields[field_count] = field;
      field_count++;
      skipWhitespaces(string);
    }
    else if(field_count > 0 && c ==  ',')
    {
      *string = *string+1;
      skipWhitespaces(string);
      field = parseJSONNamed(string);
      if(field == NULL) goto abort_init_object;
      fields[field_count] = field;
      field_count++;
      skipWhitespaces(string);
    }
    else if(c == '}')
      break;
    else if(c == '\0')
    {
      fprintf(stderr, "Unexpected end of string\n");
      goto abort_init_object;
    }
    else
    {
      fprintf(stderr, "Unexpected symbol: %c\n", c);
      goto abort_init_object;
    }
  }
  
  *string = *string+1;
  
  object->fields = fields;
  object->field_count = field_count;
  return object;
  
abort_init_object:
  while(field_count > 0)
  {
    field_count--;
    freeJSONNamed(fields[field_count]);
  }
  free(fields);
  free(object);
  return NULL;
}

void freeJSONObject(JSONObject* jsonObject)
{
  for(int i=0; i<jsonObject->field_count; i++)
    freeJSONNamed(jsonObject->fields[i]);
  free(jsonObject->fields);
  free(jsonObject);
}

void printJSONObject(JSONObject* jsonObject, int indention)
{
  printf("{\n%*s", indention+INDENT_WIDTH, &indenter);
  if(jsonObject->field_count > 0)
    printJSONNamed(jsonObject->fields[0], indention+INDENT_WIDTH);
  for(int i=1; i<jsonObject->field_count; i++)
  {
    printf(",\n%*s", indention+INDENT_WIDTH, &indenter);
    printJSONNamed(jsonObject->fields[i], indention+INDENT_WIDTH);
  }
  printf("\n%*s}", indention, &indenter);
}

JSONSomething* getField(JSONObject* jsonObject, char const* id)
{
  if(jsonObject->type != JSONObjectType)
  {
    fprintf(stderr, "Invalid JSON query, getField only possible with JSONObjects\n");
    return NULL;
  }
  
  for(int i=0; i<jsonObject->field_count; i++)
  {
    JSONNamed* n = jsonObject->fields[i];
    if(strncmp(id, n->name, n->name_length) == 0)
      return n->something;
  }
  
  fprintf(stderr, "Element %s not found\n", id);
  return NULL;
}

/******************************************************************************/

struct JSONString_
{
  JSONType type;
  char* string;
  size_t string_length;
};

JSONString* parseJSONString(char** string)
{
  if(**string != '"')
  {
    fprintf(stderr, "String not starting at the given position\n");
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
  
  JSONString* jstring = (JSONString*)malloc(sizeof(JSONString));
  jstring->type = JSONStringType;
  jstring->string = start;
  jstring->string_length = (size_t)(i - start);
  return jstring;
}

void freeJSONString(JSONString* jsonString)
{
  free(jsonString);
  return;
}

void printJSONString(JSONString* jsonString, int UNUSED)
{
  (void)UNUSED;
  printf("\"%.*s\"", (int)jsonString->string_length, jsonString->string);
}

char* getString(JSONString* jsonString, size_t* length)
{
  if(jsonString->type != JSONStringType)
  {
    fprintf(stderr, "Invalid JSON query, getString only possible with JSONStrings\n");
    *length = 0;
    return NULL;
  }
  
  *length = jsonString->string_length;
  return jsonString->string;
}

/******************************************************************************/

struct JSONNumber_
{
  JSONType type;
  double value;
};

JSONNumber* parseJSONNumber(char** string)
{
  char* endptr;
  double val = strtod(*string, &endptr);
  
  if(endptr == *string)
  {
    fprintf(stderr, "Could not convert string to number\n");
    return NULL;
  }
  
  JSONNumber* number = (JSONNumber*)malloc(sizeof(JSONNumber));
  number->type = JSONNumberType;
  number->value = val;
  *string = endptr;
  return number;
}

void freeJSONNumber(JSONNumber* jsonNumber)
{
  free(jsonNumber);
  return;
}

void printJSONNumber(JSONNumber* jsonNumber, int UNUSED)
{
  (void)UNUSED;
  printf("%lf", jsonNumber->value);
}

double getNumber(JSONNumber* jsonNumber)
{
  if(jsonNumber->type != JSONNumberType)
  {
    fprintf(stderr, "Invalid JSON query, getNumber only possible with JSONNumbers\n");
    return 0.0f/0.0f;
  }
  
  return jsonNumber->value;
}

/******************************************************************************/

struct JSONBool_
{
  JSONType type;
  char value;
};

JSONBool* parseJSONBool(char** string)
{
  char val = -1;
  if(strncmp(*string, "false", 5) == 0)
  {
    val = 0;
    *string = *string + 5;
  }
  else if(strncmp(*string, "true", 4) == 0)
  {
    val = 1;
    *string = *string + 4;
  }
  else
  {
    fprintf(stderr, "Could not detect neither true nor false: %.*s", 5, *string);
    fprintf(stderr, "\n");
    return NULL;
  }
    
  JSONBool* boolean = (JSONBool*)malloc(sizeof(JSONBool));
  boolean->type = JSONBoolType;
  boolean->value = val;
  return boolean;
}

void freeJSONBool(JSONBool* jsonBool)
{
  free(jsonBool);
  return;
}

void printJSONBool(JSONBool* jsonBool, int UNUSED)
{
  (void)UNUSED;
  printf(jsonBool->value ? "true" : "false");
}

char getBool(JSONBool* jsonBool)
{
  if(jsonBool->type != JSONBoolType)
  {
    fprintf(stderr, "Invalid JSON query, getBool only possible with JSONBools\n");
    return -1;
  }
  
  return jsonBool->value;
}

/******************************************************************************/

struct JSONNull_
{
  JSONType type;
};

JSONNull* parseJSONNull(char** string)
{
  if(strncmp(*string, "null", 4) == 0)
  {
    *string = *string + 4;
    JSONNull* null = (JSONNull*)malloc(sizeof(JSONNull));
    null->type = JSONNullType;
    return null;
  }

  fprintf(stderr, "Could not detect null: %.*s", 4, *string);
  fprintf(stderr, "\n");
  return NULL;
}

void freeJSONNull(JSONNull* jsonNull)
{
  free(jsonNull);
  return;
}

void printJSONNull(JSONNull* jsonNull, int UNUSED)
{
  (void)UNUSED;
  (void)jsonNull;
  printf("null");
}

/******************************************************************************/

union JSONSomething_
{
  JSONType type;
  JSONArray array;
  JSONObject object;
  JSONString string;
  JSONNumber number;
  JSONBool boolean;
  JSONNull null;
};

/**
 * Parses the first JSONSomthing it encounters,
 * string is set to the next (untouched) character.
 */
JSONSomething* parseJSON(char* string)
{
  return parseJSONSomething(&string);
}

JSONSomething* parseJSONSomething(char** string)
{
  skipWhitespaces(string);
  char c = **string;
  switch(c)
  {
  case '\0':
    fprintf(stderr, "Unexpected end of string");
    return NULL;
  case '"':
    return (JSONSomething*)parseJSONString(string);
  case '{':
    return (JSONSomething*)parseJSONObject(string);
  case '[':
    return (JSONSomething*)parseJSONArray(string);
  default:
    if((c >= '0' && c <= '9') || c == '.' || c == '+' || c == '-')
      return (JSONSomething*)parseJSONNumber(string);
    else if(c == 't' || c == 'f')
      return (JSONSomething*)parseJSONBool(string);
    else if(c == 'n')
      return (JSONSomething*)parseJSONNull(string);
    break;
  }
  fprintf(stderr, "No valid JSON detected: %c", c);
  fprintf(stderr, " (%d)\n", c);
  return NULL;
}

void freeJSON(JSONSomething* json)
{
  if(json == NULL)
    return;
  
  switch(json->type)
  {
  case JSONObjectType:
    freeJSONObject(&json->object);
    break;
  case JSONArrayType:
    freeJSONArray(&json->array);
    break;
  case JSONStringType:
    freeJSONString(&json->string);
    break;
  case JSONNumberType:
    freeJSONNumber(&json->number);
    break;
  case JSONBoolType:
    freeJSONBool(&json->boolean);
    break;
  case JSONNullType:
    freeJSONNull(&json->null);
    break;
  default:
    break;
  }
  return;
}

void printJSONSomething(JSONSomething* json, int indention)
{
  if(json == NULL)
    return;
  
  switch(json->type)
  {
  case JSONObjectType:
    printJSONObject(&json->object, indention);
    break;
  case JSONArrayType:
    printJSONArray(&json->array, indention);
    break;
  case JSONStringType:
    printJSONString(&json->string, indention);
    break;
  case JSONNumberType:
    printJSONNumber(&json->number, indention);
    break;
  case JSONBoolType:
    printJSONBool(&json->boolean, indention);
    break;
  case JSONNullType:
    printJSONNull(&json->null, indention);
    break;
  default:
    break;
  }
  return;
}

/**
 * Use this method to print JSONSomethings.
 * Calling the specialized methods directly is unsafe.
 */
void printJSON(JSONSomething* json)
{
  printJSONSomething(json, 0);
  printf("\n");
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

