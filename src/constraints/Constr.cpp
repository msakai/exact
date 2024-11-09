/**********************************************************************
This file is part of Exact.

Copyright (c) 2022-2024 Jo Devriendt, Nonfiction Software

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
  stats.NENCFORMULA += o == Origin::FORMULA;
  stats.NENCDOMBREAKER += o == Origin::DOMBREAKER;
  stats.NENCLEARNED += o == Origin::LEARNED;
  stats.NENCBOUND += isBound(o) || o == Origin::REFORMBOUND;
  stats.NENCCOREGUIDED += o == Origin::COREGUIDED || o == Origin::BOTTOMUP;
  stats.NLPENCGOMORY += o == Origin::GOMORY;
  stats.NLPENCDUAL += o == Origin::DUAL;
  stats.NLPENCFARKAS += o == Origin::FARKAS;
  stats.NENCDETECTEDAMO += o == Origin::DETECTEDAMO;
  stats.NENCREDUCED += o == Origin::REDUCED;
  stats.NENCEQ += o == Origin::EQUALITY;
  stats.NENCIMPL += o == Origin::IMPLICATION;
  ++stats.NRESOLVESTEPS;
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
  for (uint32_t i = 0; i < 2; ++i) adj[data[i]].emplace_back(cr, 2 * INF, data[1 - i]);  // add blocking literal
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
      adj[l].emplace_back(w.cref, 2 * INF, otherwatch);
      ++next_watch_idx;
      stats.NWATCHCHECKS += next_watch_idx - start + 1;
      return WatchStatus::DROPWATCH;
    }
  }
  next_watch_idx = 2;
  for (; next_watch_idx < start; ++next_watch_idx) {
    if (const Lit l = data[next_watch_idx]; !isFalse(level, l)) {
      data[next_watch_idx] = watch;
      data[widx] = l;
      adj[l].emplace_back(w.cref, 2 * INF, otherwatch);
      stats.NWATCHCHECKS += size() - start + next_watch_idx - 1;
      ++next_watch_idx;
      return WatchStatus::DROPWATCH;
    }
  }
  stats.NWATCHCHECKS += size() - 2;

  assert(isFalse(level, watch));
  for (uint32_t i = 2; i < size(); ++i) assert(isFalse(level, data[i]));
  if (isFalse(level, otherwatch)) {
    assert(isCorrectlyConflicting(solver));
    return WatchStatus::CONFLICTING;
  }
  assert(!isTrue(level, otherwatch));
  ++stats.NPROPCLAUSE;
  assert(isCorrectlyPropagating(solver, otherwatch == data[1]));
  solver.propagate(otherwatch, w.cref);
  ++stats.NPROPCHECKS;
  return WatchStatus::KEEPWATCH;
}

uint32_t Clause::resolveWith(CeSuper& confl, const Lit l, Solver& solver, IntSet& actSet) const {
  return confl->resolveWith(data, size(), 1, id(), l, solver.getLevel(), solver.getPos(), actSet);
}
uint32_t Clause::subsumeWith(CeSuper& confl, const Lit l, Solver& solver, IntSet& saturatedLits) const {
  return confl->subsumeWith(data, size(), 1, id(), l, solver.getLevel(), solver.getPos(), saturatedLits);
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
  if (!isEquality) {
    IntSet& hasImplieds = isp.take();
    for (uint32_t i = 0; i < getUnsaturatedIdx(); ++i) {
      if (const Lit l = data[i]; implications.hasImplieds(l)) hasImplieds.add(-l);
    }
    if (!hasImplieds.isEmpty()) {
      for (uint32_t i = 0; i < getUnsaturatedIdx(); ++i) {
        if (hasImplieds.has(data[i])) {
          isp.release(hasImplieds);
          return true;
        }
      }
    }
    isp.release(hasImplieds);
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
  for (uint32_t i = 0; i <= degr; ++i) adj[data[i]].emplace_back(cr, i + INF, 0);  // add watch index
}

WatchStatus Cardinality::checkForPropagation(Watch& w, [[maybe_unused]] const Lit p, Solver& solver, Stats& stats) {
  const auto& level = solver.level;
  auto& adj = solver.adj;

  assert(data[w.idx - INF] == p);
  assert(next_watch_idx > degr);

  const uint32_t start = next_watch_idx;
  for (; next_watch_idx < size(); ++next_watch_idx) {
    if (const Lit l = data[next_watch_idx]; !isFalse(level, l)) {
      const uint32_t old_idx = w.idx - INF;
      data[next_watch_idx] = data[old_idx];
      data[old_idx] = l;
      adj[l].emplace_back(w.cref, w.idx, 0);
      stats.NWATCHCHECKS += next_watch_idx - start + 1;
      return WatchStatus::DROPWATCH;
    }
  }
  next_watch_idx = degr + 1;
  for (; next_watch_idx < start; ++next_watch_idx) {
    if (const Lit l = data[next_watch_idx]; !isFalse(level, l)) {
      const uint32_t old_idx = w.idx - INF;
      data[next_watch_idx] = data[old_idx];
      data[old_idx] = l;
      adj[l].emplace_back(w.cref, w.idx, 0);
      stats.NWATCHCHECKS += size() - start + next_watch_idx - degr + 1;
      return WatchStatus::DROPWATCH;
    }
  }
  stats.NWATCHCHECKS += size() - degr - 1;

  assert(isFalse(level, data[w.idx - INF]));
  for (uint32_t i = degr + 1; i < size(); ++i) assert(isFalse(level, data[i]));
  const uint32_t old_idx = w.idx - INF;
  for (uint32_t i = 0; i <= degr; ++i) {
    if (i != old_idx && isFalse(level, data[i])) {
      assert(isCorrectlyConflicting(solver));
      return WatchStatus::CONFLICTING;
    }
  }
  int cardprops = 0;
  for (uint32_t i = 0; i <= degr; ++i) {
    if (const Lit l = data[i]; i != old_idx && !isTrue(level, l)) {
      ++cardprops;
      assert(isCorrectlyPropagating(solver, i));
      solver.propagate(l, w.cref);
    }
  }
  stats.NPROPCHECKS += degr + 1;
  stats.NPROPCARD += cardprops;
  return WatchStatus::KEEPWATCH;
}

uint32_t Cardinality::resolveWith(CeSuper& confl, const Lit l, Solver& solver, IntSet& actSet) const {
  return confl->resolveWith(data, size(), degr, id(), l, solver.getLevel(), solver.getPos(), actSet);
}
uint32_t Cardinality::subsumeWith(CeSuper& confl, const Lit l, Solver& solver, IntSet& saturatedLits) const {
  return confl->subsumeWith(data, size(), degr, id(), l, solver.getLevel(), solver.getPos(), saturatedLits);
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

template <typename CF, typename DG>
bool Watched<CF, DG>::hasWatch(uint32_t i) const {
  return data[i] & 1;
}
template <typename CF, typename DG>
void Watched<CF, DG>::flipWatch(uint32_t i) {
  data[i] = data[i] ^ 1;
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
    DG diff = lrgstCf - watchslack;
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

template <typename CF, typename DG>
WatchStatus Watched<CF, DG>::checkForPropagation(Watch& w, [[maybe_unused]] const Lit p, Solver& solver, Stats& stats) {
  const auto& level = solver.level;
  const auto& position = solver.position;
  auto& adj = solver.adj;

  assert(lit(w.idx) == p);
  assert(hasWatch(w.idx));

  const int p_pos = position[toVar(p)];
  if (isTrue(level, blocking) && position[toVar(blocking)] < p_pos) {
    // entered the constraint so watch simply has the wrong blocking literal
    w.blocking = blocking;
    return WatchStatus::KEEPWATCH;
  }

  const CF& lrgstCf = cf(0);
  const bool lookForWatches = watchslack >= lrgstCf;
  watchslack -= cf(w.idx);
  // look for new watches if previously, watchslack was at least lrgstCf
  // else we did not find enough watches last time, so we can skip looking for them now

  if (lookForWatches) {
    uint32_t start_watch_idx = next_watch_idx;
    stats.NWATCHCHECKS -= next_watch_idx;
    for (; next_watch_idx < unsaturatedIdx && watchslack < lrgstCf; ++next_watch_idx) {
      if (const Lit l = lit(next_watch_idx); !isFalse(level, l)) {
        if (next_watch_idx < unsaturatedIdx && position[toVar(l)] < p_pos) {
          assert(isTrue(level, l));
          blocking = l;
          w.blocking = l;
          watchslack += cf(w.idx);
          stats.NWATCHCHECKS += next_watch_idx;
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
    stats.NWATCHCHECKS += next_watch_idx;

    if (watchslack < lrgstCf) {
      next_watch_idx = 0;
      for (; next_watch_idx < std::min(unsaturatedIdx, start_watch_idx) && watchslack < lrgstCf; ++next_watch_idx) {
        if (const Lit l = lit(next_watch_idx); !isFalse(level, l)) {
          if (next_watch_idx < unsaturatedIdx && position[toVar(l)] < p_pos) {
            assert(isTrue(level, l));
            blocking = l;
            w.blocking = l;
            watchslack += cf(w.idx);
            stats.NWATCHCHECKS += next_watch_idx;
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
      stats.NWATCHCHECKS += next_watch_idx;
    }
    assert(watchslack >= lrgstCf || next_watch_idx == start_watch_idx);
  }

  assert(hasCorrectSlack(solver));
  assert(hasCorrectWatches(solver));

  if (watchslack >= lrgstCf) {
    flipWatch(w.idx);
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
      ++stats.NPROPWATCH;
      assert(isCorrectlyPropagating(solver, prop_idx));
      solver.propagate(l, w.cref);
    }  // NOTE: third innermost loop
  }
  stats.NPROPCHECKS += prop_idx;

  // NOTE: when skipping the watch calculation in subsequent propagation phases, it can happen that the constraint
  // became conflicting.
  return prop_idx >= size() && true_sum < degr ? WatchStatus::CONFLICTING : WatchStatus::KEEPWATCH;
}

template <typename CF, typename DG>
void Watched<CF, DG>::undoFalsified(uint32_t i) {
  assert(i < INF);
  assert(hasWatch(i));
  watchslack += cf(i);
}

template <typename CF, typename DG>
uint32_t Watched<CF, DG>::resolveWith(CeSuper& confl, const Lit l, Solver& solver, IntSet& actSet) const {
  return confl->resolveWith(data, (CF*)data + size(), size(), degr, id(), getOrigin(), l, solver.getLevel(),
                            solver.getPos(), actSet);
}
template <typename CF, typename DG>
uint32_t Watched<CF, DG>::subsumeWith(CeSuper& confl, const Lit l, Solver& solver, IntSet& saturatedLits) const {
  return confl->subsumeWith(data, (CF*)data + size(), size(), degr, id(), l, solver.getLevel(), solver.getPos(),
                            saturatedLits);
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
  if (!isEquality) {
    IntSet& hasImplieds = isp.take();
    for (uint32_t i = 0; i < getUnsaturatedIdx(); ++i) {
      if (const Lit l = lit(i); implications.hasImplieds(l)) hasImplieds.add(-l);
    }
    if (!hasImplieds.isEmpty()) {
      for (uint32_t i = 0; i < getUnsaturatedIdx(); ++i) {
        if (hasImplieds.has(lit(i))) {
          isp.release(hasImplieds);
          return true;
        }
      }
    }
    isp.release(hasImplieds);
  }
  return false;
}

template <typename CF, typename DG>
bool WatchedSafe<CF, DG>::hasWatch(uint32_t i) const {
  return lits[i] & 1;
}
template <typename CF, typename DG>
void WatchedSafe<CF, DG>::flipWatch(uint32_t i) {
  lits[i] = lits[i] ^ 1;
}

template <typename CF, typename DG>
void WatchedSafe<CF, DG>::initializeWatches(CRef cr, Solver& solver) {
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
    DG diff = lrgstCf - watchslack;
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

template <typename CF, typename DG>
WatchStatus WatchedSafe<CF, DG>::checkForPropagation(Watch& w, [[maybe_unused]] const Lit p, Solver& solver,
                                                     Stats& stats) {
  const auto& level = solver.level;
  const auto& position = solver.position;
  auto& adj = solver.adj;

  assert(lit(w.idx) == p);
  assert(hasWatch(w.idx));

  const int p_pos = position[toVar(p)];
  if (isTrue(level, blocking) && position[toVar(blocking)] < p_pos) {
    // entered the constraint so watch simply has the wrong blocking literal
    w.blocking = blocking;
    return WatchStatus::KEEPWATCH;
  }

  const CF& lrgstCf = cf(0);
  const bool lookForWatches = watchslack >= lrgstCf;
  watchslack -= cf(w.idx);
  // look for new watches if previously, watchslack was at least lrgstCf
  // else we did not find enough watches last time, so we can skip looking for them now
  if (lookForWatches) {
    uint32_t start_watch_idx = next_watch_idx;
    stats.NWATCHCHECKS -= next_watch_idx;
    for (; next_watch_idx < unsaturatedIdx && watchslack < lrgstCf; ++next_watch_idx) {
      if (const Lit l = lit(next_watch_idx); !isFalse(level, l)) {
        if (next_watch_idx < unsaturatedIdx && position[toVar(l)] < p_pos) {
          assert(isTrue(level, l));
          blocking = l;
          w.blocking = l;
          watchslack += cf(w.idx);
          stats.NWATCHCHECKS += next_watch_idx;
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
    stats.NWATCHCHECKS += next_watch_idx;

    if (watchslack < lrgstCf) {
      next_watch_idx = 0;
      for (; next_watch_idx < std::min(unsaturatedIdx, start_watch_idx) && watchslack < lrgstCf; ++next_watch_idx) {
        if (const Lit l = lit(next_watch_idx); !isFalse(level, l)) {
          if (next_watch_idx < unsaturatedIdx && position[toVar(l)] < p_pos) {
            assert(isTrue(level, l));
            blocking = l;
            w.blocking = l;
            watchslack += cf(w.idx);
            stats.NWATCHCHECKS += next_watch_idx;
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
      stats.NWATCHCHECKS += next_watch_idx;
    }
    assert(watchslack >= lrgstCf || next_watch_idx == start_watch_idx);
  }

  assert(hasCorrectSlack(solver));
  assert(hasCorrectWatches(solver));

  if (watchslack >= lrgstCf) {
    flipWatch(w.idx);
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
      ++stats.NPROPWATCH;
      assert(isCorrectlyPropagating(solver, prop_idx));
      solver.propagate(l, w.cref);
    }  // NOTE: third innermost loop
  }
  stats.NPROPCHECKS += prop_idx;

  // NOTE: when skipping the watch calculation in subsequent propagation phases, it can happen that the constraint
  // became conflicting.
  return prop_idx >= size() && true_sum < degr ? WatchStatus::CONFLICTING : WatchStatus::KEEPWATCH;
}

template <typename CF, typename DG>
void WatchedSafe<CF, DG>::undoFalsified(uint32_t i) {
  assert(i < INF);
  assert(hasWatch(i));
  watchslack += cf(i);
}

template <typename CF, typename DG>
uint32_t WatchedSafe<CF, DG>::resolveWith(CeSuper& confl, const Lit l, Solver& solver, IntSet& actSet) const {
  return confl->resolveWith(lits, cfs, size(), degr, id(), getOrigin(), l, solver.getLevel(), solver.getPos(), actSet);
}
template <typename CF, typename DG>
uint32_t WatchedSafe<CF, DG>::subsumeWith(CeSuper& confl, const Lit l, Solver& solver, IntSet& saturatedLits) const {
  return confl->subsumeWith(lits, cfs, size(), degr, id(), l, solver.getLevel(), solver.getPos(), saturatedLits);
}

template <typename CF, typename DG>
CePtr<CF, DG> WatchedSafe<CF, DG>::expandTo(ConstrExpPools& cePools) const {
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
CeSuper WatchedSafe<CF, DG>::toExpanded(ConstrExpPools& cePools) const {
  return expandTo(cePools);
}

template <typename CF, typename DG>
bool WatchedSafe<CF, DG>::isSatisfiedAtRoot(const IntMap<int>& level) const {
  DG eval = -degr;
  for (uint32_t i = 0; i < size() && eval < 0; ++i) {
    if (isUnit(level, lit(i))) eval += cf(i);
  }
  return eval >= 0;
}

template <typename CF, typename DG>
bool WatchedSafe<CF, DG>::canBeSimplified(const IntMap<int>& level, Equalities& equalities, Implications& implications,
                                          IntSetPool& isp) const {
  const bool isEquality = getOrigin() == Origin::EQUALITY;
  for (uint32_t i = 0; i < size(); ++i) {
    if (const Lit l = lit(i); isUnit(level, l) || isUnit(level, -l) || (!isEquality && !equalities.isCanonical(l)))
      return true;
  }
  if (!isEquality) {
    IntSet& hasImplieds = isp.take();
    for (uint32_t i = 0; i < getUnsaturatedIdx(); ++i) {
      if (const Lit l = lit(i); implications.hasImplieds(l)) hasImplieds.add(-l);
    }
    if (!hasImplieds.isEmpty()) {
      for (uint32_t i = 0; i < getUnsaturatedIdx(); ++i) {
        if (hasImplieds.has(lit(i))) {
          isp.release(hasImplieds);
          return true;
        }
      }
    }
    isp.release(hasImplieds);
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

template <typename CF, typename DG>
bool WatchedSafe<CF, DG>::hasCorrectSlack(const Solver& solver) {
  return true;  // comment to run check
  DG slk = -degr;
  for (int i = 0; i < (int)size(); ++i) {
    if (hasWatch(i) && (solver.getPos()[toVar(lit(i))] >= solver.qhead || !isFalse(solver.getLevel(), lit(i))))
      slk += cf(i);
  }
  return (slk == watchslack);
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

template <typename CF, typename DG>
bool WatchedSafe<CF, DG>::hasCorrectWatches(const Solver& solver) {
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

template struct Watched<int, long long>;

template struct WatchedSafe<long long, int128>;
template struct WatchedSafe<int128, int128>;
template struct WatchedSafe<int128, int256>;
template struct WatchedSafe<bigint, bigint>;

}  // namespace xct
