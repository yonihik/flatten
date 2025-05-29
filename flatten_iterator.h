#pragma once

#include "range/v3/range/concepts.hpp"
#include <concepts>
#include <iterator>
#include <ranges>
namespace flatten {

template <typename OuterSentinel, typename InnerSentinel> struct sentinel {
  sentinel() = default;
  sentinel(OuterSentinel outer_end, InnerSentinel inner_end)
      : m_outer_end(outer_end), m_inner_end(inner_end) {}

  OuterSentinel m_outer_end;
  InnerSentinel m_inner_end;
};
template <std::input_iterator OuterIter,
          std::sentinel_for<OuterIter> OuterSentinel>
  requires std::ranges::input_range<std::iter_value_t<OuterIter>>
class iterator {

  using Inner = std::iter_value_t<OuterIter>;
  using InnerIter = std::ranges::iterator_t<Inner>;
  using InnerSentinel = std::ranges::sentinel_t<Inner>;

public:
  using value_type = std::ranges::range_value_t<Inner>;

  iterator() = default;

  iterator(OuterIter outer_it, OuterSentinel outer_end)
    requires std::input_iterator<OuterIter> &&
                 std::ranges::input_range<Inner>
      : m_outer_it(outer_it), m_outer_end(outer_end) {
    if (m_outer_it != m_outer_end) {
      m_inner_view = *m_outer_it;
      m_inner_it = std::ranges::begin(*m_inner_view);
    }
  }
  // assumes outer_it contains at least inner_idx elements
  iterator(OuterIter outer_it, OuterSentinel outer_end, size_t inner_idx)
    requires std::input_iterator<OuterIter> &&
                 std::ranges::random_access_range<Inner>
      : m_outer_it(outer_it), m_outer_end(outer_end) {
    if (m_outer_it != m_outer_end) {
      m_inner_view = *m_outer_it;
      m_inner_it = std::ranges::begin(*m_inner_view) + inner_idx;
    }
  }
  // i have no idea how to copy if Inner is not a random acess range
  iterator(const iterator &other)
      : iterator(other.m_outer_it, other.m_outer_end, other.inner_index()) {}

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
           m_inner_it == std::ranges::begin(*m_inner_view)) {
      --m_outer_it;
      m_inner_view = *m_outer_it;
      m_inner_it = std::ranges::end(*m_inner_view);
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
  bool operator==(const sentinel<OuterSentinel, InnerSentinel> &s) const {
    return m_outer_it == s.m_outer_end && m_inner_it == s.m_inner_end;
  }
  bool operator>=(const iterator &other) const
    requires std::forward_iterator<OuterIter> &&
             std::ranges::forward_range<Inner>
  {
    if (m_outer_it >= other.m_outer_it) {
      return true;
    } else if (m_outer_it < other.m_outer_it) {
      return false;
    }
    return inner_index() >= other.inner_index();
  }

  bool operator>(const iterator &other) const
    requires std::forward_iterator<OuterIter> &&
             std::ranges::forward_range<Inner>
  {
    if (m_outer_it > other.m_outer_it) {
      return true;
    } else if (m_outer_it < other.m_outer_it) {
      return false;
    }
    return inner_index() > other.inner_index();
  }

  bool operator<=(const iterator &other) const
    requires std::forward_iterator<OuterIter> &&
             std::ranges::forward_range<Inner>

  {
    if (m_outer_it < other.m_outer_it) {
      return true;
    } else if (m_outer_it > other.m_outer_it) {
      return false;
    }
    return inner_index() <= other.inner_index();
  }

  bool operator<(const iterator &other) const
    requires std::forward_iterator<OuterIter> &&
             std::ranges::forward_range<Inner>

  {
    if (m_outer_it < other.m_outer_it) {
      return true;
    } else if (m_outer_it > other.m_outer_it) {
      return false;
    }
    return m_inner_it < other.m_inner_it;
  }

  bool operator==(const iterator &other) const
    requires std::forward_iterator<OuterIter> &&
             std::ranges::forward_range<Inner>
  {
    return m_outer_it == other.m_outer_it &&
           inner_index() == other.inner_index();
  }

  bool operator!=(const iterator &other) const { return !(*this == other); }

  // Random access support
  iterator &operator+=(size_t n)
    requires std::random_access_iterator<InnerIter> &&
             std::ranges::sized_range<Inner>
  {
    while (n > 0) {
      size_t remain =
          std::ranges::size(*m_inner_view) -
          std::ranges::distance(std::ranges::begin(*m_inner_view), m_inner_it);
      if (n < remain) {
        m_inner_it += n;
        break;
      } else {
        n -= remain;
        ++m_outer_it;
        if (m_outer_it == m_outer_end) {
          m_inner_view = std::nullopt;
          m_inner_it = InnerIter{};
          break; // maybe continue?
        }
        m_inner_view = *m_outer_it;
        m_inner_it = std::ranges::begin(*m_inner_view);
      }
    }
    return *this;
  }

  // Random access support
  iterator &operator-=(size_t n)
    requires std::random_access_iterator<InnerIter> &&
             std::ranges::sized_range<Inner>
  {
    while (n > 0) {
      auto remain = m_inner_it - std::ranges::begin(*m_inner_view);
      if (n < remain) {
        m_inner_it -= n;
        break;
      } else {
        n -= remain;
        --m_outer_it;
        m_inner_view = *m_outer_it;
        m_inner_it = std::ranges::begin(*m_inner_view) +
                     std::ranges::size(*m_inner_view);
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

  std::ptrdiff_t operator-(const iterator &other) const
    requires std::random_access_iterator<InnerIter> &&
             std::ranges::sized_range<Inner>
  {
    // Only valid if both iterators are from the same flatten_view and other
    // <= *this
    std::ptrdiff_t dist = 0;
    auto it = other;
    while (it.m_outer_it != m_outer_it) {
      dist += std::ranges::size(*it.m_inner_view) - it.inner_index();
      ++it.m_outer_it;
      it.m_inner_view = *it.m_outer_it;
      it.m_inner_it = std::ranges::begin(*it.m_inner_view);
    }
    dist +=
        std::ranges::distance(std::ranges::begin(*m_inner_view), m_inner_it) -
        std::ranges::distance(std::ranges::begin(*it.m_inner_view),
                              it.m_inner_it);
    return dist;
  }

  value_type operator[](std::ptrdiff_t n) const
    requires std::random_access_iterator<InnerIter> &&
             std::ranges::sized_range<Inner>
  {
    auto tmp = *this;
    tmp += n;
    return *tmp;
  }

private:
  void skip_empty()
    requires std::ranges::input_range<Inner>
  {
    while (m_inner_it == std::ranges::end(*m_inner_view)) {
      ++m_outer_it;
      if (m_outer_it == m_outer_end) {
        // Reached the end of the outer range
        m_inner_view = std::nullopt;
        m_inner_it = InnerIter{};
        return;
      }
      m_inner_view = *m_outer_it;
      m_inner_it = std::ranges::begin(*m_inner_view);
    }
  }

  size_t inner_index() const
    requires std::ranges::forward_range<Inner>
  {
    return std::ranges::distance(std::ranges::begin(*m_inner_view), m_inner_it);
  }

  OuterIter m_outer_it;
  OuterSentinel m_outer_end;
  mutable std::optional<Inner>
      m_inner_view; // mutable to allow const position calculation
  InnerIter m_inner_it;
};
} // namespace flatten