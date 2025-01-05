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

#include "constraints/ConstrSimple.hpp"
#include "datastructures/IntSet.hpp"
#include "typedefs.hpp"

namespace xct {

struct Global;
class Solver;

template <typename SMALL, typename LARGE>
struct LazyVar {
  Solver& solver;
  int coveredVars;
  int upperBound;
  Var currentVar;
  ID atLeastID = ID_Undef;
  ID atMostID = ID_Undef;
  ConstrSimple32 atLeast;  // X >= k + y1 + ... + yi
  ConstrSimple32 atMost;   // k + y1 + ... + yi-1 + (1+n-k-i)yi >= X

  SMALL mult;

  LazyVar(Solver& slvr, const Ce32& cardCore, Var startVar, const SMALL& m, const LARGE& upperBnd);
  ~LazyVar();

  void addVar(Var v);
  void addAtLeastConstraint();
  void addAtMostConstraint();
  void addSymBreakingConstraint(Var prevvar) const;
  void addFinalAtMost();
  [[nodiscard]] int remainingVars() const;
  void setUpperBound(const LARGE& normalizedUpperBound);
};

template <typename SMALL, typename LARGE>
std::ostream& operator<<(std::ostream& o, const LazyVar<SMALL, LARGE>& lv) {
  o << lv.atLeast << "\n" << lv.atMost;
  return o;
}

struct IntConstraint;
class OptimizationSuper;
using Optim = std::shared_ptr<OptimizationSuper>;

class OptimizationSuper {
 protected:
  Solver& solver;
  Global& global;
  const bigint offset;
  const IntSet& assumptions;  // external assumptions
  LitVec assumps;             // all assumptions passed to solver

 public:
  virtual bigint getUpperBound() const = 0;
  virtual bigint getLowerBound() const = 0;
  virtual CeSuper getOrigObj() const = 0;

  static Optim make(const IntConstraint& obj, Solver& solver, const IntSet& assumps);

  [[nodiscard]] virtual SolveState run(bool optimize, double timeout) = 0;
  [[nodiscard]] virtual SolveState runFull(bool optimize, double timeout) = 0;

  virtual void boundObjByLastSol() = 0;

  OptimizationSuper(Solver& s, const bigint& offs, const IntSet& assumps);
  virtual ~OptimizationSuper() = default;
};

template <typename SMALL, typename LARGE>
class Optimization final : public OptimizationSuper {
 public:
  const CePtr<SMALL, LARGE> origObj;

 private:
  CePtr<SMALL, LARGE> reformObj;

  LARGE lower_bound;
  LARGE upper_bound;
  ID lastUpperBound;
  ID lastLowerBound;
  ID lastReformUpperBound;

  std::vector<std::unique_ptr<LazyVar<SMALL, LARGE>>> lazyVars;

  LARGE boundingVal;
  Var boundingVar;

 public:
  explicit Optimization(const CePtr<SMALL, LARGE>& obj, Solver& s, const bigint& offset, const IntSet& assumps);
  ~Optimization();

  bigint getUpperBound() const;
  bigint getLowerBound() const;
  CeSuper getOrigObj() const;

  void printObjBounds(bool upperImproved);
  void checkLazyVariables();
  void addLowerBound();
  void addReformUpperBound(bool deletePrevious);

  Ce32 reduceToCardinality(const CeSuper& core);                            // does not modify core
  State reformObjective(const CeSuper& core);                               // modifies core
  [[nodiscard]] Lit getKnapsackLit(const CePtr<SMALL, LARGE>& core) const;  // modifies core
  void handleInconsistency(const CeSuper& core);                            // modifies core
  void boundObjByLastSol();
  void boundBottomUp();

  [[nodiscard]] SolveState run(bool optimize, double timeout);
  [[nodiscard]] SolveState runFull(bool optimize, double timeout);
};

}  // namespace xct
