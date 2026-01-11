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

#include "Solver.hpp"
#include "auxiliary.hpp"
#include "constraints/Constr.hpp"

namespace xct {

// ---------------------------------------------------------------------
// Initialization

Solver::Solver(Global& g)
    : lastCore(g.cePools.take32()),
      global(g),
      gen(std::random_device{}()),
      heur(g),
      equalities(*this),
      implications(*this),
      nconfl_to_reduce(global.options.dbBase.get()),
      nconfl_to_restart(global.options.lubyMult.get()) {
  ca.capacity(1048576);  // 4MiB
  position.resize(1, INF);
  isorig.resize(1, true);
  objective = std::make_shared<ConstrExpArb>(global);
  assert(!lastGlobalDual);
}

Solver::~Solver() {
  for (CRef cr : constraints) {
    ca[cr].cleanup();
  }
  ca.cleanup();
}

bool Solver::foundSolution() const { return lastSol.has_value(); }

int Solver::getNbVars() const { return n; }

void Solver::setNbVars(int nvars, bool orig) {
  assert(nvars >= 0);
  assert(nvars < INF);
  if (nvars <= n) {
    assert(isOrig(nvars) == orig);
    return;
  }
  adj.resize(nvars, {});
  level.resize(nvars, INF);
  position.resize(nvars + 1, INF);
  reason.resize(nvars + 1, CRef_Undef);
  heur.resize(nvars + 1);
  global.cePools.resize(nvars + 1);
  equalities.setNbVars(nvars);
  implications.setNbVars(nvars);
  isorig.resize(nvars + 1, orig);
  lit2cons.resize(nvars, {});
  lit2consOldSize.resize(nvars, std::numeric_limits<int>::max());
  assumptions.resize(nvars + 1);  // ensure assumptions.getIndex() can be used as level
  if (orig) {
    global.stats.NORIGVARS.z += nvars - n;
  } else {
    global.stats.NAUXVARS.z += nvars - n;
  }
  n = nvars;
}

Var Solver::addVar(bool orig) {
  setNbVars(n + 1, orig);
  return n;
}

bool Solver::isOrig(Var v) const {
  assert(v >= 0);
  assert(v <= getNbVars());
  return isorig[v];
}

const bigint& Solver::getSymbBoundUpper() const { return lastSymbBoundUpper; }
const bigint& Solver::getSymbBoundLower() const { return lastSymbBoundLower; }

void Solver::setSymbBoundUpper(const bigint& ub) {
  assert(decisionLevel() == 0);
  assert(lastSymbBoundUpper < ub);  // upper bound already inverted
  lastSymbBoundUpper = ub;
  if (global.options.liftDegreeSymbolic.get() == 2) {
    improveSymbBounds();
  }
}
void Solver::setSymbBoundLower(const bigint& lb) {
  assert(lastSymbBoundLower < lb);
  assert(decisionLevel() == 0);
  lastSymbBoundLower = lb;
  if (global.options.liftDegreeSymbolic.get() == 2) {
    improveSymbBounds();
  }
}
void Solver::improveSymbBounds() {
  const unordered_map<CRef, SymbolicBound> symbbounds_clone = symbbounds;
  // clone needed to not alter symbbounds in loop
  for (const auto& [cr, sb] : symbbounds_clone) {
    const Constr& c = ca[cr];
    if (!isLearned(c.getOrigin()) || sb.getDegree(getSymbBoundUpper(), getSymbBoundLower()) <= c.degree()) continue;
    ++global.stats.NSYMBBOUND;
    CeSuper ce = c.toExpanded(global.cePools);
    removeConstraint(cr);
    ce->symbBoundPostProcess(*this);
    learnConstraint(ce);
  }
}

void Solver::setObjective(const CeArb& obj) {
  global.options.setClausalConstraints(false);
  objectiveSet = true;
  objective = obj;
  auto [lb, ub] = obj->getLhsExtrema();
  lastSymbBoundLower = lb - 1;
  lastSymbBoundUpper = -ub - 1;  // inverted for ease of use
  symbbounds.clear();
  if (lpSolver) lpSolver->setObjective(objective);
}

void Solver::ignoreLastObjective() { objectiveSet = false; }

bool Solver::objectiveIsSet() const { return objectiveSet; }

void Solver::reportUnsat(const CeSuper& confl) {
  assert(!unsatReached);  // not a problem, but should not occur
  assert(confl->hasNegativeSlack(getLevel()));
  ID id = global.logger.logUnsat(confl, getLevel(), getPos());
  unsatReached = true;
  Ce32 tmp = global.cePools.take32();
  tmp->addRhs(1);
  tmp->resetBuffer(id);
  lastCore = tmp;
  throw UnsatEncounter();
}

Options& Solver::getOptions() { return global.options; }
Stats& Solver::getStats() { return global.stats; }
Logger& Solver::getLogger() { return global.logger; }

void Solver::fixPhase(const std::vector<std::pair<Var, Lit>>& vls, bool bump) {
  for (const auto& vl : vls) {
    heur.setFixedPhase(vl.first, vl.second);
  }
  if (bump) {
    VarVec vs = aux::comprehension(vls, [](const std::pair<Var, Lit>& vl) { return vl.first; });
    heur.vBumpActivity(vs, getPos(), global.options.varWeight.get(), global.stats.getNConfl());
  }
}

int Solver::assumptionLevelForVarAct() const { return global.options.varAssump ? 0 : assumptionLevel(); }
int Solver::assumptionLevelForLbd() const { return global.options.dbAssump ? 0 : assumptionLevel(); }

// ---------------------------------------------------------------------
// Assignment manipulation

void Solver::enqueueUnit([[maybe_unused]] Lit l, Var v, CRef r) {
  assert(toVar(l) == v);
  assert(global.stats.NUNITS.z == trail.size());
  ++global.stats.NUNITS.z;
  reason[v] = CRef_Undef;  // no need to keep track of reasons for unit literals
  if (global.logger.isActive()) {
    CeSuper tmp = ca[r].toExpanded(global.cePools);
    tmp->simplifyToUnit(level, position, v);
    global.logger.logUnit(tmp);
    assert(global.logger.getNbUnitIDs() == (int)trail.size() + 1);
  }
}

void Solver::uncheckedEnqueue(Lit l, CRef r) {
  assert(!isTrue(level, l));
  assert(!isFalse(level, l));
  assert(isUnknown(position, l));
  Var v = toVar(l);
  reason[v] = r;
  if (decisionLevel() == 0) {
    enqueueUnit(l, v, r);
  }
  level[l] = decisionLevel();
  position[v] = (int)trail.size();
  trail.push_back(l);
}

void Solver::undoOne() {
  assert(!trail.empty());
  ++global.stats.NTRAILPOPS.z;
  Lit l = trail.back();
  if (qhead == (int)trail.size()) {
    --qhead;
    for (const Watch& w : adj[-l]) {
      if (w.idx < 3 * UINF) {  // avoids the cardinality and clausal case
        if (Lit blocking = w.blocking; !isTrue(level, blocking) || position[toVar(blocking)] >= position[toVar(l)]) {
          ca[w.cref].undoFalsified(w.idx);
          ++global.stats.NWATCHLOOKUPSBJ.z;
        }
      }
    }
  }
  Var v = toVar(l);
  trail.pop_back();
  level[l] = INF;
  position[v] = INF;
  heur.undoOne(v, l);
  if (isDecided(reason, v)) {
    assert(!trail_lim.empty());
    assert(trail_lim.back() == (int)trail.size());
    trail_lim.pop_back();
    if (decisionLevel() < assumptionLevel()) {
      assumptions_lim.pop_back();
    }
    assert(assumptionLevel() <= decisionLevel());
  }
  equalities.notifyBackjump();
  implications.notifyBackjump();
}

void Solver::backjumpTo(int lvl) {
  assert(lvl >= 0);
  while (decisionLevel() > lvl) undoOne();
}

void Solver::decide(Lit l) {
  ++global.stats.NDECIDE.z;
  trail_lim.push_back(trail.size());
  uncheckedEnqueue(l, CRef_Undef);
}

void Solver::propagate(Lit l, CRef r) {
  assert(isValid(r));
  ++global.stats.NPROP.z;
  uncheckedEnqueue(l, r);
}

State Solver::probe(Lit l, bool deriveImplications) {
  assert(decisionLevel() == 0);
  assert(isUnknown(getPos(), l));
  ++global.stats.NPROBINGS.z;
  while (isUnknown(getPos(), l)) {
    decide(l);
    CeSuper confl = aux::timeCall<CeSuper>([&] { return runPropagation(); }, global.stats.PROPTIME.z);
    if (confl) {
      CeSuper analyzed = aux::timeCall<CeSuper>([&] { return analyze(confl); }, global.stats.CATIME.z);
      aux::timeCallVoid([&] { learnConstraint(analyzed); }, global.stats.LEARNTIME.z);
      return State::FAIL;
    }
    // NOTE: we may have backjumped to level 0 due to a learned constraint that did not propagate.
    // in that case, we just decide l again.
  }
  if (decisionLevel() != 1) {
    assert(decisionLevel() == 0);
    return State::FAIL;  // derived l >= 1 or 0 >= l
  }
  if (deriveImplications) {
    implications.removeImplied(l);
    for (int i = trail_lim[0] + 1; i < (int)trail.size(); ++i) {
      implications.addImplied(-trail[i], -l);  // the system may not be able to derive these
    }
    global.stats.NPROBINGIMPLMEM.z =
        std::max<StatNum>(global.stats.NPROBINGIMPLMEM.z, implications.nImpliedsInMemory());
  }
  return State::SUCCESS;
}

/**
 * Unit propagation with watched literals.
 * @post: all watches up to trail[qhead] have been propagated
 * @return: a CeNull when no conflict is detected, otherwise the conflicting constraint
 */
CeSuper Solver::runDatabasePropagation() {
  Lit p;
  while (qhead < (int)trail.size()) {
    p = trail[qhead++];
    assert(isTrue(level, p));
    std::vector<Watch>& ws = adj[-p];
    for (int32_t it_ws = 0; it_ws < std::ssize(ws); ++it_ws) {
      const uint32_t& idx = ws[it_ws].idx;
      const Lit& blocking = ws[it_ws].blocking;

      if (isTrue(level, blocking)) {  // blocking literal check
        if (idx >= CLAUSE_IDX || position[toVar(blocking)] < position[toVar(p)]) {
          assert(dynamic_cast<Clause*>(&ca[ws[it_ws].cref]) != nullptr ||
                 dynamic_cast<Binary*>(&ca[ws[it_ws].cref]) != nullptr || idx < UINF ||
                 (idx < 3 * UINF && idx >= 2 * UINF));
          global.stats.NBLOCKINGSUCCESS.z += idx < CLAUSE_IDX;  // not a clause or binary
          continue;
        }
      }
      global.stats.NBLOCKINGFAILS.z += idx < CLAUSE_IDX && blocking != 0;  // not a clause or binary

      Watch& w = ws[it_ws];
      WatchStatus wstat = WatchStatus::DROPWATCH;
      if (idx == BINARY_IDX) {
        assert(!isTrue(level, blocking));  // already checked for blocking literal
        if (isFalse(level, blocking)) {
          wstat = WatchStatus::CONFLICTING;
        } else {
          propagate(blocking, w.cref);
          wstat = WatchStatus::KEEPWATCH;
        }
      } else {
        ++global.stats.NWATCHLOOKUPS.z;
        Constr& c = ca[w.cref];
        if (!c.isMarkedForDelete()) {
          // Try to avoid the vTable indirection
          if (idx < UINF) {
            wstat = static_cast<Watched32&>(c).checkForPropagation(w, -p, *this, global.stats);
          } else if (idx == CLAUSE_IDX) {
            wstat = static_cast<Clause&>(c).checkForPropagation(w, -p, *this, global.stats);
          } else {
            assert(idx < CLAUSE_IDX);  // not a clause or binary
            assert(idx >= 2 * UINF);   // not a Watched32
            wstat = c.checkForPropagation(w, -p, *this, global.stats);
          }
        }
      }

      if (wstat == WatchStatus::DROPWATCH) {
        plf::single_reorderase(ws, ws.begin() + it_ws);
        --it_ws;
      } else if (wstat == WatchStatus::CONFLICTING) {  // clean up current level and stop propagation
        Constr& c = ca[w.cref];
        ++global.stats.NTRAILPOPS.z;
        --qhead;
        for (uint32_t i = 0; i <= static_cast<uint32_t>(it_ws); ++i) {
          const Watch& wa = ws[i];
          if (wa.idx < 3 * UINF) {  // avoids the cardinality and clausal case
            if (Lit blocking = wa.blocking;
                !isTrue(level, blocking) || position[toVar(blocking)] >= position[toVar(p)]) {
              ca[wa.cref].undoFalsified(wa.idx);
              ++global.stats.NWATCHLOOKUPSBJ.z;
            }
          }
        }
        CeSuper result = expandWithSymbBound(c);
        assert(result);
        c.fixEncountered(result->getLbd(level, assumptionLevelForLbd()), global);
        return expandWithSymbBound(c);
      } else {
        assert(wstat == WatchStatus::KEEPWATCH);
      }
    }
  }
  return CeNull();
}

CeSuper Solver::runPropagation() {
  while (true) {
    CeSuper confl = runDatabasePropagation();
    if (confl) return confl;
    if (equalities.propagate() == State::FAIL) continue;
    if (implications.propagate() == State::FAIL) continue;
    return CeNull();
  }
}

CeSuper Solver::runPropagationWithLP() {
  if (CeSuper result = runPropagation(); result) return result;
  if (lpSolver) {
    auto [state, constraint] = aux::timeCall<std::pair<LpStatus, CeSuper>>(
        [&] { return lpSolver->checkFeasibility(false); }, global.stats.LPTOTALTIME.z);
    // NOTE: calling LP solver may increase the propagations on the trail due to added constraints
    if (state == LpStatus::INFEASIBLE || state == LpStatus::OPTIMAL) {
      // added a Farkas/bound constraint and potentially backjumped, so we propagate again
      return runPropagation();
    }
  }
  return CeNull();
}

// ---------------------------------------------------------------------
// Conflict analysis

CeSuper Solver::getAnalysisCE(const CeSuper& conflict) const {
  int bitsOverflow = global.options.getBitsOverflow();
  if (bitsOverflow == 0 || bitsOverflow > limitBitConfl<int128, int256>()) {
    CeArb confl = global.cePools.takeArb();
    conflict->copyTo(confl);
    return confl;
  } else if (global.options.getBitsOverflow() > limitBitConfl<int128, int128>()) {
    Ce128 confl = global.cePools.take128();
    conflict->copyTo(confl);
    return confl;
  } else if (global.options.getBitsOverflow() > limitBitConfl<int64_t, int128>()) {
    Ce96 confl = global.cePools.take96();
    conflict->copyTo(confl);
    return confl;
  } else if (global.options.getBitsOverflow() > limitBitConfl<int, int64_t>()) {
    Ce64 confl = global.cePools.take64();
    conflict->copyTo(confl);
    return confl;
  } else {
    Ce32 confl = global.cePools.take32();
    conflict->copyTo(confl);
    return confl;
  }
}

CeSuper Solver::analyze(const CeSuper& conflict) {
  global.logger.logComment("Analyze");
  assert(conflict->hasNegativeSlack(level));
  conflict->removeUnitsAndZeroes(level, position);
  conflict->saturateAndFixOverflow(getLevel(), 0, global.options.getBitsOverflow(), global.options.getBitsReduced(), 0,
                                   true);

  CeSuper confl = getAnalysisCE(conflict);
  confl->orig = Origin::LEARNED;
  confl->setTmpSlack(level);
  confl->setTmpPrevious(level, decisionLevel());

  if (!global.options.varConstraint.is("learned")) {
    assert(confl->hasNoZeroes());
    const int assumpLevel = assumptionLevelForVarAct();
    VarVec vars = aux::to_vector(confl->getVars() | std::views::filter([&](Var v) {
                                   const int falseLvl = level[-confl->getLit(v)];
                                   return falseLvl < INF && falseLvl > assumpLevel;
                                 }));
    aux::timeCallVoid(
        [&] { heur.vBumpActivity(vars, getPos(), global.options.varWeight.get(), global.stats.getNConfl()); },
        global.stats.HEURTIME.z);
  }

resolve:
  while (decisionLevel() > 0) {
    quit::checkInterrupt(global);
    const Lit l = trail.back();
    if (confl->hasLit(-l)) {
      assert(confl->hasNegativeSlack(level));
      if (confl->canPropagateOnPrevious(getPos(), decisionPos())) {
        break;
      }
      if (global.options.skipResolution && confl->fixCoefSmallerThanTmpSlack(-l)) {
        confl->undoOneTmpPrevious(trail, trail_lim);
        undoOne();
        continue;
      }
      assert(isPropagated(reason, l));
      Constr& reasonC = ca[reason[toVar(l)]];

      unsigned int lbd = reasonC.resolveWith(confl, l, *this);
      reasonC.fixEncountered(lbd, global);
    }
    confl->undoOneTmpSlack(l);  // TODO: not strictly needed?
    confl->undoOneTmpPrevious(trail, trail_lim);
    undoOne();
  }
  assert(confl->hasCorrectTmpSlack(level));
  assert(decisionLevel() == 0 || confl->hasCorrectTmpPrevious(level, decisionLevel()));

  if (global.options.learnedMin && decisionLevel() > 0) {
    minimize(confl);
    if (!confl->setTmpPrevious(level, decisionLevel())) {
      goto resolve;
    }
  }

  if (!global.options.varConstraint.is("conflict")) {
    assert(confl->hasNoZeroes());
    const int assumpLevel = assumptionLevelForVarAct();
    VarVec vars = aux::to_vector(confl->getVars() | std::views::filter([&](Var v) {
                                   const int falseLvl = level[-confl->getLit(v)];
                                   return falseLvl < INF && falseLvl > assumpLevel;
                                 }));
    aux::timeCallVoid(
        [&] { heur.vBumpActivity(vars, getPos(), global.options.varWeight.get(), global.stats.getNConfl()); },
        global.stats.HEURTIME.z);
  }

  assert(confl->hasNegativeSlack(level));
  return confl;
}

void Solver::minimize(CeSuper& conflict) {
  assert(conflict->isSaturated());
  assert(litsToSubsumeMem.empty());
  if (conflict->symbBound.isValid()) return;
  IntSet& saturatedLits = global.isPool.take();
  conflict->removeZeroes();
  conflict->getSaturatedLits(saturatedLits);
  if (saturatedLits.isEmpty()) {
    global.isPool.release(saturatedLits);
    return;
  }

  for (Var v : conflict->getVars()) {
    Lit l = conflict->getLit(v);
    if (isFalse(getLevel(), l) && isPropagated(reason, l)) {
      litsToSubsumeMem.push_back({position[v], l});
    }
  }
  boost::sort::pdqsort(litsToSubsumeMem.begin(), litsToSubsumeMem.end(),
                       [&](const std::pair<int, Lit>& x, const std::pair<int, Lit>& y) { return x.first > y.first; });

  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
  for (const std::pair<int, Lit>& pr : litsToSubsumeMem) {
    Lit l = pr.second;
    assert(conflict->getLit(toVar(l)) != 0);
    Constr& reasonC = ca[reason[toVar(l)]];
    unsigned int lbd = reasonC.subsumeWith(conflict, -l, *this, saturatedLits);
    if (lbd > 0) reasonC.fixEncountered(lbd, global);  // otherwise no subsumption
    if (saturatedLits.isEmpty()) break;
  }
  global.stats.MINTIME.z +=
      std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start).count();
  conflict->removeZeroes();  // remove weakened literals
  global.isPool.release(saturatedLits);
  litsToSubsumeMem.clear();
}

