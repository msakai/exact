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

#include "Optimization.hpp"
#include "Global.hpp"
#include "Solver.hpp"
#include "constraints/ConstrExp.hpp"
#include "interface/IntConstraint.hpp"

namespace xct {

template <typename SMALL, typename LARGE>
LazyVar<SMALL, LARGE>::LazyVar(Solver& slvr, const Ce32& cardCore, Var startVar, const SMALL& m, const LARGE& upperBnd)
    : solver(slvr), coveredVars(cardCore->getDegree()), upperBound(cardCore->nVars()), mult(m) {
  setUpperBound(upperBnd);
  assert(remainingVars() > 0);
  cardCore->copyTo(atLeast);
  atLeast.orig = Origin::COREGUIDED;
  atLeast.toNormalFormLit();
  assert(atLeast.rhs == cardCore->getDegree());
  atMost.orig = Origin::COREGUIDED;
  atMost.rhs = -atLeast.rhs;
  atMost.terms.reserve(atLeast.size() + 1);
  for (auto& t : atLeast.terms) {
    atMost.terms.emplace_back(-t.c, t.l);
  }
  currentVar = startVar;
  atLeast.terms.emplace_back(-1, startVar);
  atMost.terms.emplace_back(remainingVars(), startVar);
  ++coveredVars;
}

template <typename SMALL, typename LARGE>
LazyVar<SMALL, LARGE>::~LazyVar() {
  solver.dropExternal(atLeastID, false, false);
  solver.dropExternal(atMostID, false, false);
}

template <typename SMALL, typename LARGE>
int LazyVar<SMALL, LARGE>::remainingVars() const {
  return upperBound - coveredVars;
}

template <typename SMALL, typename LARGE>
void LazyVar<SMALL, LARGE>::setUpperBound(const LARGE& normalizedUpperBound) {
  assert(normalizedUpperBound >= 0);
  assert(mult > 0);
  const LARGE tmp = normalizedUpperBound / mult;
  if (tmp < upperBound) upperBound = static_cast<int>(tmp);
}

template <typename SMALL, typename LARGE>
void LazyVar<SMALL, LARGE>::addVar(Var v) {
  currentVar = v;
  atLeast.terms.emplace_back(-1, v);
  Term32& last = atMost.terms.back();
  last = {1, last.l};
  atMost.terms.emplace_back(remainingVars(), v);
  ++coveredVars;
}

template <typename SMALL, typename LARGE>
void LazyVar<SMALL, LARGE>::addAtLeastConstraint() {
  assert(atLeast.terms.back().l == currentVar);
  solver.dropExternal(atLeastID, true, false);  // TODO: should old constraints be force deleted?
  solver.addConstraint(atLeast);
}

template <typename SMALL, typename LARGE>
void LazyVar<SMALL, LARGE>::addAtMostConstraint() {
  assert(atMost.terms.back().l == currentVar);
  solver.dropExternal(atMostID, true, false);
  solver.addConstraint(atMost);
}

template <typename SMALL, typename LARGE>
void LazyVar<SMALL, LARGE>::addSymBreakingConstraint(Var prevvar) const {
  assert(prevvar < currentVar);
  // y-- + ~y >= 1 (equivalent to y-- >= y)
  solver.addBinaryConstraint(prevvar, -currentVar, Origin::COREGUIDED);
}

template <typename SMALL, typename LARGE>
void LazyVar<SMALL, LARGE>::addFinalAtMost() {
  solver.dropExternal(atMostID, true, false);
  Term32& last = atMost.terms.back();
  last = {1, last.l};
  solver.addConstraint(atMost);
}

OptimizationSuper::OptimizationSuper(Solver& s, const bigint& os, const IntSet& assumps)
    : solver(s), global(s.global), offset(os), assumptions(assumps) {}

Optim OptimizationSuper::make(const IntConstraint& ico, Solver& solver, const IntSet& assumps) {
  CeArb obj = solver.global.cePools.takeArb();
  ico.toConstrExp(obj, true);
  obj->removeEqualities(solver.getEqualities());
  obj->removeUnitsAndZeroes(solver.getLevel(), solver.getPos());
  bigint offs = -obj->getDegree();
  obj->addRhs(offs);
  assert(obj->getDegree() == 0);
  solver.setObjective(obj);

  bigint maxVal = obj->absCoeffSum();
  // The argument that maxVal is a safe upper bound is that we may *increase* the coefficient of assumption literals
  // during core-based reformulation, but never higher than the original coefficient sum.
  // E.g., ax+by+cz
  // assuming q to false may yield the sequence of cores q+x>=1, q+y>=1, q+z>=1.
  // these will lead to the following reformulation equalities: x=u+~q, y=v+~q, z=w+~q.
  // with final reformulation (a+b+c)~q + au + bv + cz.
  // Sum of coefficients is a sensible upper bound, since the assumption of the corresponding literal to true would
  // mean that the objective is at its maximal value.
  //
  // Since q is assumed to false (user assumption) and auxiliaries u, v, w are assumed to false (reformulated objective
  // literals), the only cores that will increase q at the expense of u, v, w are those where both q and u, v, w are
  // positive. However, since q and u, v, w have opposed polarity in the reformulation equalities, no such cores exist.
  // Hence, q's coefficient will not be reformulated beyond the sum of coefficients.
  // TODO: get a more rigorous proof from this argument?

  if (maxVal <= static_cast<bigint>(limitAbs<int, int64_t>())) {  // TODO: try to internalize this check in ConstrExp
    Ce32 o = solver.global.cePools.take32();
    obj->copyTo(o);
    return std::make_shared<Optimization<int, int64_t>>(o, solver, offs, assumps);
  }
  if (maxVal <= static_cast<bigint>(limitAbs<int64_t, int128>())) {
    Ce64 o = solver.global.cePools.take64();
    obj->copyTo(o);
    return std::make_shared<Optimization<int64_t, int128>>(o, solver, offs, assumps);
  }
  // TODO: below yielded a bug during coreguided search - not sure where. Multiplying two coefficients?
  //  if (maxVal <= static_cast<bigint>(limitAbs<int128, int128>())) {
  //    Ce96 o = solver.global.cePools.take96();
  //    obj->copyTo(o);
  //    return std::make_shared<Optimization<int128, int128>>(o, solver, offs, assumps);
  //  }
  if (maxVal <= static_cast<bigint>(limitAbs<int128, int256>())) {
    Ce128 o = solver.global.cePools.take128();
    obj->copyTo(o);
    return std::make_shared<Optimization<int128, int256>>(o, solver, offs, assumps);
  }
  CeArb o = solver.global.cePools.takeArb();
  obj->copyTo(o);
  return std::make_shared<Optimization<bigint, bigint>>(o, solver, offs, assumps);
}

template <typename SMALL, typename LARGE>
void simplifyAssumps(CePtr<SMALL, LARGE>& c, const IntSet& assumps) {
  for (const Lit l : assumps.getKeys()) {  // remove assumptions from objective
    const Var v = toVar(l);
    if (c->hasVar(v)) {
      if (c->hasLit(l)) c->addRhs(aux::abs(c->coefs[v]));
      c->coefs[v] = 0;
    }
  }
}

template <typename SMALL, typename LARGE>
Optimization<SMALL, LARGE>::Optimization(const CePtr<SMALL, LARGE>& obj, Solver& s, const bigint& os,
                                         const IntSet& assumps)
    : OptimizationSuper(s, os, assumps),
      origObj(obj),
      lower_bound(0),
      upper_bound(obj->absCoeffSum() + 1),
      lastUpperBound(ID_Undef),
      lastLowerBound(ID_Undef),
      lastReformUpperBound(ID_Undef),
      boundingVal(0),
      boundingVar(0) {
  assert(origObj->getDegree() == 0);
  global.logger.logObjective(origObj);
  if (global.options.optCoreguided) {
    reformObj = global.cePools.take<SMALL, LARGE>();
    origObj->copyTo(reformObj);
    reformObj->removeEqualities(solver.getEqualities());
    simplifyAssumps(reformObj, assumptions);
    reformObj->removeUnitsAndZeroes(solver.getLevel(), solver.getPos());

    lower_bound = -reformObj->getDegree();
  }
}

template <typename SMALL, typename LARGE>
Optimization<SMALL, LARGE>::~Optimization() {
  // NOTE: do not make the upper bound erasable, otherwise the solver is not guaranteed to remain optimal
  solver.dropExternal(lastUpperBound, false, false);
  solver.dropExternal(lastLowerBound, true, false);
  solver.dropExternal(lastReformUpperBound, true, false);
}

template <typename SMALL, typename LARGE>
bigint Optimization<SMALL, LARGE>::getUpperBound() const {
  return offset + upper_bound;
}
template <typename SMALL, typename LARGE>
bigint Optimization<SMALL, LARGE>::getLowerBound() const {
  return offset + lower_bound;
}
template <typename SMALL, typename LARGE>
CeSuper Optimization<SMALL, LARGE>::getOrigObj() const {
  return origObj;
}

template <typename SMALL, typename LARGE>
void Optimization<SMALL, LARGE>::printObjBounds(bool upperImproved) {
  if (!solver.objectiveIsSet()) return;
  if (!global.options.uniformOut && (global.options.fileFormat.is("opb") || global.options.fileFormat.is("wbo") ||
                                     global.options.fileFormat.is("wcnf"))) {
    if (upperImproved) std::cout << "o " << getUpperBound() << std::endl;
    return;
  }
  if (global.options.verbosity.get() == 0) return;
  std::cout << "c     bounds ";
  if (solver.foundSolution()) {
    std::cout << getUpperBound();
  } else {
    std::cout << "-";
  }
  std::cout << " >= " << getLowerBound() << " @ " << global.stats.getTime() << ", " << global.stats.NCONFL.z << "\n";
}

template <typename SMALL, typename LARGE>
void Optimization<SMALL, LARGE>::checkLazyVariables() {
  // TODO: take *upper* bound on reformed objective into account.
  // E.g.: objective x+y+z+w =< 3 for core x+y+z+w >= 1 means we can rewrite to x+y+z+w = 1+a+b instead of = 1+a+b+c
  for (int i = 0; i < (int)lazyVars.size(); ++i) {
    LazyVar<SMALL, LARGE>& lv = *lazyVars[i];
    if (reformObj->getLit(lv.currentVar) == 0) {
      lv.setUpperBound(upper_bound);
      if (lv.remainingVars() == 0 ||
          isUnit(solver.getLevel(), -lv.currentVar)) {  // binary constraints make all new auxiliary variables unit
        lv.addFinalAtMost();
        plf::single_reorderase(lazyVars, lazyVars.begin() + i);  // fully expanded, no need to keep in memory
        --i;
      } else {  // add auxiliary variable
        int newN = solver.addVar(false);
        Var oldvar = lv.currentVar;
        lv.addVar(newN);
        // reformulate the objective
        reformObj->addLhs(lv.mult, newN);
        // add necessary lazy constraints
        lv.addAtLeastConstraint();
        lv.addAtMostConstraint();
        lv.addSymBreakingConstraint(oldvar);
        if (lv.remainingVars() == 0) {
          plf::single_reorderase(lazyVars, lazyVars.begin() + i);  // fully expanded, no need to keep in memory
          --i;
        }
      }
    }
  }
}

template <typename SMALL, typename LARGE>
void Optimization<SMALL, LARGE>::addLowerBound() {
  CePtr<SMALL, LARGE> aux = global.cePools.take<SMALL, LARGE>();
  origObj->copyTo(aux);
  aux->orig = Origin::LOWERBOUND;
  aux->addRhs(lower_bound);
  assert(static_cast<bigint>(limitAbs<SMALL, LARGE>()) == 0 ||
         aux->getDegree() < static_cast<bigint>(limitAbs<SMALL, LARGE>()));
  for (Lit l : assumptions.getKeys()) {
    aux->addLhs(static_cast<SMALL>(aux->getDegree()), -l);  // bound only holds under assumptions
  }

  solver.dropExternal(lastLowerBound, true, true);

  if (global.options.liftDegreeSymbolic.get() > 0) {
    solver.setSymbBoundLower(lower_bound);
    aux->symbBound.mult_upper = 0;
    aux->symbBound.mult_lower = 1;
    aux->symbBound.offset = aux->getDegree() - lower_bound;
    assert(aux->symbBound.getDegree(solver.getSymbBoundUpper(), solver.getSymbBoundLower()) == aux->getDegree());
    assert(aux->symbBound.isValid());
  } else {
    assert(!aux->symbBound.isValid());
  }

  std::pair<ID, ID> res = solver.addConstraint(aux);
  lastLowerBound = res.second;
}

template <typename SMALL, typename LARGE>
void Optimization<SMALL, LARGE>::addReformUpperBound(bool deletePrevious) {
  if (!reformObj || reformObj->vars.empty()) return;
  CePtr<SMALL, LARGE> aux = global.cePools.take<SMALL, LARGE>();
  reformObj->copyTo(aux);
  aux->orig = Origin::REFORMBOUND;
  aux->invert();
  aux->addRhs(-upper_bound + 1);
  assert(aux->getDegree() < static_cast<bigint>(limitAbs<SMALL, LARGE>()));
  for (Lit l : assumptions.getKeys()) {
    aux->addLhs(static_cast<SMALL>(aux->getDegree()), -l);  // bound only holds under assumptions
  }
  solver.dropExternal(lastReformUpperBound, true, deletePrevious);
  std::pair<ID, ID> res = solver.addConstraint(aux);
  lastReformUpperBound = res.second;
}

template <typename SMALL, typename LARGE>
Ce32 Optimization<SMALL, LARGE>::reduceToCardinality(const CeSuper& core) {  // does not modify core
  assert(core->hasNoZeroes());
  assert(core->hasNoUnits(solver.getLevel()));
  if (core->isClause()) {
    Ce32 result = global.cePools.take32();
    core->copyTo(result);
    return result;
  }

  CeSuper card = core->clone(global.cePools);
  // sort in decreasing coef order to minimize number of auxiliary variables, but break ties so that *small*
  // objective coefficient literals are removed first, to maximize the chances of a strong lower bound.
  card->sortInDecreasingCoefOrder(
      [&](Var v1, Var v2) { return reformObj->getCoef(card->getLit(v1)) > reformObj->getCoef(card->getLit(v2)); });
  card->simplifyToCardinality(false, card->getCardinalityDegree());

  Ce32 result = global.cePools.take32();
  card->copyTo(result);
  return result;
}

template <typename SMALL, typename LARGE>
Lit Optimization<SMALL, LARGE>::getKnapsackLit(const CePtr<SMALL, LARGE>& core) const {
  core->sortWithCoefTiebreaker([&](Var v1, Var v2) {
    const LARGE o1r2 = reformObj->getLit(v1) == core->getLit(v1) ? aux::abs(reformObj->coefs[v1] * core->coefs[v2]) : 0;
    const LARGE o2r1 = reformObj->getLit(v2) == core->getLit(v2) ? aux::abs(reformObj->coefs[v2] * core->coefs[v1]) : 0;
    return aux::sgn(o1r2 - o2r1);
    // TODO: check whether sorting the literals is a bottleneck
    // TODO: cast to LARGE when using smaller SMALL, LARGE
  });
  LARGE range = core->getDegree();
  int i = core->nVars();
  while (range >= 0 && i > 0) {
    --i;
    range -= core->nthCoef(i);
  }
  ++i;
  assert(i <= core->nVars());
  assert(i >= 0);
  return core->getLit(core->vars[i]);
}

template <typename SMALL, typename LARGE>
State Optimization<SMALL, LARGE>::reformObjective(const CeSuper& core) {  // modifies core
  core->weaken([&](Lit l) { return !assumptions.has(-l) && !reformObj->hasLit(l); });
  if (core->isTautology()) return State::FAIL;
  core->removeUnitsAndZeroes(solver.getLevel(), solver.getPos());
  if (core->isUnsat()) {
    solver.addConstraint(core);
    return State::FAIL;
  }
  if (!core->hasNegativeSlack(solver.getAssumptions().getIndex())) return State::FAIL;
  core->saturate(true, false);
  Ce32 cardCore = reduceToCardinality(core);
  cardCore->orig = Origin::COREGUIDED;
  assert(cardCore->hasNoZeroes());

  // adjust the lower bound
  assert(!cardCore->empty());
  SMALL mult = 0;
  for (Var v : cardCore->getVars()) {
    if (mult == 1) break;
    if (!reformObj->hasLit(cardCore->getLit(v))) continue;  // in case of user assumption
    mult = (mult == 0) ? aux::abs(reformObj->coefs[v]) : std::min(mult, aux::abs(reformObj->coefs[v]));
  }
  if (mult == 0) return State::FAIL;  // no further literals of the objective remain

  global.stats.NCGNONCLAUSALCORES.z += !cardCore->isClause();

  assert(!cardCore->isTautology());
  assert(!cardCore->isUnsat());
  if (cardCore->getDegree() == cardCore->nVars()) {
    // we can just add them as unit constraints
    solver.addConstraint(cardCore);
    reformObj->removeUnitsAndZeroes(solver.getLevel(), solver.getPos());
    lower_bound = std::max(lower_bound, -reformObj->getDegree());
    return State::SUCCESS;
  }
  // now we need at least 1 variable, unless the upper bound is already tight
  bool needAuxiliary = upper_bound / mult > cardCore->getDegree();
  if (needAuxiliary) {
    // add auxiliary variable
    int newN = solver.addVar(false);
    reformObj->addLhs(mult, newN);  // add only one variable for now

    // add first lazy constraint
    lazyVars.push_back(std::make_unique<LazyVar<SMALL, LARGE>>(solver, cardCore, newN, mult, upper_bound));
    lazyVars.back()->addAtLeastConstraint();
    lazyVars.back()->addAtMostConstraint();
  }
  // else the cardinality is actually an equality, and no auxiliary variables are needed.

  // reformulate the objective
  cardCore->invert();
  reformObj->addUp(cardCore, mult);
  simplifyAssumps(reformObj, assumptions);

  if (!needAuxiliary) {
    // since the cardinality is actually an equality, we can add its inverted form to the solver
    solver.addConstraint(cardCore);
  }

  lower_bound = std::max(lower_bound, -reformObj->getDegree());
  return State::SUCCESS;
}

template <typename SMALL, typename LARGE>
void Optimization<SMALL, LARGE>::handleInconsistency(const CeSuper& core) {  // modifies core
  assert(!core->hasNegativeSlack(solver.getLevel()));  // root inconsistency was handled by solver's learnConstraint
  assert(!core->isUnsat());

  if (toVar(assumps.back()) == boundingVar) {
    // simple bottom-up
    assert(lower_bound < boundingVal + 1);
    lower_bound = boundingVal + 1;
    return;
  }
  assert(global.options.proofAssumps);

  assert(reformObj);
  reformObj->removeUnitsAndZeroes(solver.getLevel(), solver.getPos());
  if (lower_bound < -reformObj->getDegree()) {
    ++global.stats.NCGUNITCORES.z;
    lower_bound = -reformObj->getDegree();
  }

  core->removeUnitsAndZeroes(solver.getLevel(), solver.getPos());
  core->saturate(true, false);

  if (!core->isTautology()) {
    assert(core->hasNegativeSlack(solver.getAssumptions().getIndex()));
    State result = State::SUCCESS;
    while (result == State::SUCCESS) {
      result = reformObjective(core);
      if (!global.options.optReuseCores) break;
    }
    simplifyAssumps(reformObj, assumptions);
    addReformUpperBound(false);
  }  // else only violated unit assumptions were derived

  checkLazyVariables();
}

template <typename SMALL, typename LARGE>
void Optimization<SMALL, LARGE>::boundObjByLastSol() {
  if (!solver.foundSolution()) throw InvalidArgument("No solution to add objective bound.");

  const LitVec& sol = solver.getLastSolution();
  upper_bound = -origObj->getRhs();
  for (Var v : origObj->getVars()) upper_bound += sol[v] > 0 ? origObj->coefs[v] : 0;
  const LARGE upbound = -upper_bound + 1;

  CePtr<SMALL, LARGE> aux = global.cePools.take<SMALL, LARGE>();
  origObj->copyTo(aux);
  aux->orig = Origin::UPPERBOUND;
  aux->invert();
  aux->addRhs(upbound);

  solver.dropExternal(lastUpperBound, true, true);

  if (global.options.proofAssumps) addReformUpperBound(true);

  if (global.options.liftDegreeSymbolic.get() > 0) {
    solver.setSymbBoundUpper(upbound);
    aux->symbBound.mult_upper = 1;
    aux->symbBound.mult_lower = 0;
    aux->symbBound.offset = aux->getDegree() - upbound;
    assert(aux->symbBound.getDegree(solver.getSymbBoundUpper(), solver.getSymbBoundLower()) == aux->getDegree());
    assert(aux->symbBound.isValid());
  } else {
    assert(!aux->symbBound.isValid());
  }

  std::pair<ID, ID> res = solver.addConstraint(aux);
  lastUpperBound = res.second;
}

template <typename SMALL, typename LARGE>
SolveState Optimization<SMALL, LARGE>::run(bool optimize, double timeout) {
  try {
    solver.presolve();  // will run only once, but also short-circuits (throws UnsatEncounter) when unsat was reached
  } catch (const UnsatEncounter&) {
    lower_bound = upper_bound;
    return SolveState::UNSAT;
  }
  while (true) {
    if (timeout != 0 && global.stats.getRunTime() > timeout) return SolveState::TIMEOUT;
    quit::checkInterrupt(global);
    // NOTE: it's possible that upper_bound < lower_bound, since at the point of optimality, the objective-improving
    // constraint yields UNSAT, at which case core-guided search can derive any constraint.
    StatNum current_time = global.stats.getDetTime();

    // There are three possibilities:
    // - no assumptions are set (because something happened during previous core-guided step)
    // - only the given assumptions are set (because the previous step was not core-guided)
    // - more than only the given assumptions are set (because the previous core-guided step did not finish)

    assumps.clear();
    bool topdown = false;
    if (optimize && !origObj->empty() && lower_bound < upper_bound &&
        (global.options.optRatio.get() >= 1 ||
         global.stats.DETTIMEBOTTOMUP.z <
             global.options.optRatio.get() * (global.stats.DETTIMETOPDOWN.z + global.stats.DETTIMEBOTTOMUP.z))) {
      // figure out and set new bottom-up assumptions
      if (global.options.optCoreguided && global.options.proofAssumps) {
        assert(reformObj);
        reformObj->removeEqualities(solver.getEqualities());
        simplifyAssumps(reformObj, assumptions);
        reformObj->removeUnitsAndZeroes(solver.getLevel(), solver.getPos());
        if (!reformObj->isSortedInDecreasingCoefOrder()) {
          reformObj->sortInDecreasingCoefOrder([](Var v1, Var v2) { return v1 < v2; });
        }
        if (reformObj->empty()) {
          solver.setAssumptions(assumptions.getKeys(), false);
        } else {
          assumps.insert(assumps.end(), assumptions.getKeys().begin(), assumptions.getKeys().end());
          VarVec& refVars = reformObj->vars;
          if (global.options.optStratification.get() == 0) {
            for (const Var v : refVars) assumps.push_back(-reformObj->getLit(v));
          } else {
            LARGE allowed =
                (upper_bound - lower_bound) - (upper_bound - lower_bound) / global.options.optStratification.get();
            assert(allowed > 0);
            SMALL lastCoef = 0;
            SMALL cf = 0;
            for (const Var v : refVars) {
              cf = aux::abs(reformObj->coefs[v]);
              if (allowed <= 0 && cf != lastCoef) break;
              assumps.push_back(-reformObj->getLit(v));
              allowed -= cf;
              lastCoef = cf;
            }
          }
          solver.setAssumptions(assumps, true);
        }
      } else {
        boundBottomUp();
        assumps.insert(assumps.end(), assumptions.getKeys().begin(), assumptions.getKeys().end());
        assumps.push_back(-boundingVar);
        solver.setAssumptions(assumps, true);
      }
    } else {  // set regular assumptions
      topdown = true;
      solver.setAssumptions(assumptions.getKeys(), false);
    }

    SolveState reply;
    if (solver.lastGlobalDual && solver.lastGlobalDual->hasNegativeSlack(solver.getAssumptions().getIndex())) {
      // NOTE: this optimization really helps with knapsack instances, because it immediately fixes a bunch of variables
      // TODO: generalize this: reformulate the *original* objective with the new bound after an inconsistency?
      reply = SolveState::INCONSISTENT;
      solver.lastCore = solver.lastGlobalDual;
    } else {
      try {
        reply = aux::timeCall<SolveState>([&] { return solver.solve(); },
                                          topdown ? global.stats.SOLVETIMETOPDOWN.z : global.stats.SOLVETIMEBOTTOMUP.z);
      } catch (const UnsatEncounter&) {
        reply = SolveState::UNSAT;
      }
      if (topdown) {
        global.stats.DETTIMETOPDOWN += global.stats.getDetTime() - current_time;
      } else {
        global.stats.DETTIMEBOTTOMUP += global.stats.getDetTime() - current_time;
      }
    }

    if (reply == SolveState::SAT) {
      assert(solver.foundSolution());
      ++global.stats.NSOLS;
      if (optimize) {
        boundObjByLastSol();
        printObjBounds(true);
      }
      solver.clearAssumptions();
      return SolveState::SAT;
    } else if (reply == SolveState::INCONSISTENT) {
      assert(!solver.getAssumptions().isEmpty());
      ++global.stats.NCORES;
      if (solver.getAssumptions().size() > assumptions.size()) {
        if (solver.lastCore->falsifiedBy(assumptions)) {
          solver.clearAssumptions();
          lower_bound = upper_bound;
          return SolveState::INCONSISTENT;
        }
        current_time = global.stats.getDetTime();
        aux::timeCallVoid([&] { handleInconsistency(solver.lastCore); }, global.stats.SOLVETIMEBOTTOMUP);
        global.stats.DETTIMEBOTTOMUP += global.stats.getDetTime() - current_time;
        if (global.options.proofAssumps) addLowerBound();
        solver.clearAssumptions();
        printObjBounds(false);
      } else {
        assert(solver.getAssumptions().size() == assumptions.size());  // no bottom-up assumptions
        assert(solver.lastCore->falsifiedBy(assumptions));
        lower_bound = upper_bound;
        return SolveState::INCONSISTENT;
      }
    } else {
      assert(reply == SolveState::INPROCESSED || reply == SolveState::UNSAT);
      // if (global.options.printCsvData) {
      //   global.stats.printCsvLine(static_cast<StatNum>(lower_bound), static_cast<StatNum>(upper_bound));
      // }
      if (reply == SolveState::UNSAT) {
        lower_bound = upper_bound;
      }
      return reply;
    }
  }
}

template <typename SMALL, typename LARGE>
SolveState Optimization<SMALL, LARGE>::runFull(bool optimize, double timeout) {
  SolveState result = SolveState::INPROCESSED;
  while (result == SolveState::INPROCESSED || (result == SolveState::SAT && optimize)) {
    result = run(optimize, timeout);
  }
  return result;
}

template <typename SMALL, typename LARGE>
void Optimization<SMALL, LARGE>::boundBottomUp() {
  assert(lower_bound < upper_bound);
  LARGE bnd = lower_bound + (global.options.optPrecision.get() == 0
                                 ? static_cast<LARGE>(0)
                                 : (upper_bound - lower_bound) / global.options.optPrecision.get());
  assert(bnd < upper_bound);
  assert(bnd >= lower_bound);
  if (boundingVar == 0 || bnd != boundingVal) {
    boundingVal = bnd;
    CeArb bound = global.cePools.takeArb();
    origObj->copyTo(bound);
    assert(bound->getDegree() == 0);
    bound->addRhs(boundingVal);  // objective must be at least middle
    bound->invert();             // objective must be at most middle
    boundingVar = solver.addVar(false);
    bound->addLhs(bound->getDegree(), boundingVar);  // ~boundingVar enables bisect constraint
    bound->orig = Origin::BOTTOMUP;
    solver.addConstraint(bound);
  }
}

template class Optimization<int, int64_t>;
template class Optimization<int64_t, int128>;
template class Optimization<int128, int128>;
template class Optimization<int128, int256>;
template class Optimization<bigint, bigint>;

template struct LazyVar<int, int64_t>;
template struct LazyVar<int64_t, int128>;
template struct LazyVar<int128, int128>;
template struct LazyVar<int128, int256>;
template struct LazyVar<bigint, bigint>;

}  // namespace xct
