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

#include "../auxiliary.hpp"
#include "../constraints/ConstrExp.hpp"
#include "../constraints/ConstrSimple.hpp"
#include "../typedefs.hpp"

#if WITHSOPLEX

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#include "soplex.h"
#pragma GCC diagnostic pop

#endif  // WITHSOPLEX

namespace xct {

enum class LpStatus { INFEASIBLE, OPTIMAL, PIVOTLIMIT, UNDETERMINED };

struct RowData {
  ID id;
  bool removable;
  RowData() = default;
  RowData(ID i, bool r) : id(i), removable(r){};
};

struct AdditionData {
  ConstrSimple64 cs;
  bool removable = false;
};

struct BoundData {
  ID id = ID_Trivial;
  ConstrSimple64 cs;
};

#if WITHSOPLEX

class LpSolver;
struct CandidateCut {
  ConstrSimple64 simpcons;
  CRef cr = CRef_Undef;
  double norm = 1;
  double ratSlack = 0;

  CandidateCut() = default;
  CandidateCut(const CeSuper& in, const std::vector<double>& sol);
  CandidateCut(const Constr& in, CRef cr, const std::vector<double>& sol, ConstrExpPools& pools);
  [[nodiscard]] double cosOfAngleTo(const CandidateCut& other) const;

 private:
  void initialize(const std::vector<double>& sol);
};
std::ostream& operator<<(std::ostream& o, const CandidateCut& cc);

class Solver;

class LpSolver {
  friend class Solver;
  friend struct CandidateCut;

  soplex::SoPlex lp;
  Solver& solver;
  Global& global;  // TODO: needed? We already have solver?

  double lpPivotMult = 1;
  constexpr static double INFTY = 1e100;
  constexpr static double maxMult = 1e22;  // fits in 74 bits
  // we use 128 bit coefficients to calculate dual constraints.
  // 1 sign bit
  // 52 bits for the coefficients
  // x bits for the number of non-zero multiplied constraints
  // 74-x bits for the max dual multiplier
  // +++ 127 bits total, 1 to preempt off-by-one errors
  bool madeInternalCall = false;
  bool canInProcess() const;

  soplex::DVectorReal lpSol;
  std::vector<double> lpSolution;
  soplex::DVectorReal lpSlackSolution;
  soplex::DVectorReal lpMultipliers;
  soplex::DVectorReal upperBounds;
  soplex::DVectorReal lowerBounds;
  soplex::DSVectorReal lpRow;

  std::vector<RowData> row2data;
  std::vector<int> toRemove;  // rows
  unordered_map<ID, AdditionData> toAdd;
  BoundData boundsToAdd[2];  // [0] is upper bound, [1] lower bound

  std::vector<CandidateCut> candidateCuts;

 public:
  explicit LpSolver(Solver& s);
  void setNbVariables(int n);
  void setObjective(const CeArb& o);

  [[nodiscard]] std::pair<LpStatus, CeSuper> checkFeasibility(
      bool inProcessing);  // TODO: don't use objective function here?
  [[nodiscard]] CeSuper inProcess(bool overrideHeur = false);

  void addConstraint(const CeSuper& c, bool removable, bool upperbound = false, bool lowerbound = false);
  void addConstraint(CRef cr, bool removable, bool upperbound = false, bool lowerbound = false);

 private:
  int getNbCols() const;
  int getNbRows() const;

  void flushConstraints();

  void convertConstraint(const ConstrSimple64& c, soplex::DSVectorReal& row, double& rhs);
  void resetBasis();
  CeSuper createLinearCombinationFarkas(soplex::DVectorReal& mults);
  CandidateCut createLinearCombinationGomory(soplex::DVectorReal& mults);
  static double getScaleFactor(soplex::DVectorReal& mults, bool removeNegatives);
  Ce64 rowToConstraint(int row);
  void constructGomoryCandidates();
  void constructLearnedCandidates();
  void addFilteredCuts();
  void pruneCuts();

  inline static double nonIntegrality(double a) { return aux::abs(std::round(a) - a); }
  inline static bool validVal(double a) { return std::round(a) == a && aux::abs(a) < INFLPINT; }
  // NOTE: double type can only store ranges of integers up to ~9e15
};

#else

class Solver;
struct Global;

class LpSolver {
 public:
  LpSolver([[maybe_unused]] Solver& s){};
  void setNbVariables([[maybe_unused]] int n) {};
  void setObjective([[maybe_unused]] const CeArb& o) {};

  std::pair<LpStatus, CeSuper> checkFeasibility([[maybe_unused]] bool inProcessing) {
    assert(false);
    return {LpStatus::UNDETERMINED, CeNull()};
  }
  CeSuper inProcess([[maybe_unused]] bool overrideHeur) { return CeNull(); }
  bool canInProcess() { return false; }

  void addConstraint([[maybe_unused]] const CeSuper& c, [[maybe_unused]] bool removable,
                     [[maybe_unused]] bool upperbound, [[maybe_unused]] bool lowerbound) {}
  void addConstraint([[maybe_unused]] CRef cr, [[maybe_unused]] bool removable, [[maybe_unused]] bool upperbound,
                     [[maybe_unused]] bool lowerbound) {}
};

#endif  // WITHSOPLEX

}  // namespace xct
