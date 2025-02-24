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

#include "Constr.hpp"
#include <cmath>
#include "../Solver.hpp"

namespace xct {
Constr::Constr(ID i, const Origin o, bool lkd, uint32_t lngth, float strngth, uint32_t maxLBD)
    : header{0, 0, lkd, static_cast<uint32_t>(o), i}, priority(static_cast<float>(maxLBD + 1) - strngth), sze(lngth) {
  assert(strngth <= 1);
  assert(strngth > 0);  // so we know that 1-strngth < 1 and it will not interfere with the LBD when stored together
  assert(maxLBD <= MAXLBD);
  assert(lngth < INF);
}

std::ostream& operator<<(std::ostream& o, const Constr& c) {
  for (uint32_t i = 0; i < c.size(); ++i) {
    o << c.coef(i) << "x" << c.lit(i) << " ";
  }
  return o << ">= " << c.degree();
}

uint32_t Constr::size() const { return sze; }
void Constr::setLocked(const bool lkd) { header.locked = lkd; }
bool Constr::isLocked() const { return header.locked; }
Origin Constr::getOrigin() const { return static_cast<Origin>(header.origin); }
void Constr::decreaseLBD(const uint32_t lbd) {
  float integral;
  float fractional = std::modf(priority, &integral);
  priority = std::min<float>(static_cast<float>(lbd), integral) + fractional;
}
void Constr::decayLBD(const uint32_t decay, const uint32_t maxLBD) {
  assert(maxLBD <= MAXLBD);
  float integral;
  float fractional = std::modf(priority, &integral);
  priority = std::min<float>(integral + static_cast<float>(decay), static_cast<float>(maxLBD)) + fractional;
}
uint32_t Constr::lbd() const { return static_cast<uint32_t>(priority); }
float Constr::strength() const {
  float tmp;
  return 1 - std::modf(priority, &tmp);
}
bool Constr::isMarkedForDelete() const { return header.markedfordel; }
bool Constr::isSeen() const { return header.seen; }
void Constr::setSeen(const bool s) { header.seen = s; }
ID Constr::id() const { return header.id; }

void Constr::fixEncountered(Stats& stats) const {  // TODO: better as method of Stats?
  const Origin o = getOrigin();
  stats.NENCFORMULA.z += o == Origin::FORMULA;
  stats.NENCDOMBREAKER.z += o == Origin::DOMBREAKER;
  stats.NENCLEARNED.z += o == Origin::LEARNED;
  stats.NENCBOUND.z += isBound(o) || o == Origin::REFORMBOUND;
  stats.NENCCOREGUIDED.z += o == Origin::COREGUIDED || o == Origin::BOTTOMUP;
  stats.NLPENCGOMORY.z += o == Origin::GOMORY;
  stats.NLPENCDUAL.z += o == Origin::DUAL;
  stats.NLPENCFARKAS.z += o == Origin::FARKAS;
  stats.NENCDETECTEDAMO.z += o == Origin::DETECTEDAMO;
  stats.NENCREDUCED.z += o == Origin::REDUCED;
  stats.NENCEQ.z += o == Origin::EQUALITY;
  stats.NENCIMPL.z += o == Origin::IMPLICATION;
  stats.NRESOLVESTEPS.z += 1;
}

size_t Binary::getMemSize(const uint32_t) { return aux::ceildiv(sizeof(Binary), maxAlign); }
size_t Binary::getMemSize() const { return getMemSize(2); }

bigint Binary::degree() const { return 1; }
bigint Binary::coef(uint32_t) const { return 1; }
Lit Binary::lit(const uint32_t i) const { return data[i]; }
bool Binary::hasWatch([[maybe_unused]] uint32_t i) const {
  assert(i < 2);
  return true;
}
uint32_t Binary::getUnsaturatedIdx() const { return 2; }
bool Binary::isClauseOrCard() const { return true; }
bool Binary::isAtMostOne() const { return true; }

void Binary::initializeWatches(CRef cr, Solver& solver) {
  const auto& level = solver.level;
  auto& adj = solver.adj;

  assert(!isFalse(level, data[0]) || !isFalse(level, data[1]));  // no conflict during initialization
  if (isFalse(level, data[0]) && !isTrue(level, data[1])) {
    solver.propagate(data[1], cr);
  }
  if (isFalse(level, data[1]) && !isTrue(level, data[0])) {
    solver.propagate(data[0], cr);
  }
  adj[data[0]].emplace_back(cr, BINARY_IDX, data[1]);
  adj[data[1]].emplace_back(cr, BINARY_IDX, data[0]);
}

WatchStatus Binary::checkForPropagation(Watch&, const Lit, Solver&, Stats&) {
  assert(false);  // internal propagation check should never happen for binary clauses
  return WatchStatus::KEEPWATCH;
}

uint32_t Binary::resolveWith(CeSuper& confl, const Lit l, Solver& solver, IntSet& actSet) const {
  // TODO: simplify resolving with Binary
  return confl->resolveWith(data, 1, id(), l, solver.getLevel(), solver.getPos(), actSet);
}
uint32_t Binary::subsumeWith(CeSuper& confl, const Lit l, Solver& solver, IntSet& saturatedLits) const {
  // TODO: simplify resolving with Binary
  return confl->subsumeWith(data, 1, id(), l, solver.getLevel(), solver.getPos(), saturatedLits);
}

CeSuper Binary::toExpanded(ConstrExpPools& cePools) const {
  Ce32 result = cePools.take32();
  result->addRhs(1);
  result->addLhs(1, data[0]);
  result->addLhs(1, data[1]);
  result->orig = getOrigin();
  result->resetBuffer(id());
  return result;
}

bool Binary::isSatisfiedAtRoot(const IntMap<int>& level) const {
  if (isUnit(level, data[0])) return true;
  if (isUnit(level, data[1])) return true;
  return false;
}

bool Binary::canBeSimplified(const IntMap<int>& level, Equalities& equalities, Implications& implications,
                             IntSetPool&) const {
  const bool isEquality = getOrigin() == Origin::EQUALITY;
  return isUnit(level, data[0]) || isUnit(level, -data[0]) || isUnit(level, data[1]) || isUnit(level, -data[1]) ||
         (!isEquality && (!equalities.isCanonical(data[0]) || !equalities.isCanonical(data[1]) ||
                          implications.getImplieds(data[0]).contains(-data[1]) ||
                          implications.getImplieds(data[1]).contains(-data[0])));
}

size_t Clause::getMemSize(const uint32_t length) {
  return aux::ceildiv(sizeof(Clause) + sizeof(Lit) * length, maxAlign);
}
size_t Clause::getMemSize() const { return getMemSize(size()); }

bigint Clause::degree() const { return 1; }
bigint Clause::coef(uint32_t) const { return 1; }
Lit Clause::lit(const uint32_t i) const { return data[i]; }
bool Clause::hasWatch(uint32_t i) const { return i < 2; }
uint32_t Clause::getUnsaturatedIdx() const { return size(); }
bool Clause::isClauseOrCard() const { return true; }
bool Clause::isAtMostOne() const { return size() == 2; }

void Clause::initializeWatches(CRef cr, Solver& solver) {
  const auto& level = solver.level;
  auto& adj = solver.adj;

  assert(size() >= 1);
  if (size() == 1) {
    assert(solver.decisionLevel() == 0);
    assert(isCorrectlyPropagating(solver, 0));
    solver.propagate(data[0], cr);
    return;
  }

  uint32_t watch = 0;
  for (uint32_t i = 0; i < size() && watch <= 1; ++i) {
    if (const Lit l = data[i]; !isFalse(level, l)) {
      data[i] = data[watch];
      data[watch++] = l;
    }
  }
  assert(watch >= 1);  // we found enough watches to satisfy the constraint
  assert((watch == 1) == isFalse(level, data[1]));
  if (watch == 1) {
    assert(!isFalse(level, data[0]));
    if (!isTrue(level, data[0])) {
      assert(isCorrectlyPropagating(solver, 0));
      solver.propagate(data[0], cr);
    }
    for (uint32_t i = 2; i < size(); ++i) {  // ensure last watch is last falsified literal
      assert(isFalse(level, data[i]));
      if (const Lit l = data[i]; level[-l] > level[-data[1]]) {
        data[i] = data[1];
        data[1] = l;
      }
    }
  }
  for (uint32_t i = 0; i < 2; ++i) adj[data[i]].emplace_back(cr, CLAUSE_IDX, data[1 - i]);  // add blocking literal
}

WatchStatus Clause::checkForPropagation(Watch& w, const Lit p, Solver& solver, Stats& stats) {
  const auto& level = solver.level;
  auto& adj = solver.adj;

  assert(p == data[0] || p == data[1]);
  assert(size() > 1);
  int widx = 0;
  Lit watch = data[0];
  Lit otherwatch = data[1];
  if (p == data[1]) {
    widx = 1;
    watch = data[1];
    otherwatch = data[0];
  }
  assert(p == watch);
  assert(p != otherwatch);
  if (isTrue(level, otherwatch)) {
    w.blocking = otherwatch;        // set new blocking literal
    return WatchStatus::KEEPWATCH;  // constraint is satisfied
  }

  const uint32_t start = next_watch_idx;
  for (; next_watch_idx < size(); ++next_watch_idx) {
    if (const Lit l = data[next_watch_idx]; !isFalse(level, l)) {
      data[next_watch_idx] = watch;
      data[widx] = l;
      adj[l].emplace_back(w.cref, CLAUSE_IDX, otherwatch);
      ++next_watch_idx;
      stats.NWATCHCHECKS.z += next_watch_idx - start + 1;
      return WatchStatus::DROPWATCH;
    }
  }
  next_watch_idx = 2;
  for (; next_watch_idx < start; ++next_watch_idx) {
    if (const Lit l = data[next_watch_idx]; !isFalse(level, l)) {
      data[next_watch_idx] = watch;
      data[widx] = l;
      adj[l].emplace_back(w.cref, CLAUSE_IDX, otherwatch);
      stats.NWATCHCHECKS.z += size() - start + next_watch_idx - 1;
      ++next_watch_idx;
      return WatchStatus::DROPWATCH;
    }
  }
  stats.NWATCHCHECKS.z += size() - 2;

  assert(isFalse(level, watch));
  for (uint32_t i = 2; i < size(); ++i) assert(isFalse(level, data[i]));
  if (isFalse(level, otherwatch)) {
    assert(isCorrectlyConflicting(solver));
    return WatchStatus::CONFLICTING;
  }
  assert(!isTrue(level, otherwatch));
  ++stats.NPROPCLAUSE.z;
  assert(isCorrectlyPropagating(solver, otherwatch == data[1]));
  solver.propagate(otherwatch, w.cref);
  ++stats.NPROPCHECKS.z;
  return WatchStatus::KEEPWATCH;
}

uint32_t Clause::resolveWith(CeSuper& confl, const Lit l, Solver& solver, IntSet& actSet) const {
  return confl->resolveWith({data, size()}, 1, id(), l, solver.getLevel(), solver.getPos(), actSet);
}
uint32_t Clause::subsumeWith(CeSuper& confl, const Lit l, Solver& solver, IntSet& saturatedLits) const {
  return confl->subsumeWith({data, size()}, 1, id(), l, solver.getLevel(), solver.getPos(), saturatedLits);
}

CeSuper Clause::toExpanded(ConstrExpPools& cePools) const {
  Ce32 result = cePools.take32();
  result->addRhs(1);
  for (uint32_t i = 0; i < size(); ++i) {
    result->addLhs(1, data[i]);
  }
  result->orig = getOrigin();
  result->resetBuffer(id());
  return result;
}

bool Clause::isSatisfiedAtRoot(const IntMap<int>& level) const {
  for (uint32_t i = 0; i < size(); ++i) {
    if (isUnit(level, data[i])) return true;
  }
  return false;
}

bool Clause::canBeSimplified(const IntMap<int>& level, Equalities& equalities, Implications& implications,
                             IntSetPool& isp) const {
  const bool isEquality = getOrigin() == Origin::EQUALITY;
  for (uint32_t i = 0; i < size(); ++i) {
    if (const Lit l = data[i]; isUnit(level, l) || isUnit(level, -l) || (!isEquality && !equalities.isCanonical(l))) {
      return true;
    }
  }
  if (!isEquality && getUnsaturatedIdx() > 0) {
    IntSet& saturateds = isp.take();
    for (uint32_t i = 0; i < getUnsaturatedIdx(); ++i) {
      saturateds.add(data[i]);
    }
    for (uint32_t i = 0; i < size(); ++i) {
      for (Lit l : implications.getImplieds(data[i])) {
        if (saturateds.has(l)) {
          isp.release(saturateds);
          return true;
        }
      }
    }
    isp.release(saturateds);
  }
  return false;
}

size_t Cardinality::getMemSize(const uint32_t length) {
  return aux::ceildiv(sizeof(Cardinality) + sizeof(Lit) * length, maxAlign);
}
size_t Cardinality::getMemSize() const { return getMemSize(size()); }

bigint Cardinality::degree() const { return degr; }
bigint Cardinality::coef(uint32_t) const { return 1; }
Lit Cardinality::lit(const uint32_t i) const { return data[i]; }
bool Cardinality::hasWatch(uint32_t i) const { return i < degr; }
uint32_t Cardinality::getUnsaturatedIdx() const { return 0; }
bool Cardinality::isClauseOrCard() const { return true; }
bool Cardinality::isAtMostOne() const { return degr == size() - 1; }

void Cardinality::initializeWatches(CRef cr, Solver& solver) {
  assert(degr > 1);  // otherwise not a cardinality
  const auto& level = solver.level;
  [[maybe_unused]] const auto& position = solver.position;
  auto& adj = solver.adj;

  if (degr >= size()) {
    assert(solver.decisionLevel() == 0);
    for (uint32_t i = 0; i < size(); ++i) {
      assert(isUnknown(position, data[i]));
      assert(isCorrectlyPropagating(solver, i));
      solver.propagate(data[i], cr);
    }
    return;
  }

  uint32_t watch = 0;
  for (uint32_t i = 0; i < size() && watch <= degr; ++i) {
    if (const Lit l = data[i]; !isFalse(level, l)) {
      data[i] = data[watch];
      data[watch++] = l;
    }
  }
  assert(watch >= degr);  // we found enough watches to satisfy the constraint
  if (isFalse(level, data[degr])) {
    for (uint32_t i = 0; i < degr; ++i) {
      assert(!isFalse(level, data[i]));
      if (!isTrue(level, data[i])) {
        assert(isCorrectlyPropagating(solver, i));
        solver.propagate(data[i], cr);
      }
    }
    for (uint32_t i = degr + 1; i < size(); ++i) {  // ensure last watch is last falsified literal
      assert(isFalse(level, data[i]));
      if (const Lit l = data[i]; level[-l] > level[-data[degr]]) {
        data[i] = data[degr];
        data[degr] = l;
      }
    }
  }
  for (uint32_t i = 0; i <= degr; ++i) adj[data[i]].emplace_back(cr, i + 3 * UINF, 0);  // add watch index
}

WatchStatus Cardinality::checkForPropagation(Watch& w, [[maybe_unused]] const Lit p, Solver& solver, Stats& stats) {
  const auto& level = solver.level;
  auto& adj = solver.adj;

  const uint32_t widx = w.idx - 3 * UINF;
  assert(data[widx] == p);
  assert(next_watch_idx > degr);

  const uint32_t start = next_watch_idx;
  for (; next_watch_idx < size(); ++next_watch_idx) {
    if (const Lit l = data[next_watch_idx]; !isFalse(level, l)) {
      data[next_watch_idx] = data[widx];
      data[widx] = l;
      adj[l].emplace_back(w);
      stats.NWATCHCHECKS.z += next_watch_idx - start + 1;
      return WatchStatus::DROPWATCH;
    }
  }
  next_watch_idx = degr + 1;
  for (; next_watch_idx < start; ++next_watch_idx) {
    if (const Lit l = data[next_watch_idx]; !isFalse(level, l)) {
      data[next_watch_idx] = data[widx];
      data[widx] = l;
      adj[l].emplace_back(w);
      stats.NWATCHCHECKS.z += size() - start + next_watch_idx - degr + 1;
      return WatchStatus::DROPWATCH;
    }
  }
  stats.NWATCHCHECKS.z += size() - degr - 1;

  assert(isFalse(level, data[widx]));
  for (uint32_t i = degr + 1; i < size(); ++i) assert(isFalse(level, data[i]));
  for (uint32_t i = 0; i <= degr; ++i) {
    if (i != widx && isFalse(level, data[i])) {
      assert(isCorrectlyConflicting(solver));
      return WatchStatus::CONFLICTING;
    }
  }
  int cardprops = 0;
  for (uint32_t i = 0; i <= degr; ++i) {
    if (const Lit l = data[i]; i != widx && !isTrue(level, l)) {
      ++cardprops;
      assert(isCorrectlyPropagating(solver, i));
      solver.propagate(l, w.cref);
    }
  }
  stats.NPROPCHECKS.z += degr + 1;
  stats.NPROPCARD.z += cardprops;
  return WatchStatus::KEEPWATCH;
}

uint32_t Cardinality::resolveWith(CeSuper& confl, const Lit l, Solver& solver, IntSet& actSet) const {
  return confl->resolveWith({data, size()}, degr, id(), l, solver.getLevel(), solver.getPos(), actSet);
}
uint32_t Cardinality::subsumeWith(CeSuper& confl, const Lit l, Solver& solver, IntSet& saturatedLits) const {
  return confl->subsumeWith({data, size()}, degr, id(), l, solver.getLevel(), solver.getPos(), saturatedLits);
}

CeSuper Cardinality::toExpanded(ConstrExpPools& cePools) const {
  Ce32 result = cePools.take32();
  result->addRhs(degr);
  for (uint32_t i = 0; i < size(); ++i) {
    result->addLhs(1, data[i]);
  }
  result->orig = getOrigin();
  result->resetBuffer(id());
  return result;
}

bool Cardinality::isSatisfiedAtRoot(const IntMap<int>& level) const {
  int eval = -static_cast<int>(degr);
  for (uint32_t i = 0; i < size() && eval < 0; ++i) {
    eval += isUnit(level, data[i]);
  }
  return eval >= 0;
}

bool Cardinality::canBeSimplified(const IntMap<int>& level, Equalities& equalities, Implications&, IntSetPool&) const {
  const bool isEquality = getOrigin() == Origin::EQUALITY;
  for (uint32_t i = 0; i < size(); ++i) {
    if (const Lit l = data[i]; isUnit(level, l) || isUnit(level, -l) || (!isEquality && !equalities.isCanonical(l))) {
      return true;
    }
  }
  // NOTE: no saturated literals in a cardinality, so no need to check for self-subsumption
  return false;
}

size_t Watched32::getMemSize(uint32_t length) {
  return aux::ceildiv(sizeof(Watched32) + sizeof(Lit) * length * 2, maxAlign);
}
size_t Watched32::getMemSize() const { return getMemSize(size()); }

bigint Watched32::degree() const { return degr; }
const int32_t& Watched32::cf(uint32_t i) const { return data[sze + i]; }
bigint Watched32::coef(uint32_t i) const { return cf(i); }
Lit Watched32::lit(uint32_t i) const { return data[i] >> 1; }
uint32_t Watched32::getUnsaturatedIdx() const { return unsaturatedIdx; }
bool Watched32::isClauseOrCard() const {
  assert(cf(0) > 1);
  return false;
}
bool Watched32::isAtMostOne() const {
  assert(!isClauseOrCard());
  return false;
}

bool Watched32::hasWatch(uint32_t i) const { return data[i] & 1; }

void Watched32::flipWatch(uint32_t i) { data[i] = data[i] ^ 1; }

void Watched32::initializeWatches(CRef cr, Solver& solver) {
  const auto& level = solver.level;
  const auto& position = solver.position;
  auto& adj = solver.adj;
  const auto& qhead = solver.qhead;

  watchslack = -degr;
  const int32_t& lrgstCf = cf(0);
  for (uint32_t i = 0; i < size() && watchslack < lrgstCf; ++i) {
    const Lit l = lit(i);
    const int pos_l = position[toVar(l)];
    if (pos_l >= qhead || !isFalse(level, l)) {
      assert(!hasWatch(i));
      watchslack += cf(i);
      flipWatch(i);
      adj[l].emplace_back(cr, i, 0);
      // NOTE: not adding blocked literals to backjumps incorrectly skipping watchslack updates
    }
  }
  assert(watchslack >= 0);
  assert(hasCorrectSlack(solver));
  if (watchslack < lrgstCf) {
    // set sufficient falsified watches
    std::vector<uint32_t>& falsifiedIdcs = solver.falsifiedIdcsMem;
    assert(falsifiedIdcs.empty());
    for (uint32_t i = 0; i < size(); ++i) {
      if (isFalse(level, lit(i)) && position[toVar(lit(i))] < qhead) falsifiedIdcs.push_back(i);
    }
    std::sort(falsifiedIdcs.begin(), falsifiedIdcs.end(),
              [&](uint32_t i1, uint32_t i2) { return position[toVar(lit(i1))] > position[toVar(lit(i2))]; });
    int64_t diff = lrgstCf - watchslack;
    for (uint32_t i : falsifiedIdcs) {
      assert(!hasWatch(i));
      diff -= cf(i);
      flipWatch(i);
      adj[lit(i)].emplace_back(cr, i, 0);
      if (diff <= 0) break;
    }
    // perform initial propagation
    for (uint32_t i = 0; i < size() && cf(i) > watchslack; ++i) {
      if (isUnknown(position, lit(i))) {
        assert(isCorrectlyPropagating(solver, i));
        solver.propagate(lit(i), cr);
      }
    }
    falsifiedIdcs.clear();
  }
}

WatchStatus Watched32::checkForPropagation(Watch& w, [[maybe_unused]] const Lit p, Solver& solver, Stats& stats) {
  const auto& level = solver.level;
  const auto& position = solver.position;
  auto& adj = solver.adj;

  const uint32_t& widx = w.idx;
  assert(lit(widx) == p);
  assert(hasWatch(widx));

  const int p_pos = position[toVar(p)];
  if (isTrue(level, blocking) && position[toVar(blocking)] < p_pos) {
    // entered the constraint so watch simply has the wrong blocking literal
    w.blocking = blocking;
    return WatchStatus::KEEPWATCH;
  }

  const int32_t& lrgstCf = cf(0);
  const bool lookForWatches = watchslack >= lrgstCf;
  watchslack -= cf(widx);
  // look for new watches if previously, watchslack was at least lrgstCf
  // else we did not find enough watches last time, so we can skip looking for them now

  if (lookForWatches) {
    uint32_t start_watch_idx = next_watch_idx;
    stats.NWATCHCHECKS.z -= next_watch_idx;
    for (; next_watch_idx < unsaturatedIdx && watchslack < lrgstCf; ++next_watch_idx) {
      if (const Lit l = lit(next_watch_idx); !isFalse(level, l)) {
        if (position[toVar(l)] < p_pos) {
          assert(isTrue(level, l));
          blocking = l;
          w.blocking = l;
          watchslack += cf(widx);
          stats.NWATCHCHECKS.z += next_watch_idx;
          return WatchStatus::KEEPWATCH;
        }
        if (!hasWatch(next_watch_idx)) {
          watchslack += cf(next_watch_idx);
          flipWatch(next_watch_idx);
          adj[l].emplace_back(w.cref, next_watch_idx, blocking);
        }
      }
    }  // NOTE: first innermost loop
    for (; next_watch_idx < size() && watchslack < lrgstCf; ++next_watch_idx) {
      if (const Lit l = lit(next_watch_idx); !hasWatch(next_watch_idx) && !isFalse(level, l)) {
        watchslack += cf(next_watch_idx);
        flipWatch(next_watch_idx);
        adj[l].emplace_back(w.cref, next_watch_idx, blocking);
      }
    }  // NOTE: second innermost loop
    stats.NWATCHCHECKS.z += next_watch_idx;

    if (watchslack < lrgstCf) {
      next_watch_idx = 0;
      for (; next_watch_idx < std::min(unsaturatedIdx, start_watch_idx) && watchslack < lrgstCf; ++next_watch_idx) {
        if (const Lit l = lit(next_watch_idx); !isFalse(level, l)) {
          if (position[toVar(l)] < p_pos) {
            assert(isTrue(level, l));
            blocking = l;
            w.blocking = l;
            watchslack += cf(widx);
            stats.NWATCHCHECKS.z += next_watch_idx;
            return WatchStatus::KEEPWATCH;
          }
          if (!hasWatch(next_watch_idx)) {
            watchslack += cf(next_watch_idx);
            flipWatch(next_watch_idx);
            adj[l].emplace_back(w.cref, next_watch_idx, blocking);
          }
        }
      }  // NOTE: first innermost loop
      for (; next_watch_idx < start_watch_idx && watchslack < lrgstCf; ++next_watch_idx) {
        if (const Lit l = lit(next_watch_idx); !hasWatch(next_watch_idx) && !isFalse(level, l)) {
          watchslack += cf(next_watch_idx);
          flipWatch(next_watch_idx);
          adj[l].emplace_back(w.cref, next_watch_idx, blocking);
        }
      }  // NOTE: second innermost loop
      stats.NWATCHCHECKS.z += next_watch_idx;
    }
    assert(watchslack >= lrgstCf || next_watch_idx == start_watch_idx);
  }

  assert(hasCorrectSlack(solver));
  assert(hasCorrectWatches(solver));

  if (watchslack >= lrgstCf) {
    flipWatch(widx);
    return WatchStatus::DROPWATCH;
  }
  if (watchslack < 0) {
    assert(isCorrectlyConflicting(solver));
    return WatchStatus::CONFLICTING;
  }
  // keep the watch, check for propagation
  uint32_t prop_idx = 0;
  int64_t true_sum = 0;
  for (; prop_idx < size() && true_sum < degr && cf(prop_idx) > watchslack; ++prop_idx) {
    const Lit l = lit(prop_idx);
    if (isTrue(level, l)) {
      true_sum += cf(prop_idx);
    } else if (isUnknown(position, l)) {
      true_sum += cf(prop_idx);
      ++stats.NPROPWATCH.z;
      assert(isCorrectlyPropagating(solver, prop_idx));
      solver.propagate(l, w.cref);
    }  // NOTE: third innermost loop
  }
  stats.NPROPCHECKS.z += prop_idx;

  // NOTE: when skipping the watch calculation in subsequent propagation phases, it can happen that the constraint
  // became conflicting.
  return prop_idx >= size() && true_sum < degr ? WatchStatus::CONFLICTING : WatchStatus::KEEPWATCH;
}

void Watched32::undoFalsified(uint32_t i) {
  assert(i < UINF);
  assert(hasWatch(i));
  watchslack += cf(i);
}

uint32_t Watched32::resolveWith(CeSuper& confl, const Lit l, Solver& solver, IntSet& actSet) const {
  return confl->resolveWith(data, data + size(), size(), degr, id(), getOrigin(), l, solver.getLevel(), solver.getPos(),
                            actSet);
}
uint32_t Watched32::subsumeWith(CeSuper& confl, const Lit l, Solver& solver, IntSet& saturatedLits) const {
  return confl->subsumeWith(data, data + size(), size(), degr, id(), l, solver.getLevel(), solver.getPos(),
                            saturatedLits);
}

Ce32 Watched32::expandTo(ConstrExpPools& cePools) const {
  Ce32 result = cePools.take32();
  result->addRhs(degr);
  for (uint32_t i = 0; i < size(); ++i) {
    result->addLhs(cf(i), lit(i));
  }
  result->orig = getOrigin();
  result->resetBuffer(id());
  assert(result->isSortedInDecreasingCoefOrder());
  return result;
}

CeSuper Watched32::toExpanded(ConstrExpPools& cePools) const { return expandTo(cePools); }

bool Watched32::isSatisfiedAtRoot(const IntMap<int>& level) const {
  int64_t eval = -degr;
  for (uint32_t i = 0; i < size() && eval < 0; ++i) {
    if (isUnit(level, lit(i))) eval += cf(i);
  }
  return eval >= 0;
}

bool Watched32::canBeSimplified(const IntMap<int>& level, Equalities& equalities, Implications& implications,
                                IntSetPool& isp) const {
  const bool isEquality = getOrigin() == Origin::EQUALITY;
  for (uint32_t i = 0; i < size(); ++i) {
    if (const Lit l = lit(i); isUnit(level, l) || isUnit(level, -l) || (!isEquality && !equalities.isCanonical(l)))
      return true;
  }
  if (!isEquality && getUnsaturatedIdx() > 0) {
    IntSet& saturateds = isp.take();
    for (uint32_t i = 0; i < getUnsaturatedIdx(); ++i) {
      saturateds.add(lit(i));
    }
    for (uint32_t i = 0; i < size(); ++i) {
      for (Lit l : implications.getImplieds(lit(i))) {
        if (saturateds.has(l)) {
          isp.release(saturateds);
          return true;
        }
      }
    }
    isp.release(saturateds);
  }
  return false;
}

template <typename CF, typename DG>
size_t Watched<CF, DG>::getMemSize(uint32_t length) {
  return aux::ceildiv(sizeof(Watched<CF, DG>) + sizeof(Lit) * length, maxAlign);
}
template <typename CF, typename DG>
size_t Watched<CF, DG>::getMemSize() const {
  return getMemSize(size());
}

template <typename CF, typename DG>
bigint Watched<CF, DG>::degree() const {
  return bigint(degr);
}
template <typename CF, typename DG>
const CF& Watched<CF, DG>::cf(uint32_t i) const {
  return cfs[i];
}
template <typename CF, typename DG>
bigint Watched<CF, DG>::coef(uint32_t i) const {
  return cf(i);
}
template <typename CF, typename DG>
Lit Watched<CF, DG>::lit(uint32_t i) const {
  return lits[i] >> 1;
}
template <typename CF, typename DG>
uint32_t Watched<CF, DG>::getUnsaturatedIdx() const {
  return unsaturatedIdx;
}
template <typename CF, typename DG>
bool Watched<CF, DG>::isClauseOrCard() const {
  assert(cf(0) > 1);
  return false;
}
template <typename CF, typename DG>
bool Watched<CF, DG>::isAtMostOne() const {
  assert(!isClauseOrCard());
  return false;
}

template <typename CF, typename DG>
bool Watched<CF, DG>::hasWatch(uint32_t i) const {
  return lits[i] & 1;
}
template <typename CF, typename DG>
void Watched<CF, DG>::flipWatch(uint32_t i) {
  lits[i] = lits[i] ^ 1;
}

template <typename CF, typename DG>
void Watched<CF, DG>::initializeWatches(CRef cr, Solver& solver) {
  const auto& level = solver.level;
  const auto& position = solver.position;
  auto& adj = solver.adj;
  const auto& qhead = solver.qhead;

  watchslack = -degr;
  const CF& lrgstCf = cf(0);
  for (uint32_t i = 0; i < size() && watchslack < lrgstCf; ++i) {
    const Lit l = lit(i);
    const int pos_l = position[toVar(l)];
    if (pos_l >= qhead || !isFalse(level, l)) {
      assert(!hasWatch(i));
      watchslack += cf(i);
      flipWatch(i);
      adj[l].emplace_back(cr, i + 2 * UINF, 0);
      // NOTE: not adding blocked literals to backjumps incorrectly skipping watchslack updates
    }
  }
  assert(watchslack >= 0);
  assert(hasCorrectSlack(solver));
  if (watchslack < lrgstCf) {
    // set sufficient falsified watches
    std::vector<uint32_t>& falsifiedIdcs = solver.falsifiedIdcsMem;
    assert(falsifiedIdcs.empty());
    for (uint32_t i = 0; i < size(); ++i) {
      if (isFalse(level, lit(i)) && position[toVar(lit(i))] < qhead) falsifiedIdcs.push_back(i);
    }
    std::sort(falsifiedIdcs.begin(), falsifiedIdcs.end(),
              [&](uint32_t i1, uint32_t i2) { return position[toVar(lit(i1))] > position[toVar(lit(i2))]; });
    DG diff = lrgstCf - watchslack;
    for (uint32_t i : falsifiedIdcs) {
      assert(!hasWatch(i));
      diff -= cf(i);
      flipWatch(i);
      adj[lit(i)].emplace_back(cr, i + 2 * UINF, 0);
      if (diff <= 0) break;
    }
    // perform initial propagation
    for (uint32_t i = 0; i < size() && cf(i) > watchslack; ++i) {
      if (isUnknown(position, lit(i))) {
        assert(isCorrectlyPropagating(solver, i));
        solver.propagate(lit(i), cr);
      }
    }
    falsifiedIdcs.clear();
  }
}

template <typename CF, typename DG>
WatchStatus Watched<CF, DG>::checkForPropagation(Watch& w, [[maybe_unused]] const Lit p, Solver& solver, Stats& stats) {
  const auto& level = solver.level;
  const auto& position = solver.position;
  auto& adj = solver.adj;

  const uint32_t widx = w.idx - 2 * UINF;
  assert(lit(widx) == p);
  assert(hasWatch(widx));

  const int p_pos = position[toVar(p)];
  if (isTrue(level, blocking) && position[toVar(blocking)] < p_pos) {
    // entered the constraint so watch simply has the wrong blocking literal
    w.blocking = blocking;
    return WatchStatus::KEEPWATCH;
  }

  const CF& lrgstCf = cf(0);
  const bool lookForWatches = watchslack >= lrgstCf;
  watchslack -= cf(widx);
  // look for new watches if previously, watchslack was at least lrgstCf
  // else we did not find enough watches last time, so we can skip looking for them now
  if (lookForWatches) {
    uint32_t start_watch_idx = next_watch_idx;
    stats.NWATCHCHECKS.z -= next_watch_idx;
    for (; next_watch_idx < unsaturatedIdx && watchslack < lrgstCf; ++next_watch_idx) {
      if (const Lit l = lit(next_watch_idx); !isFalse(level, l)) {
        if (position[toVar(l)] < p_pos) {
          assert(isTrue(level, l));
          blocking = l;
          w.blocking = l;
          watchslack += cf(widx);
          stats.NWATCHCHECKS.z += next_watch_idx;
          return WatchStatus::KEEPWATCH;
        }
        if (!hasWatch(next_watch_idx)) {
          watchslack += cf(next_watch_idx);
          flipWatch(next_watch_idx);
          adj[l].emplace_back(w.cref, next_watch_idx + 2 * UINF, blocking);
        }
      }
    }  // NOTE: first innermost loop
    for (; next_watch_idx < size() && watchslack < lrgstCf; ++next_watch_idx) {
      if (const Lit l = lit(next_watch_idx); !hasWatch(next_watch_idx) && !isFalse(level, l)) {
        watchslack += cf(next_watch_idx);
        flipWatch(next_watch_idx);
        adj[l].emplace_back(w.cref, next_watch_idx + 2 * UINF, blocking);
      }
    }  // NOTE: second innermost loop
    stats.NWATCHCHECKS.z += next_watch_idx;

    if (watchslack < lrgstCf) {
      next_watch_idx = 0;
      for (; next_watch_idx < std::min(unsaturatedIdx, start_watch_idx) && watchslack < lrgstCf; ++next_watch_idx) {
        if (const Lit l = lit(next_watch_idx); !isFalse(level, l)) {
          if (position[toVar(l)] < p_pos) {
            assert(isTrue(level, l));
            blocking = l;
            w.blocking = l;
            watchslack += cf(widx);
            stats.NWATCHCHECKS.z += next_watch_idx;
            return WatchStatus::KEEPWATCH;
          }
          if (!hasWatch(next_watch_idx)) {
            watchslack += cf(next_watch_idx);
            flipWatch(next_watch_idx);
            adj[l].emplace_back(w.cref, next_watch_idx + 2 * UINF, blocking);
          }
        }
      }  // NOTE: first innermost loop
      for (; next_watch_idx < start_watch_idx && watchslack < lrgstCf; ++next_watch_idx) {
        if (const Lit l = lit(next_watch_idx); !hasWatch(next_watch_idx) && !isFalse(level, l)) {
          watchslack += cf(next_watch_idx);
          flipWatch(next_watch_idx);
          adj[l].emplace_back(w.cref, next_watch_idx + 2 * UINF, blocking);
        }
      }  // NOTE: second innermost loop
      stats.NWATCHCHECKS.z += next_watch_idx;
    }
    assert(watchslack >= lrgstCf || next_watch_idx == start_watch_idx);
  }

  assert(hasCorrectSlack(solver));
  assert(hasCorrectWatches(solver));

  if (watchslack >= lrgstCf) {
    flipWatch(widx);
    return WatchStatus::DROPWATCH;
  }
  if (watchslack < 0) {
    assert(isCorrectlyConflicting(solver));
    return WatchStatus::CONFLICTING;
  }

  // keep the watch, check for propagation
  uint32_t prop_idx = 0;
  DG true_sum = 0;
  for (; prop_idx < size() && true_sum < degr && cf(prop_idx) > watchslack; ++prop_idx) {
    const Lit l = lit(prop_idx);
    if (isTrue(level, l)) {
      true_sum += cf(prop_idx);
    } else if (isUnknown(position, l)) {
      true_sum += cf(prop_idx);
      ++stats.NPROPWATCH.z;
      assert(isCorrectlyPropagating(solver, prop_idx));
      solver.propagate(l, w.cref);
    }  // NOTE: third innermost loop
  }
  stats.NPROPCHECKS.z += prop_idx;

  // NOTE: when skipping the watch calculation in subsequent propagation phases, it can happen that the constraint
  // became conflicting.
  return prop_idx >= size() && true_sum < degr ? WatchStatus::CONFLICTING : WatchStatus::KEEPWATCH;
}

template <typename CF, typename DG>
void Watched<CF, DG>::undoFalsified(uint32_t i) {
  assert(i < 3 * UINF);
  assert(i >= 2 * UINF);
  assert(hasWatch(i - 2 * UINF));
  watchslack += cf(i - 2 * UINF);
}

template <typename CF, typename DG>
uint32_t Watched<CF, DG>::resolveWith(CeSuper& confl, const Lit l, Solver& solver, IntSet& actSet) const {
  return confl->resolveWith(lits, cfs, size(), degr, id(), getOrigin(), l, solver.getLevel(), solver.getPos(), actSet);
}
template <typename CF, typename DG>
uint32_t Watched<CF, DG>::subsumeWith(CeSuper& confl, const Lit l, Solver& solver, IntSet& saturatedLits) const {
  return confl->subsumeWith(lits, cfs, size(), degr, id(), l, solver.getLevel(), solver.getPos(), saturatedLits);
}

template <typename CF, typename DG>
CePtr<CF, DG> Watched<CF, DG>::expandTo(ConstrExpPools& cePools) const {
  CePtr<CF, DG> result = cePools.take<CF, DG>();
  result->addRhs(degr);
  for (uint32_t i = 0; i < size(); ++i) {
    result->addLhs(cf(i), lit(i));
  }
  result->orig = getOrigin();
  result->resetBuffer(id());
  assert(result->isSortedInDecreasingCoefOrder());
  return result;
}

template <typename CF, typename DG>
CeSuper Watched<CF, DG>::toExpanded(ConstrExpPools& cePools) const {
  return expandTo(cePools);
}

template <typename CF, typename DG>
bool Watched<CF, DG>::isSatisfiedAtRoot(const IntMap<int>& level) const {
  DG eval = -degr;
  for (uint32_t i = 0; i < size() && eval < 0; ++i) {
    if (isUnit(level, lit(i))) eval += cf(i);
  }
  return eval >= 0;
}

template <typename CF, typename DG>
bool Watched<CF, DG>::canBeSimplified(const IntMap<int>& level, Equalities& equalities, Implications& implications,
                                      IntSetPool& isp) const {
  const bool isEquality = getOrigin() == Origin::EQUALITY;
  for (uint32_t i = 0; i < size(); ++i) {
    if (const Lit l = lit(i); isUnit(level, l) || isUnit(level, -l) || (!isEquality && !equalities.isCanonical(l)))
      return true;
  }
  if (!isEquality && getUnsaturatedIdx() > 0) {
    IntSet& saturateds = isp.take();
    for (uint32_t i = 0; i < getUnsaturatedIdx(); ++i) {
      saturateds.add(lit(i));
    }
    for (uint32_t i = 0; i < size(); ++i) {
      for (Lit l : implications.getImplieds(lit(i))) {
        if (saturateds.has(l)) {
          isp.release(saturateds);
          return true;
        }
      }
    }
    isp.release(saturateds);
  }
  return false;
}

// TODO: keep below test methods?

bool Constr::isCorrectlyConflicting(const Solver& solver) const {
  return true;  // comment to run check
  bigint slack = -degree();
  for (int i = 0; i < (int)size(); ++i) {
    slack += isFalse(solver.getLevel(), lit(i)) ? 0 : coef(i);
  }
  return slack < 0;
}

bool Constr::isCorrectlyPropagating(const Solver& solver, int idx) const {
  return true;  // comment to run check
  assert(isUnknown(solver.getPos(), lit(idx)));
  bigint slack = -degree();
  for (uint32_t i = 0; i < size(); ++i) {
    slack += isFalse(solver.getLevel(), lit(i)) ? 0 : coef(i);
  }
  return slack < coef(idx);
}

void Constr::print(const Solver& solver) const {
  for (uint32_t i = 0; i < size(); ++i) {
    const int pos = solver.getPos()[toVar(lit(i))];
    std::cout << coef(i) << "x" << lit(i)
              << (pos < solver.qhead ? (isTrue(solver.getLevel(), lit(i)) ? "t" : "f") : "u")
              << (hasWatch(i) ? "*" : "") << (pos >= INF ? -1 : pos) << " ";
  }
  std::cout << ">= " << degree() << std::endl;
}

bool Watched32::hasCorrectSlack(const Solver& solver) {
  return true;  // comment to run check
  int64_t slk = -degr;
  for (int i = 0; i < (int)size(); ++i) {
    if (hasWatch(i) && (solver.getPos()[toVar(lit(i))] >= solver.qhead || !isFalse(solver.getLevel(), lit(i))))
      slk += cf(i);
  }
  return (slk == watchslack);
}

template <typename CF, typename DG>
bool Watched<CF, DG>::hasCorrectSlack(const Solver& solver) {
  return true;  // comment to run check
  DG slk = -degr;
  for (int i = 0; i < (int)size(); ++i) {
    if (hasWatch(i) && (solver.getPos()[toVar(lit(i))] >= solver.qhead || !isFalse(solver.getLevel(), lit(i))))
      slk += cf(i);
  }
  return (slk == watchslack);
}

bool Watched32::hasCorrectWatches(const Solver& solver) {
  return true;  // comment to run check
  if (watchslack >= cf(0)) return true;
  // for (int i = 0; i < (int)watchIdx; ++i) assert(isKnown(solver.getPos(), lit(i)));
  for (int i = 0; i < (int)size(); ++i) {
    if (!(hasWatch(i) || isFalse(solver.getLevel(), lit(i)))) {
      std::cout << i << " " << cf(i) << " " << isFalse(solver.getLevel(), lit(i)) << std::endl;
      print(solver);
    }
    assert(hasWatch(i) || isFalse(solver.getLevel(), lit(i)));
  }
  return true;
}

template <typename CF, typename DG>
bool Watched<CF, DG>::hasCorrectWatches(const Solver& solver) {
  return true;  // comment to run check
  if (watchslack >= cf(0)) return true;
  // for (int i = 0; i < (int)watchIdx; ++i) assert(isKnown(solver.getPos(), lit(i)));
  for (int i = 0; i < (int)size(); ++i) {
    if (!(hasWatch(i) || isFalse(solver.getLevel(), lit(i)))) {
      std::cout << i << " " << cf(i) << " " << isFalse(solver.getLevel(), lit(i)) << std::endl;
      print(solver);
    }
    assert(hasWatch(i) || isFalse(solver.getLevel(), lit(i)));
  }
  return true;
}

template struct Watched<int64_t, int128>;
template struct Watched<int128, int128>;
template struct Watched<int128, int256>;
template struct Watched<bigint, bigint>;

}  // namespace xct