Ce32 Solver::getUnitClause(Lit l) const {
  assert(isUnit(level, l));
  Ce32 core = global.cePools.take32();
  core->addRhs(1);
  core->addLhs(1, l);
  core->resetBuffer(global.logger.getUnitID(l, position));
  return core;
}

CeSuper Solver::extractCore(const CeSuper& conflict, Lit l_assump) {
  if (l_assump != 0) {  // l_assump is an assumption propagated to the opposite value
    assert(assumptions.has(l_assump));
    assert(isFalse(level, l_assump));
    assert(!isUnit(level, -l_assump));
    int pos = position[toVar(l_assump)];
    while ((int)trail.size() > pos) undoOne();
    assert(isUnknown(position, l_assump));
    decide(l_assump);
  }

  // Set all assumptions in front of the trail, all propagations later. This makes it easy to do decision learning.
  // For this, we first copy the trail, then backjump to 0, then rebuild the trail.
  // Otherwise, reordering the trail messes up the slacks of the watched constraints (see undoOne()).
  LitVec decisions;  // holds the decisions
  decisions.reserve(decisionLevel());
  LitVec props;  // holds the propagations
  props.reserve(trail.size());
  assert(!trail_lim.empty());
  for (int i = trail_lim[0]; i < (int)trail.size(); ++i) {
    Lit l = trail[i];
    if (assumptions.has(l) && !isPropagated(reason, l)) {
      decisions.push_back(l);
    } else {
      props.push_back(l);
    }
  }
  backjumpTo(0);

  for (Lit l : decisions) decide(l);
  for (Lit l : props) propagate(l, reason[toVar(l)]);

  assert(conflict->hasNegativeSlack(level));
  conflict->removeUnitsAndZeroes(level, position);
  conflict->saturateAndFixOverflow(getLevel(), 0, global.options.getBitsOverflow(), global.options.getBitsReduced(), 0,
                                   true);
  assert(conflict->hasNegativeSlack(level));
  CeSuper core = getAnalysisCE(conflict);
  core->orig = Origin::LEARNED;

  core->setTmpSlack(level);
  core->setTmpPrevious(level, decisionLevel());  // TODO: unnecessary overhead for extracting core
  while (decisionLevel() > 0 && isPropagated(reason, trail.back())) {
    quit::checkInterrupt(global);
    Lit l = trail.back();
    if (core->hasLit(-l)) {
      assert(isPropagated(reason, l));
      Constr& reasonC = ca[reason[toVar(l)]];

      uint32_t lbd = reasonC.resolveWith(core, l, *this);
      reasonC.fixEncountered(aux::max<uint32_t>(1, lbd), global);
    }
    core->undoOneTmpSlack(l);
    core->undoOneTmpPrevious(trail, trail_lim);
    undoOne();
  }

  // weaken non-falsifieds
  assert(core->hasNegativeSlack(assumptions.getIndex()));
  assert(core->hasCorrectTmpSlack(level));
  assert(!core->isTautology());
  assert(core->isSaturated());
  aux::timeCallVoid([&] { learnConstraint(core); }, global.stats.LEARNTIME.z);
  // NOTE: takes care of inconsistency
  backjumpTo(0);
  for (Lit l : assumptions.getKeys()) {
    if (isUnit(level, -l)) return getUnitClause(-l);
  }
  core->postProcess(getLevel(), getPos(), getHeuristic(), true, global.stats);
  return core;
}

