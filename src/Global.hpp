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

#pragma once

#include "Logger.hpp"
#include "Options.hpp"
#include "Stats.hpp"
#include "constraints/ConstrExpPools.hpp"
#include "datastructures/IntSet.hpp"

namespace xct {

struct Global {
  Options options;
  Stats stats;
  Logger logger;
  ConstrExpPools cePools;
  IntSetPool isPool;

  Global(const Options& opts) : options(opts), logger(stats), cePools(*this) {}
  Global() : logger(stats), cePools(*this) {}  // intended for test purposes
};

}  // namespace xct
