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

#pragma once

#include "../typedefs.hpp"
#include "ConstrExp.hpp"

namespace xct {

enum class WatchStatus { DROPWATCH, KEEPWATCH, CONFLICTING };

class Solver;
class Equalities;
struct Stats;

struct Constr {  // internal solver constraint optimized for fast propagation
  virtual size_t getMemSize() const = 0;

  struct {
    unsigned seen : 1;  // utility bit to avoid hash maps
    unsigned markedfordel : 1;
    unsigned locked : 1;
    const unsigned origin : 5;
    const unsigned long long id : 56;  // plenty of bits to store ID
  } header;
  float priority;  // Integer part is LBD (0 to 1e3), fractional part is 1-strength. Lower is better.
  const uint32_t sze;

  Constr(ID i, Origin o, bool lkd, uint32_t lngth, float strngth, uint32_t maxLBD);
  virtual ~Constr() {}
  virtual void cleanup() = 0;  // poor man's destructor

  uint32_t size() const;
  void setLocked(bool lkd);
  bool isLocked() const;
  Origin getOrigin() const;
  void decreaseLBD(uint32_t lbd);
  void decayLBD(uint32_t decay, uint32_t maxLBD);
  uint32_t lbd() const;
  float strength() const;
  bool isMarkedForDelete() const;
  bool isSeen() const;
  void setSeen(bool s);
  ID id() const;
  void fixEncountered(Stats& stats) const;

  // TODO: remove direct uses of these bigint methods, convert to ConstrExp instead
  // NOTE: useful for debugging though!
  virtual bigint degree() const = 0;
  virtual bigint coef(uint32_t i) const = 0;
  virtual Lit lit(uint32_t i) const = 0;
  virtual uint32_t getUnsaturatedIdx() const = 0;
  virtual bool hasWatch(uint32_t i) const = 0;
  virtual bool isClauseOrCard() const = 0;
  virtual bool isAtMostOne() const = 0;

  virtual void initializeWatches(CRef cr, Solver& solver) = 0;
  virtual WatchStatus checkForPropagation(Watch& w, Lit p, Solver& slvr, Stats& stats) = 0;
  virtual void undoFalsified(uint32_t i) = 0;
  virtual uint32_t resolveWith(CeSuper& confl, Lit l, Solver& solver, IntSet& actSet) const = 0;
  virtual uint32_t subsumeWith(CeSuper& confl, Lit l, Solver& solver, IntSet& saturatedLits) const = 0;

  virtual CeSuper toExpanded(ConstrExpPools& cePools) const = 0;
  virtual bool isSatisfiedAtRoot(const IntMap<int>& level) const = 0;
  virtual bool canBeSimplified(const IntMap<int>& level, Equalities& equalities, Implications& implications,
                               IntSetPool& isp) const = 0;

  void print(const Solver& solver) const;

  bool isCorrectlyConflicting(const Solver& solver) const;
  bool isCorrectlyPropagating(const Solver& solver, int idx) const;
};
std::ostream& operator<<(std::ostream& o, const Constr& c);

struct Binary final : Constr {
  const std::array<Lit, 2> data;

  static size_t getMemSize(uint32_t length);
  size_t getMemSize() const;

  bigint degree() const;
  bigint coef(uint32_t) const;
  Lit lit(uint32_t i) const;
  bool hasWatch(uint32_t i) const;
  uint32_t getUnsaturatedIdx() const;
  bool isClauseOrCard() const;
  bool isAtMostOne() const;

  template <typename SMALL, typename LARGE>
  Binary(const ConstrExp<SMALL, LARGE>* constraint, bool locked, ID _id)
      : Constr(_id, constraint->orig, locked, 2, 0.5f, constraint->global.options.dbMaxLBD.get()),
        data({constraint->getLit(constraint->getVars()[0]), constraint->getLit(constraint->getVars()[1])}) {
    assert(_id > ID_Trivial);
    assert(constraint->nVars() == 2);
    assert(constraint->getDegree() == 1);

    for (uint32_t i = 0; i < 2; ++i) {
      assert(constraint->getLit(constraint->getVars()[i]) != 0);
    }
  }

  void cleanup() {}

  void initializeWatches(CRef cr, Solver& solver);
  WatchStatus checkForPropagation(Watch& w, Lit p, Solver& solver, Stats& stats);
  void undoFalsified([[maybe_unused]] uint32_t i) { assert(false); }
  uint32_t resolveWith(CeSuper& confl, Lit l, Solver& solver, IntSet& actSet) const;
  uint32_t subsumeWith(CeSuper& confl, Lit l, Solver& solver, IntSet& saturatedLits) const;

  CeSuper toExpanded(ConstrExpPools& cePools) const;
  bool isSatisfiedAtRoot(const IntMap<int>& level) const;
  bool canBeSimplified(const IntMap<int>& level, Equalities& equalities, Implications& implications,
                       IntSetPool& isp) const;
};

struct Clause final : Constr {
  uint32_t next_watch_idx;
  Lit data[];  // Flexible Array Member

