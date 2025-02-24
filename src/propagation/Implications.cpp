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

#include "Implications.hpp"
#include "../Solver.hpp"

namespace xct {

void Implications::setNbVars(int nvars) { implieds.resize(nvars, {}); }

void Implications::addImplied(Lit a, Lit b) {
  assert(a != b);
  implInMem += implieds[a].insert(b).second;
  resetPropagation();
}

void Implications::removeImplied(Lit a) {
  auto& el = implieds[a];
  implInMem -= el.size();
  el.clear();
}

const unordered_set<Lit>& Implications::getImplieds(Lit a) const { return implieds[a]; }

bool Implications::hasImplieds(Lit a) const { return !implieds[a].empty(); }

int64_t Implications::nImpliedsInMemory() const { return implInMem; }

State Implications::propagate() {
  for (; nextTrailPos < std::ssize(solver.trail); ++nextTrailPos) {
    Lit a = solver.trail[nextTrailPos];
    assert(isTrue(solver.getLevel(), a));
    if (implieds[a].empty()) continue;
    int lvl_a = solver.getLevel()[a];
    Lit found = 0;
    for (Lit b : implieds[a]) {
      if (lvl_a < solver.getLevel()[b]) {
        ++solver.getStats().NPROBINGIMPLS;
        ID id = solver.getLogger().logRUP(-a, b);
        solver.learnClause(-a, b, Origin::IMPLICATION, id);
        found = b;
        break;
      }
      assert(lvl_a >= solver.getLevel()[b]);
    }
    if (found != 0) {
      implieds[a].erase(found);
      return State::FAIL;
    }
  }
  return State::SUCCESS;
}

}  // namespace xct