// ---------------------------------------------------------------------
// Constraint management

CRef Solver::attachConstraint(const CeSuper& constraint, bool locked, uint32_t lbd) {
  assert(constraint->isSortedInDecreasingCoefOrder());
  assert(constraint->isSaturated());
  assert(constraint->hasNoZeroes());
  assert(constraint->hasNoUnits(getLevel()));
  assert(!constraint->isTautology());
  assert(!constraint->empty());
  assert(!constraint->hasNegativeSlack(getLevel()));
  assert(constraint->orig != Origin::UNKNOWN);
  assert(lbd > 0);

  global.options.setClausalConstraints(constraint->isClause());

  CRef cr = constraint->toConstr(ca, locked, global.logger.logProofLineWithInfo(constraint, "Attach"));
  if (constraint->symbBound.isValid()) {
    symbbounds[cr] = constraint->symbBound;
    ++global.stats.NSYMBBOUNDADDED.z;
    assert(symbbounds[ca(ca[cr])] == constraint->symbBound);
  }
  Constr& c = ca[cr];
  c.fixEncountered(lbd, global, false);
  c.initializeWatches(cr, *this);
  constraints.push_back(cr);
  const Origin& orig = constraint->orig;
  if (isNonImplied(orig)) {
    for (unsigned int i = 0; i < c.size(); ++i) {
      Lit l = c.lit(i);
      assert(isOrig(toVar(l)));
      lit2cons[l].insert({cr, i});
    }
  }
  if (c.isAtMostOne() && c.size() > 2) {
    auto lambda = [&](unsigned i) { return c.lit(i); };
    auto lits = std::views::iota(0u, c.size()) | std::views::transform(lambda);
    const uint64_t hash = aux::hashForSet<Lit>(lits);
    if (auto bestsize = atMostOneHashes.find(hash); bestsize == atMostOneHashes.end() || bestsize->second < c.size()) {
      atMostOneHashes[hash] = c.size();
    }
  }

  const bool learned = isLearned(orig);
  if (learned) {
    global.stats.LEARNEDLENGTHSUM.z += c.size();
    global.stats.LEARNEDDEGREESUM.z += static_cast<StatNum>(c.degree());
  } else {
    global.stats.EXTERNLENGTHSUM.z += c.size();
    global.stats.EXTERNDEGREESUM.z += static_cast<StatNum>(c.degree());
  }
  if (c.degree() == 1) {
    global.stats.NCLAUSESLEARNED.z += learned;
    global.stats.NCLAUSESEXTERN.z += !learned;
  } else if (c.isClauseOrCard()) {
    global.stats.NCARDINALITIESLEARNED.z += learned;
    global.stats.NCARDINALITIESEXTERN.z += !learned;
  } else {
    global.stats.NGENERALSLEARNED.z += learned;
    global.stats.NGENERALSEXTERN.z += !learned;
  }

  global.stats.NCONSFORMULA.z += orig == Origin::FORMULA;
  global.stats.NCONSDOMBREAKER.z += orig == Origin::DOMBREAKER;
  global.stats.NCONSLEARNED.z += orig == Origin::LEARNED;
  global.stats.NCONSBOUND.z += isBound(orig);
  global.stats.NCONSCOREGUIDED.z += orig == Origin::COREGUIDED || orig == Origin::BOTTOMUP;
  global.stats.NLPGOMORYCUTS.z += orig == Origin::GOMORY;
  global.stats.NLPDUAL.z += orig == Origin::DUAL;
  global.stats.NLPFARKAS.z += orig == Origin::FARKAS;
  global.stats.NPURELITS.z += orig == Origin::PURE;
  global.stats.NCONSREDUCED.z += orig == Origin::REDUCED;

  // NOTE: propagation is not necessary, but do it at first level to make sure to derive as many unit lits as possible
  if (decisionLevel() == 0) {
    CeSuper confl = aux::timeCall<CeSuper>([&] { return runDatabasePropagation(); }, global.stats.PROPTIME);
    if (confl) {
      reportUnsat(confl);  // throws UnsatEncounter
    }
  }

  return cr;
}

