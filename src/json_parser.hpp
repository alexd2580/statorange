#ifndef __COMPACT_JSON_PARSER__
#define __COMPACT_JSON_PARSER__

#include <fmt/format.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "util.hpp"

namespace JSON
{
class Node;

class Value
{
  protected:
    Value(void) = default;

  public:
    virtual ~Value(void) = default;
};

class Node;

class Object final : public Value
{
    friend class Node;

  private:
    std::map<std::string, std::shared_ptr<Value>> map;
    void parse_kv_pair(char const*& str);
    Object(char const*& str);
};

class Array final : public Value
{
    friend class Node;

  private:
    std::vector<std::shared_ptr<Value>> vector;
    Array(char const*& str);
};

class String final : public Value
{
    friend class Node;

  private:
    std::string string_value;
    explicit String(char const*& str);
};

class Number final : public Value
{
    friend class Node;

  private:
    std::string string_value;
    explicit Number(char const*& str);
};

class Bool final : public Value
{
    friend class Node;

  private:
    bool b;
    explicit Bool(char const*& str);
};

class Null final : public Value
{
    friend class Node;

  private:
    static std::shared_ptr<Null> static_null;
    Null(void) = default;
    Null(char const*& str);
};

class Node final
{
  private:
    std::shared_ptr<Value> value;

    template <typename ValueType> ValueType const* as(void) const
    {
        return dynamic_cast<ValueType*>(value.get());
    }

  public:
    static std::shared_ptr<Value> parse(char const*& str)
    {
        whitespace(str);
        // Lookahead, don't consume `c`.
        char c = *str;
        switch(c)
        {
        case '\0':
            throw "Unexpected end of string.";
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
            if((c >= '0' && c <= '9') || c == '.' || c == '+' || c == '-')
                return std::shared_ptr<Number>(new Number(str));
            break;
        }

        throw fmt::format("No valid Value detected: {}.", c);
    }

    Node(char const* str)
    {
        value = parse(str);
    }
    Node(std::shared_ptr<Value> ptr)
    {
        value = ptr;
    }

    bool exists(void) const
    {
        // We expect the pointer to not be convertible to `Null`.
        return value.get() != nullptr &&
               dynamic_cast<Null*>(value.get()) == nullptr;
    }

    // Object functions.
    class ObjectConstIterator
    {
      private:
        using container_type = std::map<std::string, std::shared_ptr<Value>>;
        using iterator_type = container_type::const_iterator;
        iterator_type iterator;

      public:
        ObjectConstIterator(iterator_type iterator_)
        {
            iterator = iterator_;
        }
        ObjectConstIterator& operator++(void)
        {
            iterator++;
            return *this;
        }
        bool operator!=(ObjectConstIterator const& other) const
        {
            return iterator != other.iterator;
        }
        std::pair<std::string const&, Node const> operator*(void)
        {
            return std::make_pair(iterator->first, Node(iterator->second));
        }
    };
    class ObjectWrapper
    {
      private:
        using container_type = std::map<std::string, std::shared_ptr<Value>>;
        // Used to create and use `ObjectWrapper` objects as r-values.
        std::shared_ptr<Value> const source_json;
        container_type const& map;

      public:
        ObjectWrapper(std::shared_ptr<Value> json, container_type const& map_)
            : source_json(json), map(map_)
        {
        }
        ObjectConstIterator begin(void) const
        {
            return ObjectConstIterator(map.cbegin());
        }
        ObjectConstIterator end(void) const
        {
            return ObjectConstIterator(map.cend());
        }
    };
    const ObjectWrapper object(void) const
    {
        Object const* object_ptr = as<Object>();
        if(object_ptr == nullptr)
            throw "Cannot iterate ovev non-object JSON type.";
        return ObjectWrapper(value, object_ptr->map);
    }
    const Node object_at(std::string const& key) const
    {
        Object const* object_ptr = as<Object>();
        if(object_ptr == nullptr)
            return Node(Null::static_null);

        const auto& map = object_ptr->map;
        auto const iter = map.find(key);
        return Node(iter == map.cend() ? Null::static_null : iter->second);
    }
    const Node operator[](std::string const& key) const
    {
        return object_at(key);
    }
    const Node operator[](char const* key) const
    {
        return object_at(std::string(key));
    }

    // Array functions.
    class ArrayConstIterator
    {
      private:
        using container_type = std::vector<std::shared_ptr<Value>>;
        using iterator_type = container_type::const_iterator;
        iterator_type iterator;

      public:
        ArrayConstIterator(iterator_type iterator_)
        {
            iterator = iterator_;
        }
        ArrayConstIterator& operator++(void)
        {
            iterator++;
            return *this;
        }
        bool operator!=(ArrayConstIterator const& other) const
        {
            return iterator != other.iterator;
        }
        Node const operator*(void)
        {
            return Node(*iterator);
        }
    };
    class ArrayWrapper
    {
      private:
        using container_type = std::vector<std::shared_ptr<Value>>;
        // Used to create and use `ArrayWrapper` objects as r-values.
        std::shared_ptr<Value> const source_json;
        container_type const& array;

      public:
        ArrayWrapper(
            std::shared_ptr<Value> const json, container_type const& array_)
            : source_json(json), array(array_)
        {
        }
        size_t size(void) const
        {
            return array.size();
        }
        ArrayConstIterator begin(void) const
        {
            return ArrayConstIterator(array.cbegin());
        }
        ArrayConstIterator end(void) const
        {
            return ArrayConstIterator(array.cend());
        }
    };
    const ArrayWrapper array(void) const
    {
        Array const* array_ptr = as<Array>();
        if(array_ptr == nullptr)
            throw "Cannot iterate ovev non-array JSON type.";
        return ArrayWrapper(value, array_ptr->vector);
    }
    const Node array_at(size_t index) const
    {
        Array const* array_ptr = as<Array>();
        if(array_ptr == nullptr)
            return Node(Null::static_null);

        const auto& vector = array_ptr->vector;
        return Node(index >= vector.size() ? Null::static_null : vector[index]);
    }
    template <
        typename IndexType,
        typename std::enable_if_t<std::is_integral<IndexType>::value>::type* =
            nullptr>
    Node operator[](IndexType index) const
    {
        return array_at((size_t)index);
    }

    // String functions.
    std::string const& string(std::string const& default_value = "") const
    {
        String const* string_ptr = as<String>();
        if(string_ptr != nullptr)
            return string_ptr->string_value;
        return default_value;
    }

    // Number functions.
    template <typename T> T number(T default_value = 0) const
    {
        Number const* number_ptr = as<Number>();
        if(number_ptr != nullptr)
            return convertToNumber<T>(
                number_ptr->string_value.c_str(), default_value);
        return default_value;
    }

    // Bool functions.
    bool boolean(bool default_value = false) const
    {
        Bool const* bool_ptr = as<Bool>();
        if(bool_ptr != nullptr)
            return bool_ptr->b;
        return default_value;
    }
};
}

#endif
