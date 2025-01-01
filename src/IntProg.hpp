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

#pragma once

#include <string>
#include "Global.hpp"
#include "Optimization.hpp"
#include "Solver.hpp"
#include "datastructures/IntSet.hpp"
#include "typedefs.hpp"

namespace xct {
enum class Encoding { ORDER, LOG, ONEHOT };
Encoding opt2enc(const std::string& opt);

struct IntVar {
  explicit IntVar(const std::string& n, Solver& solver, bool nameAsId, const bigint& lb, const bigint& ub, Encoding e);

  [[nodiscard]] const std::string& getName() const { return name; }
  [[nodiscard]] const bigint& getUpperBound() const { return upperBound; }
  [[nodiscard]] const bigint& getLowerBound() const { return lowerBound; }

  [[nodiscard]] bigint getRange() const { return upperBound - lowerBound; }  // TODO: Boolean range is 1?
  [[nodiscard]] bool isBoolean() const { return lowerBound == 0 && upperBound == 1; }

  [[nodiscard]] Encoding getEncoding() const { return encoding; }
  [[nodiscard]] const VarVec& getEncodingVars() const { return encodingVars; }
  [[nodiscard]] bigint getValue(const LitVec& sol) const;

 private:
  const std::string name;
  const bigint lowerBound;
  const bigint upperBound;

  const Encoding encoding;
  VarVec encodingVars;
};
std::ostream& operator<<(std::ostream& o, const IntVar& x);
std::ostream& operator<<(std::ostream& o, IntVar* x);

struct IntTerm {
  bigint c;
  IntVar* v;
  // TODO constructors needed because Apple clang does not support parenthesized initialization of aggregates
  IntTerm(const bigint& _c, IntVar* _v);
  IntTerm() = default;
  IntTerm(IntTerm&&) = default;
  IntTerm& operator=(IntTerm&&) = default;
  IntTerm(const IntTerm&) = default;
  IntTerm& operator=(const IntTerm&) = default;
};
std::ostream& operator<<(std::ostream& o, const IntTerm& x);
using IntTermVec = std::vector<IntTerm>;
}  // namespace xct

template <>
struct std::hash<xct::IntVar*> {
  size_t operator()(xct::IntVar* iv) const noexcept;
};

template <>
struct std::hash<xct::IntTerm> {
  size_t operator()(const xct::IntTerm& it) const noexcept;
};

template <>
struct std::hash<xct::IntTermVec> {
  size_t operator()(const xct::IntTermVec& itv) const noexcept;
};

namespace xct {

using Core = std::unique_ptr<unordered_set<IntVar*>>;
Core emptyCore();
// NOTE: Core is a unique pointer because it is eagerly calculated and ownership is transferred to caller

class IntProg;

struct IntConstraint {
  IntTermVec lhs = {};
  std::optional<bigint> lowerBound = 0;
  std::optional<bigint> upperBound = std::nullopt;

  static IntTermVec zip(const std::vector<bigint>& coefs, const std::vector<IntVar*>& vars);

  [[nodiscard]] bigint getRange() const;
  [[nodiscard]] int64_t size() const;
  void invert();

  void toConstrExp(CeArb&, bool useLowerBound) const;
};
std::ostream& operator<<(std::ostream& o, const IntConstraint& x);

struct OptRes {
  SolveState state;
  bigint optval;
  Core core;
};

template <typename T>
struct WithState {
  SolveState state;
  T val;
};

struct TimeOut {
  bool reinitialize;
  double limit;
};

struct ReifInfo {
  IntVar* head = nullptr;
  bool sign = false;
  bool left = false;
  IntConstraint body;
};

class IntProg {
 public:
  Global global;
  bigint obj_denominator;  // denominator for rational objectives arising from LP/MPS files

 private:
  Solver solver;
  Optim optim;

  std::vector<std::unique_ptr<IntVar>> vars;
  IntConstraint obj;  // NOTE: we could erase this, but then we would not store the untransformed input objective
  bool minimize = true;
  unordered_map<std::string, IntVar*> name2var;
  unordered_map<Var, IntVar*> var2var;

  int inputVarLimit = INF;
  int64_t nConstrs = 0;

  IntSet assumptions;
  unordered_map<VarVec, Var, aux::IntVecHash> multAuxs;