/**
 * Adds c as a learned constraint with origin orig.
 * Backjumps to the level where c is no longer conflicting, as otherwise we might miss propagations.
 */
void Solver::learnConstraint(const CeSuper& ce) {
  assert(!unsatReached);  // may work, but should not happen, as solve() and presolve() both guard for this
  assert(ce);
  assert(isLearned(ce->orig));
  CeSuper learned = ce->clone(global.cePools);
  // NOTE: below line can cause conflict analysis loops when the equalities are not yet propagated, as the conflict
  // constraint becomes non-falsified
  // if (learned->orig != Origin::EQUALITY) {
  //   learned->removeEqualities(getEqualities());
  // }
  if (!learned->symbBound.isValid()) {
    learned->selfSubsumeImplications(implications);  // only strengthens the constraint
  }
  learned->removeUnitsAndZeroes(getLevel(), getPos());
  if (learned->isTautology()) return;
  learned->saturateAndFixOverflow(getLevel(), 0, global.options.getBitsLearned(), global.options.getBitsLearned(), 0,
                                  false);
  const std::vector<ActNode>& actList = getHeuristic().getActList();
  if (!learned->isClause()) {
    learned->sortInDecreasingCoefOrder([&](Var v1, Var v2) { return actList[v1].activity > actList[v2].activity; });
  }
  auto [assertionLevel, isAsserting] = learned->getAssertionStatus(level, position, assertionStateMem);
  if (assertionLevel < 0) {
    backjumpTo(0);
    assert(learned->isUnsat());
    reportUnsat(learned);  // throws  UnsatEncounter
  }
  backjumpTo(assertionLevel);
  assert(!learned->hasNegativeSlack(level));
  if (isAsserting) learned->heuristicWeakening(level, position);
  learned->postProcess(getLevel(), getPos(), getHeuristic(), false, global.stats);
  assert(learned->isSaturated());
  if (learned->isTautology()) return;
  if (learned->symbBound.isValid() && learned->symbBound.getDegree(getSymbBoundUpper(), getSymbBoundLower()) <= 0) {
    learned->symbBound.reset();
  }
  CRef cr = attachConstraint(learned, false,
                             isAsserting ? learned->getLbd(level, assumptionLevelForLbd()) : learned->nVars());
  Constr& c = ca[cr];
  // the LBD of non-asserting constraints is undefined, so we take a safe upper bound
  global.stats.LEARNEDLBDSUM += c.lbd();
}

void Solver::learnUnitConstraint(Lit l, Origin orig, ID id) {
  assert(isLearned(orig));
  assert(!isUnit(getLevel(), l));
  assert(!isUnit(getLevel(), -l));
  // so no conflict after learning

  backjumpTo(0);
  Ce32 unit = global.cePools.take32();
  unit->orig = orig;
  unit->addRhs(1);
  unit->addLhs(1, l);
  unit->resetBuffer(id);
  [[maybe_unused]] CRef cr = attachConstraint(unit, false, 1);
  assert(cr != CRef_Undef);
}

void Solver::learnClause(Lit l1, Lit l2, Origin orig, ID id) {
  Ce32 ce = global.cePools.take32();
  ce->addRhs(1);
  ce->addLhs(1, l1);
  ce->addLhs(1, l2);
  ce->orig = orig;
  ce->resetBuffer(std::to_string(id) + " ");
  aux::timeCallVoid([&] { learnConstraint(ce); }, global.stats.LEARNTIME);
}

std::pair<ID, ID> Solver::addInputConstraint(const CeSuper& ce) {  // NOTE: should not throw UnsatEncounter
  if (unsatReached) return {ID_Undef, ID_Undef};

  assert(isInput(ce->orig));
  assert(decisionLevel() == 0);
  ID input = ID_Undef;
  switch (ce->orig) {
    case Origin::FORMULA:
      input = global.logger.logInput(ce);
      break;
    case Origin::BOTTOMUP:
      input = global.logger.logBottomUp(ce);
      break;
    case Origin::UPPERBOUND:
      input = global.logger.logUpperBound(ce, getLastSolution());
      break;
      // TODO: reactivate below when VeriPB's redundant rule becomes stronger
      // case Origin::PURE:
      //   input = global.logger.logPure(ce);
      //   break;
      // case Origin::DOMBREAKER:
      //   input = global.logger.logDomBreaker(ce);
      //   break;
    default:
      input = global.logger.logAssumption(ce, global.options.proofAssumps.operator bool());
  }
  ce->strongPostProcess(*this);
  if (ce->isTautology()) {
    return {input, ID_Undef};  // already satisfied.
  }

  if (ce->hasNegativeSlack(level)) {
    assert(decisionLevel() == 0);
    assert(ce->hasNoUnits(level));
    assert(ce->isUnsat());
    if (global.options.verbosity.get() > 0) {
      std::cout << "c Conflicting input constraint" << std::endl;
    }
    try {
      reportUnsat(ce);
    } catch (const UnsatEncounter&) {
      // expected, do not rethrow
    }
    return {ID_Undef, ID_Undef};
  }

  try {
    CRef cr = attachConstraint(ce, true, 1);
    assert(cr != CRef_Undef);
    ID id = ca[cr].id();
    Origin orig = ca[cr].getOrigin();
    if (isExternal(orig)) {
      external[id] = cr;
    }
    if (lpSolver && (orig == Origin::FORMULA || orig == Origin::UPPERBOUND || orig == Origin::LOWERBOUND)) {
      lpSolver->addConstraint(cr, false, orig == Origin::UPPERBOUND, orig == Origin::LOWERBOUND);
    }

    return {input, id};
  } catch (const UnsatEncounter&) {
    return {ID_Undef, ID_Undef};
  }
}

std::pair<ID, ID> Solver::addConstraint(const CeSuper& c) {
  // NOTE: copy to temporary constraint guarantees original constraint is not changed and does not need
  // global.logger
  CeSuper ce = c->clone(global.cePools);
  return addInputConstraint(ce);
}

std::pair<ID, ID> Solver::addConstraint(const ConstrSimpleSuper& c) {
  CeSuper ce = c.toExpanded(global.cePools);
  return addInputConstraint(ce);
}

std::pair<ID, ID> Solver::addUnitConstraint(Lit l, Origin orig) {
  Ce32 ce = global.cePools.take32();
  ce->addRhs(1);
  ce->addLhs(1, l);
  ce->orig = orig;
  return addInputConstraint(ce);
}

std::pair<ID, ID> Solver::addBinaryConstraint(Lit l1, Lit l2, Origin orig) {
  Ce32 ce = global.cePools.take32();
  ce->addRhs(1);
  ce->addLhs(1, l1);
  ce->addLhs(1, l2);
  ce->orig = orig;
  return addInputConstraint(ce);
}

std::pair<ID, ID> Solver::addClauseConstraint(const LitVec& clause, Origin orig) {
  Ce32 ce = global.cePools.take32();
  ce->addRhs(1);
  ce->orig = orig;
  for (Lit l : clause) ce->addLhs(1, l);
  return addInputConstraint(ce);
}

