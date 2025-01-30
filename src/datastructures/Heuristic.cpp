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

/**********************************************************************
Copyright (c) 2014-2020, Jan Elffers
Copyright (c) 2019-2021, Jo Devriendt
Copyright (c) 2020-2021, Stephan Gocht
Copyright (c) 2014-2021, Jakob Nordström

Parts of the code were copied or adapted from MiniSat.

MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
           Copyright (c) 2007-2010  Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**********************************************************************/

#include "Heuristic.hpp"
#include "../constraints/ConstrExp.hpp"

namespace xct {

Heuristic::Heuristic() : nextDecision(0) {
  phase.resize(1);
  phase[0] = {0, 0};
  actList.resize(1);
  actList[0].prev = 0;
  actList[0].next = 0;
  actList[0].activity = std::numeric_limits<ActValV>::max();
}

int Heuristic::nVars() const { return phase.size(); }

bool Heuristic::before(Var v1, Var v2) const {
  return actList[v1].activity > actList[v2].activity || (actList[v1].activity == actList[v2].activity && v1 < v2);
}

void Heuristic::resize(int nvars) {
  assert(nvars == 1 || nvars > (int)phase.size());
  int old_n = nVars();  // at least one after initialization
  assert(old_n >= 1);
  phase.resize(nvars);
  actList.resize(nvars);
  for (Var v = old_n; v < nvars; ++v) {
    phase[v] = {0, -v};
    ActNode& node = actList[v];
    node.activity = -v / static_cast<ActValV>(INF);  // early variables have slightly higher initial activity
    actList[v].next = v + 1;
    actList[v].prev = v - 1;
    assert(before(nextDecision, v));
  }
  Var oldTail = actList[0].prev;
  actList[old_n].prev = oldTail;
  actList[oldTail].next = old_n;
  actList[0].prev = nvars - 1;
  actList[nvars - 1].next = 0;
}

void Heuristic::undoOne(Var v, Lit l) {
  setPhase(v, l);
  if (before(v, nextDecision)) nextDecision = v;
}

void Heuristic::setPhase(Var v, Lit l) { phase[v].second = l; }
void Heuristic::setFixedPhase(Var v, Lit l) { phase[v].first = l; }

ActValV Heuristic::getActivity(Var v) const {
  assert(v > 0);
  assert(v < nVars());
  return actList[v].activity;
}

const std::vector<ActNode>& Heuristic::getActList() const { return actList; }

void Heuristic::randomize(const std::vector<int>& position) {
  VarVec vars(std::ssize(actList) - 1);
  for (Var v = 1; v < std::ssize(actList); ++v) {
    vars[v - 1] = v;
    actList[v].activity += aux::getRand(0, INF) / static_cast<ActValV>(INF);
  }
  nextDecision = 0;
  vBumpActivity(vars, position, 0, 0);
}

void Heuristic::bumpObjective(const CeArb& obj, const std::vector<int>& position) {
  assert(obj->hasNoZeroes());
  // set initial phase and activity so that we try optimal objective assignment first
  VarVec vars = obj->vars;
  std::sort(vars.begin(), vars.end(), [&](Var v1, Var v2) {
    const bigint diff = obj->absCoef(v1) - obj->absCoef(v2);
    return diff > 0 || (diff == 0 && actList[v1].activity > actList[v2].activity);  // takes into account randomization
  });
  ActValV tmpsize = std::ssize(vars);
  for (int64_t i = 0; i < std::ssize(vars); ++i) {
    const Var v = vars[i];
    setPhase(v, obj->coefs[v] < 0 ? v : -v);
    assert(2 + (actList[v].activity - i) / tmpsize >= actList[v].activity);
    // NOTE: overrides randomization activity, so needs to be larger
    actList[v].activity = 2 + (actList[v].activity - i) / tmpsize;
  }
  nextDecision = 0;
  vBumpActivity(vars, position, 0, 0);
}

void Heuristic::vBumpActivity(VarVec& vars, const std::vector<int>& position, ActValV weightNew, int64_t nConfl) {
  assert(weightNew >= 0);
  assert(weightNew <= 1);
  ActValV weightOld = 1 - weightNew;
  ActValV toAdd = weightNew * (nConfl + 1);
  for (Var v : vars) {
    assert(v > 0);  // not a literal, not 0
    actList[v].activity = aux::max(actList[v].activity, weightOld * actList[v].activity + toAdd);
    // NOTE: max guard is needed when bumpObjective was used
  }
  std::sort(vars.begin(), vars.end(), [&](const Var& v1, const Var& v2) { return before(v1, v2); });
  // NOTE: order is complete, breaking ties on variable index. This means weightNew == 1 (== VMTF) will always sort ties
  // based on variable index. This is probably not the greatest idea. TODO: fix?
  for (Var v : vars) {
    if (before(nextDecision, v)) {
      break;  // vars is sorted
    }
    if (isUnknown(position, v)) {
      nextDecision = v;
      break;  // vars is sorted
    }
  }
  Var current = actList[0].next;
  for (Var v : vars) {
    while (current != 0 && before(current, v)) {
      current = actList[current].next;
    }
    if (current == v) continue;
    // eject v from list
    actList[actList[v].next].prev = actList[v].prev;
    actList[actList[v].prev].next = actList[v].next;
    // insert v before current
    Var before = actList[current].prev;
    actList[v].prev = before;
    actList[v].next = current;
    actList[before].next = v;
    actList[current].prev = v;
  }
  assert(testActList(position));
}

// NOTE: so far, this is only called when the returned lit will be decided shortly
Lit Heuristic::pickBranchLit(const std::vector<int>& position, bool coreguided) {
  assert(phase[0].first == 0);     // so will return right phase
  assert(phase[0].second == 0);    // so will return right phase
  assert(isUnknown(position, 0));  // so will eventually stop
  // Activity based decision:
  if (nextDecision == 0) {
    nextDecision = actList[0].next;
  }
  while (isKnown(position, nextDecision)) {
    nextDecision = actList[nextDecision].next;
  }
  return (!coreguided && phase[nextDecision].first) ? phase[nextDecision].first : phase[nextDecision].second;
}

Var Heuristic::nextInActOrder(Var v) const { return actList[v].next; }

Var Heuristic::firstInActOrder() const { return nextInActOrder(0); }

bool Heuristic::testActList([[maybe_unused]] const std::vector<int>& position) const {
  // printActList(position);
  Var current = actList[0].next;
  [[maybe_unused]] int tested = 1;
  while (current != 0) {
    ++tested;
    Var next = actList[current].next;
    assert(next == 0 || before(current, next));
    assert(actList[next].prev == current);
    current = next;
  }
  assert(tested == (int)actList.size());
  current = nextDecision == 0 ? 0 : actList[nextDecision].prev;
  while (current != 0) {
    assert(isKnown(position, current));
    current = actList[current].prev;
  }
  return true;
}

void Heuristic::printActList(const std::vector<int>& position) const {
  std::cout << nextDecision << " :: ";
  for (Var v = 0; v < (int)actList.size(); ++v) {
    std::cout << actList[v].prev << "->" << v << "->" << actList[v].next << " " << actList[v].activity << " "
              << isKnown(position, v) << std::endl;
  }
  std::cout << std::endl;
}

}  // namespace xct
