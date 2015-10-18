#ifndef __COMPACT_JSON_SEARCH__
#define __COMPACT_JSON_SEARCH__

#include<cstdint>

char* skipJSONSomething(char*);

//JSONSomething* getElem(JSONArray* jsonArray, int i); TODO
//int getArrayLength(JSONArray* jsonArray);
char* getJSONObject(char* string, size_t* f);
char* getJSONObjectField(char* string, char const* fn, size_t fl);
char* getJSONString(char* string, char** s, size_t* l);
char* getJSONNumber(char* string, double*);
char* getJSONBool(char* string, uint8_t* b);

#endif