void Solver::invalidateLastSol(const VarVec& vars) {
  assert(foundSolution());
  ConstrSimple32 invalidator;
  invalidator.orig = Origin::INVALIDATOR;
  invalidator.terms.reserve(global.stats.NORIGVARS.z);
  invalidator.rhs = 1;
  for (Var v : vars) {
    invalidator.terms.push_back({1, -lastSol.value()[v]});
  }
  addConstraint(invalidator);
}

void Solver::removeConstraint(const CRef& cr, [[maybe_unused]] bool override) {
  Constr& c = ca[cr];
  assert(override || !c.isLocked());
  assert(!c.isMarkedForDelete());
  assert(!external.count(c.id()));
  c.header.markedfordel = 1;
  ca.wasted += c.getMemSize();
  if (isNonImplied(c.getOrigin())) {
    for (unsigned int i = 0; i < c.size(); ++i) {
      Lit l = c.lit(i);
      assert(isOrig(toVar(l)));
      assert(lit2cons[l].count(cr));
      lit2cons[l].erase(cr);
    }
    // global.logger.logDeletion(c.id); TODO: needed?
  }
  symbbounds.erase(cr);
}

void Solver::dropExternal(ID id, bool erasable, bool forceDelete) {
  assert(erasable || !forceDelete);
  if (id == ID_Undef) return;
  auto old_it = external.find(id);
  assert(old_it != external.end());
  if (old_it == external.end()) return;  // TODO: should never happen, happened in some old test run. Temporary fix...
  // happened on memout in MIPLIB/scpk4.mps
  CRef cr = old_it->second;
  external.erase(old_it);
  ca[cr].setLocked(!erasable);
  if (forceDelete) removeConstraint(cr);
}

int Solver::getNbConstraints() const { return constraints.size(); }

const std::vector<CRef>& Solver::getRawConstraints() const { return constraints; }

const ConstraintAllocator& Solver::getCA() const { return ca; }

const SymbolicBound* Solver::getSymbBound(const Constr* c) const {
  auto it = symbbounds.find(ca(*c));
  if (it != symbbounds.end()) {
    return &(it->second);
  }
  return nullptr;
}

CeSuper Solver::expandWithSymbBound(const Constr& c) const {
  CeSuper result = c.toExpanded(global.cePools);
  auto sb_it = symbbounds.find(ca(c));
  if (sb_it != symbbounds.end()) {
    result->symbBound = sb_it->second;
  }
  return result;
}

// ---------------------------------------------------------------------
// Assumptions

void Solver::setAssumptions(const LitVec& assumps, bool cg) {
  clearAssumptions();
  if (assumps.empty()) return;
  for (Lit l : assumps) {
    assumptions.add(l);
  }
  assumptions_lim.reserve((int)assumptions.size() + 1);
  coreguided = cg;
}

void Solver::clearAssumptions() {
  assumptions.clear();
  backjumpTo(0);
  assert(assumptionLevel() == 0);
  assumptions_lim[0] = 0;
  coreguided = false;
}

const IntSet& Solver::getAssumptions() const { return assumptions; }

bool Solver::assumptionsClashWithUnits() const {
  return std::any_of(assumptions.getKeys().cbegin(), assumptions.getKeys().cend(),
                     [&](Lit l) { return isUnit(getLevel(), -l); });
}

int Solver::getNbUnits() const { return trail.size(); }

LitVec Solver::getUnits() const {
  if (decisionLevel() == 0) return trail;
  LitVec units;
  units.reserve(trail_lim[0]);
  for (int i = 0; i < trail_lim[0]; ++i) {
    Lit l = trail[i];
    if (!isOrig(toVar(l))) continue;
    units.push_back(l);
  }
  return units;
}

const LitVec& Solver::getLastSolution() const {
  assert(foundSolution());
  return lastSol.value();
}

void Solver::printHeader() const {
  if (global.options.verbosity.get() > 0) {
    std::cout << "c #vars " << getNbVars() << " #constraints " << getNbConstraints() << std::endl;
  }
}

// ---------------------------------------------------------------------
// Garbage collection

void Solver::rebuildLit2Cons() {
  for (unordered_map<CRef, int>& col : lit2cons) {
    col.clear();
  }
  for (const CRef& cr : constraints) {
    Constr& c = ca[cr];
    if (c.isMarkedForDelete() || !isNonImplied(c.getOrigin())) continue;
    for (unsigned int i = 0; i < c.size(); ++i) {
      assert(isOrig(toVar(c.lit(i))));
      lit2cons[c.lit(i)].insert({cr, c.isClauseOrCard() ? INF : i});
    }
  }
}

void updatePtr(const unordered_map<uint32_t, CRef>& crefmap, CRef& cr) { cr = crefmap.at(cr.ofs); }

// We assume in the garbage collection method that reduceDB() is the only place where constraints are deleted.
void Solver::garbage_collect() {
  assert(decisionLevel() == 0);  // otherwise reason CRefs need to be taken care of
  if (global.options.verbosity.get() > 1) std::cout << "c GARBAGE COLLECT" << std::endl;

  ca.wasted = 0;
  ca.at = 0;
  unordered_map<uint32_t, CRef> crefmap;
  unordered_map<CRef, SymbolicBound> new_symbbounds;
  for (int i = 1; i < (int)constraints.size(); ++i) assert(constraints[i - 1].ofs < constraints[i].ofs);
  for (CRef& cr : constraints) {
    CRef old_cr = cr;
    uint32_t offset = cr.ofs;
    size_t memSize = ca[cr].getMemSize();
    std::memmove(ca.memory + maxAlign * ca.at, ca.memory + maxAlign * cr.ofs, maxAlign * memSize);
    cr.ofs = ca.at;
    ca.at += memSize;
    crefmap[offset] = cr;
    auto node = symbbounds.find(old_cr);
    if (node != symbbounds.end()) {
      new_symbbounds[cr] = node->second;
    }
  }
  std::swap(symbbounds, new_symbbounds);

  for (Lit l = -n; l <= n; ++l) {
    for (Watch& w : adj[l]) updatePtr(crefmap, w.cref);
  }
  for (auto& ext : external) {
    updatePtr(crefmap, ext.second);
  }
  rebuildLit2Cons();
}