  static size_t getMemSize(uint32_t length);
  size_t getMemSize() const;

  bigint degree() const;
  bigint coef(uint32_t) const;
  Lit lit(uint32_t i) const;
  bool hasWatch(uint32_t i) const;
  uint32_t getUnsaturatedIdx() const;
  bool isClauseOrCard() const;
  bool isAtMostOne() const;

  template <typename SMALL, typename LARGE>
  Clause(const ConstrExp<SMALL, LARGE>* constraint, bool locked, ID _id)
      : Constr(_id, constraint->orig, locked, constraint->nVars(), 1.0 / static_cast<double>(constraint->nVars()),
               constraint->global.options.dbMaxLBD.get()),
        next_watch_idx(sze) {
    assert(_id > ID_Trivial);
    assert(constraint->nVars() < INF);
    assert(constraint->getDegree() == 1);

    for (uint32_t i = 0; i < size(); ++i) {
      Var v = constraint->getVars()[i];
      assert(constraint->getLit(v) != 0);
      data[i] = constraint->getLit(v);
    }
  }

  void cleanup() {}

  void initializeWatches(CRef cr, Solver& solver);
  WatchStatus checkForPropagation(Watch& w, Lit p, Solver& solver, Stats& stats);
  void undoFalsified([[maybe_unused]] uint32_t i) { assert(false); }
  uint32_t resolveWith(CeSuper& confl, Lit l, Solver& solver, IntSet& actSet) const;
  uint32_t subsumeWith(CeSuper& confl, Lit l, Solver& solver, IntSet& saturatedLits) const;

  CeSuper toExpanded(ConstrExpPools& cePools) const;
  bool isSatisfiedAtRoot(const IntMap<int>& level) const;
  bool canBeSimplified(const IntMap<int>& level, Equalities& equalities, Implications& implications,
                       IntSetPool& isp) const;
};

struct Cardinality final : Constr {
  const uint32_t degr;
  uint32_t next_watch_idx;
  Lit data[];  // Flexible Array Member

  static size_t getMemSize(uint32_t length);
  size_t getMemSize() const;

  bigint degree() const;
  bigint coef(uint32_t) const;
  Lit lit(uint32_t i) const;
  bool hasWatch(uint32_t i) const;
  uint32_t getUnsaturatedIdx() const;
  bool isClauseOrCard() const;
  bool isAtMostOne() const;

  template <typename SMALL, typename LARGE>
  Cardinality(const ConstrExp<SMALL, LARGE>* constraint, bool locked, ID _id)
      : Constr(_id, constraint->orig, locked, constraint->nVars(), constraint->getStrength(),
               constraint->global.options.dbMaxLBD.get()),
        degr(static_cast<uint32_t>(constraint->getDegree())),
        next_watch_idx(constraint->nVars()) {
    assert(degr > 1);  // otherwise should be a clause
    assert(_id > ID_Trivial);
    assert(constraint->nVars() < INF);
    assert(aux::abs(constraint->coefs[constraint->getVars()[0]]) == 1);
    assert(constraint->getDegree() <= (LARGE)constraint->nVars());

    for (uint32_t i = 0; i < size(); ++i) {
      Var v = constraint->getVars()[i];
      assert(constraint->getLit(v) != 0);
      data[i] = constraint->getLit(v);
    }
  }

  void cleanup() {}

  void initializeWatches(CRef cr, Solver& solver);
  WatchStatus checkForPropagation(Watch& w, Lit p, Solver& solver, Stats& stats);
  void undoFalsified([[maybe_unused]] uint32_t i) { assert(false); }
  uint32_t resolveWith(CeSuper& confl, Lit l, Solver& solver, IntSet& actSet) const;
  uint32_t subsumeWith(CeSuper& confl, Lit l, Solver& solver, IntSet& saturatedLits) const;

  CeSuper toExpanded(ConstrExpPools& cePools) const;
  bool isSatisfiedAtRoot(const IntMap<int>& level) const;
  bool canBeSimplified(const IntMap<int>& level, Equalities& equalities, Implications& implications,
                       IntSetPool& isp) const;
};

struct Watched32 final : Constr {
  uint32_t next_watch_idx;
  uint32_t unsaturatedIdx;
  const int64_t degr;
  int64_t watchslack;
  Lit blocking;
  Lit data[0];  // Flexible Array Member - gcc complains about destruction when using the proper syntax '[]'
  // WARNING: Watched only works for int coefficients for now (they take up the same bytes as Lit)
  // use Watched for other coefficient types

  static size_t getMemSize(uint32_t length);
  size_t getMemSize() const;

  bigint degree() const;
  const int32_t& cf(uint32_t i) const;
  bigint coef(uint32_t i) const;
  Lit lit(uint32_t i) const;
  uint32_t getUnsaturatedIdx() const;
  bool isClauseOrCard() const;
  bool isAtMostOne() const;

