#include "flatten.h"
#include "range/v3/view/any_view.hpp"
#include <gtest/gtest.h>
#include <iterator>
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
  std::vector<any_view> combined{any_view(flatten_view(transformed)),
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

TEST(FlattenViewTest, input_range) {
  std::vector<std::vector<size_t>> v{{{1, 2}, {3, 4, 5}, {}, {6}}};
  ranges::any_view<std::vector<size_t>, ranges::category::input> av = ranges::views::all(v);
  std::vector<int> expected{1, 2, 3, 4, 5, 6};
  std::vector<int> result;
  for (int elem : flatten_view(av)) {
    result.push_back(elem);
  }
  EXPECT_EQ(result, expected);
}