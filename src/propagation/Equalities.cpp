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

#include "Equalities.hpp"
#include "../Solver.hpp"

namespace xct {

const Repr& Equalities::getRepr(Lit a) {
  Repr& repr = canonical[a];
  assert(repr.l != 0);
  if (a == repr.l || canonical[repr.l].l == repr.l) return repr;
  assert(toVar(repr.l) < toVar(a));
  const Repr& reprChild = getRepr(repr.l);
  assert(toVar(reprChild.l) < toVar(repr.l));
  repr.l = reprChild.l;
  assert(reprChild.id != ID_Trivial);  // as we know that canonical[repr.l]!=repr.l
  repr.id = solver.getLogger().logResolvent(repr.id, reprChild.id);
  return repr;
}

void Equalities::merge(Lit a, Lit b) {
  resetPropagation();
  const Repr& reprA = getRepr(a);
  const Repr& reprB = getRepr(b);
  const Repr& reprAneg = getRepr(-a);
  const Repr& reprBneg = getRepr(-b);
  assert(reprA.l == -reprAneg.l);
  assert(reprB.l == -reprBneg.l);
  Lit reprAl = reprA.l;
  Lit reprBl = reprB.l;
  if (reprAl == reprBl) return;  // already equal
  assert(reprAl != -reprBl);     // no inconsistency
  ++solver.getStats().NPROBINGEQS;
  auto [reprAImpReprB, reprBImpReprA] =
      solver.getLogger().logEquality(a, b, reprA.id, reprAneg.id, reprB.id, reprBneg.id, reprAl, reprBl);
  Repr& reprAlRepr = canonical[reprAl];
  Repr& reprAlNegRepr = canonical[-reprAl];
  Repr& reprBlRepr = canonical[reprBl];
  Repr& reprBlNegRepr = canonical[-reprBl];
  assert(reprAlRepr.equals.size() == reprAlNegRepr.equals.size());
  assert(reprBlRepr.equals.size() == reprBlNegRepr.equals.size());
  if (toVar(reprBl) < toVar(reprAl)) {
    reprBlRepr.equals.push_back(reprAl);
    reprBlNegRepr.equals.push_back(-reprAl);
    aux::appendTo(reprBlRepr.equals, reprAlRepr.equals);
    aux::appendTo(reprBlNegRepr.equals, reprAlNegRepr.equals);
    reprAlRepr = {reprBl, reprAImpReprB, {}};
    reprAlNegRepr = {-reprBl, reprBImpReprA, {}};
  } else {
    reprAlRepr.equals.push_back(reprBl);
    reprAlNegRepr.equals.push_back(-reprBl);
    aux::appendTo(reprAlRepr.equals, reprBlRepr.equals);
    aux::appendTo(reprAlNegRepr.equals, reprBlNegRepr.equals);
    reprBlRepr = {reprAl, reprBImpReprA, {}};
    reprBlNegRepr = {-reprAl, reprAImpReprB, {}};
  }
}

bool Equalities::isCanonical(Lit l) { return getRepr(l).l == l; }

bool Equalities::isPartOfEquality(Lit l) {
  const Repr& repr = getRepr(l);
  return repr.l != l || !repr.equals.empty();
}

void Equalities::setNbVars(int nvars) {
  int oldNvars = canonical.reserved() / 2;
  canonical.resize(nvars, {0, ID_Trivial, {}});
  int newNvars = canonical.reserved() / 2;
  for (Var v = oldNvars + 1; v <= newNvars; ++v) {
    canonical[v].l = v;
    canonical[-v].l = -v;
  }
}

State Equalities::propagate() {
  while (nextTrailPos < std::ssize(solver.trail)) {
    Lit l = solver.trail[nextTrailPos];
    ++nextTrailPos;
    assert(isTrue(solver.getLevel(), l));
    const Repr& repr = getRepr(l);
    if (repr.l == l && repr.equals.empty()) continue;
    bool added = false;
    int lvl_l = solver.getLevel()[l];
    if (lvl_l < solver.getLevel()[repr.l]) {
      solver.learnClause(-l, repr.l, Origin::EQUALITY, repr.id);
      added = true;
      lvl_l = solver.getLevel()[l];  // a backjump may have happened
      if (lvl_l >= INF) return State::FAIL;
    }
    assert(lvl_l >= solver.getLevel()[repr.l]);
    for (Lit ll : repr.equals) {
      if (lvl_l < solver.getLevel()[ll]) {
        assert(getRepr(ll).l == l);
        solver.learnClause(-l, ll, Origin::EQUALITY, getRepr(-ll).id);
        added = true;
        lvl_l = solver.getLevel()[l];  // a backjump may have happened
        if (lvl_l >= INF) return State::FAIL;
      }
      assert(lvl_l >= solver.getLevel()[ll]);
    }
    if (added) return State::FAIL;
  }
  return State::SUCCESS;
}

}  // namespace xct
