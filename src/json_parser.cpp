#include <map>
#include <string>
#include <vector>

#include <cstring>

#include "json_parser.hpp"
#include "util.hpp"

namespace JSON {
void Object::parse_kv_pair(char const*& str) {
    std::string key(escaped_string(str));
    whitespace(str);
    char c = next(str);
    if(c != ':') {
        throw std::string("Expected ':', got: '") + c + "'.";
    }

    whitespace(str);
    map.emplace(move(key), Node::parse(str));
}

Object::Object(char const*& str) {
    char c = *str;
    if(c != '{') {
        throw std::string("Expected '{', got: '") + c + "'.";
    }

    while(true) {
        whitespace(str);
        c = next(str);
        char delimiter = map.empty() ? '{' : ',';
        if(c == delimiter) {
            whitespace(str);
            // Lookahead, don't consume `c`.
            c = *str;
            if(c == '}' && map.empty()) {
                str++;
                break;
            }
            parse_kv_pair(str);
        } else if(c == '}') {
            break;
        } else if(c == '\0') {
            throw "Unexpected end of string.";
        } else {
            throw std::string("Expected '") + delimiter + "' or '}', got '" + c + "'.";
        }
    }
}

Array::Array(char const*& str) {
    char c = *str;
    if(c != '[') {
        throw "Array not starting at the given position.";
    }

    while(true) {
        whitespace(str);
        c = next(str);
        char delimiter = vector.empty() ? '[' : ',';
        if(c == delimiter) {
            whitespace(str);
            // Lookahead (don't consume `c`).
            c = *str;
            if(c == ']' && vector.empty()) {
                str++;
                break;
            }

            vector.emplace_back(Node::parse(str));
        } else if(c == ']') {
            break;
        } else if(c == '\0') {
            throw "Unexpected end of string.";
        } else {
            throw std::string("Expected '") + delimiter + "' or ']', got '" + c + "'.";
        }
    }
}

String::String(char const*& str) { string_value = escaped_string(str); }

Number::Number(char const*& str) { string_value = number_str(str); }

Bool::Bool(char const*& str) {
    if(strncmp(str, "false", 5) == 0) {
        b = false;
        str += 5;
    } else if(strncmp(str, "true", 4) == 0) {
        b = true;
        str += 4;
    } else {
        throw std::string("Could not detect neither true nor false: '") + std::string(str, 5) + "'.";
    }
}

Null::Null(char const*& str) {
    if(strncmp(str, "null", 4) == 0) {
        str += 4;
    } else {
        throw std::string("Could not detect null: '") + std::string(str, 4) + "'.";
    }
}

std::shared_ptr<Null> Null::static_null;
} // namespace JSON
