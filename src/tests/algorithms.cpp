/**********************************************************************
This file is part of Exact.

Copyright (c) 2022-2025 Jo Devriendt, Nonfiction Software

Exact is free software: you can redistribute it and/or modify it under
the terms of the GNU Affero General Public License version 3 as
published by the Free Software Foundation.

Exact is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public
License version 3 for more details.

You should have received a copy of the GNU Affero General Public
License version 3 along with Exact. See the file used_licenses/COPYING
or run with the flag --license=AGPLv3. If not, see
<https://www.gnu.org/licenses/>.
**********************************************************************/

#include "../external/doctest/doctest.h"
#include "constraints/ConstrExp.hpp"

using namespace xct;

TEST_SUITE_BEGIN("Algorithms test");

void test_dp(const Global& global, const std::vector<int32_t>& vals, int32_t target,
             std::vector<std::pair<int32_t, int32_t>>& stack, unordered_map<int32_t, int32_t>& subset, int32_t result) {
  CHECK(subsetsum_dp_withsol(global, vals, target, stack, &subset) == result);
  CHECK(aux::summulti<int32_t, int32_t>(subset) == result);

  auto [sum, haslast] = subsetsum_dp(global, vals, target, std::accumulate(vals.cbegin(), vals.cend(), 0), stack);
  CHECK(sum == result);
  CHECK(haslast == subset.contains(vals.back()));
}

void test_set(const Global& global, const std::vector<int32_t>& vals, int64_t target,
              unordered_map<int64_t, int32_t>& sums, std::vector<std::pair<int64_t, int32_t>>& stack,
              unordered_map<int32_t, int32_t>& subset, int64_t result) {
  CHECK(subsetsum_set_withsol(global, vals, target, sums, stack, &subset) == result);
  CHECK(aux::summulti<int32_t, int32_t>(subset) == result);

  auto [sum, haslast] = subsetsum_set(
      global, vals, target, static_cast<int64_t>(std::accumulate(vals.cbegin(), vals.cend(), 0)), sums, stack);
  CHECK(sum == result);
  CHECK(haslast == subset.contains(vals.back()));
}

TEST_CASE("subset sum dp") {
  Global global;
  std::vector<std::pair<int32_t, int32_t>> stack;

  unordered_map<int32_t, int32_t> subset;

  test_dp(global, {67, 56, 45, 34, 23}, 88, stack, subset, 90);
  test_dp(global, {34, 12, 5, 4, 3, 2}, 9, stack, subset, 9);
  test_dp(global, {5, 4, 3, 2, 1}, 14, stack, subset, 14);
  test_dp(global, {5, 4, 3, 2, 1}, 1, stack, subset, 1);
  for (int i = 15; i < 79; ++i) {
    test_dp(global, {11, 11, 11, 11, 7, 7, 7, 7, 5, 5, 5, 5}, i, stack, subset, i);
  }
  unordered_set<int32_t> off_by_one{1, 3, 8, 10, 13, 15, 20, 22, 25, 27, 32, 37, 42, 44, 47, 49, 54, 56, 59, 61, 66};
  for (int i = 1; i < 67; ++i) {
    test_dp(global, {34, 12, 12, 5, 4, 2}, i, stack, subset, i + off_by_one.count(i));
  }
}

TEST_CASE("subset sum set") {
  Global global;
  unordered_map<int64_t, int32_t> sums;
  std::vector<std::pair<int64_t, int32_t>> stack;

  unordered_map<int32_t, int32_t> subset;
  test_set(global, {67, 56, 45, 34, 23}, 88, sums, stack, subset, 90);
  test_set(global, {34, 12, 5, 4, 3, 2}, 9, sums, stack, subset, 9);
  test_set(global, {5, 4, 3, 2, 1}, 14, sums, stack, subset, 14);
  test_set(global, {5, 4, 3, 2, 1}, 1, sums, stack, subset, 1);
  for (int i = 15; i < 79; ++i) {
    test_set(global, {11, 11, 11, 11, 7, 7, 7, 7, 5, 5, 5, 5}, i, sums, stack, subset, i);
  }
  unordered_set<int32_t> off_by_one{1, 3, 8, 10, 13, 15, 20, 22, 25, 27, 32, 37, 42, 44, 47, 49, 54, 56, 59, 61, 66};
  for (int i = 1; i < 67; ++i) {
    test_set(global, {34, 12, 12, 5, 4, 2}, i, sums, stack, subset, i + off_by_one.count(i));
  }
}

TEST_SUITE_END();
