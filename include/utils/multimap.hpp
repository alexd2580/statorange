#ifndef UTILS_MULTIMAP_HPP
#define UTILS_MULTIMAP_HPP

#include <iterator>
#include <map>
#include <type_traits>
#include <utility>

template <typename index_t, typename values_t>
struct Iterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;

  private:
    using raw_value_t = typename values_t::mapped_type;
    using value_t = std::conditional_t<std::is_const_v<index_t>, raw_value_t const, raw_value_t>;
    using iter_t = typename index_t::iterator;
    using const_iter_t = typename index_t::const_iterator;
    using index_iter_t = std::conditional_t<std::is_const_v<index_t>, const_iter_t, iter_t>;

    values_t& m_values;
    index_iter_t m_index_iter;

  public:
    Iterator(values_t& values, index_iter_t index_iter) : m_values(values), m_index_iter(index_iter) {}

    value_t& operator*() const { return m_values.at(m_index_iter->second); }
    value_t* operator->() { return *this; }

    Iterator& operator++() {
        m_index_iter++;
        return *this;
    }

    Iterator operator++(int) {
        Iterator tmp(*this);
        ++*this;
        return tmp;
    }

    Iterator operator+(int offset) const {
        auto new_iter = m_index_iter;
        std::advance(new_iter, offset);
        return Iterator(m_values, new_iter);
    }

    friend bool operator==(Iterator const& a, Iterator const& b) { return a.m_index_iter == b.m_index_iter; }
    friend bool operator!=(Iterator const& a, Iterator const& b) { return a.m_index_iter != b.m_index_iter; }
};

template <typename index_t, typename values_t>
struct PrimedForTwoStageIter {
    values_t& m_values;
    index_t& m_index;

    decltype(auto) begin() { return Iterator<index_t, values_t>(m_values, m_index.begin()); }
    decltype(auto) end() { return Iterator<index_t, values_t>(m_values, m_index.end()); }

    decltype(auto) cbegin() const { return Iterator<index_t const, values_t const>(m_values, m_index.cbegin()); }
    decltype(auto) cend() const { return Iterator<index_t const, values_t const>(m_values, m_index.cend()); }
};

template <typename K1, typename K2, typename V>
class Multimap {
    int next_free = 0;

    std::map<int, V> values;
    std::map<K1, int> index1;
    std::map<K2, int> index2;

  public:
    template <class... Args>
    decltype(auto) emplace(K1 k1, K2 k2, Args&&... args) {
        auto exists = values.try_emplace(next_free, args...).second;
        index1.emplace(k1, next_free);
        index2.emplace(k2, next_free);
        next_free++;
        return std::make_pair(Iterator<decltype(index1), decltype(values)>(values, index1.find(k1)), exists);
    }

    template <typename = std::enable_if_t<!std::is_same_v<K1, K2>>>
    V& operator[](K1 k1) {
        return values[index1[k1]];
    }
    template <typename = std::enable_if_t<!std::is_same_v<K1, K2>>>
    V& at(K1 k1) {
        return values.at(index1.at(k1));
    }
    template <typename = std::enable_if_t<!std::is_same_v<K1, K2>>>
    V const& at(K1 k1) const {
        return values.at(index1.at(k1));
    }
    template <typename = std::enable_if_t<!std::is_same_v<K1, K2>>>
    V& operator[](K2 k2) {
        return values[index2[k2]];
    }
    template <typename = std::enable_if_t<!std::is_same_v<K1, K2>>>
    V& at(K2 k2) {
        return values.at(index2.at(k2));
    }
    template <typename = std::enable_if_t<!std::is_same_v<K1, K2>>>
    V const& at(K2 k2) const {
        return values.at(index2.at(k2));
    }

    template <typename = std::enable_if_t<!std::is_same_v<K1, K2>>>
    bool contains(K1 k1) const {
        return index1.find(k1) != index1.cend();
    }
    template <typename = std::enable_if_t<!std::is_same_v<K1, K2>>>
    bool contains(K2 k2) const {
        return index2.find(k2) != index2.cend();
    }

    void clear() {
        values.clear();
        index1.clear();
        index2.clear();
    }

    struct PrimedForTwoStageIter<decltype(index1), decltype(values)> iterableFromIndex1() {
        return {this->values, this->index1};
    }

    struct PrimedForTwoStageIter<decltype(index1) const, decltype(values) const>
    iterableFromIndex1() const {
        return {this->values, this->index1};
    }

    struct PrimedForTwoStageIter<decltype(index2), decltype(values)>
    iterableFromIndex2() {
        return {this->values, this->index2};
    }

    struct PrimedForTwoStageIter<decltype(index2) const, decltype(values) const>
    iterableFromIndex2() const {
        return {this->values, this->index2};
    }
};

#endif
