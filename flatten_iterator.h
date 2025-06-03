#pragma once

#include "range/v3/range/concepts.hpp"
#include <concepts>
#include <iterator>
#include <ranges>
namespace flatten {
template <typename Rng>
concept borrowed_range =
    (std::ranges::borrowed_range<Rng> || ranges::borrowed_range<Rng>);
template <typename OuterSentinel, typename InnerSentinel> struct sentinel {
  sentinel() = default;

  sentinel(OuterSentinel outer_end,
           std::optional<InnerSentinel> inner_end = std::nullopt)
      : m_outer_end(outer_end), m_inner_end(inner_end) {}

  OuterSentinel m_outer_end;
  std::optional<InnerSentinel> m_inner_end;
};
/**
 * @brief An iterator that flattens a range of ranges into a single sequence.
 *
 * This iterator is designed to traverse a "range of ranges" (i.e., a range
 * whose elements are themselves ranges), presenting all the elements of the
 * inner ranges as a single, flat sequence. It adapts to the capabilities of the
 * underlying outer and inner iterators/ranges, providing the strongest iterator
 * category possible.
 *
 * @tparam OuterIter The iterator type for the outer range (the range of
 * ranges).
 * @tparam OuterSentinel The sentinel type for the outer range.
 *
 * The iterator maintains its position using:
 * - `m_outer_it`: The current iterator in the outer range.
 * - `m_outer_end`: The end sentinel for the outer range.
 * - `m_inner_view`: An optional holding the current inner range.
 * - `m_inner_it`: The current iterator in the inner range.
 *
 * ### Iterator Category and Concept
 * The iterator adapts its interface and category based on the concepts
 * satisfied by its template arguments:
 * - models input iterator if `OuterIter` is an input iterator and
 *   `Inner` is either a borrowed input range or a random-access range.
 * - models forward iterator if `OuterIter` is a forward iterator and
 *  `Inner` is either a borrowed forward iterator or a random-acess range.
 * - models random access iterator if `OuterIter` is a bidirectional iterator
 *   and `Inner` is a random access and sized range.
 * - models sized range if `Inner` is a sized range and `Outer` is either a view
 * or const iterable.
 *
 */
template <std::input_iterator OuterIter,
          std::sentinel_for<OuterIter> OuterSentinel>
  requires std::ranges::input_range<std::iter_value_t<OuterIter>>
class iterator {

  using Inner = std::iter_value_t<OuterIter>;
  using InnerIter = std::ranges::iterator_t<Inner>;
  using InnerSentinel = std::ranges::sentinel_t<Inner>;

public:
  using value_type = std::ranges::range_value_t<Inner>;
  using difference_type = std::ptrdiff_t;

  iterator() = default;
  template <typename Iter, typename Sentinel>
  iterator(Iter &&outer_it, Sentinel &&outer_end)
      : m_outer_it(std::forward<Iter>(outer_it)),
        m_outer_end(std::forward<Sentinel>(outer_end)) {
    if (m_outer_it != m_outer_end) {
      m_inner_view = *m_outer_it;
      m_inner_it = std::ranges::begin(*m_inner_view);
      m_inner_index = 0;
    }
  }
  // assumes outer_it contains at least inner_idx elements
  template <typename Iter, typename Sentinel>
  iterator(Iter &&outer_it, Sentinel &&outer_end, size_t inner_idx)
    requires std::ranges::random_access_range<Inner>
      : m_outer_it(std::forward<Iter>(outer_it)),
        m_outer_end(std::forward<Sentinel>(outer_end)),
        m_inner_index(inner_idx) {
    if (m_outer_it != m_outer_end) {
      m_inner_view = *m_outer_it;
      m_inner_it = std::ranges::begin(*m_inner_view) + *m_inner_index;
    }
  }

  iterator(const iterator &other)
    requires std::ranges::random_access_range<Inner> &&
             std::copyable<OuterIter> && std::copyable<OuterSentinel> &&
             (!borrowed_range<Inner>)
      : iterator(other.m_outer_it, other.m_outer_end, *other.m_inner_index) {}

  iterator(const iterator &other)
    requires borrowed_range<Inner>
      : m_outer_it(other.m_outer_it), m_outer_end(other.m_outer_end),
        m_inner_view(other.m_inner_view), m_inner_it(other.m_inner_it),
        m_inner_index(other.m_inner_index) {}

  friend void swap(iterator &first, iterator &second) {
    using std::swap;

    // by swapping the members of two objects,
    // the two objects are effectively swapped
    swap(first.m_outer_it, second.m_outer_it);
    swap(first.m_outer_end, second.m_outer_end);
    swap(first.m_inner_view, second.m_inner_view);
    swap(first.m_inner_it, second.m_inner_it);
    swap(first.m_inner_index, second.m_inner_index);
  }

