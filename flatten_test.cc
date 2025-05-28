#include "flatten.h"
#include <gtest/gtest.h>
#include <iterator>
#include <numeric> // for std::iota
#include <ranges>
#include <vector>

TEST(FlattenViewTest, FlattensNestedVectors) {
  std::vector<std::vector<int>> nested{{1, 2}, {3, 4, 5}, {}, {6}};
  std::vector<int> expected{1, 2, 3, 4, 5, 6};

  std::vector<int> result;
  for (int v : flatten_view(nested)) {
    result.push_back(v);
  }
  EXPECT_EQ(result, expected);
}

TEST(FlattenViewTest, WorksWithTransformAdaptor) {
  std::vector<std::vector<int>> nested{{1, 2}, {3, 4}};
  // Double each element before flattening
  auto transformed = nested | std::views::transform([](const auto &v) {
                       std::vector<int> out;
                       for (int x : v)
                         out.push_back(x * 2);
                       return out;
                     });

  std::vector<int> expected{2, 4, 6, 8};
  std::vector<int> result;
  auto view = flatten_view(transformed);
  auto it = view.begin();
  std::cout << (it == view.end()) << std::endl; // Debugging line

  it++;
  std::cout << (it == view.end()) << std::endl; // Debugging line

  it++;
  std::cout << (it == view.end()) << std::endl; // Debugging line

  it++;
  std::cout << (it == view.end()) << std::endl; // Debugging line
  for (int v : flatten_view(transformed)) {
    result.push_back(v);
  }
  EXPECT_EQ(result, expected);
}

TEST(FlattenViewTest, RecuresiveFlattening) {
  std::vector<std::vector<int>> v{{1, 2}, {3, 4, 5}};
  auto view = std::views::all(v);
//   // auto view2 = flatten_view(view);
  std::vector views{flatten_view(v), flatten_view(v)};
  auto flattened = flatten_view(views);
//   std::cout << "v[0] == v[1]: " << (v[0] == v[0]) << std::endl;
  
//   auto outer_begin = std::ranges::begin(views);
//   auto outer_end = std::ranges::end(views);
//   auto inner_view = *outer_begin;
//   auto inner_it = std::ranges::begin(inner_view);

//   auto it = flatten_view<decltype(std::ranges::views::all(views))>::iterator(outer_begin, outer_end, 0);
//   std::cout << *inner_it << std::endl; // Debugging line
//   auto it = flatten_view<decltype(std::ranges::views::all(views))>::iterator(outer_begin, outer_end, 0);
//   auto pos = [](auto it) {return {std::distance}}
  auto it = flattened.begin();
  std::cout << (it - flattened.begin()) << ": " << *it << std::endl;
  ++it;
  std::cout << (it - flattened.begin()) << ": " << *it << std::endl;
  ++it;
  std::cout << (it - flattened.begin()) << ": " << *it << std::endl;
  ++it;
  std::cout << (it - flattened.begin()) << ": " << *it << std::endl;
  ++it;
  std::cout << (it - flattened.begin()) << ": " << *it << std::endl;

  //   std::cout << (it == flattened.end()) << std::endl; // Debugging line
//   std::cout << (*it) << std::endl; // Debugging line
//   ++it;
//   std::cout << (*it) << std::endl; // Debugging line
//   ++it;
//   std::cout << (*it) << std::endl; // Debugging line
//   ++it;
//   std::cout << (*it) << std::endl; // Debugging line
//     ++it;
//   std::cout << (*it) << std::endl; // Debugging line
//     ++it;
    // it.m_inner_view
//   std::cout << (*it) << std::endl; // Debugging line
//   std::cout << *(std::ranges::begin(it.m_inner_view) + 0) << std::endl;
//   for (int elem : it.m_inner_view) {
//     std::cout << elem << " "; // Debugging line
//   }
//   ++it;
//   // static_assert(std::sentinel_for<decltype(flatten_view(view).end()),
//   // decltype(flatten_view(view).begin())>);
//   // static_assert(std::ranges::range<decltype(flatten_view(view))>);
//   std::vector<int> expected{1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6};
//   std::vector<int> result;
//   for (int elem : flattened) {
//     std::cout << elem << std::endl;
//     result.push_back(elem);
//   }
//   EXPECT_EQ(result, expected);
}