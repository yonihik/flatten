#pragma once

#include <concepts>
#include <iterator>
#include <ranges>

#include <iostream>

template <std::ranges::random_access_range Outer>
  requires std::ranges::view<Outer> &&
           std::ranges::random_access_range<std::ranges::range_value_t<Outer>>
class flatten_view : public std::ranges::view_interface<flatten_view<Outer>> {
  Outer m_outer;

public:
  using Inner = std::ranges::range_value_t<Outer>;
  using OuterIter = std::ranges::iterator_t<Outer>;
  using OuterSentinel = std::ranges::sentinel_t<Outer>;
  using InnerIter = std::ranges::iterator_t<Inner>;
  using InnerSentinel = std::ranges::sentinel_t<Inner>;

public:
  class iterator;

  class sentinel {
    friend class iterator;

  public:
    sentinel() = default;
    sentinel(OuterSentinel outer_end, InnerSentinel inner_end)
        : m_outer_end(outer_end), m_inner_end(inner_end) {
      std::cout << "sentinel constructor called" << std::endl;
    }

  private:
    OuterSentinel m_outer_end;
    InnerSentinel m_inner_end;
  };

public:
  class iterator {
    friend class sentinel;

  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = std::ranges::range_value_t<Inner>;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type &;
    using iterator_concept = std::random_access_iterator_tag;

    iterator() = default;
    iterator(OuterIter outer_it, OuterSentinel outer_end,
             std::ptrdiff_t inner_idx)
        : m_outer_it(outer_it), m_outer_end(outer_end),
          m_inner_view(*m_outer_it) {
      auto it = std::ranges::begin(m_inner_view) + inner_idx;
      m_inner_it = it;
      std::cout << "iterator constructor called" << std::endl;
    }

    // what if we copt an iterator that reached the end?
    iterator(const iterator &other)
        : m_outer_it(other.m_outer_it), m_outer_end(other.m_outer_end), m_inner_view(other.m_inner_view) {
      if (m_outer_it != m_outer_end) {
        m_inner_view = *m_outer_it;
        m_inner_it =
            std::ranges::begin(m_inner_view) +
            (other.m_inner_it - std::ranges::begin(other.m_inner_view));
      }
    }

    friend void swap(iterator &first, iterator &second) {
      using std::swap;

      // by swapping the members of two objects,
      // the two objects are effectively swapped
      swap(first.m_outer_it, second.m_outer_it);
      swap(first.m_outer_end, second.m_outer_end);
      swap(first.m_inner_view, second.m_inner_view);
      swap(first.m_inner_it, second.m_inner_it);
    }

    iterator &operator=(const iterator &other) {
      iterator tmp(other);
      swap(*this, tmp);
      return *this;
    }
    value_type operator*() const { return *m_inner_it; }

    iterator &operator++() {
      ++m_inner_it;
      skip_empty();
      return *this;
    }

    iterator &operator--() {
      while (m_inner_it == InnerIter{} ||
             m_inner_it == std::ranges::begin(m_inner_view)) {
        --m_outer_it;
        m_inner_view = *m_outer_it;
        m_inner_it = std::ranges::end(m_inner_view);
      }
      --m_inner_it;

      return *this;
    }

    iterator operator--(int) {
      auto tmp = *this;
      --*this;
      return tmp;
    }

