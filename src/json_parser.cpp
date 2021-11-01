#include <map>
#include <string>
#include <vector>

#include <cstring>

#include<fmt/format.h>

#include "json_parser.hpp"

#include "utils/parse.hpp"

namespace JSON {
void Object::parse_kv_pair(StringPointer& str) {
    std::string key(str.escaped_string());
    str.whitespace();
    char c = str.next();
    if(c != ':') {
        throw std::string("Expected ':', got: '") + c + "'.";
    }

    str.whitespace();
    map.emplace(move(key), Node::parse(str));
}

Object::Object(StringPointer& str) {
    char c = str.peek();
    if(c != '{') {
        throw std::string("Expected '{', got: '") + c + "'.";
    }

    while(true) {
        str.whitespace();
        c = str.next();
        char delimiter = map.empty() ? '{' : ',';
        if(c == delimiter) {
            str.whitespace();
            // Lookahead, don't consume `c`.
            c = str.peek();
            if(c == '}' && map.empty()) {
                str.skip(1);
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

std::ostream& Object::to_stream(std::ostream& stream) {
    stream << "{" << std::endl;

    bool first = true;
    for (auto const& item : map) {
        if (first) {
            first = false;
        } else {
            stream << ",\n";
        }
        item.second->to_stream(stream << fmt::format("\"{}\": ", item.first));
    }
    return stream << std::endl << "}";
}

Array::Array(StringPointer& str) {
    char c = str.peek();
    if(c != '[') {
        throw "Array not starting at the given position.";
    }

    while(true) {
        str.whitespace();
        c = str.next();
        char delimiter = vector.empty() ? '[' : ',';
        if(c == delimiter) {
            str.whitespace();
            // Lookahead (don't consume `c`).
            c = str.peek();
            if(c == ']' && vector.empty()) {
                str.skip(1);
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

std::ostream& Array::to_stream(std::ostream& stream) {
    stream << "[" << std::endl;

    bool first = true;
    for (auto const& item : vector) {
        if (first) {
            first = false;
        } else {
            stream << ",\n";
        }
        item->to_stream(stream);
    }
    return stream << std::endl << "]";
}

String::String(StringPointer& str) { string_value = str.escaped_string(); }

std::ostream& String::to_stream(std::ostream& stream) {
    return stream << fmt::format("\"{}\"", string_value);
}

Number::Number(StringPointer& str) { string_value = str.number_str<long double>(); }

std::ostream& Number::to_stream(std::ostream& stream) {
    return stream << string_value;
}

Bool::Bool(StringPointer& str) {
    if(strncmp(str, "false", 5) == 0) {
        b = false;
        str.skip(5);
    } else if(strncmp(str, "true", 4) == 0) {
        b = true;
        str.skip(4);
    } else {
        throw std::string("Could not detect neither true nor false: '") + std::string(str, 5) + "'.";
    }
}

std::ostream& Bool::to_stream(std::ostream& stream) {
    return stream << (b ? "true" : "false");
}

Null::Null(StringPointer& str) {
    if(strncmp(str, "null", 4) == 0) {
        str.skip(4);
    } else {
        throw std::string("Could not detect null: '") + std::string(str, 4) + "'.";
    }
}

std::ostream& Null::to_stream(std::ostream& stream) {
    return stream << "null";
}

std::shared_ptr<Null> Null::static_null;
} // namespace JSON