// We assume in the garbage collection method that reduceDB() is the
// only place where constraints are removed from memory.
void Solver::reduceDB() {
  backjumpTo(0);  // otherwise reason CRefs need to be taken care of
  std::vector<std::pair<CRef, double>> ordered_learnts;

  removeSatisfiedNonImpliedsAtRoot();
  const int64_t nconfl = global.stats.getNConfl();
  for (const CRef& cr : constraints) {
    Constr& c = ca[cr];
    if (c.isMarkedForDelete() || c.isLocked() || external.count(c.id())) {
      continue;
    }
    assert(!isNonImplied(c.getOrigin()));
    if (c.isSatisfiedAtRoot(getLevel())) {
      ++global.stats.NSATISFIEDSREMOVED;
      removeConstraint(cr);
    } else {
      ordered_learnts.emplace_back(cr, 0);
      if (global.options.dbCleaningPriority.is("lbd")) {
        const double nconfl_norm = (c.activity + 1) / static_cast<double>(nconfl + 1);
        ordered_learnts.back().second = nconfl_norm - static_cast<double>(c.lbd());
      } else if (global.options.dbCleaningPriority.is("activity")) {
        ordered_learnts.back().second = (c.activity + 1) - static_cast<double>(c.lbd()) / static_cast<double>(MAXLBD);
      } else if (global.options.dbCleaningPriority.is("combo")) {
        ordered_learnts.back().second = (c.activity + 1) / static_cast<double>(c.lbd());
      } else {
        assert(false);
      }
    }
  }

  if (global.options.dbCleaningPriority.is("random")) {
    std::ranges::shuffle(ordered_learnts, gen);
  } else {
    boost::sort::pdqsort(
        ordered_learnts.begin(), ordered_learnts.end(),
        [&](const std::pair<CRef, double>& x, const std::pair<CRef, double>& y) { return x.second > y.second; });
  }
  db_learnts.clear();
  for (const std::pair<CRef, double>& lrnt : ordered_learnts) db_learnts.push_back(lrnt.first);

  int64_t limit = global.options.dbScale.get() *
                  std::pow(std::log(static_cast<double>(global.stats.NCONFL.z)), global.options.dbExp.get());
  // NOTE: cast to double to avoid an issue with GCC13/14 giving NaN after std::log with -03 and single source on
  // commit 3f9584893afe38dfe30dc9c4a1322a903a231859 with SoPlex
  // NOTE 2: does not matter, we get long double errors in boost::multiprecision::cpp_int...
  // Let's go with Clang for now.
  for (size_t i = limit; i < db_learnts.size(); ++i) {
    removeConstraint(db_learnts[i]);
  }

  int64_t currentConstraints = constraints.size();
  for (int64_t i = 0; i < currentConstraints; ++i) {
    CRef cr = constraints[i];
    Constr& c = ca[cr];
    if (c.isMarkedForDelete() || external.count(c.id()) || !c.canBeSimplified(*this, global.isPool)) {
      continue;
    }
    ++global.stats.NCONSREADDED;
    CeSuper ce = expandWithSymbBound(c);
    const bool isLocked = c.isLocked();
    const uint32_t lbd = c.lbd();
    assert(lbd > 0);
    ce->strongPostProcess(*this);
    if (ce->isUnsat()) reportUnsat(ce);
    if (ce->isTautology()) {
      removeConstraint(cr, true);
      continue;
    }
    CRef crnew = attachConstraint(ce, isLocked, lbd);  // NOTE: this invalidates ce!
    if (crnew == CRef_Undef) continue;
    removeConstraint(cr, true);  // NOTE: remove after attaching a stronger version
  }

  std::vector<int> cardPoints;
  for (size_t i = limit; i < db_learnts.size(); ++i) {
    Constr& c = ca[db_learnts[i]];
    assert(c.isMarkedForDelete());
    if (!c.isClauseOrCard()) {
      CeSuper ce = expandWithSymbBound(c);
      ce->orig = Origin::REDUCED;
      ce->removeUnitsAndZeroes(getLevel(), getPos());
      if (ce->isTautology()) continue;  // possible due to further root propagations during rewriting of constraints
      ce->simplifyToCardinality(false, ce->getMaxStrengthCardinalityDegree(cardPoints));
      aux::timeCallVoid([&] { learnConstraint(ce); }, global.stats.LEARNTIME);
    }
  }

  size_t j = 0;
  for (size_t i = 0; i < constraints.size(); ++i) {
    if (Constr& c = ca[constraints[i]]; c.isMarkedForDelete()) {
      c.cleanup();  // free up indirectly owned memory before implicitly deleting c during garbage collect
    } else {
      constraints[j++] = constraints[i];
    }
  }
  constraints.resize(j);

  // shorten watches
  for (Lit l = -n; l <= n; ++l) {
    adj[l].erase(
        std::remove_if(adj[l].begin(), adj[l].end(), [&](const Watch& w) { return ca[w.cref].isMarkedForDelete(); }),
        adj[l].end());
  }

  if (static_cast<double>(ca.wasted) / static_cast<double>(ca.at) > 0.2) {
    aux::timeCallVoid([&] { garbage_collect(); }, global.stats.GCTIME);
  }
}

// ---------------------------------------------------------------------
// Solving

double Solver::luby(double y, int i) {
  // Find the finite subsequence that contains index 'i', and the
  // size of that subsequence:
  int size, seq;
  for (size = 1, seq = 0; size < i + 1; seq++, size = 2 * size + 1) {
  }
  while (size != i + 1) {
    size = (size - 1) >> 1;
    --seq;
    assert(size != 0);
    i = i % size;
  }
  return std::pow(y, seq);
}

bool Solver::checkSAT() const {
  if (!foundSolution()) return false;
  return std::all_of(constraints.cbegin(), constraints.cend(), [&](CRef cr) {
    const Constr& c = ca[cr];
    return c.getOrigin() != Origin::FORMULA || c.toExpanded(global.cePools)->isSatisfied(lastSol.value());
  });
}

void Solver::inProcess() {
  assert(decisionLevel() == 0);
  removeSatisfiedNonImpliedsAtRoot();
  if (global.options.pureLits && global.options.proofAssumps) derivePureLits();
  if (global.options.domBreakLim.get() != 0 && global.options.proofAssumps) dominanceBreaking();
  if (global.options.inpAMO.get() != 0) {
    aux::timeCallVoid([&] { runAtMostOneDetection(); }, global.stats.ATMOSTONETIME);
  }
  // TODO: timing methods should be done via wrapper methods?

#if WITHSOPLEX
  if (lpSolver && lpSolver->canInProcess()) {
    CeSuper bound = aux::timeCall<CeSuper>([&] { return lpSolver->inProcess(); }, global.stats.LPTOTALTIME);
    if (bound) lastGlobalDual = bound;
  }
#endif  // WITHSOPLEX
}

void Solver::presolve() {
  if (!firstRun) return;
  firstRun = false;
  global.logger.flush();  // flush objective and formula, no need to keep in memory

  if (unsatReached) throw UnsatEncounter();

  if (global.options.verbosity.get() > 0) std::cout << "c PRESOLVE" << std::endl;
  aux::timeCallVoid([&] { heur.randomize(getPos()); }, global.stats.HEURTIME);
  if (objectiveIsSet() && global.options.varObjective) {
    aux::timeCallVoid([&] { heur.bumpObjective(objective, getPos()); }, global.stats.HEURTIME);
  }
  aux::timeCallVoid([&] { inProcess(); }, global.stats.INPROCESSTIME);

#if WITHSOPLEX
  if (global.options.hasOnlyClausalConstraints()) {
    global.options.lpTimeRatio.set(0);  // no use having an LP solver for a clausal decision problem
  }

  if (global.options.lpTimeRatio.get() > 0) {
    lpSolver = std::make_shared<LpSolver>(*this);
    lpSolver->setObjective(objective);
    CeSuper bound = aux::timeCall<CeSuper>([&] { return lpSolver->inProcess(true); }, global.stats.LPTOTALTIME);
    // NOTE: calling LP solver here ensures it gets run once before the actual search begins
    if (bound) lastGlobalDual = bound;
  }
#endif
}

void Solver::removeSatisfiedNonImpliedsAtRoot() {
  assert(decisionLevel() == 0);
  std::vector<CRef> toCheck;
  for (int i = lastRemoveSatisfiedsTrail; i < (int)trail.size(); ++i) {
    Lit l = trail[i];
    if (!isOrig(toVar(l))) continue;  // no column view for auxiliary variables for now
    for (const auto& [cr, _] : lit2cons[l]) {
      Constr& c = ca[cr];
      assert(!c.isMarkedForDelete());  // should be erased from lit2cons when marked for delete
      if (c.isSeen()) continue;
      c.setSeen(true);
      toCheck.push_back(cr);
    }
  }
  for (const CRef& cr : toCheck) {
    Constr& c = ca[cr];
    assert(c.isSeen());
    c.setSeen(false);
    if (c.isSatisfiedAtRoot(getLevel()) &&
        external.count(c.id()) == 0) {  // upper bound constraints may yet be external
      ++global.stats.NSATISFIEDSREMOVED;
      removeConstraint(cr, true);
    }
  }
  lastRemoveSatisfiedsTrail = trail.size();
}

void Solver::derivePureLits() {
  assert(decisionLevel() == 0);
  for (Lit l = -getNbVars(); l <= getNbVars(); ++l) {
    quit::checkInterrupt(global);
    if (l == 0 || !isOrig(toVar(l)) || isKnown(getPos(), l) || objective->hasLit(l) || equalities.isPartOfEquality(l) ||
        !lit2cons[-l].empty()) {
      continue;  // NOTE: core-guided variables will not be eliminated
    }
    addUnitConstraint(l, Origin::PURE);
    removeSatisfiedNonImpliedsAtRoot();
  }
}