    iterator operator++(int) {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    bool operator==(const sentinel &s) const {
      return m_outer_it == s.m_outer_end && m_inner_it == s.m_inner_end;
      ;
    }

    bool operator>=(const iterator &other) const {
      if (m_outer_it > other.m_outer_it) {
        return true;
      } else if (m_outer_it < other.m_outer_it) {
        return false;
      }
      return m_inner_it >= other.m_inner_it;
    }

    bool operator>(const iterator &other) const {
      if (m_outer_it > other.m_outer_it) {
        return true;
      } else if (m_outer_it < other.m_outer_it) {
        return false;
      }
      return m_inner_it > other.m_inner_it;
    }

    bool operator<=(const iterator &other) const {
      if (m_outer_it < other.m_outer_it) {
        return true;
      } else if (m_outer_it > other.m_outer_it) {
        return false;
      }
      return m_inner_it <= other.m_inner_it;
    }

    bool operator<(const iterator &other) const {
      if (m_outer_it < other.m_outer_it) {
        return true;
      } else if (m_outer_it > other.m_outer_it) {
        return false;
      }
      return m_inner_it < other.m_inner_it;
    }

    bool operator==(const iterator &other) const {
      return m_outer_it == other.m_outer_it && m_inner_it == other.m_inner_it;
    }

    bool operator!=(const iterator &other) const { return !(*this == other); }

    // Random access support
    iterator &operator+=(std::ptrdiff_t n) {
      while (n > 0) {
        auto remain = std::ranges::end(m_inner_view) - m_inner_it;
        if (n < remain) {
          m_inner_it += n;
          break;
        } else {
          n -= remain;
          ++m_outer_it;
          m_inner_view = *m_outer_it;
          m_inner_it = std::ranges::begin(m_inner_view);
        }
      }

      return *this;
    }

    // Random access support
    iterator &operator-=(std::ptrdiff_t n) {
      while (n > 0) {
        auto remain = m_inner_it - std::ranges::begin(m_inner_view);
        if (n < remain) {
          m_inner_it -= n;
          break;
        } else {
          n -= remain;
          --m_outer_it;
          m_inner_view = *m_outer_it;
          m_inner_it = std::ranges::end(m_inner_view);
        }
      }

      return *this;
    }

    iterator operator+(std::ptrdiff_t n) const {
      iterator tmp = *this;
      tmp += n;
      return tmp;
    }
    // Friend operator+ for ADL
    friend iterator operator+(std::ptrdiff_t n, const iterator &it) {
      return it + n;
    }

    iterator operator-(std::ptrdiff_t n) const {
      iterator tmp = *this;
      tmp -= n;
      return tmp;
    }

    std::ptrdiff_t operator-(const iterator &other) const {
      // Only valid if both iterators are from the same flatten_view and other
      // <= *this
      std::ptrdiff_t dist = 0;
      auto it = other;
      // auto oit = other.m_outer_it;
      // auto inner_view = *oit;
      // auto iit = inner_view.begin() + (other.m_inner_it -
      // other.m_inner_view.begin());
      while (it.m_outer_it != m_outer_it) {
        // maybe simply add std::ranges::size
        // std::cout << std::ranges::size(it.m_inner_view) << std::endl;
        // std::cout << (it.m_inner_it - std::ranges::begin(it.m_inner_view))
        //           << std::endl;
        dist += std::ranges::size(it.m_inner_view) -
                (it.m_inner_it - std::ranges::begin(it.m_inner_view));
        ++it.m_outer_it;
        it.m_inner_view = *it.m_outer_it;
        it.m_inner_it = std::ranges::begin(it.m_inner_view);
      }
      dist +=
          std::ranges::distance(std::ranges::begin(m_inner_view), m_inner_it) -
          std::ranges::distance(std::ranges::begin(it.m_inner_view),
                                it.m_inner_it);
      return dist;
    }

    value_type operator[](std::ptrdiff_t n) const {
      auto tmp = *this;
      tmp += n;
      return *tmp;
    }

  public:
    void skip_empty() {
      while (m_inner_it == std::ranges::end(m_inner_view)) {
        ++m_outer_it;
        if (m_outer_it == m_outer_end) {
          // Reached the end of the outer range
          m_inner_it = InnerIter{};
          return;
        }
        m_inner_view = *m_outer_it;
        m_inner_it = std::ranges::begin(m_inner_view);
      }
    }

    OuterIter m_outer_it;
    OuterSentinel m_outer_end;
    mutable Inner m_inner_view;
    InnerIter m_inner_it;
  };

public:
  flatten_view() = default;
  explicit flatten_view(Outer outer) : m_outer(std::move(outer)) {}

  iterator begin() {
    auto outer_begin = std::ranges::begin(m_outer);
    auto outer_end = std::ranges::end(m_outer);
    // std::cout << *((*outer_begin).begin()) << std::endl; // Debugging line
    return iterator(outer_begin, outer_end, 0);
  }
  sentinel end() {
    auto outer_end = std::ranges::end(m_outer);
    return sentinel(outer_end, InnerSentinel{});
  }

  iterator::value_type operator[](std::size_t n) {
    auto it = begin();
    it += n;
    return *it;
  }

  // Size support
  size_t size() const {
    std::size_t total = 0;
    for (const auto &v : m_outer)
      total += std::ranges::size(v);
    return total;
  }
};

template <std::ranges::random_access_range Outer>
flatten_view(Outer &&) -> flatten_view<std::views::all_t<Outer>>;
