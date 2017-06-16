/**
 * ONLY FOR PRIVATE USE! UNREFINED AND FAILING EVERYWHERE!
 * ESPECIALLY AT NON-VALID JSON!
 * IF YOU FIND BUGS YOU CAN KEEP 'EM!
 * I WILL EVENTUALLY (MAYBE) FIX THIS!
 */

#include "../util.hpp"
#include "json_parser.hpp"

#include <map>
#include <vector>

#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace std;

#define INDENT_WIDTH 2

/******************************************************************************/

class JSONNull : public JSON
{
  private:
    JSONNull()
    {
    }

  public:
    static JSONNull const static_null;

    JSONNull(char const*& pos)
    {
        if(strncmp(pos, "null", 4) == 0)
            pos += 4;
        else
        {
            cerr << "Could not detect null: " << string(pos, 4) << endl;
            exit(1);
        }
    }

    virtual ~JSONNull(void) = default;

    virtual bool is_null(void) const
    {
        return true;
    }

    virtual string get_type(void) const
    {
        return "null";
    }

    virtual void print(size_t) const
    {
        cout << "null";
    }

    string to_string(void) const
    {
        return "null";
    }
};

JSONNull const JSONNull::static_null;

/******************************************************************************/

class JSONArray : public JSON
{
  private:
    vector<unique_ptr<JSON>> elems;

  public:
    JSONArray(char const*& pos)
    {
        char c = *pos;
        if(c != '[')
        {
            cerr << "Array not starting at the given position" << endl;
            exit(1);
        }

        while(true)
        {
            whitespace(pos);
            c = next(pos);
            char delimiter = elems.empty() ? '[' : ',';
            if(c == delimiter)
            {
                whitespace(pos);
                // Lookahead (don't consume `c`).
                c = *pos;
                if(c == ']' && elems.empty())
                {
                    pos++;
                    break;
                }

                elems.push_back(JSON::parse(pos));
            }
            else if(c == ']')
                break;
            else if(c == '\0')
            {
                cerr << "Unexpected end of str" << endl;
                exit(1);
            }
            else
            {
                cerr << "Expected '" << delimiter << "' or ']', got '" << c
                     << "'" << endl;
                exit(1);
            }
        }
    }

    virtual ~JSONArray(void) = default;
    string get_type(void) const
    {
        return "array";
    }

    virtual void print(size_t indention) const
    {
        cout << '[' << endl << string(indention + INDENT_WIDTH, ' ');
        if(!elems.empty())
            elems[0]->print(indention + INDENT_WIDTH);
        for(auto& elem : elems)
        {
            cout << ',' << endl << string(indention + INDENT_WIDTH, ' ');
            elem->print(indention + INDENT_WIDTH);
        }
        cout << endl << string(indention, ' ') << ']';
    }

    JSON const& get(size_t i) const
    {
        return i >= elems.size() ? JSONNull::static_null : operator[](i);
    }
    JSON const& operator[](size_t i) const
    {
        return *elems[i];
    }

    size_t size(void) const
    {
        return elems.size();
    }

    vector<unique_ptr<JSON>> const& as_vector(void) const
    {
        return elems;
    }

    string to_string(void) const
    {
        return "JSONArray";
    }
};

/******************************************************************************/

void printNamed(
    map<string, unique_ptr<JSON>>::const_iterator i, size_t indention)
{
    cout << '"' << i->first << "\" : ";
    i->second->print(indention);
}

class JSONObject : public JSON
{
  private:
    map<string, unique_ptr<JSON>> fields;

    void parseNamed(char const*& pos)
    {
        string key(escaped_string(pos));
        whitespace(pos);
        char c = next(pos);
        if(c != ':')
        {
            cerr << "Expected ':', got: " << (char)c << endl;
            exit(1);
        }

        whitespace(pos);
        unique_ptr<JSON> something = JSON::parse(pos);
        decltype(fields)::value_type kvpair(move(key), move(something));
        fields.insert(move(kvpair));
    }

  public:
    JSONObject(char const*& pos)
    {
        char c = *pos;
        if(c != '{')
        {
            cerr << "Expected '{', got: " << (char)c << endl;
            exit(1);
        }

        while(true)
        {
            whitespace(pos);
            c = next(pos);
            char delimiter = fields.empty() ? '{' : ',';
            if(c == delimiter)
            {
                whitespace(pos);
                // Lookahead, don't consume `c`.
                c = *pos;
                if(c == '}' && fields.empty())
                {
                    pos++;
                    break;
                }
                parseNamed(pos);
            }
            else if(c == '}')
                break;
            else if(c == '\0')
            {
                cerr << "Unexpected end of string" << endl;
                exit(1);
            }
            else
            {
                cerr << "Expected '" << delimiter << "' or '}', got '" << c
                     << "'" << endl;
                exit(1);
            }
        }
    }