void Solver::dominanceBreaking() {
  removeSatisfiedNonImpliedsAtRoot();
  unordered_set<Lit> inUnsaturatableConstraint;
  IntSet& saturating = global.isPool.take();
  IntSet& intersection = global.isPool.take();
  for (Lit l = -getNbVars(); l <= getNbVars(); ++l) {
    if (l == 0 || !isOrig(toVar(l)) || isKnown(getPos(), l) || objective->hasLit(l) || equalities.isPartOfEquality(l)) {
      continue;
    }
    assert(saturating.isEmpty());
    unordered_map<CRef, int>& col = lit2cons[-l];
    if (col.empty()) {
      addUnitConstraint(l, Origin::PURE);
      removeSatisfiedNonImpliedsAtRoot();
      continue;
    }
    if ((global.options.domBreakLim.get() != -1 && std::ssize(col) >= global.options.domBreakLim.get()) ||
        std::ssize(col) >= lit2consOldSize[-l] || inUnsaturatableConstraint.count(-l)) {
      continue;
    }

    lit2consOldSize[-l] = col.size();
    Constr* first = &ca[col.cbegin()->first];
    unsigned int firstUnsatIdx = first->getUnsaturatedIdx();
    for (const auto& [cr, _] : col) {
      Constr& c = ca[cr];
      unsigned int unsatIdx = c.getUnsaturatedIdx();
      if (unsatIdx < firstUnsatIdx) {  // smaller number of starting lits
        first = &c;
        firstUnsatIdx = unsatIdx;
      }
      if (firstUnsatIdx == 0) {
        for (unsigned int i = 0; i < first->size(); ++i) {
          inUnsaturatableConstraint.insert(first->lit(i));
        }
        break;
      }
    }
    assert(!first->isMarkedForDelete());
    for (unsigned int i = 0; i < firstUnsatIdx; ++i) {
      Lit ll = first->lit(i);
      assert(!isTrue(getLevel(), ll));  // otherwise the constraint would be satisfied and hence removed at the root
      if (!isFalse(getLevel(), ll)) {
        saturating.add(first->lit(i));
      }
    }
    saturating.remove(-l);  // if l is false, then we can not pick it to be true ;)
    auto range = binaryImplicants.equal_range(-l);
    for (auto it = range.first; it != range.second; ++it) {
      saturating.remove(-it->second);  // not interested in anything that already implies l TODO: is this needed?
    }
    for (const auto& [cr, _] : col) {
      if (saturating.isEmpty()) break;
      Constr& c = ca[cr];
      unsigned int unsatIdx = c.getUnsaturatedIdx();
      if (unsatIdx == 0) {
        for (unsigned int i = 0; i < c.size(); ++i) {
          inUnsaturatableConstraint.insert(c.lit(i));
        }
      }
      assert(intersection.isEmpty());
      for (unsigned int i = 0; i < unsatIdx; ++i) {
        quit::checkInterrupt(global);
        Lit ll = c.lit(i);
        if (saturating.has(ll)) intersection.add(ll);
      }
      saturating = intersection;
      intersection.clear();
    }
    if (saturating.isEmpty()) continue;

    // saturating contains the intersection of the saturating literals for all constraints,
    // so asserting any literal in saturating makes l pure,
    // so we can add all these binary implicants as dominance breakers.
    for (Lit ll : saturating.getKeys()) {
      binaryImplicants.insert({ll, l});
      binaryImplicants.insert({-l, -ll});
      addBinaryConstraint(-ll, l, Origin::DOMBREAKER);
      removeSatisfiedNonImpliedsAtRoot();
    }
    saturating.clear();
  }
  global.isPool.release(saturating);
  global.isPool.release(intersection);
}

SolveState Solver::solve() {
  if (unsatReached) throw UnsatEncounter();
  StatNum lastPropTime = global.stats.PROPTIME.z;
  StatNum lastCATime = global.stats.CATIME.z;
  StatNum lastNProp = global.stats.NPROP.z;
  bool runLP = false;
  while (true) {
    quit::checkInterrupt(global);
    CeSuper confl = CeNull();
    if (runLP) {
      confl = aux::timeCall<CeSuper>([&] { return runPropagationWithLP(); }, global.stats.PROPTIME);
    } else {
      confl = aux::timeCall<CeSuper>([&] { return runPropagation(); }, global.stats.PROPTIME);
    }

    runLP = (bool)confl;
    if (confl) {
      assert(confl->hasNegativeSlack(level));
      ++global.stats.NCONFL;
      nconfl_to_restart--;
      const int64_t nconfl = global.stats.getNConfl();
      if (nconfl % 1000 == 0 && global.options.verbosity.get() > 0) {
        std::cout << "c " << nconfl << " confls " << constraints.size() << " constrs "
                  << getNbVars() - static_cast<int64_t>(global.stats.NUNITS.z) << " vars" << std::endl;
        if (global.options.verbosity.get() > 2) {
          // memory usage
          std::cout << "c total constraint space: " << ca.cap * 4 / 1024. / 1024. << "MB" << std::endl;
          std::cout << "c total #watches: ";
          int64_t cnt = 0;
          for (Lit l = -n; l <= n; l++) cnt += (int64_t)adj[l].size();
          std::cout << cnt << std::endl;
        }
      }
      if (decisionLevel() == 0) {
        reportUnsat(confl);  // throws  UnsatEncounter
      } else if (decisionLevel() > assumptionLevel()) {
        CeSuper analyzed = aux::timeCall<CeSuper>([&] { return analyze(confl); }, global.stats.CATIME);
        assert(analyzed);
        assert(analyzed->hasNegativeSlack(getLevel()));
        assert(analyzed->isSaturated());
        aux::timeCallVoid([&] { learnConstraint(analyzed); }, global.stats.LEARNTIME);
      } else {
        lastCore = aux::timeCall<CeSuper>([&] { return extractCore(confl); }, global.stats.CATIME);
        assert(lastCore->hasNegativeSlack(assumptions.getIndex()));
        assert(decisionLevel() == 0);
        return SolveState::INCONSISTENT;
      }
    } else {  // no conflict
      if (nconfl_to_restart <= 0) {
        backjumpTo(assumptionLevel());
        ++global.stats.NRESTARTS;
        double rest_base = luby(global.options.lubyBase.get(), static_cast<int>(global.stats.NRESTARTS.z));
        nconfl_to_restart = (int64_t)rest_base * global.options.lubyMult.get();
      }
      if (global.stats.getNConfl() >= nconfl_to_reduce) {
        ++global.stats.NCLEANUP;
        nconfl_to_reduce +=
            1 + global.options.dbScale.get() *
                    std::pow(std::log(static_cast<double>(global.stats.NCONFL.z)), global.options.dbExp.get());
        // NOTE: cast to double to avoid an issue with GCC13/14 giving NaN after std::log with -03 and single source on
        // commit 3f9584893afe38dfe30dc9c4a1322a903a231859 with SoPlex
        // NOTE 2: does not matter, we get long double errors in boost::multiprecision::cpp_int...
        // Let's go with Clang when building single source for now.
        if (global.options.verbosity.get() > 0) {
          StatNum propDiff = global.stats.PROPTIME.z - lastPropTime;
          StatNum cADiff = global.stats.CATIME.z - lastCATime;
          StatNum nPropDiff = global.stats.NPROP.z - lastNProp;
          std::cout << "c INPROCESSING " << propDiff << " proptime " << nPropDiff / propDiff << " prop/sec "
                    << propDiff / cADiff << " prop/ca" << std::endl;
          lastPropTime = global.stats.PROPTIME.z;
          lastCATime = global.stats.CATIME.z;
          lastNProp = global.stats.NPROP.z;
        }
        aux::timeCallVoid([&] { reduceDB(); }, global.stats.CLEANUPTIME);
        aux::timeCallVoid([&] { inProcess(); }, global.stats.INPROCESSTIME);
        return SolveState::INPROCESSED;
      }
      Lit next = 0;
      assert(assumptionLevel() <= decisionLevel());
      if (assumptions_lim.back() < std::ssize(assumptions)) {
        for (int i = (decisionLevel() == 0 ? 0 : trail_lim.back()); i < (int)trail.size(); ++i) {
          Lit l = trail[i];
          if (assumptions.has(-l)) {  // found conflicting assumption
            if (isUnit(level, l)) {
              backjumpTo(0);
              lastCore = getUnitClause(l);
            } else {
              lastCore = aux::timeCall<CeSuper>(
                  [&] { return extractCore(expandWithSymbBound(ca[reason[toVar(l)]]), -l); }, global.stats.CATIME);
            }
            assert(lastCore->hasNegativeSlack(assumptions.getIndex()));
            assert(decisionLevel() == 0);
            return SolveState::INCONSISTENT;
          }
        }
      }
      while (assumptions_lim.back() < std::ssize(assumptions)) {
        assert(decisionLevel() == assumptionLevel());
        Lit l_assump = assumptions.getKeys()[assumptions_lim.back()];
        assert(!isFalse(level, l_assump));  // otherwise above check should have caught this
        if (isTrue(level, l_assump)) {      // assumption already propagated
          ++assumptions_lim.back();
        } else {  // unassigned assumption
          next = l_assump;
          assumptions_lim.push_back(assumptions_lim.back() + 1);
          break;
        }
      }
      if (next == 0) {
        next = aux::timeCall<Lit>([&] { return heur.pickBranchLit(getPos(), coreguided); }, global.stats.HEURTIME);
      }
      if (next == 0) {
        assert((int)trail.size() == getNbVars());
        if (!lastSol.has_value()) lastSol = LitVec();
        lastSol.value().resize(getNbVars() + 1);
        lastSol.value()[0] = 0;
        if (getOptions().varSol) {
          for (Var v = 1; v <= getNbVars(); ++v) {
            Lit image = isTrue(level, v) ? v : -v;
            lastSol.value()[v] = image;
            heur.setFixedPhase(v, image);
          }
        } else {
          for (Var v = 1; v <= getNbVars(); ++v) lastSol.value()[v] = isTrue(level, v) ? v : -v;
        }
        assert(checkSAT());
        backjumpTo(0);
        return SolveState::SAT;
      }
      assert(next != 0);
      if (global.options.inpProbing && decisionLevel() == 0 && toVar(lastRestartNext) != toVar(next)) {
        aux::timeCallVoid([&] { return probeRestart(next); }, global.stats.PROBETIME);
        // continue potentially without deciding variable if unit literals were found
        // NOTE: unit literals may contradict assumptions, so impossible to just decide next here
      } else {
        assert(!assumptions.has(-next));  // do not decide opposite of assumptions
        decide(next);
        assert(isKnown(getPos(), next));
      }
    }
  }
}