  // only for printing purposes:
  const bool keepInput;
  std::vector<IntConstraint> constraints;
  std::vector<ReifInfo> reifications;
  // value Lit implies lower bound or upper bound on key
  unordered_map<IntVar*, std::multimap<bigint, Lit>> reifs;
  unordered_map<IntVar*, std::multimap<bigint, Lit>> right_reifs;
  unordered_map<IntVar*, std::multimap<bigint, Lit>> left_reifs;

  std::vector<std::vector<IntVar*>> multiplications;  // last two are bounds

  IntVar* addFlag();
  Var fixObjective(const IntConstraint& ico, const bigint& opt);
  void addSingleAssumption(IntVar* iv, const bigint& val);

 public:
  explicit IntProg(const Options& opts, bool keepIn = false);

  const Solver& getSolver() const;
  Solver& getSolver();
  const Optim& getOptim() const;
  void setInputVarLimit();
  int getInputVarLimit() const;

  IntVar* addVar(const std::string& name, const bigint& lowerbound = 0, const bigint& upperbound = 1,
                 Encoding encoding = Encoding::LOG, bool nameAsId = false);
  IntVar* getVarFor(const std::string& name) const;  // returns nullptr if it does not exist
  std::vector<IntVar*> getVariables() const;

  void setObjective(const IntTermVec& terms, bool min = true, const bigint& offset = 0);
  IntConstraint& getObjective();
  const IntConstraint& getObjective() const;

  void setAssumptions(const std::vector<std::pair<IntVar*, std::vector<bigint>>>& ivs);
  void setAssumptions(const std::vector<std::pair<IntVar*, bigint>>& ivs);
  void clearAssumptions();
  void clearAssumptions(const std::vector<IntVar*>& ivs);
  bool hasAssumption(IntVar* iv) const;
  std::vector<bigint> getAssumption(IntVar* iv) const;

  void setSolutionHints(const std::vector<std::pair<IntVar*, bigint>>& hints);
  void clearSolutionHints(const std::vector<IntVar*>& ivs);

  void addConstraint(const IntConstraint& ic);
  void addReification(IntVar* head, bool sign, const IntConstraint& ic);
  void addRightReification(IntVar* head, bool sign, const IntConstraint& ic);
  void addLeftReification(IntVar* head, bool sign, const IntConstraint& ic);
  void addMultiplication(const std::vector<IntVar*>& factors, IntVar* lower_bound = nullptr,
                         IntVar* upper_bound = nullptr);

  void addImplsRightReif(Lit head, IntVar* lhs, const bigint& lb);
  void addImplsLeftReif(Lit head, IntVar* lhs, const bigint& lb);

  void fix(IntVar* iv, const bigint& val);
  void invalidateLastSol();
  void invalidateLastSol(const std::vector<IntVar*>& ivs, Var flag = 0);

  bigint getLowerBound() const;
  bigint getUpperBound() const;
  ratio getUpperBoundRatio() const;

  bool hasLastSolution() const;
  bigint getLastSolutionFor(IntVar* iv) const;
  std::vector<bigint> getLastSolutionFor(const std::vector<IntVar*>& vars) const;

  Core getLastCore();

  void printOrigSol() const;
  void printFormula();
  std::ostream& printFormula(std::ostream& out);
  std::ostream& printInput(std::ostream& out) const;
  std::ostream& printVars(std::ostream& out) const;
  long long getNbVars() const;
  long long getNbConstraints() const;

  OptRes toOptimum(IntConstraint& objective, bool keepstate, const TimeOut& to = {false, 0});
  WithState<Ce32> getSolIntersection(const std::vector<IntVar*>& ivs, bool keepstate, const TimeOut& to = {false, 0});
  WithState<std::vector<std::pair<bigint, bigint>>> propagate(const std::vector<IntVar*>& ivs, bool keepstate,
                                                              const TimeOut& to = {false, 0});
  WithState<std::vector<std::vector<bigint>>> pruneDomains(const std::vector<IntVar*>& ivs, bool keepstate,
                                                           const TimeOut& to = {false, 0});
  WithState<int64_t> count(const std::vector<IntVar*>& ivs, bool keepstate, const TimeOut& to = {false, 0});
  WithState<std::vector<unordered_map<bigint, int64_t>>> count(const std::vector<IntVar*>& ivs_base,
                                                               const std::vector<IntVar*>& ivs_counts, bool keepstate,
                                                               const TimeOut& to = {false, 0});
  WithState<Core> extractMUS(const TimeOut& to = {false, 0});

  void runFromCmdLine();
};
std::ostream& operator<<(std::ostream& o, const IntProg& x);

}  // namespace xct