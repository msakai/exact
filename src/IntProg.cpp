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

#include "IntProg.hpp"
#include <stdexcept>
#include "Optimization.hpp"

namespace xct {

Encoding opt2enc(const std::string& opt) {
  assert(opt == "order" || opt == "log" || opt == "onehot");
  return opt == "order" ? Encoding::ORDER : opt == ("log") ? Encoding::LOG : Encoding::ONEHOT;
}

std::ostream& operator<<(std::ostream& o, const IntVar& x) {
  return o << x.getName() << "[" << x.getLowerBound() << "," << x.getUpperBound() << "]";
}
std::ostream& operator<<(std::ostream& o, IntVar* x) { return o << *x; }
std::ostream& operator<<(std::ostream& o, const IntTerm& x) {
  return o << (x.c < 0 ? "" : "+") << (x.c == 1 ? "" : aux::str(x.c) + "*") << *x.v;
}
void lhs2str(std::ostream& o, const IntConstraint& x) {
  std::vector<std::string> terms;
  terms.reserve(x.lhs.size());
  for (const IntTerm& t : x.lhs) {
    terms.push_back(aux::str(t));
  }
  std::sort(terms.begin(), terms.end());
  for (const std::string& s : terms) o << s << " ";
}
std::ostream& operator<<(std::ostream& o, const IntConstraint& x) {
  if (x.upperBound.has_value()) o << x.upperBound.value() << " >= ";
  lhs2str(o, x);
  if (x.lowerBound.has_value()) o << ">= " << x.lowerBound.value();
  return o;
}

LitVec val2lits(IntVar* iv, const bigint& val) {
  const VarVec& encoding = iv->getEncodingVars();
  LitVec res;
  res.reserve(encoding.size());
  if (iv->getEncoding() == Encoding::LOG) {
    bigint value = val - iv->getLowerBound();
    assert(value >= 0);
    for (Var v : encoding) {
      res.push_back(value % 2 == 0 ? -v : v);
      value /= 2;
    }
    assert(value == 0);
    return res;
  }
  assert(val - iv->getLowerBound() <= iv->getEncodingVars().size());
  int val_int = static_cast<int>(val - iv->getLowerBound());
  if (iv->getEncoding() == Encoding::ONEHOT) {
    for (int i = 0; i < (int)encoding.size(); ++i) {
      res.push_back(i == val_int ? encoding[i] : -encoding[i]);
    }
    return res;
  }
  assert(iv->getEncoding() == Encoding::ORDER);
  for (int i = 0; i < (int)encoding.size(); ++i) {
    res.push_back(i < val_int ? encoding[i] : -encoding[i]);
  }
  return res;
}

void log2assumptions(const VarVec& encoding, const bigint& value, const bigint& lowerbound, xct::IntSet& assumptions) {
  bigint val = value - lowerbound;
  assert(val >= 0);
  for (Var v : encoding) {
    assumptions.add(val % 2 == 0 ? -v : v);
    val /= 2;
  }
  assert(val == 0);
}

IntVar::IntVar(const std::string& n, Solver& solver, bool nameAsId, const bigint& lb, const bigint& ub, Encoding e)
    : name(n), lowerBound(lb), upperBound(ub), encoding(getRange() <= 1 ? Encoding::ORDER : e) {
  assert(lb <= ub);

  if (nameAsId) {
    assert(isBoolean());
    Var next = std::stoi(getName());
    solver.setNbVars(next, true);
    encodingVars.emplace_back(next);
  } else {
    const bigint range = getRange();
    assert(range > 1 || encoding == Encoding::ORDER);
    int oldvars = solver.getNbVars();
    int newvars = oldvars + (encoding == Encoding::LOG
                                 ? aux::msb(range) + 1  // NOTE: msb is 0-based, so we add another bit
                                 : static_cast<int>(range) + static_cast<int>(encoding == Encoding::ONEHOT));

    solver.setNbVars(newvars, true);
    for (Var var = oldvars + 1; var <= newvars; ++var) {
      encodingVars.emplace_back(var);
    }
    if (encoding == Encoding::LOG) {  // upper bound constraint
      assert(!encodingVars.empty());
      ConstrSimpleArb csa({}, -range);
      csa.terms.reserve(encodingVars.size());
      csa.orig = Origin::FORMULA;
      bigint base = -1;
      for (const Var v : encodingVars) {
        csa.terms.emplace_back(base, v);
        base *= 2;
      }
      // NOTE: last variable could have a smaller coefficient if the range is not a nice power of two - 1
      // This would actually increase the number of solutions to the constraint. It would also not guarantee that each
      // value for an integer variable had a unique Boolean representation. Bad idea probably.
      solver.addConstraint(csa);
    } else if (encoding == Encoding::ORDER) {
      assert(!encodingVars.empty() || range == 0);
      for (Var var = oldvars + 1; var < solver.getNbVars(); ++var) {
        solver.addBinaryConstraint(var, -(var + 1), Origin::FORMULA);
      }
    } else {
      assert(!encodingVars.empty());
      assert(encoding == Encoding::ONEHOT);
      ConstrSimple32 cs1({}, 1);
      cs1.terms.reserve(encodingVars.size());
      cs1.orig = Origin::FORMULA;
      ConstrSimple32 cs2({}, -1);
      cs2.terms.reserve(encodingVars.size());
      cs2.orig = Origin::FORMULA;
      for (int var = oldvars + 1; var <= solver.getNbVars(); ++var) {
        cs1.terms.emplace_back(1, var);
        cs2.terms.emplace_back(-1, var);
      }
      solver.addConstraint(cs1);
      solver.addConstraint(cs2);
    }
  }
}

bigint IntVar::getValue(const LitVec& sol) const {
  bigint val = getLowerBound();
  if (encoding == Encoding::LOG) {
    bigint base = 1;
    for (Var v : getEncodingVars()) {
      assert(v != 0);
      assert(v < (int)sol.size());
      assert(toVar(sol[v]) == v);
      if (sol[v] > 0) val += base;
      base *= 2;
    }
  } else if (encoding == Encoding::ORDER) {
    int sum = 0;
    for (Var v : getEncodingVars()) {
      assert(v < (int)sol.size());
      assert(toVar(sol[v]) == v);
      sum += sol[v] > 0;
    }
    val += sum;
  } else {
    assert(encoding == Encoding::ONEHOT);
    int ith = 0;
    for (Var v : getEncodingVars()) {
      assert(v < (int)sol.size());
      assert(toVar(sol[v]) == v);
      if (sol[v] > 0) {
        val += ith;
        break;
      }
      ++ith;
    }
  }
  return val;
}

IntTerm::IntTerm(const bigint& _c, IntVar* _v) : c(_c), v(_v) {}

Core emptyCore() { return std::make_unique<unordered_set<IntVar*>>(); }

std::vector<IntTerm> IntConstraint::zip(const std::vector<bigint>& coefs, const std::vector<IntVar*>& vars) {
  assert(coefs.size() == vars.size());
  std::vector<IntTerm> res;
  res.reserve(coefs.size());
  for (int i = 0; i < (int)coefs.size(); ++i) {
    res.push_back({coefs[i], vars[i]});
  }
  return res;
}

bigint IntConstraint::getRange() const {
  bigint res = 0;
  for (const IntTerm& t : lhs) {
    assert(t.v->getRange() >= 0);
    res += aux::abs(t.c) * t.v->getRange();
  }
  return res;
}

int64_t IntConstraint::size() const { return std::ssize(lhs); }

void IntConstraint::invert() {
  if (lowerBound) lowerBound = -lowerBound.value();
  if (upperBound) upperBound = -upperBound.value();
  for (IntTerm& it : lhs) it.c = -it.c;
}

void IntConstraint::toConstrExp(CeArb& input, bool useLowerBound) const {
  input->orig = Origin::FORMULA;
  if (useLowerBound) {
    assert(lowerBound.has_value());
    input->addRhs(lowerBound.value());
  } else {
    assert(upperBound.has_value());
    input->addRhs(upperBound.value());
  }
  for (const IntTerm& t : lhs) {
    if (t.c == 0) continue;
    if (t.v->getLowerBound() != 0) input->addRhs(-t.c * t.v->getLowerBound());
    if (t.v->getEncoding() == Encoding::LOG) {
      assert(!t.v->getEncodingVars().empty());
      bigint base = 1;
      for (const Var v : t.v->getEncodingVars()) {
        input->addLhs(base * t.c, v);
        base *= 2;
      }
    } else if (t.v->getEncoding() == Encoding::ORDER) {
      assert(t.v->getRange() == 0 || !t.v->getEncodingVars().empty());
      for (const Var v : t.v->getEncodingVars()) {
        input->addLhs(t.c, v);
      }
    } else {
      assert(t.v->getEncoding() == Encoding::ONEHOT);
      assert(!t.v->getEncodingVars().empty());
      int ith = 0;
      for (const Var v : t.v->getEncodingVars()) {
        input->addLhs(ith * t.c, v);
        ++ith;
      }
    }
  }
  if (!useLowerBound) input->invert();
}

IntProg::IntProg(const Options& opts, bool keepIn)
    : global(opts), obj_denominator(1), solver(global), keepInput(keepIn) {
  global.stats.startTime = std::chrono::steady_clock::now();
  aux::rng::seed = global.options.randomSeed.get();
  global.logger.activate(global.options.proofLog.get(), (bool)global.options.proofZip);
  setObjective({}, true, {});
}

const Solver& IntProg::getSolver() const { return solver; }
Solver& IntProg::getSolver() { return solver; }
const Optim& IntProg::getOptim() const { return optim; }
void IntProg::setInputVarLimit() { inputVarLimit = solver.getNbVars(); }
int IntProg::getInputVarLimit() const { return inputVarLimit; }

IntVar* IntProg::addVar(const std::string& name, const bigint& lowerbound, const bigint& upperbound, Encoding encoding,
                        bool nameAsId) {
  assert(!getVarFor(name));
  if (upperbound < lowerbound) {
    throw InvalidArgument((std::stringstream() << "Upper bound " << upperbound << " of " << name
                                               << " is smaller than lower bound " << lowerbound)
                              .str());
  }
  vars.push_back(std::make_unique<IntVar>(name, solver, nameAsId, lowerbound, upperbound, encoding));
  IntVar* iv = vars.back().get();
  name2var.insert({name, iv});
  for (Var v : iv->getEncodingVars()) {
    var2var.insert({v, iv});
  }
  return iv;
}

IntVar* IntProg::getVarFor(const std::string& name) const {
  if (auto it = name2var.find(name); it != name2var.end()) return it->second;
  return nullptr;
}

std::vector<IntVar*> IntProg::getVariables() const {
  return aux::comprehension(name2var, [](auto pair) { return pair.second; });
}

void IntProg::setObjective(const std::vector<IntTerm>& terms, bool min, const bigint& offset) {
  // TODO: pass IntConstraint instead of terms?
  obj = {terms, -offset};
  minimize = min;
  if (!min) obj.invert();
  optim = OptimizationSuper::make(obj, solver, assumptions);
}
IntConstraint& IntProg::getObjective() { return obj; }
const IntConstraint& IntProg::getObjective() const { return obj; }

void IntProg::addSingleAssumption(IntVar* iv, const bigint& val) {
  if (iv->getEncoding() == Encoding::LOG) {
    log2assumptions(iv->getEncodingVars(), val, iv->getLowerBound(), assumptions);
  } else {
    assert(val - iv->getLowerBound() <= iv->getEncodingVars().size());
    int val_int = static_cast<int>(val - iv->getLowerBound());
    if (iv->getEncoding() == Encoding::ORDER) {
      if (val_int > 0) {
        assumptions.add(iv->getEncodingVars()[val_int - 1]);
      }
      if (val_int < (int)iv->getEncodingVars().size()) {
        assumptions.add(-iv->getEncodingVars()[val_int]);
      }
    } else {
      assert(iv->getEncoding() == Encoding::ONEHOT);
      assumptions.add(iv->getEncodingVars()[val_int]);
    }
  }
}

void IntProg::setAssumptions(const std::vector<std::pair<IntVar*, std::vector<bigint>>>& ivs) {
  for (auto [iv, dom] : ivs) {
    assert(iv);
    if (dom.empty()) {
      throw InvalidArgument("No possible values given when setting assumptions for " + iv->getName() + ".");
    }
    for (const bigint& vals_i : dom) {
      if (vals_i < iv->getLowerBound() || vals_i > iv->getUpperBound())
        throw InvalidArgument("Assumption value " + aux::str(vals_i) + " for " + iv->getName() +
                              " exceeds variable bounds.");
    }
    for (Var v : iv->getEncodingVars()) {
      assumptions.remove(v);
      assumptions.remove(-v);
    }
    if (dom.size() == 1) {
      addSingleAssumption(iv, dom[0]);
    } else {
      unordered_set<bigint> toCheck(dom.begin(), dom.end());
      if (toCheck.size() == iv->getUpperBound() - iv->getLowerBound() + 1) continue;
      if (iv->getEncoding() != Encoding::ONEHOT) {
        throw InvalidArgument("Variable " + iv->getName() + " is not one-hot encoded but has " +
                              std::to_string(toCheck.size()) +
                              " (more than one and less than its range) values to assume.");
      }
      bigint val = iv->getLowerBound();
      for (Var v : iv->getEncodingVars()) {
        if (!toCheck.count(val)) {
          assumptions.add(-v);
        }
        ++val;
      }
    }
  }
  optim = OptimizationSuper::make(obj, solver, assumptions);
}

void IntProg::setAssumptions(const std::vector<std::pair<IntVar*, bigint>>& ivs) {
  for (auto [iv, val] : ivs) {
    assert(iv);
    if (val < iv->getLowerBound() || val > iv->getUpperBound())
      throw InvalidArgument("Assumption value " + aux::str(val) + " for " + iv->getName() +
                            " exceeds variable bounds.");
    for (Var v : iv->getEncodingVars()) {
      assumptions.remove(v);
      assumptions.remove(-v);
    }
    addSingleAssumption(iv, val);
  }
  optim = OptimizationSuper::make(obj, solver, assumptions);
}

bool IntProg::hasAssumption(IntVar* iv) const {
  return std::any_of(iv->getEncodingVars().begin(), iv->getEncodingVars().end(),
                     [&](Var v) { return assumptions.has(v) || assumptions.has(-v); });
}
std::vector<bigint> IntProg::getAssumption(IntVar* iv) const {
  if (!hasAssumption(iv)) {
    std::vector<bigint> res;
    res.reserve(size_t(iv->getUpperBound() - iv->getLowerBound() + 1));
    for (bigint i = iv->getLowerBound(); i <= iv->getUpperBound(); ++i) {
      res.push_back(i);
    }
    return res;
  }
  assert(hasAssumption(iv));
  if (iv->getEncoding() == Encoding::LOG) {
    bigint val = iv->getLowerBound();
    bigint base = 1;
    for (const Var& v : iv->getEncodingVars()) {
      if (assumptions.has(v)) val += base;
      base *= 2;
    }
    return {val};
  } else if (iv->getEncoding() == Encoding::ORDER) {
    int i = 0;
    for (const Var& v : iv->getEncodingVars()) {
      if (assumptions.has(-v)) break;
      ++i;
    }
    return {iv->getLowerBound() + i};
  }
  assert(iv->getEncoding() == Encoding::ONEHOT);
  std::vector<bigint> res;
  int i = 0;
  for (const Var& v : iv->getEncodingVars()) {
    if (assumptions.has(v)) {
      return {iv->getLowerBound() + i};
    }
    if (!assumptions.has(-v)) {
      res.emplace_back(iv->getLowerBound() + i);
    }
    ++i;
  }
  return res;
}

void IntProg::clearAssumptions() {
  assumptions.clear();
  optim = OptimizationSuper::make(obj, solver, assumptions);
}
void IntProg::clearAssumptions(const std::vector<IntVar*>& ivs) {
  for (IntVar* iv : ivs) {
    for (Var v : iv->getEncodingVars()) {
      assumptions.remove(v);
      assumptions.remove(-v);
    }
  }
  optim = OptimizationSuper::make(obj, solver, assumptions);
}

void IntProg::setSolutionHints(const std::vector<std::pair<IntVar*, bigint>>& hnts) {
  std::vector<std::pair<Var, Lit>> hints;
  for (const std::pair<IntVar*, bigint>& hnt : hnts) {
    assert(hnt.first);
    assert(hnt.second >= hnt.first->getLowerBound());
    assert(hnt.second <= hnt.first->getUpperBound());
    for (Lit l : val2lits(hnt.first, hnt.second)) {
      assert(l != 0);
      hints.emplace_back(toVar(l), l);
    }
  }
  solver.fixPhase(hints, true);
}
void IntProg::clearSolutionHints(const std::vector<IntVar*>& ivs) {
  std::vector<std::pair<Var, Lit>> hints;
  for (IntVar* iv : ivs) {
    for (const Var& v : iv->getEncodingVars()) {
      hints.emplace_back(v, 0);
    }
  }
  solver.fixPhase(hints);
}

void IntProg::addConstraint(const IntConstraint& ic) {
  if (ic.size() > 1e9) throw InvalidArgument("Constraint has more than 1e9 terms.");
  ++nConstrs;
  if (keepInput) constraints.push_back(ic);
  if (ic.lowerBound.has_value()) {
    CeArb input = global.cePools.takeArb();
    ic.toConstrExp(input, true);
    solver.addConstraint(input);
  }
  if (ic.upperBound.has_value()) {
    CeArb input = global.cePools.takeArb();
    ic.toConstrExp(input, false);
    solver.addConstraint(input);
  }
}

// head <=> rhs -- head iff rhs
void IntProg::addReification(IntVar* head, bool sign, const IntConstraint& ic) {
  addLeftReification(head, sign, ic);
  addRightReification(head, sign, ic);
}

// head => rhs -- head implies rhs
void IntProg::addRightReification(IntVar* head, bool sign, const IntConstraint& ic) {
  if (ic.size() >= 1e9) throw InvalidArgument("Reification has more than 1e9 terms.");
  if (!head->isBoolean()) throw InvalidArgument("Head of reification is not Boolean.");

  ++nConstrs;
  if (keepInput) reifications.push_back({head, sign, false, ic});

  bool run[2] = {ic.upperBound.has_value(), ic.lowerBound.has_value()};
  for (int i = 0; i < 2; ++i) {
    if (!run[i]) continue;
    CeArb leq = global.cePools.takeArb();
    ic.toConstrExp(leq, i);
    leq->postProcess(solver.getLevel(), solver.getPos(), solver.getHeuristic(), true, global.stats);

    Var h = head->getEncodingVars()[0];
    leq->addLhs(leq->degree, sign ? -h : h);
    solver.addConstraint(leq);
  }
}

// head <= rhs -- rhs implies head
void IntProg::addLeftReification(IntVar* head, bool sign, const IntConstraint& ic) {
  if (ic.size() >= 1e9) throw InvalidArgument("Reification has more than 1e9 terms.");
  if (!head->isBoolean()) throw InvalidArgument("Head of reification is not Boolean.");

  ++nConstrs;
  if (keepInput) reifications.push_back({head, sign, true, ic});

  bool run[2] = {ic.upperBound.has_value(), ic.lowerBound.has_value()};
  for (int i = 0; i < 2; ++i) {
    if (!run[i]) continue;
    CeArb geq = global.cePools.takeArb();
    ic.toConstrExp(geq, i);
    geq->postProcess(solver.getLevel(), solver.getPos(), solver.getHeuristic(), true, global.stats);

    Var h = head->getEncodingVars()[0];
    geq->addRhs(-1);
    geq->invert();
    geq->addLhs(geq->degree, sign ? h : -h);
    solver.addConstraint(geq);
  }
}

void IntProg::addMultiplication(const std::vector<IntVar*>& factors, IntVar* lower_bound, IntVar* upper_bound) {
  if (!lower_bound && !upper_bound) return;
  if (factors.empty()) {
    if (lower_bound) addConstraint({{{1, lower_bound}}, std::nullopt, 1});
    if (upper_bound) addConstraint({{{1, upper_bound}}, 1});
    return;
  }
  if (factors.size() == 1) {
    if (lower_bound) addConstraint({{{1, factors.back()}, {-1, lower_bound}}, 0});
    if (upper_bound) addConstraint({{{1, factors.back()}, {-1, upper_bound}}, std::nullopt, 0});
    return;
  }

  ++nConstrs;
  if (keepInput) {
    multiplications.push_back(factors);
    multiplications.back().reserve(factors.size() + 2);
    multiplications.back().push_back(lower_bound);
    multiplications.back().push_back(upper_bound);
  }

  std::vector<std::pair<bigint, VarVec>> terms = {{1, {}}};
  std::vector<std::pair<bigint, VarVec>> terms_new;
  for (IntVar* f : factors) {
    assert(terms_new.empty());
    terms_new.reserve(terms.size());
    for (const std::pair<bigint, VarVec>& t : terms) {
      if (f->getLowerBound() != 0) terms_new.emplace_back(f->getLowerBound() * t.first, t.second);
      if (f->getRange() == 0) continue;
      if (f->getEncoding() == Encoding::LOG) {
        bigint base = 1;
        for (Var v : f->getEncodingVars()) {
          terms_new.emplace_back(base * t.first, t.second);
          terms_new.back().second.push_back(v);
          base *= 2;
        }
      } else if (f->getEncoding() == Encoding::ONEHOT) {
        for (int64_t i = 1; i < (int64_t)f->getEncodingVars().size(); ++i) {
          terms_new.emplace_back(i * t.first, t.second);
          terms_new.back().second.push_back(f->getEncodingVars()[i]);
        }
      } else {
        assert(f->getEncoding() == Encoding::ORDER);
        for (Var v : f->getEncodingVars()) {
          terms_new.emplace_back(t.first, t.second);
          terms_new.back().second.push_back(v);
        }
      }
    }
    terms = std::move(terms_new);
  }

  LitVec clause;
  std::vector<TermArb> lhs;
  lhs.reserve(terms.size());
  for (std::pair<bigint, VarVec>& t : terms) {
    assert(t.first != 0);
    std::sort(t.second.begin(), t.second.end());
    Var aux;
    if (multAuxs.contains(t.second)) {
      aux = multAuxs[t.second];
    } else {
      aux = solver.addVar(true);
      multAuxs[t.second] = aux;
    }
    lhs.emplace_back(t.first, aux);
    clause.clear();
    clause.emplace_back(aux);
    for (Var v : t.second) {
      clause.emplace_back(-v);
      solver.addBinaryConstraint(-aux, v, Origin::FORMULA);
    }
    solver.addClauseConstraint(clause, Origin::FORMULA);
  }

  std::array bounds = {lower_bound, upper_bound};
  for (int j = 0; j < 2; ++j) {
    IntVar* iv = bounds[j];
    if (!iv) continue;
    CeArb ca = global.cePools.takeArb();
    ca->orig = Origin::FORMULA;
    ca->addRhs(iv->getLowerBound());
    for (const TermArb& ta : lhs) {
      ca->addLhs(ta.c, ta.l);
    }
    if (lower_bound->getEncoding() == Encoding::LOG) {
      bigint base = -1;
      for (Var v : lower_bound->getEncodingVars()) {
        ca->addLhs(base, v);
        base *= 2;
      }
    } else if (lower_bound->getEncoding() == Encoding::ONEHOT) {
      for (int64_t i = 1; i < (int64_t)lower_bound->getEncodingVars().size(); ++i) {
        ca->addLhs(-i, lower_bound->getEncodingVars()[i]);
      }
    } else {
      assert(lower_bound->getEncoding() == Encoding::ORDER);
      for (Var v : lower_bound->getEncodingVars()) {
        ca->addLhs(-1, v);
      }
    }
    if (j > 0) ca->invert();
    solver.addConstraint(ca);
  }
}

void IntProg::fix(IntVar* iv, const bigint& val) { addConstraint(IntConstraint{{{1, iv}}, val, val}); }

void IntProg::invalidateLastSol() {
  if (!solver.foundSolution()) throw InvalidArgument("No solution to add objective bound.");

  VarVec vars;
  vars.reserve(name2var.size());
  for (const auto& tup : name2var) {
    aux::appendTo(vars, tup.second->getEncodingVars());
  }
  solver.invalidateLastSol(vars);
}

void IntProg::invalidateLastSol(const std::vector<IntVar*>& ivs, Var flag) {
  if (!solver.foundSolution()) throw InvalidArgument("No solution to add objective bound.");

  VarVec vars;
  vars.reserve(ivs.size() + (flag != 0));
  for (IntVar* iv : ivs) {
    aux::appendTo(vars, iv->getEncodingVars());
  }
  if (flag != 0) {
    vars.push_back(flag);
  }
  solver.invalidateLastSol(vars);
}

void IntProg::printFormula() { printFormula(std::cout); }

std::ostream& IntProg::printFormula(std::ostream& out) {
  int nbConstraints = 0;
  for (const CRef& cr : solver.getRawConstraints()) {
    const Constr& c = solver.getCA()[cr];
    nbConstraints += isNonImplied(c.getOrigin());
  }
  out << "* #variable= " << solver.getNbVars() << " #constraint= " << nbConstraints << "\n";
  if (assumptions.size() != 0) {
    out << "* #assumptions=";
    for (Lit l : assumptions.getKeys()) {
      out << (l < 0 ? " ~x" : " x") << toVar(l);
    }
    out << "\n";
  }
  if (!optim->getOrigObj()->empty()) {
    out << "min: ";
    optim->getOrigObj()->toStreamAsOPBlhs(out, true);
    out << ";\n";
  }
  for (Lit l : solver.getUnits()) {
    out << std::pair<int, Lit>{1, l} << " >= 1 ;\n";
  }
  for (Var v = 1; v <= solver.getNbVars(); ++v) {
    if (solver.getEqualities().isCanonical(v)) continue;
    out << std::pair<int, Lit>{1, v} << " " << std::pair<int, Lit>{-1, solver.getEqualities().getRepr(v).l}
        << " = 0 ;\n";
  }
  for (const CRef& cr : solver.getRawConstraints()) {
    const Constr& c = solver.getCA()[cr];
    if (isNonImplied(c.getOrigin())) {
      CeSuper ce = c.toExpanded(global.cePools);
      ce->toStreamAsOPB(out);
      out << "\n";
    }
  }
  return out;
}

std::ostream& IntProg::printInput(std::ostream& out) const {
  out << "OBJ ";
  if (minimize) {
    out << "MIN ";
    lhs2str(out, obj);
  } else {
    out << "MAX ";
    IntConstraint tmpObj = obj;
    tmpObj.invert();
    lhs2str(out, tmpObj);
  }
  out << std::endl;

  std::vector<std::string> strs;
  for (const auto& pr : reifications) {
    std::stringstream ss;
    ss << (pr.sign ? "!" : "") << *pr.head << (pr.left ? " <- " : " -> ") << pr.body;
    strs.push_back(ss.str());
  }
  std::sort(strs.begin(), strs.end());
  for (const std::string& s : strs) out << s << std::endl;

  strs.clear();
  for (const IntConstraint& ic : constraints) {
    strs.push_back(aux::str(ic));
  }
  std::sort(strs.begin(), strs.end());
  for (const std::string& s : strs) out << s << std::endl;

  strs.clear();
  for (const std::vector<IntVar*>& m : multiplications) {
    assert(m.size() > 3);  // at least two factors and two bounds
    std::stringstream ss;
    IntVar* lower_bound = m.at(m.size() - 2);
    IntVar* upper_bound = m.at(m.size() - 1);
    if (lower_bound) ss << lower_bound << " =< ";
    ss << "1";
    for (int64_t i = 0; i < (int64_t)m.size() - 2; ++i) {
      ss << "*" << m[i];
    }
    if (upper_bound) ss << " =< " << upper_bound;
    strs.push_back(ss.str());
  }
  std::sort(strs.begin(), strs.end());
  for (const std::string& s : strs) out << s << std::endl;

  return out;
}

std::ostream& IntProg::printVars(std::ostream& out) const {
  for (const auto& v : vars) {
    out << *v << std::endl;
  }
  return out;
}

long long IntProg::getNbVars() const { return std::ssize(vars); }

long long IntProg::getNbConstraints() const { return nConstrs; }

bigint IntProg::getLowerBound() const { return minimize ? optim->getLowerBound() : -optim->getLowerBound(); }
bigint IntProg::getUpperBound() const { return minimize ? optim->getUpperBound() : -optim->getUpperBound(); }
ratio IntProg::getUpperBoundRatio() const { return ratio{getUpperBound(), obj_denominator}; }

bool IntProg::hasLastSolution() const { return solver.foundSolution(); }

bigint IntProg::getLastSolutionFor(IntVar* iv) const {
  if (!solver.foundSolution()) throw InvalidArgument("No solution to return.");
  return iv->getValue(solver.getLastSolution());
}

std::vector<bigint> IntProg::getLastSolutionFor(const std::vector<IntVar*>& vars) const {
  if (!solver.foundSolution()) throw InvalidArgument("No solution to return.");
  return aux::comprehension(vars, [&](IntVar* iv) { return getLastSolutionFor(iv); });
}

Core IntProg::getLastCore() {
  Core core = emptyCore();
  for (Lit l : assumptions.getKeys()) {
    if (isUnit(solver.getLevel(), -l)) {
      core->insert(var2var.at(toVar(l)));
      return core;
    }
  }
  if (solver.lastCore->isTautology()) return nullptr;
  CeSuper clone = solver.lastCore->clone(global.cePools);
  clone->weaken([&](Lit l) { return !assumptions.has(-l); });
  clone->removeUnitsAndZeroes(solver.getLevel(), solver.getPos());
  if (clone->isTautology()) return nullptr;
  clone->simplifyToClause();
  for (Var v : clone->vars) {
    core->insert(var2var.at(v));
  }
  return core;
}

void IntProg::printOrigSol() const {
  if (!solver.foundSolution()) throw InvalidArgument("No solution to return.");
  for (const std::unique_ptr<IntVar>& iv : vars) {
    bigint val = iv->getValue(solver.getLastSolution());
    if (val != 0) {
      std::cout << iv->getName() << " " << val << "\n";
    }
  }
}

WithState<Ce32> IntProg::getSolIntersection(const std::vector<IntVar*>& ivs, bool keepstate, const TimeOut& to) {
  auto [result, optval, optcore] = toOptimum(obj, keepstate, to);
  if (result == SolveState::INCONSISTENT || result == SolveState::UNSAT || result == SolveState::TIMEOUT) {
    return {result, CeNull()};
  }
  assert(result == SolveState::SAT);

  Ce32 invalidator = global.cePools.take32();
  invalidator->orig = Origin::INVALIDATOR;
  invalidator->addRhs(1);
  for (IntVar* iv : ivs) {
    for (Var v : iv->getEncodingVars()) {
      invalidator->addLhs(1, -solver.getLastSolution()[v]);
    }
  }
  // NOTE: assumptions and input can overlap, and that is ok

  Var marker = 0;
  Optim opt = optim;
  if (keepstate) {
    marker = fixObjective(obj, optval);
    invalidator->addLhs(1, -marker);
    assert(invalidator->isClause());
    opt = OptimizationSuper::make(IntConstraint{}, solver, assumptions);
  }

  result = opt->runFull(false, to.limit);
  assert(result == SolveState::SAT || result == SolveState::TIMEOUT);
  if (result == SolveState::SAT) {
    while (true) {
      solver.addConstraint(invalidator);
      result = opt->runFull(false, to.limit);
      if (result != SolveState::SAT) break;
      for (Var v : invalidator->getVars()) {
        Lit invallit = invalidator->getLit(v);
        if (solver.getLastSolution()[v] == invallit) {  // solution matches invalidator
          invalidator->addLhs(-1, invallit);
        }
      }
      invalidator->removeZeroes();
    }
  }

  if (keepstate) {
    assert(marker != 0);
    invalidator->addLhs(-1, -marker);
    invalidator->removeZeroes();
    assumptions.remove(marker);
    solver.addUnitConstraint(-marker, Origin::INVALIDATOR);
  }
  assert(invalidator->isClause());

  assert(result != SolveState::INPROCESSED);
  assert(result != SolveState::SAT);
  if (result == SolveState::TIMEOUT) return {SolveState::TIMEOUT, CeNull()};
  assert(result == SolveState::INCONSISTENT || result == SolveState::UNSAT);
  return {SolveState::SAT, invalidator};
}

IntVar* IntProg::addFlag() {
  // TODO: ensure unique variable names
  return addVar("__flag" + std::to_string(solver.getNbVars() + 1), 0, 1, Encoding::ORDER);
}

OptRes IntProg::toOptimum(IntConstraint& objective, bool keepstate, const TimeOut& to) {
  if (to.reinitialize) global.stats.runStartTime = std::chrono::steady_clock::now();
  SolveState res = optim->runFull(false, to.limit);
  if (res == SolveState::TIMEOUT || res == SolveState::UNSAT) return {res, 0, emptyCore()};
  if (res == SolveState::INCONSISTENT) {
    return {SolveState::INCONSISTENT, 0, getLastCore()};
  }
  bigint objrange = objective.getRange();
  if (objrange == 0) {
    return {SolveState::SAT, 0, emptyCore()};
  }
  IntVar* flag = addFlag();
  assert(flag->getEncodingVars().size() == 1);
  Var flag_v = flag->getEncodingVars()[0];
  assumptions.add(flag_v);
  bigint cf = 1 + (keepstate ? objrange : 0);
  assert(cf > 0);
  objective.lhs.push_back({cf, flag});
  Optim opt = OptimizationSuper::make(objective, solver, assumptions);
  res = opt->runFull(true, to.limit);
  if (res == SolveState::TIMEOUT) {
    objective.lhs.pop_back();
    assumptions.remove(flag_v);
    return {SolveState::TIMEOUT, 0, emptyCore()};
  }
  assert(res == SolveState::INCONSISTENT);
  // NOTE: UNSAT should not happen, as this should have been caught in first runFull.
  Core optcore = getLastCore();
  assert(optcore);
  optcore->erase(flag);
  bigint bound = opt->getUpperBound() - cf;
  assert(objective.lhs.back().v == flag);
  objective.lhs.pop_back();
  assumptions.remove(flag_v);
  solver.addUnitConstraint(-flag_v, Origin::FORMULA);
  return {SolveState::SAT, bound, std::move(optcore)};
}

// NOTE: also throws AsynchronousInterrupt
WithState<std::vector<std::pair<bigint, bigint>>> IntProg::propagate(const std::vector<IntVar*>& ivs, bool keepstate,
                                                                     const TimeOut& to) {
  solver.printHeader();
  auto [result, optval, optcore] = toOptimum(obj, keepstate, to);
  assert(result != SolveState::INPROCESSED);
  if (result != SolveState::SAT) return {result, {}};
  assert(result == SolveState::SAT);

  Var marker = 0;
  if (keepstate) {  // still need to fix the objective, since toOptimum has not done this with keepState==true
    marker = fixObjective(obj, optval);
  }

  std::vector<std::pair<bigint, bigint>> consequences(ivs.size());
  std::vector<IntVar*> bools;
  bools.reserve(ivs.size());
  int64_t i = 0;
  for (IntVar* iv : ivs) {
    if (iv->getEncodingVars().empty()) {
      consequences[i] = {iv->getLowerBound(), iv->getUpperBound()};
    } else if (iv->isBoolean()) {
      bools.push_back(iv);
      consequences[i] = {0, 1};
    } else {
      IntConstraint varObjPos = {{{1, iv}}, 0};
      auto [lowerstate, lowerbound, optcore1] = toOptimum(varObjPos, true, {false, to.limit});
      if (lowerstate == SolveState::TIMEOUT) {
        assumptions.remove(marker);
        return {SolveState::TIMEOUT, {}};
      }
      assert(lowerstate == SolveState::SAT);
      IntConstraint varObjNeg = {{{-1, iv}}, 0};
      auto [upperstate, upperbound, optcore2] = toOptimum(varObjNeg, true, {false, to.limit});
      if (upperstate == SolveState::TIMEOUT) {
        assumptions.remove(marker);
        return {SolveState::TIMEOUT, {}};
      }
      assert(upperstate == SolveState::SAT);
      consequences[i] = {lowerbound, -upperbound};
      if (!keepstate && consequences[i].first > iv->getLowerBound()) {
        addConstraint({{{1, iv}}, consequences[i].first});
      }
      if (!keepstate && consequences[i].second < iv->getUpperBound()) {
        addConstraint({{{1, iv}}, std::nullopt, consequences[i].second});
      }
    }
    assert(consequences[i].first <= consequences[i].second);
    ++i;
  }
  assumptions.remove(marker);

  auto [intersectstate, invalidator] = getSolIntersection(bools, keepstate, {false, to.limit});
  // TODO: getSolIntersection will do double work (e.g., use its own marker literal)
  if (intersectstate == SolveState::TIMEOUT) return {SolveState::TIMEOUT, {}};

  i = -1;
  for (IntVar* iv : ivs) {
    ++i;
    if (iv->getEncodingVars().empty() || !iv->isBoolean()) continue;
    assert(iv->getEncodingVars().size() == 1);
    Lit l = iv->getEncodingVars()[0];
    if (invalidator->hasLit(l)) consequences[i].second = 0;
    if (invalidator->hasLit(-l)) consequences[i].first = 1;
    assert(consequences[i].first <= consequences[i].second);
  }

  return {SolveState::SAT, consequences};
}

// NOTE: also throws AsynchronousInterrupt
WithState<std::vector<std::vector<bigint>>> IntProg::pruneDomains(const std::vector<IntVar*>& ivs, bool keepstate,
                                                                  const TimeOut& to) {
  for (IntVar* iv : ivs) {
    if (iv->getEncodingVars().size() != 1 && iv->getEncoding() != Encoding::ONEHOT) {
      throw InvalidArgument("Non-Boolean variable " + iv->getName() +
                            " is passed to pruneDomains but is not one-hot encoded.");
    }
  }
  solver.printHeader();
  auto [result, invalidator] = getSolIntersection(ivs, keepstate, to);
  assert(result != SolveState::INPROCESSED);
  if (result != SolveState::SAT) return {result, {}};
  assert(result == SolveState::SAT);

  std::vector<std::vector<bigint>> doms(ivs.size());
  for (int i = 0; i < (int)ivs.size(); ++i) {
    IntVar* iv = ivs[i];
    if (iv->getEncodingVars().empty()) {
      assert(iv->getLowerBound() == iv->getUpperBound());
      doms[i].push_back(iv->getLowerBound());
      continue;
    }
    if (iv->getEncodingVars().size() == 1) {
      assert(iv->getEncoding() == Encoding::ORDER);
      Var v = iv->getEncodingVars()[0];
      if (!invalidator->hasLit(-v)) {
        doms[i].push_back(iv->getLowerBound());
      }
      if (!invalidator->hasLit(v)) {
        doms[i].push_back(iv->getUpperBound());
      }
      continue;
    }
    assert(iv->getEncoding() == Encoding::ONEHOT);
    bigint val = iv->getLowerBound();
    for (Var v : iv->getEncodingVars()) {
      if (!invalidator->hasLit(v)) {
        doms[i].push_back(val);
      }
      ++val;
    }
  }
  return {SolveState::SAT, doms};
}

Var IntProg::fixObjective(const IntConstraint& ico, const bigint& optval) {
  IntVar* flag = addFlag();
  Var flag_v = flag->getEncodingVars()[0];
  assumptions.add(flag_v);
  if (ico.getRange() > 0) {
    IntConstraint ic = ico;
    assert(ico.lowerBound.has_value());
    ic.upperBound = optval + ico.lowerBound.value();
    ic.lowerBound.reset();
    addRightReification(flag, true, ic);
  }
  return flag_v;
}

WithState<int64_t> IntProg::count(const std::vector<IntVar*>& ivs, bool keepstate, const TimeOut& to) {
  WithState<std::vector<unordered_map<bigint, int64_t>>> result = count(ivs, {}, keepstate, to);
  assert(result.val.size() <= 1);
  if (result.val.empty()) return {result.state, 0};
  assert(result.val[0].contains(0));
  return {result.state, result.val[0][0]};
}

WithState<std::vector<unordered_map<bigint, int64_t>>> IntProg::count(const std::vector<IntVar*>& ivs_base,
                                                                      const std::vector<IntVar*>& ivs_counts,
                                                                      bool keepstate, const TimeOut& to) {
  solver.printHeader();
  auto [result, optval, optcore] = toOptimum(obj, keepstate, to);
  if (result == SolveState::INCONSISTENT || result == SolveState::UNSAT || result == SolveState::TIMEOUT) {
    return {result, {}};
  }
  assert(result == SolveState::SAT);

  Optim opt = optim;
  Var flag_v = 0;
  if (keepstate) {
    flag_v = fixObjective(obj, optval);
    opt = OptimizationSuper::make(IntConstraint{}, solver, assumptions);
  }

  std::vector<unordered_map<bigint, int64_t>> counts(std::ssize(ivs_counts));
  bool has_counts = !counts.empty();
  if (!has_counts) counts.emplace_back();

  result = opt->runFull(false, to.limit);
  while (result == SolveState::SAT) {
    if (has_counts) {
      assert(solver.foundSolution());
      int64_t i = 0;
      for (IntVar* iv : ivs_counts) {
        counts[i][iv->getValue(solver.getLastSolution())] += 1;
        ++i;
      }
    } else {
      counts[0][0] += 1;
    }
    invalidateLastSol(ivs_base, flag_v);
    result = opt->runFull(false, to.limit);
  }

  if (keepstate) {
    assert(assumptions.has(flag_v));
    assumptions.remove(flag_v);
    solver.addUnitConstraint(-flag_v, Origin::FORMULA);
  }

  if (result == SolveState::TIMEOUT) {
    return {SolveState::TIMEOUT, counts};
  }
  return {SolveState::SAT, counts};
}

WithState<Core> IntProg::extractMUS(const TimeOut& to) {
  solver.printHeader();
  if (to.reinitialize) global.stats.runStartTime = std::chrono::steady_clock::now();
  SolveState result = optim->runFull(false, to.limit);
  if (result == SolveState::SAT || result == SolveState::TIMEOUT) return {result, nullptr};
  if (result == SolveState::UNSAT) return {SolveState::UNSAT, emptyCore()};
  assert(result == SolveState::INCONSISTENT);

  Core last_core = getLastCore();
  assert(last_core);
  IntSet newAssumps;
  std::vector<IntVar*> toCheck;
  toCheck.reserve(last_core->size());
  Core needed = emptyCore();

  for (IntVar* iv : *last_core) {
    toCheck.push_back(iv);
    for (Var v : iv->getEncodingVars()) {
      if (assumptions.has(v)) newAssumps.add(v);
      if (assumptions.has(-v)) newAssumps.add(-v);
    }
  }

  Optim opt = OptimizationSuper::make(IntConstraint{}, solver, newAssumps);

  while (!toCheck.empty()) {
    IntVar* current = toCheck.back();
    toCheck.pop_back();
    for (Var v : current->getEncodingVars()) {
      newAssumps.remove(v);
      newAssumps.remove(-v);
    }
    result = opt->runFull(false, to.limit);
    if (result == SolveState::TIMEOUT) {
      return {SolveState::TIMEOUT, nullptr};
    } else if (result == SolveState::INCONSISTENT) {
      last_core = getLastCore();
      assert(last_core);
      for (int64_t i = 0; i < (int64_t)toCheck.size(); ++i) {
        if (!last_core->contains(toCheck[i])) {
          plf::single_reorderase(toCheck, toCheck.begin() + i);
          --i;
        }
      }
    } else {
      assert(result == SolveState::SAT);
      needed->insert(current);
      for (Var v : current->getEncodingVars()) {
        if (assumptions.has(v)) newAssumps.add(v);
        if (assumptions.has(-v)) newAssumps.add(-v);
      }
    }
  }

  return {needed->empty() ? SolveState::UNSAT : SolveState::INCONSISTENT, std::move(needed)};
}

void IntProg::runFromCmdLine() {
  global.stats.startTime = std::chrono::steady_clock::now();

  if (global.options.verbosity.get() > 0) {
    std::cout << "c Exact - branch " EXPANDED(GIT_BRANCH) " commit " EXPANDED(GIT_COMMIT_HASH) << std::endl;
  }
  if (global.options.printCsvData) global.stats.printCsvHeader();

  aux::timeCallVoid([&] { parsing::file_read(*this); }, global.stats.PARSETIME.z);

  if (global.options.printOpb) printFormula();
  if (global.options.noSolve) throw EarlyTermination();

  solver.printHeader();

  global.stats.runStartTime = std::chrono::steady_clock::now();
  [[maybe_unused]] SolveState res = optim->runFull(true, 0);
}

}  // namespace xct
