#include "flatten.h"
#include "range/v3/view/any_view.hpp"
#include <gtest/gtest.h>
#include <iterator>
#include <numeric>
#include <ranges>
#include <vector>

TEST(FlattenViewTest, FlattensNestedVectors) {
  std::vector<std::vector<int>> nested{{{1, 2}, {3, 4, 5}, {}, {6}}};
  std::vector<int> expected{1, 2, 3, 4, 5, 6};
  std::vector<int> result;
  for (int v : flatten_view(nested)) {
    result.push_back(v);
  }
  EXPECT_EQ(result, expected);
}

TEST(FlattenViewTest, WorksWithTransformAdaptor) {
  std::vector<std::vector<int>> nested{{{1, 2}, {3, 4}}};
  // Double each element before flattening
  auto transformed = nested | std::views::transform([](const auto &v) {
                       std::vector<int> out;
                       for (int x : v)
                         out.push_back(x * 2);
                       return out;
                     });
  std::vector<int> expected{2, 4, 6, 8, 1, 2, 3, 4};
  std::vector<int> result;
  using any_view = ranges::any_view<int, ranges::category::random_access>;
  std::vector<any_view> combined{
      any_view(flatten_view(transformed)),
      any_view(ranges::views::all(flatten_view(nested)))};
  for (int v : flatten_view(combined)) {
    result.push_back(v);
  }
  EXPECT_EQ(result, expected);
}

TEST(FlattenViewTest, RecuresiveFlattening) {
  std::vector<std::vector<size_t>> v{{{1, 2}, {3, 4, 5}, {}, {6}}};
  std::vector views = {flatten_view(v), flatten_view(v)};
  std::vector<int> expected{1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6};
  std::vector<int> result;
  for (int elem : flatten_view(views)) {
    result.push_back(elem);
  }
  EXPECT_EQ(result, expected);
}

TEST(FlattenViewTest, input_of_random_access) {
  std::vector<std::vector<size_t>> v{{{1, 2}, {3, 4, 5}, {}, {6}}};
  ranges::any_view<std::vector<size_t>, ranges::category::input> av =
      ranges::views::all(v);
  std::vector<int> expected{1, 2, 3, 4, 5, 6};
  std::vector<int> result;
  for (int elem : flatten_view(av)) {
    result.push_back(elem);
  }
  EXPECT_EQ(result, expected);
}

TEST(FlattenViewTest, input_of_input_iteration) {
  // This test checks that we can flatten input_range of borrowed input_ranges.
  using any_input_view = ranges::any_view<size_t, ranges::category::input>;
  std::vector<size_t> vec{1, 2};
  any_input_view v = ranges::views::all(vec);
  std::vector<std::ranges::ref_view<any_input_view>> views{
      std::ranges::ref_view(v), std::ranges::ref_view(v)};
  ranges::any_view<std::ranges::ref_view<any_input_view>,
                   ranges::category::input>
      av = ranges::views::all(views);
  any_input_view flattened = flatten_view(av);
  std::vector<int> expected{1, 2, 1, 2};
  std::vector<int> result;
  for (int elem : flattened) {
    result.push_back(elem);
  }
  EXPECT_EQ(result, expected);
}
TEST(FlattenViewTest, forward_of_forward) {
  // This test checks that we can flatten input_range of borrowed input_ranges.
  using any_forward_view = ranges::any_view<size_t, ranges::category::forward>;
  std::vector<size_t> vec{1, 2};
  any_forward_view v = ranges::views::all(vec);
  std::vector<std::ranges::ref_view<any_forward_view>> views{
      std::ranges::ref_view(v), std::ranges::ref_view(v)};
  ranges::any_view<std::ranges::ref_view<any_forward_view>,
                   ranges::category::forward>
      av = ranges::views::all(views);
  any_forward_view flattened = flatten_view(av);
  std::vector<int> expected{1, 2, 1, 2};
  std::vector<int> result;
  for (int elem : flattened) {
    result.push_back(elem);
  }
  EXPECT_EQ(result, expected);
}

TEST(FlattenViewTest, bidirectional_of_bidirectional) {
  // This test checks that we can flatten input_range of borrowed input_ranges.
  using inner_view = ranges::any_view<size_t, ranges::category::random_access |
                                                  ranges::category::sized>;

  std::vector<size_t> vec{1, 2};
  inner_view v = ranges::views::all(vec);
  std::vector<std::ranges::ref_view<inner_view>> views{
      std::ranges::ref_view(v), std::ranges::ref_view(v)};
  ranges::any_view<std::ranges::ref_view<inner_view>,
                   ranges::category::bidirectional>
      av = ranges::views::all(views);
  ranges::any_view<size_t, ranges::category::bidirectional> flattened =
      flatten_view(av);
  // std::ranges::begin(flattened);
  // static_assert(std::ranges::random_access_range<std::ranges::range_value_t<decltype(av)>>);
  // static_assert(std::ranges::borrowed_range<std::ranges::range_value_t<decltype(av)>>);
  std::vector<int> expected{1, 2, 1, 2};
  std::vector<int> result;
  for (int elem : flattened) {
    result.push_back(elem);
  }
  EXPECT_EQ(result, expected);
}

TEST(FlattenViewTest, bidirectional_of_random_access) {
  // This test checks that we can flatten input_range of borrowed input_ranges.
  using inner_view = ranges::any_view<size_t, ranges::category::random_access |
                                                  ranges::category::sized>;

  std::vector<size_t> vec{1, 2};
  inner_view v = ranges::views::all(vec);
  std::vector<std::ranges::ref_view<inner_view>> views{
      std::ranges::ref_view(v), std::ranges::ref_view(v)};
  ranges::any_view<std::ranges::ref_view<inner_view>,
                   ranges::category::bidirectional>
      av = ranges::views::all(views);
  ranges::any_view<size_t, ranges::category::random_access |
  ranges::category::sized> flattened =
      flatten_view(av);
  std::vector<int> expected{1, 2, 1, 2};
  std::vector<int> result;
  for (int elem : flattened) {
    result.push_back(elem);
  }
  EXPECT_EQ(result, expected);
}