#ifndef __COMPACT_JSON_SEARCH__
#define __COMPACT_JSON_SEARCH__

#include<cstddef>

char const* skipJSONSomething(char const*);

//JSONSomething* getElem(JSONArray* jsonArray, int i); TODO
//int getArrayLength(JSONArray* jsonArray);
char const* getJSONObject(char const* string, size_t* f);
char const* getJSONObjectField(char const* string, char const* fn, size_t fl);
char const* getJSONString(char const* string, char const** s, size_t* l);
char const* getJSONNumber(char const* string, double*);
char const* getJSONBool(char const* string, bool* b);

#endif