    virtual ~JSONObject(void) = default;
    virtual string get_type(void) const
    {
        return "object";
    }

    virtual void print(size_t indention) const
    {
        size_t nested_indention_level = indention + INDENT_WIDTH;
        string nested_indention(nested_indention_level, ' ');
        cout << '{' << endl << nested_indention;

        auto i = fields.begin();
        size_t field_size = fields.size();
        if(field_size > 0)
            printNamed(i, nested_indention_level);
        i++;
        for(; i != fields.end(); i++)
        {
            cout << ',' << endl << nested_indention;
            printNamed(i, nested_indention_level);
        }
        cout << endl << string(indention, ' ') << '}';
    }

    bool has(char const* key) const
    {
        string skey(key);
        return has(skey);
    }

    bool has(string const& key) const
    {
        auto it = fields.find(key);
        return it != fields.end();
    }

    JSON const& get(char const* key) const
    {
        string skey(key);
        return get(skey);
    }

    JSON const& get(string const& key) const
    {
        auto it = fields.find(key);
        return it == fields.end() ? JSONNull::static_null
                                  : (JSON const&)*it->second;
    }

    JSON const& operator[](char const* key) const
    {
        string skey(key);
        return operator[](skey);
    }

    JSON const& operator[](string const& key) const
    {
        auto it = fields.find(key);
        if(it == fields.end())
        {
            cerr << "Element " << key << " not found" << endl;
            exit(1);
        }
        return *it->second;
    }

    string to_string(void) const
    {
        return "JSONObject";
    }
};

/******************************************************************************/

class JSONString : public JSON
{
  private:
    string string_;

  public:
    JSONString(char const*& pos)
    {
        string_.assign(escaped_string(pos));
    }

    virtual ~JSONString(void) = default;

    virtual string get_type(void) const
    {
        return "string";
    }

    virtual void print(size_t) const
    {
        cout << '"' << string_ << '"';
    }

    operator string const&() const
    {
        return string_;
    }
    string const& as_string_with_default(string const&) const
    {
        return string_;
    }

    string to_string(void) const
    {
        return string_;
    }
};

/******************************************************************************/

class JSONNumber : public JSON
{
  private:
    double n;

  public:
    JSONNumber(char const*& pos)
    {
        n = number(pos);
    }

    virtual ~JSONNumber(void) = default;

    string get_type(void) const
    {
        return "num";
    }

    virtual void print(size_t) const
    {
        cout << n;
    }

    __attribute__((pure)) virtual operator uint8_t() const
    {
        return (uint8_t)n;
    }
    __attribute__((pure)) virtual uint8_t as_uint8_t_with_default(uint8_t) const
    {
        return (uint8_t)n;
    }
    __attribute__((pure)) virtual operator int() const
    {
        return (int)n;
    }
    __attribute__((pure)) virtual int as_int_with_default(int) const
    {
        return (int)n;
    }
    __attribute__((pure)) virtual operator unsigned() const
    {
        return (unsigned)n;
    }
    __attribute__((pure)) virtual unsigned
    as_unsigned_with_default(unsigned) const
    {
        return (unsigned)n;
    }
    __attribute__((pure)) virtual operator long() const
    {
        return (long)n;
    }
    __attribute__((pure)) virtual long as_long_with_default(long) const
    {
        return (long)n;
    }
    __attribute__((pure)) virtual operator double() const
    {
        return n;
    }
    __attribute__((pure)) virtual double as_double_with_default(double) const
    {
        return (double)n;
    }

    string to_string(void) const
    {
        return ::to_string(n);
    }
};

/******************************************************************************/

class JSONBool : public JSON
{
  private:
    bool b;

  public:
    JSONBool(char const*& pos)
    {
        if(strncmp(pos, "false", 5) == 0)
        {
            b = false;
            pos += 5;
        }
        else if(strncmp(pos, "true", 4) == 0)
        {
            b = true;
            pos += 4;
        }
        else
        {
            cerr << "Could not detect neither true nor false: "
                 << string(pos, 5) << endl;
            exit(1);
        }
    }

    virtual ~JSONBool(void) = default;

    string get_type(void) const
    {
        return "bool";
    }