  template <typename SMALL, typename LARGE>
  Watched32(const ConstrExp<SMALL, LARGE>* constraint, bool locked, ID _id, double strngth)
      : Constr(_id, constraint->orig, locked, constraint->nVars(), strngth, constraint->global.options.dbMaxLBD.get()),
        next_watch_idx(sze),
        unsaturatedIdx(0),
        degr(static_cast<int64_t>(constraint->getDegree())),
        watchslack(0),
        blocking(0) {
    assert(_id > ID_Trivial);
    assert(fitsIn<int64_t>(constraint->getDegree()));
    assert(fitsIn<int32_t>(constraint->getLargestCoef()));
    assert(strngth == constraint->getStrength());

    for (uint32_t i = 0; i < size(); ++i) {
      Var v = constraint->getVars()[i];
      assert(constraint->getLit(v) != 0);
      data[i] = constraint->getLit(v) << 1;
      data[i + size()] = static_cast<int32_t>(aux::abs(constraint->coefs[v]));
      unsaturatedIdx += cf(i) >= degr;
      assert(cf(i) <= degr);
    }
  }

  void cleanup() {}

  bool hasWatch(uint32_t) const;
  void flipWatch(uint32_t);

  void initializeWatches(CRef cr, Solver& solver);
  WatchStatus checkForPropagation(Watch& w, [[maybe_unused]] Lit p, Solver& solver, Stats& stats);
  void undoFalsified(uint32_t i);
  uint32_t resolveWith(CeSuper& confl, Lit l, Solver& solver, IntSet& actSet) const;
  uint32_t subsumeWith(CeSuper& confl, Lit l, Solver& solver, IntSet& saturatedLits) const;

  Ce32 expandTo(ConstrExpPools& cePools) const;
  CeSuper toExpanded(ConstrExpPools& cePools) const;
  bool isSatisfiedAtRoot(const IntMap<int>& level) const;
  bool canBeSimplified(const IntMap<int>& level, Equalities& equalities, Implications& implications,
                       IntSetPool& isp) const;

  bool hasCorrectSlack(const Solver& solver);
  bool hasCorrectWatches(const Solver& solver);
};

template <typename CF, typename DG>
struct Watched final : Constr {
  uint32_t next_watch_idx;
  uint32_t unsaturatedIdx;
  const DG degr;
  DG watchslack;
  CF* cfs;
  Lit blocking;
  Lit lits[0];

  static size_t getMemSize(uint32_t length);
  size_t getMemSize() const;

  bigint degree() const;
  const CF& cf(uint32_t i) const;
  bigint coef(uint32_t i) const;
  Lit lit(uint32_t i) const;
  uint32_t getUnsaturatedIdx() const;
  bool isClauseOrCard() const;
  bool isAtMostOne() const;

  template <typename SMALL, typename LARGE>
  Watched(const ConstrExp<SMALL, LARGE>* constraint, bool locked, ID _id, double strngth)
      : Constr(_id, constraint->orig, locked, constraint->nVars(), strngth, constraint->global.options.dbMaxLBD.get()),
        next_watch_idx(sze),
        unsaturatedIdx(0),
        degr(static_cast<DG>(constraint->getDegree())),
        watchslack(0),
        cfs(new CF[sze]),
        blocking(0) {
    assert(_id > ID_Trivial);
    assert(fitsIn<DG>(constraint->getDegree()));
    assert(fitsIn<CF>(constraint->getLargestCoef()));
    assert(strngth == constraint->getStrength());

    for (uint32_t i = 0; i < size(); ++i) {
      Var v = constraint->getVars()[i];
      assert(constraint->getLit(v) != 0);
      cfs[i] = static_cast<CF>(aux::abs(constraint->coefs[v]));
      lits[i] = constraint->getLit(v) << 1;
      unsaturatedIdx += cf(i) >= degr;
      assert(cf(i) <= degr);
    }
  }

  void cleanup() { delete[] cfs; }

  bool hasWatch(uint32_t) const;
  void flipWatch(uint32_t);

  void initializeWatches(CRef cr, Solver& solver);
  WatchStatus checkForPropagation(Watch& w, [[maybe_unused]] Lit p, Solver& solver, Stats& stats);
  void undoFalsified(uint32_t i);
  uint32_t resolveWith(CeSuper& confl, Lit l, Solver& solver, IntSet& actSet) const;
  uint32_t subsumeWith(CeSuper& confl, Lit l, Solver& solver, IntSet& saturatedLits) const;

  CePtr<CF, DG> expandTo(ConstrExpPools& cePools) const;
  CeSuper toExpanded(ConstrExpPools& cePools) const;
  bool isSatisfiedAtRoot(const IntMap<int>& level) const;
  bool canBeSimplified(const IntMap<int>& level, Equalities& equalities, Implications& implications,
                       IntSetPool& isp) const;

  bool hasCorrectSlack(const Solver& solver);
  bool hasCorrectWatches(const Solver& solver);
};

}  // namespace xct
