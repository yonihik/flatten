#pragma once

#include "flatten_iterator.h"
#include "range/v3/range/concepts.hpp"
#include <concepts>
#include <iterator>
#include <ranges>
template <std::ranges::input_range Outer>
  requires std::ranges::input_range<std::ranges::range_value_t<Outer>>
class flatten_view : public std::ranges::view_interface<flatten_view<Outer>>,
                     public ranges::view_base {
  Outer m_outer;

public:
  using OuterIter = std::ranges::iterator_t<Outer>;
  using OuterSentinel = std::ranges::sentinel_t<Outer>;
  using Inner = std::ranges::range_value_t<Outer>;
  using InnerSentinel = std::ranges::sentinel_t<Inner>;
  using iterator = flatten::iterator<OuterIter, OuterSentinel>;
  using sentinel = flatten::sentinel<OuterSentinel, InnerSentinel>;

public:
  flatten_view() = default;
  explicit flatten_view(Outer outer) : m_outer(std::move(outer)) {}

  iterator begin()
    requires std::ranges::input_range<Inner>
  {
    return iterator(std::ranges::begin(m_outer), std::ranges::end(m_outer));
  }
  iterator begin()
    requires std::ranges::random_access_range<Inner>
  {
    return iterator(std::ranges::begin(m_outer), std::ranges::end(m_outer), 0);
  }
  sentinel end() {
    return sentinel(std::ranges::end(m_outer), InnerSentinel{});
  }

  iterator::value_type operator[](std::size_t n) {
    auto it = begin();
    it += n;
    return *it;
  }

  // Size support
  size_t size() const
    requires std::ranges::sized_range<Inner>
  {
    std::size_t total = 0;
    if constexpr (std::ranges::view<Outer> || ranges::view_<Outer>) {
      auto outer = m_outer;
      for (const auto &v : outer) {
        total += std::ranges::size(v);
      }
    } else {
      for (const auto &v : m_outer)
        total += std::ranges::size(v);
    }
    return total;
  }
};

template <std::ranges::random_access_range Outer>
  requires std::ranges::viewable_range<Outer>
flatten_view(Outer &&) -> flatten_view<std::views::all_t<Outer>>;