void Solver::probeRestart(Lit next) {
  assert(decisionLevel() == 0);
  assert(isUnknown(getPos(), next));
  lastRestartNext = toVar(next);
  int oldUnits = trail.size();
  State state = probe(-next, true);
  if (state == State::SUCCESS) {
    IntSet& trailSet = global.isPool.take();
    for (int i = trail_lim[0] + 1; i < (int)trail.size(); ++i) {
      trailSet.add(trail[i]);
    }
    backjumpTo(0);
    LitVec newUnits;
    State state2 = probe(next, true);
    if (state2 == State::SUCCESS) {
      for (int i = trail_lim[0] + 1; i < (int)trail.size(); ++i) {
        Lit l = trail[i];
        if (trailSet.has(l)) {
          newUnits.push_back(l);
        } else if (trailSet.has(-l)) {
          equalities.merge(next, l);
        }
      }
      if (!newUnits.empty()) {
        backjumpTo(0);
        for (Lit l : newUnits) {
          assert(!isUnit(getLevel(), -l));
          if (!isUnit(getLevel(), l)) {
            aux::timeCallVoid([&] { learnUnitConstraint(l, Origin::PROBING, global.logger.logImpliedUnit(next, l)); },
                              global.stats.LEARNTIME);
          }
        }
      }
    }
    global.isPool.release(trailSet);
  }
  // at this point, either we learned unit literals and are at decision level 0
  // or we did not and are at decision level 1 having decided next
  assert(decisionLevel() == 1 || (decisionLevel() == 0 && std::ssize(trail) > oldUnits));
  global.stats.NPROBINGLITS += (decisionLevel() == 0 ? trail.size() : trail_lim[0]) - oldUnits;
  assert(assumptionLevel() == 0);
  if (decisionLevel() == 1 && assumptions_lim.back() < std::ssize(assumptions)) {
    assert(assumptions.getKeys()[assumptions_lim.back()] == next);
    assumptions_lim.push_back(assumptions_lim.back() + 1);
    // repair assumptions_lim
  }
}

void Solver::detectAtMostOne(Lit seed, unordered_set<Lit>& considered, LitVec& previousProbe) {
  assert(decisionLevel() == 0);
  DetTime currentDetTime = global.stats.getDetTime();
  DetTime oldDetTime = currentDetTime;
  if (considered.count(seed) || isKnown(getPos(), seed) || probe(-seed, true) == State::FAIL) {
    currentDetTime = global.stats.getDetTime();
    global.stats.ATMOSTONEDETTIME += currentDetTime - oldDetTime;
    return;  // if probe fails, found unit literals instead
  }

  // find candidates
  LitVec candidates = {};
  assert(decisionLevel() == 1);
  candidates.reserve(trail.size() - trail_lim[0]);
  for (int i = trail_lim[0] + 1; i < (int)trail.size(); ++i) {
    candidates.push_back(trail[i]);
  }
  backjumpTo(0);

  if (!previousProbe.empty()) {
    IntSet& previous = global.isPool.take();
    for (Lit l : previousProbe) {
      previous.add(l);
    }
    for (Lit l : candidates) {
      if (previous.has(l) && isUnknown(getPos(), l)) {
        assert(decisionLevel() == 0);
        aux::timeCallVoid([&] { learnUnitConstraint(l, Origin::PROBING, global.logger.logImpliedUnit(seed, l)); },
                          global.stats.LEARNTIME);
      } else if (previous.has(-l)) {
        equalities.merge(-seed, l);
      }
    }
    global.isPool.release(previous);
  } else {
    previousProbe = candidates;
  }

  // check whether at least three of them form a clique
  LitVec cardLits = {seed};  // clique so far
  boost::sort::pdqsort(candidates.begin(), candidates.end(), [&](Lit x, Lit y) {
    return getHeuristic().getActivity(toVar(x)) < getHeuristic().getActivity(toVar(y));
  });
  assert(candidates.size() <= 1 ||
         getHeuristic().getActivity(toVar(candidates[0])) <= getHeuristic().getActivity(toVar(candidates[1])));
  IntSet& trailSet = global.isPool.take();
  while (!candidates.empty()) {
    quit::checkInterrupt(global);
    currentDetTime = global.stats.getDetTime();
    global.stats.ATMOSTONEDETTIME += currentDetTime - oldDetTime;
    if (global.options.inpAMO.get() != 1 &&
        global.stats.ATMOSTONEDETTIME >=
            global.options.inpAMO.get() * std::max(global.options.basetime.get(), currentDetTime)) {
      break;
    }
    oldDetTime = currentDetTime;
    assert(decisionLevel() == 0);
    Lit current = candidates.back();
    candidates.pop_back();
    if (isKnown(getPos(), current)) continue;
    if (State state = probe(-current, false); state == State::FAIL) continue;
    trailSet.clear();
    for (int i = trail_lim[0] + 1; i < (int)trail.size(); ++i) {
      trailSet.add(trail[i]);
    }
    backjumpTo(0);
    for (Lit l : cardLits) {
      if (trailSet.has(-l) && isUnknown(getPos(), l)) {
        assert(decisionLevel() == 0);
        aux::timeCallVoid([&] { learnUnitConstraint(l, Origin::PROBING, global.logger.logImpliedUnit(l, current)); },
                          global.stats.LEARNTIME);
      }
    }
    if (std::any_of(candidates.begin(), candidates.end(), [&](Lit l) { return trailSet.has(l); })) {
      cardLits.push_back(current);  // found an additional cardinality lit
      candidates.erase(std::remove_if(candidates.begin(), candidates.end(), [&](Lit l) { return !trailSet.has(l); }),
                       candidates.end());
    }
  }
  global.isPool.release(trailSet);
  plf::reorderase_all_if(cardLits, cardLits.begin(), cardLits.end(), [&](Lit l) { return isUnit(getLevel(), -l); });
  for (Lit l : cardLits) {
    considered.insert(l);
  }
  if (cardLits.size() > 2) {
    uint64_t hash = aux::hashForSet<Lit>(cardLits);
    if (auto bestsize = atMostOneHashes.find(hash);
        bestsize != atMostOneHashes.end() && bestsize->second >= cardLits.size()) {
      currentDetTime = global.stats.getDetTime();
      global.stats.ATMOSTONEDETTIME += currentDetTime - oldDetTime;
      return;
    }
    atMostOneHashes[hash] = cardLits.size();
    ConstrSimple32 card{{}, (int)cardLits.size() - 1};
    for (Lit l : cardLits) {
      card.terms.push_back({1, l});
    }
    ++global.stats.ATMOSTONES;
    CeSuper ce = card.toExpanded(global.cePools);
    ce->orig = Origin::DETECTEDAMO;
    global.logger.logAtMostOne(card, ce);
    aux::timeCallVoid([&] { learnConstraint(ce); }, global.stats.LEARNTIME);
  }
  currentDetTime = global.stats.getDetTime();
  global.stats.ATMOSTONEDETTIME += currentDetTime - oldDetTime;
}

void Solver::runAtMostOneDetection() {
  assert(decisionLevel() == 0);
  int currentUnits = trail.size();
  LitVec previous;
  unordered_set<Lit> considered;
  Lit next = heur.firstInActOrder();
  while (next != 0 &&
         (global.options.inpAMO.get() == 1 ||
          global.stats.ATMOSTONEDETTIME <
              global.options.inpAMO.get() * std::max(global.options.basetime.get(), global.stats.getDetTime()))) {
    previous.clear();
    detectAtMostOne(-next, considered, previous);
    detectAtMostOne(next, considered, previous);
    next = heur.nextInActOrder(next);
  }
  global.stats.NATMOSTONEUNITS += trail.size() - currentUnits;
}

}  // namespace xct
