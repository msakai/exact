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
#include "interface/IntProg.hpp"

using namespace xct;

TEST_SUITE_BEGIN("memory");

TEST_CASE("sizeofs") {
  CHECK(sizeof(Term<int64_t>) == 16);  // 8 bit larger than needed due to alignment
  CHECK(alignof(bigint) == 16);        // special alignment!
  /*
  std::cout << sizeof(Term<int32_t>) << " " << alignof(Term<int32_t>) << std::endl;
  std::cout << sizeof(Term<int64_t>) << " " << alignof(Term<int64_t>) << std::endl;
  std::cout << sizeof(Term<int128>) << " " << alignof(Term<int128>) << std::endl;
  std::cout << sizeof(Term<int256>) << " " << alignof(Term<int256>) << std::endl;
  std::cout << sizeof(Term<bigint>) << " " << alignof(Term<bigint>) << std::endl;
  std::cout << sizeof(bigint) << " " << alignof(Term<bigint>) << std::endl;
   */
}

TEST_SUITE_END();
