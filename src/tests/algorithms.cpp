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

TEST_SUITE_BEGIN("IntProg inference tests");

TEST_CASE("subset sum") {
  CHECK(dp_subsetsum({23, 34, 45, 56, 67}, 88, 225) == 90);
  CHECK(dp_subsetsum({3, 34, 4, 12, 5, 2}, 9, 60) == 9);
  CHECK(dp_subsetsum({2, 34, 4, 12, 5, 12}, 22, 69) == 23);
  CHECK(dp_subsetsum({1, 2, 3, 4, 5}, 14, 15) == 14);
  CHECK(dp_subsetsum({1, 2, 3, 4, 5}, 1, 15) == 1);
}

TEST_SUITE_END();