    virtual void print(size_t) const
    {
        cout << (b ? "true" : "false");
    }

    __attribute__((pure)) virtual operator bool() const
    {
        return b;
    }
    __attribute__((pure)) virtual bool as_bool_with_default(bool) const
    {
        return b;
    }

    string to_string(void) const
    {
        return ::to_string(b);
    }
};

/******************************************************************************/

unique_ptr<JSON> JSON::parse(char const*& pos)
{
    whitespace(pos);
    // Lookahead, don't consume `c`.
    char c = *pos;
    switch(c)
    {
    case '\0':
        cerr << "Unexpected end of string" << endl;
        exit(1);
    case '"':
        return make_unique<JSONString>(pos);
    case '{':
        return make_unique<JSONObject>(pos);
    case '[':
        return make_unique<JSONArray>(pos);
    case 't':
    case 'f':
        return make_unique<JSONBool>(pos);
    case 'n':
        return make_unique<JSONNull>(pos);
    default:
        if((c >= '0' && c <= '9') || c == '.' || c == '+' || c == '-')
            return make_unique<JSONNumber>(pos);
        break;
    }

    cerr << "No valid JSON detected: " << c << endl;
    exit(1);
}

bool JSON::is_null(void) const
{
    return false;
}

void JSON::print(void) const
{
    print(0);
    cout << endl;
}

// JSONArray
JSON const& JSON::get(size_t) const
{
    return JSONNull::static_null;
}
JSON const& JSON::operator[](size_t) const
{
    cerr << "Cannot access " << get_type() << " type using array subscription"
         << endl;
    exit(1);
}

size_t JSON::size(void) const
{
    cerr << "Cannot access size of " << get_type() << " type" << endl;
    exit(1);
}
vector<unique_ptr<JSON>> const& JSON::as_vector(void) const
{
    cerr << "Cannot assess " << get_type() << " type as vector" << endl;
    exit(1);
}

// JSONObject
JSON const& JSON::get(char const*) const
{
    return JSONNull::static_null;
}
JSON const& JSON::get(string const&) const
{
    return get((char const*)nullptr);
}
JSON const& JSON::operator[](char const*) const
{
    cerr << "Cannot access " << get_type() << " type as a dictionary" << endl;
    exit(1);
}
JSON const& JSON::operator[](string const&) const
{
    cerr << "Cannot access " << get_type() << " type as a dictionary" << endl;
    exit(1);
}

bool JSON::has(char const*) const
{
    return operator[]("");
}
bool JSON::has(string const&) const
{
    return has((char const*)nullptr);
}

// JSONString
JSON::operator string const&() const
{
    cerr << "Cannot convert " << get_type() << " type to 'string'" << endl;
    exit(1);
}

string const& JSON::as_string_with_default(string const& default_value) const
{
    return default_value;
}

// JSONNumber
JSON::operator uint8_t() const
{
    cerr << "Cannot convert " << get_type() << " type to 'uint8_t'" << endl;
    exit(1);
}
uint8_t JSON::as_uint8_t_with_default(uint8_t default_value) const
{
    return default_value;
}
JSON::operator int() const
{
    cerr << "Cannot convert " << get_type() << " type to 'int'" << endl;
    exit(1);
}
int JSON::as_int_with_default(int default_value) const
{
    return default_value;
}
JSON::operator unsigned() const
{
    cerr << "Cannot convert " << get_type() << " type to 'unsigned'" << endl;
    exit(1);
}
unsigned JSON::as_unsigned_with_default(unsigned default_value) const
{
    return default_value;
}
JSON::operator long() const
{
    cerr << "Cannot convert " << get_type() << " type to 'long'" << endl;
    exit(1);
}
long JSON::as_long_with_default(long default_value) const
{
    return default_value;
}
JSON::operator double() const
{
    cerr << "Cannot convert " << get_type() << " type to 'double'" << endl;
    exit(1);
}
double JSON::as_double_with_default(double default_value) const
{
    return default_value;
}

// JSONBool
JSON::operator bool() const
{
    cerr << "Cannot convert " << get_type() << " type to 'bool'" << endl;
    exit(1);
}
bool JSON::as_bool_with_default(bool default_value) const
{
    return default_value;
}

/******************************************************************************/

void test_json(void)
{
    string jsonstring = "{\"asd\":\"asd\", \"qwe\":123}";
    char const* ptr = jsonstring.c_str();
    auto json_ptr = JSON::parse(ptr);
    auto& json = *json_ptr;
    string asd(json["asd"]);
    string qwe(json["qwe"]);
}
