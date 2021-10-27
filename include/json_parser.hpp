#ifndef JSON_PARSER_HPP
#define JSON_PARSER_HPP

#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "utils/parse.hpp"

namespace JSON {
class Value {
  protected:
    Value() = default;

  public:
    virtual ~Value() = default;
};

class Node;

class Object final : public Value {
    friend class Node;

  private:
    std::map<std::string, std::shared_ptr<Value>> map;
    void parse_kv_pair(StringPointer& str); // NOLINT: Reference intended.
    explicit Object(StringPointer& str);    // NOLINT: Reference intended.
};

class Array final : public Value {
    friend class Node;

  private:
    std::vector<std::shared_ptr<Value>> vector;
    explicit Array(StringPointer& str); // NOLINT: Reference intended.
};

class String final : public Value {
    friend class Node;

  private:
    std::string string_value;
    explicit String(StringPointer& str); // NOLINT: Reference intended.
};

class Number final : public Value {
    friend class Node;

  private:
    std::string string_value;
    explicit Number(StringPointer& str); // NOLINT: Reference intended.
};

class Bool final : public Value {
    friend class Node;

  private:
    bool b;
    explicit Bool(StringPointer& str); // NOLINT: Reference intended.
};

class Null final : public Value {
    friend class Node;

  private:
    static std::shared_ptr<Null> static_null;
    Null() = default;
    explicit Null(StringPointer& str); // NOLINT: Reference intended.
};

class Node final {
  private:
    std::shared_ptr<Value> value;

    template <typename ValueType>
    auto const* as() const {
        return dynamic_cast<ValueType*>(value.get());
    }

  public:
    static std::shared_ptr<Value> parse(StringPointer& str) { // NOLINT: Reference intended.
        str.whitespace();
        // Lookahead, don't consume `c`.
        char c = *str;
        switch(c) {
        case '\0':
            return std::shared_ptr<Null>(new Null());
        case '{':
            return std::shared_ptr<Object>(new Object(str));
        case '[':
            return std::shared_ptr<Array>(new Array(str));
        case '"':
            return std::shared_ptr<String>(new String(str));
        case 't':
        case 'f':
            return std::shared_ptr<Bool>(new Bool(str));
        case 'n':
            return std::shared_ptr<Null>(new Null(str));
        default:
            if((c >= '0' && c <= '9') || c == '.' || c == '+' || c == '-') {
                return std::shared_ptr<Number>(new Number(str));
            }
            break;
        }

        std::ostringstream out;
        out << "No valid Value detected: [" << c << "](" << static_cast<int>(c) << ").";
        throw out.str();
    }

    explicit Node() { value = std::shared_ptr<Null>(new Null()); }

    explicit Node(char const* str) {
        StringPointer sp(str);
        value = parse(sp);
    }
    explicit Node(std::shared_ptr<Value> ptr) { value = std::move(ptr); }

    bool exists() const {
        // We expect the pointer to not be convertible to `Null`.
        return value != nullptr && dynamic_cast<Null*>(value.get()) == nullptr;
    }

    // Object functions.
    using _ObjectConstIterBase = std::map<std::string, std::shared_ptr<Value>>::const_iterator;
    class ObjectConstIter : public _ObjectConstIterBase {
      public:
        explicit ObjectConstIter(_ObjectConstIterBase iterator) : _ObjectConstIterBase(iterator) {}
        // NOLINTNEXTLINE: `operator*` overload desired.
        std::pair<std::string const&, Node const> operator*() const {
            auto const& base_deref = _ObjectConstIterBase::operator*();
            return std::make_pair(base_deref.first, Node(base_deref.second));
        }
    };
    class ObjectWrapper {
      private:
        using container_type = std::map<std::string, std::shared_ptr<Value>>;
        // Used to create and use `ObjectWrapper` objects as r-values.
        std::shared_ptr<Value> const source_json;
        container_type const& map;