  iterator &operator=(const iterator &other) {
    iterator tmp(other);
    swap(*this, tmp);
    return *this;
  }
  value_type operator*() const
    requires std::ranges::input_range<Inner>
  {
    return *(*m_inner_it);
  }

  iterator &operator++()
    requires std::ranges::input_range<Inner>
  {
    ++(*m_inner_it);
    ++(*m_inner_index);
    skip_empty();
    return *this;
  }

  iterator &operator--()
    requires std::bidirectional_iterator<OuterIter> &&
             std::ranges::random_access_range<Inner> &&
             std::ranges::sized_range<Inner>
  {
    while (!m_inner_it.has_value() || m_inner_index == 0) {
      --m_outer_it;
      m_inner_view = *m_outer_it;
      m_inner_index = std::ranges::size(*m_inner_view);
      m_inner_it = std::ranges::begin(*m_inner_view) + *m_inner_index;
    }
    --(*m_inner_it);
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
    return m_outer_it == s.m_outer_end &&
           m_inner_it == s.m_inner_end; // maybe compare indices?
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
    return m_inner_index >= other.m_inner_index;
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
    return m_inner_index > other.m_inner_index;
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
    return m_inner_index <= other.m_inner_index;
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
    return m_inner_index < other.m_inner_index;
  }

  bool operator==(const iterator &other) const
    requires std::forward_iterator<OuterIter> &&
             std::ranges::forward_range<Inner>
  {
    return m_outer_it == other.m_outer_it &&
           m_inner_index == other.m_inner_index;
  }

  bool operator!=(const iterator &other) const { return !(*this == other); }

  // Random access support
  iterator &operator+=(size_t n)
    requires std::ranges::random_access_range<Inner> &&
             std::ranges::sized_range<Inner>
  {
    while (n > 0) {
      size_t remain = std::ranges::size(*m_inner_view) - *m_inner_index;
      if (n < remain) {
        *m_inner_it += n;
        *m_inner_index += n;
        break;
      } else {
        n -= remain;
        ++m_outer_it;
        if (m_outer_it == m_outer_end) {
          m_inner_view = std::nullopt;
          m_inner_it = std::nullopt;
          m_inner_index = std::nullopt;
          break; // maybe continue?
        }
        m_inner_view = *m_outer_it;
        m_inner_it = std::ranges::begin(*m_inner_view);
        m_inner_index = 0;
      }
    }
    return *this;
  }

  // Random access support
  iterator &operator-=(size_t n)
    requires std::ranges::random_access_range<Inner> &&
             std::ranges::sized_range<Inner>
  {
    while (n > 0) {
      auto remain = m_inner_index.value_or(0);
      if (n < remain) {
        *m_inner_it -= n;
        *m_inner_index -= n;
        break;
      } else {
        n -= remain;
        --m_outer_it;
        m_inner_view = *m_outer_it;
        m_inner_index = std::ranges::size(*m_inner_view);
        m_inner_it = std::ranges::begin(*m_inner_view) + *m_inner_index;
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
    requires std::ranges::random_access_range<Inner> &&
             std::ranges::sized_range<Inner>
  {
    // Only valid if both iterators are from the same flatten_view and other
    // <= *this
    std::ptrdiff_t dist = 0;
    auto it = other;
    while (it.m_outer_it != m_outer_it) {
      dist += std::ranges::size(*it.m_inner_view) - *it.m_inner_index;
      ++it.m_outer_it;
      it.m_inner_view = *it.m_outer_it;
      it.m_inner_index = 0;
      it.m_inner_it = std::ranges::begin(*it.m_inner_view);
    }
    dist += *m_inner_index - *it.m_inner_index;
    return dist;
  }

  value_type operator[](std::ptrdiff_t n) const
    requires std::ranges::random_access_range<Inner> &&
             std::ranges::sized_range<Inner>
  {
    auto tmp = *this;
    tmp += n;
    return *tmp;
  }

private:
  void skip_empty() {
    while (m_inner_it == std::ranges::end(*m_inner_view)) {
      ++m_outer_it;
      if (m_outer_it == m_outer_end) {
        // Reached the end of the outer range
        m_inner_view = std::nullopt;
        m_inner_it = std::nullopt;
        m_inner_index = std::nullopt;
        return;
      }
      m_inner_view = *m_outer_it;
      m_inner_it = std::ranges::begin(*m_inner_view);
      m_inner_index = 0;
    }
  }

  OuterIter m_outer_it;
  OuterSentinel m_outer_end;
  mutable std::optional<Inner>
      m_inner_view; // mutable to allow const position calculation
  std::optional<InnerIter> m_inner_it;
  std::optional<size_t> m_inner_index;
};
} // namespace flatten