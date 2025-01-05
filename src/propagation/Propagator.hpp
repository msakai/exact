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

/**********************************************************************
This file is part of the Exact program

Copyright (c) 2021 Jo Devriendt, KU Leuven

Exact is distributed under the terms of the MIT License.
You should have received a copy of the MIT License along with Exact.
See the file LICENSE or run with the flag --license=MIT.
**********************************************************************/

#pragma once

#include "../typedefs.hpp"

namespace xct {

class Solver;

class Propagator {
 protected:
  Solver& solver;
  int nextTrailPos = 0;

 public:
  Propagator(Solver& s) : solver(s) {}

  // NOTE: propagate() may backjump in case two equal literals are propagated to an opposite value at the same decision
  // level, as the clause that would prevent this would otherwise trigger unit propagation
  virtual State propagate() = 0;
  void notifyBackjump();
  void resetPropagation();
};

}  // namespace xct
