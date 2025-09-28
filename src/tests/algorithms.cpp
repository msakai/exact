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

TEST_CASE("subset sum performance test") {
  std::vector<int32_t> coefs;
  const int32_t n = 300;
  for (int32_t i = 0; i < n; ++i) {
    coefs.push_back(n + i / 100);
  }
  coefs.push_back(50000);
  coefs.push_back(750000);
  coefs.push_back(100000);
  int32_t target = std::accumulate(coefs.begin(), coefs.end(), 0) * 3 / 7;
  auto start = std::chrono::high_resolution_clock::now();
  std::cout << subsetsum_dp_topdown(coefs, target) << std::endl;
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
  std::cout << duration << std::endl;
  start = std::chrono::high_resolution_clock::now();
  std::cout << subsetsum_set_topdown(coefs, target) << std::endl;
  duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
  std::cout << duration << std::endl;
}

TEST_CASE("subset sum dp topdown") {
  unordered_map<int32_t, int32_t> subset;
  CHECK(subsetsum_dp_topdown({67, 56, 45, 34, 23}, 88, &subset) == 90);
  CHECK(aux::summulti(subset) == 90);
  CHECK(subsetsum_dp_topdown({34, 12, 5, 4, 3, 2}, 9, &subset) == 9);
  CHECK(aux::summulti(subset) == 9);
  CHECK(subsetsum_dp_topdown({5, 4, 3, 2, 1}, 14, &subset) == 14);
  CHECK(aux::summulti(subset) == 14);
  CHECK(subsetsum_dp_topdown({5, 4, 3, 2, 1}, 1, &subset) == 1);
  CHECK(aux::summulti(subset) == 1);
  for (int i = 15; i < 79; ++i) {
    CHECK(subsetsum_dp_topdown({11, 11, 11, 11, 7, 7, 7, 7, 5, 5, 5, 5}, i, &subset) == i);
    CHECK(aux::summulti(subset) == i);
  }
  unordered_set<int32_t> off_by_one{1, 3, 8, 10, 13, 15, 20, 22, 25, 27, 32, 37, 42, 44, 47, 49, 54, 56, 59, 61, 66};
  for (int i = 1; i < 67; ++i) {
    CHECK(subsetsum_dp_topdown({34, 12, 12, 5, 4, 2}, i, &subset) == i + off_by_one.count(i));
    CHECK(aux::summulti(subset) == i + off_by_one.count(i));
  }
}

TEST_CASE("subset sum set topdown") {
  unordered_map<int32_t, int32_t> subset;
  CHECK(subsetsum_set_topdown({67, 56, 45, 34, 23}, 88, &subset) == 90);
  CHECK(aux::summulti(subset) == 90);
  CHECK(subsetsum_set_topdown({34, 12, 5, 4, 3, 2}, 9, &subset) == 9);
  CHECK(aux::summulti(subset) == 9);
  CHECK(subsetsum_set_topdown({5, 4, 3, 2, 1}, 14, &subset) == 14);
  CHECK(aux::summulti(subset) == 14);
  CHECK(subsetsum_set_topdown({5, 4, 3, 2, 1}, 1, &subset) == 1);
  CHECK(aux::summulti(subset) == 1);
  for (int i = 15; i < 79; ++i) {
    CHECK(subsetsum_set_topdown({11, 11, 11, 11, 7, 7, 7, 7, 5, 5, 5, 5}, i, &subset) == i);
    CHECK(aux::summulti(subset) == i);
  }
  unordered_set<int32_t> off_by_one{1, 3, 8, 10, 13, 15, 20, 22, 25, 27, 32, 37, 42, 44, 47, 49, 54, 56, 59, 61, 66};
  for (int i = 1; i < 67; ++i) {
    CHECK(subsetsum_set_topdown({34, 12, 12, 5, 4, 2}, i, &subset) == i + off_by_one.count(i));
    CHECK(aux::summulti(subset) == i + off_by_one.count(i));
  }
}

TEST_SUITE_END();
