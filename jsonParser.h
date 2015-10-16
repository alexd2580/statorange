#ifndef __COMPACT_JSON_PARSER__
#define __COMPACT_JSON_PARSER__

typedef union JSONSomething_ JSONSomething;
typedef struct JSONArray_ JSONArray;
typedef struct JSONNamed_ JSONNamed;
typedef struct JSONObject_ JSONObject;
typedef struct JSONString_ JSONString;
typedef struct JSONNumber_ JSONNumber;
typedef struct JSONBool_ JSONBool;
typedef struct JSONNull_ JSONNull;

JSONSomething* parseJSON(char*);
void freeJSON(JSONSomething*);
void printJSON(JSONSomething*);

JSONSomething* getElem(JSONArray* jsonArray, int i);
int getArrayLength(JSONArray* jsonArray);
JSONSomething* getField(JSONObject* jsonObject, char const* id);
char* getString(JSONString* jsonString, size_t* length);
double getNumber(JSONNumber* jsonNumber);
char getBool(JSONBool* jsonBool);

#define INDENT_WIDTH 2

void test_main(void);

#endif