      public:
        ObjectWrapper(std::shared_ptr<Value> json, container_type const& map_)
            : source_json(std::move(json)), map(map_) {}
        ObjectConstIter begin() const { return ObjectConstIter{map.cbegin()}; }
        ObjectConstIter end() const { return ObjectConstIter{map.cend()}; }
    };
    const ObjectWrapper object() const {
        auto const* object_ptr = as<Object>();
        if(object_ptr == nullptr) {
            throw "Cannot iterate over non-object JSON type.";
        }
        return ObjectWrapper(value, object_ptr->map);
    }
    const Node object_at(std::string const& key) const {
        auto const* object_ptr = as<Object>();
        if(object_ptr == nullptr) {
            return Node(Null::static_null);
        }

        const auto& map = object_ptr->map;
        auto const iter = map.find(key);
        return Node(iter == map.cend() ? Null::static_null : iter->second);
    }
    // NOLINTNEXTLINE: `operator[]` overload desired.
    const Node operator[](std::string const& key) const { return object_at(key); }
    // NOLINTNEXTLINE: `operator[]` overload desired.
    const Node operator[](char const* key) const { return object_at(std::string(key)); }

    // Array functions.
    using _ArrayConstIterBase = std::vector<std::shared_ptr<Value>>::const_iterator;
    class ArrayConstIter : public _ArrayConstIterBase {
      public:
        explicit ArrayConstIter(_ArrayConstIterBase&& iterator) : _ArrayConstIterBase(iterator) {}
        // NOLINTNEXTLINE: `operator*` overload desired.
        Node const operator*() const { return Node(_ArrayConstIterBase::operator*()); }
    };
    class ArrayWrapper {
      private:
        using container_type = std::vector<std::shared_ptr<Value>>;
        // Used to create and use `ArrayWrapper` objects as r-values.
        std::shared_ptr<Value> const source_json;
        container_type const& array;

      public:
        ArrayWrapper(std::shared_ptr<Value> json, container_type const& array_)
            : source_json(std::move(json)), array(array_) {}
        size_t size() const { return array.size(); }
        ArrayConstIter begin() const { return ArrayConstIter{array.cbegin()}; }
        ArrayConstIter end() const { return ArrayConstIter{array.cend()}; }
        Node const operator[](container_type::size_type index) const { return Node(array.at(index)); }
    };
    const ArrayWrapper array() const {
        auto const* array_ptr = as<Array>();
        if(array_ptr == nullptr) {
            throw "Cannot iterate ovev non-array JSON type.";
        }
        return ArrayWrapper(value, array_ptr->vector);
    }
    const Node array_at(size_t index) const {
        auto const* array_ptr = as<Array>();
        if(array_ptr == nullptr) {
            return Node(Null::static_null);
        }
        const auto& vector = array_ptr->vector;
        return Node(index >= vector.size() ? Null::static_null : vector[index]);
    }
    template <typename IndexType, typename std::enable_if_t<std::is_integral<IndexType>::value>::type* = nullptr>
    Node operator[](IndexType index) const { // NOLINT: `operator[]` overload desired.
        return array_at(static_cast<size_t>(index));
    }

    // String functions.
    std::string const& string(std::string const& default_value = "") const {
        auto const* string_ptr = as<String>();
        return string_ptr != nullptr ? string_ptr->string_value : default_value;
    }

    // Number functions.
    template <typename T>
    T number(T default_value = 0) const {
        auto const* number_ptr = as<Number>();
        if(number_ptr != nullptr) {
            return convert_to_number<T>(number_ptr->string_value.c_str(), default_value);
        }
        return default_value;
    }

    // Bool functions.
    bool boolean(bool default_value = false) const {
        auto const* bool_ptr = as<Bool>();
        return bool_ptr != nullptr ? bool_ptr->b : default_value;
    }
}; // namespace JSON
} // namespace JSON

#endif
