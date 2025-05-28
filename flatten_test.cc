#include "flatten.h"
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

  std::vector<int> expected{2, 4, 6, 8};
  std::vector<int> result;
  for (int v : flatten_view(transformed)) {
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