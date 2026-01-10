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

#include "ConstrExp.hpp"
#include <functional>
#include "../Solver.hpp"
#include "../auxiliary.hpp"
#include "../datastructures/Heuristic.hpp"
#include "../datastructures/IntSet.hpp"
#include "../propagation/Equalities.hpp"
#include "../propagation/Implications.hpp"
#include "Constr.hpp"

namespace xct {
int32_t subsetsum_dp_withsol(const Global& global, const std::vector<int32_t>& vals, int32_t target,
                             std::vector<std::pair<int32_t, int32_t>>& sums, unordered_map<int32_t, int32_t>* subset) {
  assert(std::ranges::is_sorted(vals, std::greater<int>()));
  assert(target > 0);
  assert(!vals.empty());
  const int32_t total = std::accumulate(vals.begin(), vals.end(), 0);
  if (total == target) return target;
  assert(total > target);
  if (subset != nullptr) subset->clear();

  int32_t heur = 0;
  for (int32_t v : vals) {
    if (heur + v <= target) heur += v;
  }
  if (heur == target) {
    if (subset != nullptr) {
      heur = 0;
      for (int32_t v : vals) {
        if (heur + v <= target) {
          heur += v;
          aux::insertmulti(*subset, v);
        }
      }
    }
    return target;
  }

  heur = total;
  for (int32_t v : vals) {
    if (heur - v >= target) heur -= v;
  }
  if (heur == target) {
    if (subset != nullptr) {
      heur = total;
      for (int32_t v : vals) {
        if (heur - v >= target) {
          heur -= v;
        } else {
          aux::insertmulti(*subset, v);
        }
      }
    }
    return target;
  }

  assert(total > target);
  const int32_t w = total - target;
  sums.clear();
  sums.resize(w + 1, {total, 0});
  quit::checkInterrupt(global);
  for (const int32_t v : vals) {
    if (sums[0].first == target) break;
    for (int32_t j = 0; j <= w - v; ++j) {
      if (const int32_t newsum = sums[j + v].first - v; sums[j].first > newsum) {
        sums[j] = {newsum, v};
      }
    }
  }
  assert(sums[0].first >= target);

  if (subset != nullptr) {  // calculate subset
    for (int32_t v : vals) {
      aux::insertmulti(*subset, v);
    }
    int32_t smallest = sums[0].first;
    for (int32_t i = 0; i <= w; ++i) {
      if (sums[i].first == smallest) {
        aux::erasemulti(*subset, sums[i].second);
        smallest += sums[i].second;
      }
    }
    assert((aux::summulti<int32_t, int32_t>(*subset) == sums[0].first));
  }

  return sums[0].first;
}

std::pair<int32_t, bool> subsetsum_dp(const Global& global, const std::vector<int32_t>& vals, int32_t target,
                                      int32_t total, std::vector<std::pair<int32_t, int32_t>>& sums) {
  assert(std::ranges::is_sorted(vals, std::greater<int>()));
  assert(target > 0);
  assert(!vals.empty());
  if (total == target) return {target, true};
  assert(total > target);

  uint32_t smallestNotUsed = 0;
  int32_t heur = 0;
  for (int32_t v : vals) {
    if (heur + v <= target) {
      heur += v;
      smallestNotUsed = smallestNotUsed || v == vals.back();
      if (heur == target) {
        return {target, smallestNotUsed};
      }
    }
  }

  smallestNotUsed = 0;
  heur = total;
  for (int32_t v : vals) {
    if (heur - v >= target) {
      heur -= v;
      smallestNotUsed += v == vals.back();
      if (heur == target) {
        return {target, vals.back() == vals.at(std::ssize(vals) - smallestNotUsed - 1)};
      }
    }
  }

  assert(total > target);
  const int32_t w = total - target;
  sums.clear();
  sums.resize(w + 1, {total, 0});
  quit::checkInterrupt(global);
  for (const int32_t v : vals) {
    if (sums[0].first == target) break;
    for (int32_t j = 0; j <= w - v; ++j) {
      if (const int32_t newsum = sums[j + v].first - v; sums[j].first > newsum) {
        sums[j] = {newsum, v};
      }
    }
  }
  assert(sums[0].first >= target);

  smallestNotUsed = 0;
  int32_t sum = sums[0].first;
  for (int32_t i = 0; i <= w; ++i) {
    if (sums[i].first == sum) {
      smallestNotUsed += sums[i].second == vals.back();
      sum += sums[i].second;
    }
  }
  assert(smallestNotUsed < std::ssize(vals));
  return {sums[0].first, vals.back() == vals.at(std::ssize(vals) - smallestNotUsed - 1)};
}

void SymbolicBound::add(const SymbolicBound& sb, const bigint& m) {
  assert(m > 0);
  assert(isValid());
  mult_upper += sb.mult_upper * m;
  mult_lower += sb.mult_lower * m;
  offset += sb.offset * m;
}

void SymbolicBound::addOffset(const bigint& os) {
  if (!isValid()) return;
  offset += os;
}

void SymbolicBound::divide(const bigint& d) {
  assert(d > 0);
  if (!isValid()) return;
  mult_upper /= d;
  mult_lower /= d;
  offset /= d;
}

void SymbolicBound::multiply(const bigint& m) {
  assert(m > 0);
  if (!isValid()) return;
  mult_upper *= m;
  mult_lower *= m;
  offset *= m;
}

void SymbolicBound::reset() {
  mult_upper = 0;
  mult_lower = 0;
  offset = 0;
}

bool SymbolicBound::isValid() const {
  assert(mult_upper != 0 || mult_lower != 0 || offset == 0);
  return mult_upper != 0 || mult_lower != 0;
}

bigint SymbolicBound::getDegree(const bigint& upbound, const bigint& lowbound) const {
  const ratio r = offset + mult_upper * upbound + mult_lower * lowbound;
  return aux::ceildiv_safe(numerator(r), denominator(r));
}

std::ostream& operator<<(std::ostream& os, const SymbolicBound& bound) {
  os << "mult_upper " << bound.mult_upper << " mult_lower " << bound.mult_lower << " offset " << bound.offset;
  return os;
}

ConstrExpSuper::ConstrExpSuper(Global& g) : global(g), orig(Origin::UNKNOWN) {}

void ConstrExpSuper::resetBuffer(ID proofID) {
  if (!global.logger.isActive()) return;
  assert(proofID != ID_Undef);
  proofBuffer.clear();
  proofBuffer.str(std::string());
  proofBuffer << proofID << " ";
}

void ConstrExpSuper::resetBuffer(const std::string& line) {
  if (!global.logger.isActive()) return;
  proofBuffer.clear();
  proofBuffer.str(std::string());
  proofBuffer << line;
}

int ConstrExpSuper::nVars() const { return vars.size(); }

bool ConstrExpSuper::empty() const { return vars.empty(); }

int ConstrExpSuper::nNonZeroVars() const {
  int result = 0;
  for (Var v : vars) {
    result += hasVar(v);
  }
  return result;
}

const VarVec& ConstrExpSuper::getVars() const { return vars; }

bool ConstrExpSuper::used(Var v) const { return index[v] >= 0; }

void ConstrExpSuper::reverseOrder() {
  std::reverse(vars.begin(), vars.end());
  for (int i = 0; i < (int)vars.size(); ++i) index[vars[i]] = i;
}

void ConstrExpSuper::popLast() {
  assert(!vars.empty());
  assert(!hasVar(vars.back()));
  index[vars.back()] = -1;
  vars.pop_back();
}

void ConstrExpSuper::weakenLast() {
  if (vars.empty()) return;
  weaken(vars.back());
  popLast();
}

bool ConstrExpSuper::isUnitConstraint() const { return isClause() && nVars() == 1 && !isUnsat(); }

bool ConstrExpSuper::hasNoUnits(const IntMap<int>& level) const {
  return std::all_of(vars.cbegin(), vars.cend(), [&](Var v) { return !isUnit(level, v) && !isUnit(level, -v); });
}

// NOTE: only equivalence preserving operations over the Bools!
void ConstrExpSuper::postProcess(const IntMap<int>& level, const std::vector<int>& pos, const Heuristic& heur,
                                 bool sortFirst, Stats& stats) {
  removeUnitsAndZeroes(level, pos);
  assert(sortFirst || isSortedInDecreasingCoefOrder());  // NOTE: check this only after removing units and zeroes
  saturate(true, !sortFirst);
  if (isClause() || isCardinality()) return;
  if (sortFirst) {
    const std::vector<ActNode>& actList = heur.getActList();
    sortInDecreasingCoefOrder([&](Var v1, Var v2) { return actList[v1].activity > actList[v2].activity; });
  }
  const bool dgcd = divideByGCD();
  stats.NGCD.z += dgcd;
  if (simplifyToCardinality(true, getCardinalityDegree())) {
    ++stats.NCARDDETECT.z;
    return;
  }
  strengthen();
}

void ConstrExpSuper::strongPostProcess(Solver& solver) {
  [[maybe_unused]] int nvars = nNonZeroVars();
  if (global.options.liftDegreeSymbolic.get() == 1) {
    liftDegreeSymbolic(solver.getSymbBoundUpper(), solver.getSymbBoundLower());
  }

  removeEqualities(solver.getEqualities());
  if (!symbBound.isValid()) {
    selfSubsumeImplications(solver.getImplications());
  }
  postProcess(solver.getLevel(), solver.getPos(), solver.getHeuristic(), true, solver.getStats());
  assert(hasRhsDegreeInvariant());
  assert(nvars >= nNonZeroVars());
}

void ConstrExpSuper::symbBoundPostProcess(Solver& solver) {
  assert(global.options.liftDegreeSymbolic.get() == 2);
  liftDegreeSymbolic(solver.getSymbBoundUpper(), solver.getSymbBoundLower());
  postProcess(solver.getLevel(), solver.getPos(), solver.getHeuristic(), true, solver.getStats());
  assert(hasRhsDegreeInvariant());
}

std::ostream& operator<<(std::ostream& o, const ConstrExpSuper& ce) {
  ce.toStreamAsOPB(o);
  return o;
}
std::ostream& operator<<(std::ostream& o, const CeSuper& ce) { return o << *ce; }

template <typename SMALL, typename LARGE>
ConstrExp<SMALL, LARGE>::ConstrExp(Global& g) : ConstrExpSuper(g) {
  reset(false);
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::copyTo(const Ce32& ce) const {
  copyTo_(ce);
}
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::copyTo(const Ce64& ce) const {
  copyTo_(ce);
}
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::copyTo(const Ce96& ce) const {
  copyTo_(ce);
}
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::copyTo(const Ce128& ce) const {
  copyTo_(ce);
}
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::copyTo(const CeArb& ce) const {
  copyTo_(ce);
}

template <typename SMALL, typename LARGE>
CeSuper ConstrExp<SMALL, LARGE>::clone(ConstrExpPools& cePools) const {
  LARGE maxVal = getCutoffVal();
  if (maxVal <= static_cast<LARGE>(limitAbs<int, int64_t>())) {
    Ce32 result = cePools.take32();
    copyTo(result);
    return result;
  } else if (maxVal <= static_cast<LARGE>(limitAbs<int64_t, int128>())) {
    Ce64 result = cePools.take64();
    copyTo(result);
    return result;
  } else if (maxVal <= static_cast<LARGE>(limitAbs<int128, int128>())) {
    Ce96 result = cePools.take96();
    copyTo(result);
    return result;
  } else if (maxVal <= static_cast<LARGE>(limitAbs<int128, int256>())) {
    Ce128 result = cePools.take128();
    copyTo(result);
    return result;
  } else {
    CeArb result = cePools.takeArb();
    copyTo(result);
    return result;
  }
}

template <typename SMALL, typename LARGE>
CRef ConstrExp<SMALL, LARGE>::toConstr(ConstraintAllocator& ca, bool locked, ID id) const {
  // assert(testConstraint());
  assert(isSortedInDecreasingCoefOrder());
  assert(isSaturated());
  assert(hasNoZeroes());
  assert(!vars.empty());
  assert(!isTautology());
  assert(!isUnsat());

  CRef result = CRef{ca.at};
  SMALL maxCoef = aux::abs(coefs[vars[0]]);
  if (isClause()) {
    // assert((nNonZeroVars() < 2 || getStrength() >= std::sqrt(1.8) / vars.size()));
    // assert(getStrength() <= std::sqrt(2.2) / vars.size());
    // if (vars.size() == 2) {
    //   new (ca.alloc<Binary>(vars.size())) Binary(this, locked, id, lbd, nConfl);
    // } else {
    new (ca.alloc<Clause>(vars.size())) Clause(this, locked, id);
    // }
  } else if (maxCoef == 1) {
    // assert(getStrength() >= 0.9 * static_cast<double>(degree) / vars.size());
    // assert(getStrength() <= 1.1 * (static_cast<double>(degree) + 1) / vars.size());
    new (ca.alloc<Cardinality>(vars.size())) Cardinality(this, locked, id);
  } else {
    if (maxCoef <= static_cast<LARGE>(limitAbs<int, int64_t>())) {
      global.stats.NSMALL.z += 1;
      assert(degree >= maxCoef);
      new (ca.alloc<Watched32>(vars.size())) Watched32(this, locked, id);
    } else if (maxCoef <= static_cast<LARGE>(limitAbs<int64_t, int128>())) {
      global.stats.NLARGE.z += 1;
      new (ca.alloc<Watched64>(vars.size())) Watched64(this, locked, id);
    } else if (maxCoef <= static_cast<LARGE>(limitAbs<int128, int128>())) {
      global.stats.NLARGE.z += 1;
      new (ca.alloc<Watched96>(vars.size())) Watched96(this, locked, id);
    } else if (maxCoef <= static_cast<LARGE>(limitAbs<int128, int256>())) {
      global.stats.NLARGE.z += 1;
      new (ca.alloc<Watched128>(vars.size())) Watched128(this, locked, id);
    } else {
      global.stats.NARB.z += 1;
      new (ca.alloc<WatchedArb>(vars.size())) WatchedArb(this, locked, id);
    }
  }
  return result;
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::copyTo(ConstrSimple32& target) const {
  copyTo_(target);
}
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::copyTo(ConstrSimple64& target) const {
  copyTo_(target);
}
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::copyTo(ConstrSimple96& target) const {
  copyTo_(target);
}
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::copyTo(ConstrSimple128& target) const {
  copyTo_(target);
}
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::copyTo(ConstrSimpleArb& target) const {
  copyTo_(target);
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::add(Var v, SMALL c, bool removeZeroes, bool fixSymbBound) {
  if (c == 0) return;
  SMALL& cf = coefs[v];
  if (!used(v)) {
    assert(cf == 0);
    cf = c;
    index[v] = vars.size();
    vars.push_back(v);
  } else {
    if ((cf < 0) != (c < 0)) {
      const SMALL change = std::min(aux::abs(cf), aux::abs(c));
      degree -= change;
      if (fixSymbBound) symbBound.addOffset(-change);
    }
    cf += c;
    if (removeZeroes && cf == 0) remove(v);
  }
  assert(!(removeZeroes && cf == 0 && used(v)));
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::remove(Var v) {
  assert(used(v));
  coefs[v] = 0;
  Var replace = vars.back();
  assert(vars[index[v]] == v);
  vars[index[v]] = replace;
  index[replace] = index[v];
  index[v] = -1;
  vars.pop_back();
}

template <typename SMALL, typename LARGE>
LARGE ConstrExp<SMALL, LARGE>::calcDegree() const {
  LARGE res = rhs;
  for (Var v : vars) res -= std::min<SMALL>(0, coefs[v]);  // considering negative coefficients
  return res;
}

template <typename SMALL, typename LARGE>
LARGE ConstrExp<SMALL, LARGE>::calcRhs() const {
  LARGE res = degree;
  for (Var v : vars) res += std::min<SMALL>(0, coefs[v]);  // considering negative coefficients
  return res;
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::testConstraint() const {
  assert(degree == calcDegree());
  assert(rhs == calcRhs());
  assert(coefs.size() == index.size());
  unordered_set<Var> usedvars;
  usedvars.insert(vars.cbegin(), vars.cend());
  for (Var v = 1; v < (int)coefs.size(); ++v) {
    assert(used(v) || coefs[v] == 0);
    assert(usedvars.count(v) == used(v));
    assert(index[v] == -1 || vars[index[v]] == v);
  }
  return true;
}

// TODO: testSymbolicBound!

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::resize(size_t s) {
  if (s > coefs.size()) {
    coefs.resize(s, 0);
    index.resize(s, -1);
  }
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::isReset() const {
  return vars.empty() && rhs == 0 && degree == 0 && !symbBound.isValid();
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::reset(bool partial) {
  for (Var v : vars) {
    coefs[v] = 0;
    index[v] = -1;
  }
  vars.clear();
  rhs = 0;
  degree = 0;
  if (!partial) {
    orig = Origin::UNKNOWN;
    resetBuffer(ID_Trivial);
  }
  symbBound.reset();
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::addRhs(const LARGE& r) {
  rhs += r;
  degree += r;
}

template <typename SMALL, typename LARGE>
LARGE ConstrExp<SMALL, LARGE>::getRhs() const {
  return rhs;
}

template <typename SMALL, typename LARGE>
LARGE ConstrExp<SMALL, LARGE>::getDegree() const {
  return degree;
}

template <typename SMALL, typename LARGE>
double ConstrExp<SMALL, LARGE>::getStrength() const {
  assert(isSortedInDecreasingCoefOrder());
  LARGE coefsum = 0;
  for (Var v : vars) {
    const SMALL& cf = coefs[v];
    if (cf == 0) break;
    coefsum += aux::abs(cf);
  }
  return aux::divToDouble(degree, coefsum);
}

template <typename SMALL, typename LARGE>
SMALL ConstrExp<SMALL, LARGE>::getCoef(Lit l) const {
  assert((unsigned int)toVar(l) < coefs.size());
  return l < 0 ? -coefs[-l] : coefs[l];
}

template <typename SMALL, typename LARGE>
SMALL ConstrExp<SMALL, LARGE>::absCoef(Var v) const {  // TODO: refactor other usages
  assert(0 <= v);
  assert(v < (Var)coefs.size());
  return aux::abs(coefs[v]);
}

template <typename SMALL, typename LARGE>
SMALL ConstrExp<SMALL, LARGE>::nthCoef(int i) const {  // TODO: refactor other usages
  assert(0 <= i);
  assert(i < (int)vars.size());
  return absCoef(vars[i]);
}

template <typename SMALL, typename LARGE>
SMALL ConstrExp<SMALL, LARGE>::getLargestCoef(const VarVec& vs) const {
  SMALL result = 0;
  for (Var v : vs) result = std::max(result, aux::abs(coefs[v]));
  return result;
}

template <typename SMALL, typename LARGE>
SMALL ConstrExp<SMALL, LARGE>::getLargestCoef() const {
  return getLargestCoef(vars);
}

template <typename SMALL, typename LARGE>
SMALL ConstrExp<SMALL, LARGE>::getSmallestCoef() const {
  assert(!vars.empty());
  SMALL result = aux::abs(coefs[vars[0]]);
  for (Var v : vars) result = std::min(result, aux::abs(coefs[v]));
  return result;
}

template <typename SMALL, typename LARGE>
LARGE ConstrExp<SMALL, LARGE>::getCutoffVal() const {
  return std::max<LARGE>(getLargestCoef(), std::max(degree, aux::abs(rhs)) / INF);
}

template <typename SMALL, typename LARGE>
Lit ConstrExp<SMALL, LARGE>::getLit(Var v) const {  // NOTE: answer of 0 means coef 0
  assert(v >= 0);
  assert(v < (Var)coefs.size());
  return (coefs[v] == 0) ? 0 : (coefs[v] < 0 ? -v : v);
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::hasLit(Lit l) const {
  Var v = toVar(l);
  assert(v < (Var)coefs.size());
  return coefs[v] != 0 && (coefs[v] < 0) == (l < 0);
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::hasVar(Var v) const {
  assert(v > 0);
  assert(v < (Var)coefs.size());
  return coefs[v] != 0;
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::saturatedLit(Lit l) const {
  Var v = toVar(l);
  return (coefs[v] < 0) == (l < 0) && aux::abs(coefs[v]) >= degree;
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::saturatedVar(Var v) const {
  return aux::abs(coefs[v]) >= degree;
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::falsified(const IntMap<int>& level, Var v) const {
  assert(v > 0);
  assert((getLit(v) != 0 && !isFalse(level, getLit(v))) == (coefs[v] > 0 && !isFalse(level, v)) ||
         (coefs[v] < 0 && !isTrue(level, v)));
  return (coefs[v] > 0 && isFalse(level, v)) || (coefs[v] < 0 && isTrue(level, v));
}

template <typename SMALL, typename LARGE>
LARGE ConstrExp<SMALL, LARGE>::getSlack(const IntMap<int>& level) const {
  assert(hasRhsDegreeInvariant());
  LARGE slack = -rhs;
  for (Var v : vars) {
    if (isTrue(level, v) || (!isFalse(level, v) && coefs[v] > 0)) slack += coefs[v];
  }
  return slack;
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::setTmpSlack(const IntMap<int>& level) {
  assert(hasRhsDegreeInvariant());
  tmpSlack = -rhs;
  for (Var v : vars) {
    if (isTrue(level, v) || (!isFalse(level, v) && coefs[v] > 0)) tmpSlack += coefs[v];
  }
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::hasCorrectTmpSlack(const IntMap<int>& level) const {
  return tmpSlack == getSlack(level);
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::undoOneTmpSlack(Lit l) {
  // TODO: probably not needed in most places, as most false literals are resolved before doing undoOne
  const SMALL cf = getCoef(-l);
  if (cf > 0) {
    tmpSlack += cf;
  }
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::fixCoefSmallerThanTmpSlack(Lit l) {
  assert(hasLit(l));
  assert(tmpSlack < 0);
  const SMALL invcf = -aux::abs(coefs[toVar(l)]);
  if (invcf <= tmpSlack) return false;
  tmpSlack -= invcf;
  return true;
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::setTmpPrevious(const IntMap<int>& level, int decisionLvl) {
  assert(decisionLvl > 0);
  tmpPrevSlack = -degree;
  tmpPrevLargestCf = 0;
  for (Var v : vars) {
    Lit l = getLit(v);
    if (level[-l] >= decisionLvl) {
      // if level[-getLit(v)] < decisionLvl, then it was falsified on the previous level
      // so inverting this means it was not falsified on the previous level
      const SMALL cf = absCoef(v);
      tmpPrevSlack += cf;
      if (level[l] >= decisionLvl) {
        // it was also not true on the previous level, so it must be unknown
        tmpPrevLargestCf = aux::max(tmpPrevLargestCf, cf);
      }
    }
  }
  return tmpPrevSlack < tmpPrevLargestCf;
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::hasCorrectTmpPrevious(const IntMap<int>& level, int decisionLvl) {
  const LARGE tps = tmpPrevSlack;
  const SMALL tplcf = tmpPrevLargestCf;
  setTmpPrevious(level, decisionLvl);
  bool result = true;
  result = result && tps == tmpPrevSlack;
  result = result && tplcf >= tmpPrevLargestCf;
  tmpPrevSlack = tps;
  tmpPrevLargestCf = tplcf;
  return result;
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::canPropagateOnPrevious(const std::vector<int>& pos, int decisionPos) {
  assert(decisionPos >= 0);  // otherwise we are already at root level
  if (tmpPrevSlack >= tmpPrevLargestCf) return false;
  tmpPrevLargestCf = 0;
  for (Var v : vars) {
    if (pos[v] >= decisionPos) {  // unknown at previous level
      tmpPrevLargestCf = aux::max(tmpPrevLargestCf, absCoef(v));
    }
  }
  return tmpPrevSlack < tmpPrevLargestCf;
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::undoOneTmpPrevious(const LitVec& trail, const std::vector<int>& trail_lim) {
  assert(trail_lim.size() >= 1);
  if (trail_lim.back() + 1 != std::ssize(trail)) {  // we are not backjumping over decision, so nothing to do
    return;
  }
  // backjumping over decision: take all previous level literals into account
  for (uint32_t i = trail_lim.size() >= 2 ? trail_lim[trail_lim.size() - 2] : 0; i + 1 < trail.size(); ++i) {
    const SMALL& cf = getCoef(-trail[i]);
    tmpPrevSlack += aux::max<SMALL>(0, cf);
    tmpPrevLargestCf = aux::max(tmpPrevLargestCf, aux::abs(cf));
  }
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::hasNegativeSlack(const IntMap<int>& level) const {
  return getSlack(level) < 0;
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::isTautology() const {
  return getDegree() <= 0;
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::isUnsat() const {
  return getDegree() > absCoeffSum();
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::isSatisfied(const LitVec& assignment) const {
  LARGE eval = -degree;
  for (Var v : vars) {
    if (assignment[v] == getLit(v)) eval += aux::abs(coefs[v]);
  }
  return eval >= 0;
}

template <typename SMALL, typename LARGE>
unsigned int ConstrExp<SMALL, LARGE>::getLbd(const IntMap<int>& level) const {
  // calculate delete-lbd-e according to "On Dedicated CDCL Strategies for PB Solvers" - Le Berre & Wallon - 2021
  assert(isSortedInDecreasingCoefOrder());
  LARGE weakenedDeg = degree;
  for (Var v : vars) {  // weaken all non-falsifieds
    if (!isFalse(level, getLit(v))) {
      weakenedDeg -= aux::abs(coefs[v]);
      if (weakenedDeg <= 0) break;
    }
  }
  int i = int(vars.size()) - 1;
  for (; i >= 0 && weakenedDeg > 0; --i) {  // weaken all smallest falsifieds
    Var v = vars[i];
    if (isFalse(level, getLit(v))) weakenedDeg -= aux::abs(coefs[v]);
  }
  assert(i >= 0);  // constraint is asserting or conflicting
  return calculateLbd(vars | std::views::transform([&](Var v) { return getLit(v); }), level);
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::addLhs(const SMALL& cf, Lit l) {  // add c*(l>=0) if c>0 and -c*(-l>=0) if c<0
  if (cf == 0) return;
  assert(l != 0);
  SMALL c = cf;
  if (c < 0) {
    degree -= c;
  }
  Var v = l;
  if (l < 0) {
    rhs -= c;
    c = -c;
    v = -l;
  }
  assert(v < (Var)coefs.size());
  add(v, c);
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::weaken(const SMALL& m, Var v) {  // add m*(v>=0) if m>0 and -m*(-v>=-1) if m<0
  assert(v > 0);
  assert(v < static_cast<Var>(coefs.size()));
  assert(coefs[v] != 0);
  if (global.logger.isActive()) {
    Logger::proofWeaken(proofBuffer, v, m);
  }

  const bool tmp = m < 0;
  SMALL& c = coefs[v];
  if ((c < 0) != tmp) {
    const SMALL change = std::min(aux::abs(c), aux::abs(m));
    degree -= change;
    symbBound.addOffset(-change);
  }
  if (tmp) {
    rhs += m;
  }
  c += m;
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::weakenVar(const SMALL& m, Var v) {  // add -m*l
  assert(m > 0);
  assert(v < static_cast<Var>(coefs.size()));
  assert(v > 0);
  assert(aux::abs(coefs[v]) >= m);

  if (global.logger.isActive()) {
    Logger::proofWeaken(proofBuffer, getLit(v), -m);
  }

  degree -= m;
  symbBound.addOffset(-m);
  if (coefs[v] < 0) {
    coefs[v] += m;
  } else {
    coefs[v] -= m;
    rhs -= m;
  }
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::weaken(Var v) {  // fully weaken v
  assert(v < static_cast<Var>(coefs.size()));
  if (global.logger.isActive()) {
    Logger::proofWeaken(proofBuffer, v, -coefs[v]);
  }

  if (coefs[v] < 0) {
    degree += coefs[v];
    symbBound.addOffset(coefs[v]);
  } else {
    degree -= coefs[v];
    rhs -= coefs[v];
    symbBound.addOffset(-coefs[v]);
  }
  coefs[v] = 0;
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::weaken(const aux::predicate<Lit>& toWeaken) {
  for (Var v : vars) {
    if (coefs[v] != 0 && toWeaken(getLit(v))) {
      weaken(v);
    }
  }
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::weakenCheckSaturated(SMALL& toWeaken, Lit asserting, const IntMap<int>& level) {
  assert(toWeaken >= 0);
  assert(toWeaken < getCoef(asserting));
  if (isSaturated(asserting)) {  // indirect weakening
    global.stats.NMULTWEAKENEDINDIRECT.z += 1;
    for (int64_t i = std::ssize(vars) - 1; toWeaken != 0 && i >= 0; --i) {
      Var v = vars[i];
      if (coefs[v] == 0) continue;
      Lit l = getLit(v);
      if (!isFalse(level, l)) {
        if (toWeaken < absCoef(v)) {
          weakenVar(toWeaken, v);
          toWeaken = 0;
        } else {
          toWeaken -= aux::abs(coefs[v]);
          weaken(v);
        }
      }
    }
    removeZeroes();
  }
  assert(toWeaken >= 0);
  if (toWeaken > 0) {  // direct weakening
    global.stats.NMULTWEAKENEDDIRECT.z += 1;
    weakenVar(toWeaken, toVar(asserting));
  }
  repairOrder();
  saturate(true, true);
}

// @post: preserves order of vars
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::removeUnitsAndZeroes(const IntMap<int>& level, const std::vector<int>& pos) {
  if (global.logger.isActive()) {
    for (Var v : vars) {
      Lit l = getLit(v);
      if (l != 0) {
        if (isUnit(level, l)) {
          Logger::proofWeaken(proofBuffer, l, -getCoef(l));
        } else if (isUnit(level, -l)) {
          Logger::proofWeakenFalseUnit(proofBuffer, global.logger.getUnitID(l, pos), -getCoef(l));
        }
      }
    }
  }
  int j = 0;
  for (int i = 0; i < (int)vars.size(); ++i) {
    Var v = vars[i];
    if (coefs[v] == 0) {
      index[v] = -1;
    } else if (isUnit(level, v)) {
      rhs -= coefs[v];
      if (coefs[v] > 0) {
        degree -= coefs[v];
        symbBound.addOffset(-coefs[v]);
      }
      index[v] = -1;
      coefs[v] = 0;
    } else if (isUnit(level, -v)) {
      if (coefs[v] < 0) {
        degree += coefs[v];
        symbBound.addOffset(coefs[v]);
      }
      index[v] = -1;
      coefs[v] = 0;
    } else {
      index[v] = j;
      vars[j++] = v;
    }
  }
  vars.resize(j);
}

// @post: preserves order of vars
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::removeZeroes() {
  int j = 0;
  for (int i = 0; i < (int)vars.size(); ++i) {
    Var v = vars[i];
    if (coefs[v] == 0) {
      index[v] = -1;
    } else {
      index[v] = j;
      vars[j++] = v;
    }
  }
  vars.resize(j);
}

// @post: preserves order of vars
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::removeEqualities(Equalities& equalities) {
  int oldsize = vars.size();  // newly added literals are their own canonical representative
  for (int i = 0; i < oldsize && degree > 0; ++i) {
    Var v = vars[i];
    if (coefs[v] == 0) continue;
    Lit l = getLit(v);
    if (const Repr& repr = equalities.getRepr(l); repr.l != l) {  // literal is not its own canonical representative
      const SMALL mult = aux::abs(coefs[v]);
      if (!stillFits<SMALL>(mult + getCoef(repr.l)))
        continue;  // TODO: check can be dropped by intertwining saturation...
      if (global.logger.isActive()) Logger::proofMult(proofBuffer << repr.id << " ", mult) << "+ ";
      const SMALL repr_coef = getCoef(repr.l);
      if (repr_coef < -mult) {
        // full cancelation
        symbBound.addOffset(-mult);
      } else if (repr_coef < 0) {
        // partial cancelation
        symbBound.addOffset(repr_coef);
      }
      addLhs(mult, repr.l);
      addLhs(mult, -l);
      addRhs(mult);
      assert(coefs[v] == 0);
    }
  }
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::selfSubsumeImplications(const Implications& implications) {
  assert(!symbBound.isValid());  // almost always some form of saturation going on
  removeZeroes();
  saturate(true, false);  // needed to get the proof to agree
  IntSet& saturateds = global.isPool.take();
  getSaturatedLits(saturateds);
  for (Var v : vars) {
    Lit l = getLit(v);
    for (Lit ll : implications.getImplieds(l)) {
      if (!saturateds.has(ll)) continue;
      ++global.stats.NSUBSUMESTEPS.z;
      SMALL cf = aux::abs(coefs[v]);
      if (global.logger.isActive()) Logger::proofMult(proofBuffer << global.logger.logRUP(-l, ll) << " ", cf) << "+ s ";
      addRhs(cf);
      addLhs(cf, -l);
      assert(coefs[v] == 0);
      saturateds.remove(l);
      assert(!saturateds.has(-l));
      break;
    }
  }
  global.isPool.release(saturateds);
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::hasNoZeroes() const {
  return std::all_of(vars.cbegin(), vars.cend(), [&](Var v) { return coefs[v] != 0; });
}

// @post: preserves order of vars
// NOTE: other variables should already be saturated, otherwise proof logging will break
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::saturate(const VarVec& vs, bool check, bool sorted) {
  global.stats.NSATURATESTEPS.z += vs.size();
  assert(check || !sorted);
  if (vars.empty() || (sorted && aux::abs(coefs[vars[0]]) <= degree) ||
      (!sorted && check && getLargestCoef() <= degree)) {
    return;
  }
  if (global.logger.isActive()) proofBuffer << "s ";  // log saturation only if it modifies the constraint
  if (degree <= 0) {
    reset(true);
    return;
  }
  if (!global.options.symbDegNoSat) symbBound.reset();
  assert(getLargestCoef() > degree);
  const SMALL smallDeg = static_cast<SMALL>(degree);  // safe cast because of above assert
  for (Var v : vs) {
    if (coefs[v] < -smallDeg) {
      rhs -= coefs[v] + smallDeg;
      symbBound.addOffset(smallDeg + coefs[v]);
      coefs[v] = -smallDeg;
    } else if (coefs[v] > smallDeg) {
      symbBound.addOffset(smallDeg - coefs[v]);
      coefs[v] = smallDeg;
    } else if (sorted) {
      break;
    }
  }

  assert(isSaturated());
}

// NOTE: use judiciously, be careful of adding correct saturation lines in the proof
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::saturate(Var v) {
  assert(degree >= 0);
  if (aux::abs(coefs[v]) <= degree) return;
  SMALL smallDeg = static_cast<SMALL>(degree);
  if (coefs[v] < -smallDeg) {
    rhs -= coefs[v] + smallDeg;
    coefs[v] = -smallDeg;
  } else {
    assert(coefs[v] > smallDeg);
    coefs[v] = smallDeg;
  }
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::saturate(bool check, bool sorted) {
  saturate(vars, check, sorted);
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::isSaturated() const {
  if (degree > static_cast<LARGE>(limitAbs<SMALL, LARGE>())) return true;
  const SMALL deg = static_cast<SMALL>(degree);
  for (Var v : vars) {
    if (aux::abs(coefs[v]) > deg) {
      return false;
    }
  }
  return true;
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::isSaturated(Lit l) const {
  return getCoef(l) >= degree;
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::isSaturatedVar(Var v) const {
  return aux::abs(coefs[v]) >= degree;
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::isSaturated(const aux::predicate<Lit>& toWeaken) const {
  SMALL largest = 0;
  LARGE weakenedDeg = degree;
  for (Var v : vars) {
    SMALL cf = aux::abs(coefs[v]);
    if (toWeaken(getLit(v))) {
      weakenedDeg -= cf;
    } else {
      largest = std::max(largest, cf);
    }
  }
  return largest <= weakenedDeg;
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::getSaturatedLits(IntSet& out) const {
  assert(hasNoZeroes());
  if (isClause()) {
    for (Var v : vars) out.add(getLit(v));
    return;
  }
  for (Var v : vars) {
    if (aux::abs(coefs[v]) >= degree) out.add(getLit(v));
  }
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::invert() {
  rhs = -rhs;
  for (Var v : vars) coefs[v] = -coefs[v];
  const LARGE newDeg = calcDegree();
  degree = newDeg;
  symbBound.reset();
}

/*
 * Fixes overflow
 * @pre @post: hasNoZeroes()
 * @pre @post: isSaturated()
 * @post: nothing else if bitOverflow == 0
 * @post: the largest coefficient is less than 2^bitOverflow
 * @post: the degree and rhs are less than 2^bitOverflow * INF
 * @post: if overflow happened, all division until 2^bitReduce happened
 * @post: the constraint remains conflicting or propagating on asserting
 *
 * NOTE: largestCoef sometimes is the largest coef "to be checked".
 * If larger coefs exist, no overflow should be possible.
 */
template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::fixOverflow(const IntMap<int>& level, int decisionLvl, int bitOverflow, int bitReduce,
                                          const SMALL& largestCoef, Lit asserting) {
  assert(hasNoZeroes());
  assert(isSaturated());
  if (bitOverflow == 0) {
    return false;
  }
  assert(bitOverflow > 0);
  assert(bitReduce > 0);
  assert(bitOverflow >= bitReduce);
  LARGE maxVal = std::max<LARGE>(largestCoef, std::max(degree, aux::abs(rhs)) / INF);
  if (maxVal > 0 && aux::msb(maxVal) >= bitOverflow) {
    assert(!global.options.hasOnlyClausalConstraints());  // pure clausal never yields overflow
    assert(getCutoffVal() == maxVal);
    LARGE div = aux::ceildiv<LARGE>(maxVal, aux::powtwo<LARGE>(bitReduce) - 1);
    assert(aux::ceildiv<LARGE>(maxVal, div) <= aux::powtwo<LARGE>(bitReduce) - 1);
    weakenDivideRound(div, [&](Lit l) { return !isFalse(level, l) && l != -asserting && l != asserting; });
    if (decisionLvl > 0) {
      setTmpSlack(level);
    }
    assert(isSaturated());
    assert(hasNoZeroes());
    return true;
  }
  assert(isSaturated());
  assert(hasNoZeroes());
  // check that largestCoef indeed is big enough
  assert(getCutoffVal() <= 0 || aux::msb(getCutoffVal()) < bitOverflow);
  return false;
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::saturateAndFixOverflow(const IntMap<int>& level, int decisionLvl, int bitOverflow,
                                                     int bitReduce, Lit asserting, bool sorted) {
  assert(hasNoZeroes());
  assert(!sorted || isSortedInDecreasingCoefOrder());
  if (vars.empty()) return false;
  SMALL largest = sorted ? aux::abs(coefs[vars[0]]) : getLargestCoef();
  bool result = false;
  if (largest > degree) {
    saturate(sorted, sorted);
    largest = static_cast<SMALL>(degree);
    result = true;
  }
  return fixOverflow(level, decisionLvl, bitOverflow, bitReduce, largest, asserting) | result;
}

/*
 * Fixes overflow for rationals
 * @post: saturated
 * @post: none of the coefficients, degree, or rhs exceed INFLPINT
 */
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::saturateAndFixOverflowRational() {
  removeZeroes();
  LARGE maxRhs = std::max(getDegree(), aux::abs(getRhs()));
  // TODO: why do we look at degree and not max coefficient here?
  while (maxRhs >= INFLPINT) {
    LARGE d = aux::ceildiv<LARGE>(maxRhs, INFLPINT - 1);
    assert(d >= 2);
    divideRoundUp(d);
    saturate(true, false);
    maxRhs = std::max(getDegree(), aux::abs(getRhs()));
    // NOTE: due to cumulative round-off errors, we may not always have a sufficient small rhs/degree
    // so we keep dividing by at least two until we get there
  }
  assert(getDegree() < INFLPINT);
  assert(aux::abs(getRhs()) < INFLPINT);
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::fitsInDouble() const {
  return isSaturated() && degree < INFLPINT && rhs < INFLPINT;
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::largestCoefFitsIn(int bits) const {
  return aux::msb(getLargestCoef()) < bits;
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::hasRhsDegreeInvariant() const {
  return degree == calcDegree();
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::multiply(const SMALL& m) {
  assert(m > 0);
  if (m == 1) return;
  if (global.logger.isActive()) Logger::proofMult(proofBuffer, m);
  symbBound.multiply(m);
  for (Var v : vars) coefs[v] *= m;
  rhs *= m;
  degree *= m;
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::divideRoundUp(const LARGE& d) {
  assert(d > 0);
  if (d == 1) return;
  if (global.logger.isActive()) Logger::proofDiv(proofBuffer, d);
  symbBound.divide(d);
  for (Var v : vars) {
    // divides away from zero
    bool undivisible = coefs[v] % d != 0;
    coefs[v] = static_cast<SMALL>(coefs[v] / d) + (coefs[v] > 0 && undivisible) - (coefs[v] < 0 && undivisible);
  }
  degree = aux::ceildiv(degree, d);
  rhs = calcRhs();
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::weakenDivideRound(const LARGE& div, const aux::predicate<Lit>& toWeaken) {
  assert(div > 0);
  if (div == 1) return;
  weakenNonDivisible(toWeaken, div);
  if (isTautology()) {
    saturate(false, false);
    removeZeroes();
  } else {
    weakenSuperfluous(div, false, []([[maybe_unused]] Var v) { return true; });
    removeZeroes();
    divideRoundUp(div);
    saturate(true, false);
  }
}

// NOTE: preserves ordered-ness
// div is a divisor
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::weakenDivideRoundOrdered(const LARGE& div, const IntMap<int>& level) {
  assert(isSortedInDecreasingCoefOrder());
  assert(div > 0);
  if (div == 1) return;
  weakenNonDivisible(div, level);
  weakenSuperfluous(div);
  repairOrder();
  while (!vars.empty() && coefs[vars.back()] == 0) {
    popLast();
  }
  assert(hasNoZeroes());
  if (div >= degree) {
    simplifyToClause();
  } else if (!vars.empty() && div >= aux::abs(coefs[vars[0]])) {
    simplifyToCardinality(false, getCardinalityDegree());
  } else {
    divideRoundUp(div);
    saturate(true, true);
  }
}

// NOTE: preserves ordered-ness
// div is a divisor
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::weakenDivideRoundOrdered(const SMALL& div, const IntMap<int>& level, SMALL& slackdiff) {
  assert(isSortedInDecreasingCoefOrder());
  assert(div > 0);
  if (div == 1) return;
  weakenNonDivisible(div, level, slackdiff);
  weakenSuperfluous(div);
  repairOrder();
  while (!vars.empty() && coefs[vars.back()] == 0) {
    popLast();
  }
  assert(hasNoZeroes());
  if (div >= degree) {
    simplifyToClause();
  } else if (!vars.empty() && div >= aux::abs(coefs[vars[0]])) {
    simplifyToCardinality(false, getCardinalityDegree());
  } else {
    divideRoundUp(div);
    saturate(true, true);
  }
}

// NOTE: preserves ordered-ness
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::weakenDivideRoundOrderedCanceling(const LARGE& div, const IntMap<int>& level,
                                                                const std::vector<int>& pos, const SMALL& mult,
                                                                const ConstrExp<SMALL, LARGE>& confl) {
  assert(isSortedInDecreasingCoefOrder());
  assert(div > 0);
  if (div == 1) return;
  weakenNonDivisibleCanceling(div, level, mult, confl);
  weakenSuperfluousCanceling(div, pos);
  repairOrder();
  while (!vars.empty() && coefs[vars.back()] == 0) {
    popLast();
  }
  assert(hasNoZeroes());
  if (div >= degree) {
    simplifyToClause();
  } else if (!vars.empty() && div >= aux::abs(coefs[vars[0]])) {
    simplifyToCardinality(false, getCardinalityDegree());
  } else {
    divideRoundUp(div);
    saturate(true, true);
  }
}

// NOTE: does not preserve order, as the asserting literal is skipped and some literals are partially weakened
// NOTE: after call to weakenNonDivisible, order can be re repaired by call to repairOrder
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::weakenNonDivisible(const aux::predicate<Lit>& toWeaken, const LARGE& div) {
  assert(div > 0);
  if (div == 1) return;
  for (Var v : vars) {
    if (coefs[v] % div != 0 && toWeaken(getLit(v))) {
      weaken(-static_cast<SMALL>(coefs[v] % div), v);
    }
  }
}

// NOTE: does not preserve order, as the asserting literal is skipped and some literals are partially weakened
// NOTE: after call to weakenNonDivisible, order can be re repaired by call to repairOrder
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::weakenNonDivisible(const LARGE& div, const IntMap<int>& level) {
  assert(div > 0);
  if (div == 1) return;
  for (Var v : vars) {
    if (coefs[v] % div != 0 && !isFalse(level, getLit(v))) {
      weaken(-static_cast<SMALL>(coefs[v] % div), v);
    }
  }
}

// NOTE: does not preserve order, as the asserting literal is skipped and some literals are partially weakened
// NOTE: after call to weakenNonDivisible, order can be re repaired by call to repairOrder
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::weakenNonDivisible(const SMALL& div, const IntMap<int>& level, SMALL& slackdiff) {
  assert(div > 0);
  if (div == 1) return;

  for (Var v : vars) {
    if (const SMALL mod = coefs[v] % div; mod != 0 && !isFalse(level, getLit(v))) {
      if (slackdiff - div + mod >= 0) {  // we can safely round up non-falsified
        slackdiff -= div - mod;
      } else {
        weaken(-mod, v);
      }
    }
  }
}

// NOTE: does not preserve order, as the asserting literal is skipped and some literals are partially weakened
// NOTE: after call to weakenNonDivisible, order can be re repaired by call to repairOrder
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::weakenNonDivisibleCanceling(const LARGE& div, const IntMap<int>& level, const SMALL& mult,
                                                          const ConstrExp<SMALL, LARGE>& confl) {
  assert(div > 0);
  if (div == 1) return;
  for (Var v : vars) {
    Lit l = getLit(v);
    if (coefs[v] % div != 0 && !isFalse(level, l) && (isTrue(level, l) || confl.getCoef(-l) < mult)) {
      weaken(-static_cast<SMALL>(coefs[v] % div), v);
    }
  }
}

// NOTE: should only be used in conjunction with weakenNonDivisible
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::repairOrder() {
  int i = 1;
  int j = 0;
  for (; i < nVars(); ++i) {
    SMALL back = aux::abs(coefs[vars[i]]);
    SMALL front = aux::abs(coefs[vars[j]]);
    if (back > front) {
      std::swap(vars[i], vars[j]);
      index[vars[i]] = i;
      index[vars[j]] = j;
      ++j;
    } else if (front > back) {
      j = i;
    }
    assert(aux::abs(coefs[vars[i]]) == aux::abs(coefs[vars[j]]));
  }
  assert(isSortedInDecreasingCoefOrder());
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::weakenSuperfluous(const LARGE& div, bool sorted, const aux::predicate<Var>& toWeaken) {
  assert(div > 1);
  assert(!isTautology());
  [[maybe_unused]] LARGE quot = aux::ceildiv(degree, div);
  LARGE rem = (degree - 1) % div;
  if (!sorted) {                                             // extra iteration to weaken literals fully
    for (int i = vars.size() - 1; i >= 0 && rem > 0; --i) {  // going back to front in case the coefficients are sorted
      Var v = vars[i];
      if (!toWeaken(v) || coefs[v] == 0) continue;
      SMALL r = aux::abs(coefs[v]);
      if (r <= rem) {
        rem -= r;
        weaken(v);
      }
    }
  }
  for (int i = vars.size() - 1; i >= 0 && rem > 0; --i) {  // going back to front in case the coefficients are sorted
    Var v = vars[i];
    if (!toWeaken(v) || coefs[v] == 0 || saturatedVar(v)) continue;
    SMALL r = static_cast<SMALL>(static_cast<LARGE>(aux::abs(coefs[v])) % div);
    if (r > 0 && r <= rem) {
      rem -= r;
      weakenVar(r, v);
    }
  }
  assert(quot == aux::ceildiv(degree, div));
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::weakenSuperfluous(const LARGE& div) {
  assert(div > 1);
  assert(!isTautology());
  [[maybe_unused]] LARGE quot = aux::ceildiv(degree, div);
  LARGE rem = (degree - 1) % div;
  for (int i = vars.size() - 1; i >= 0 && rem > 0; --i) {  // going back to front in case the coefficients are sorted
    Var v = vars[i];
    if (coefs[v] == 0) continue;
    if (saturatedVar(v)) break;
    SMALL r = static_cast<SMALL>(static_cast<LARGE>(aux::abs(coefs[v])) % div);
    if (r > 0 && r <= rem) {
      rem -= r;
      weakenVar(r, v);
    }
  }
  assert(quot == aux::ceildiv(degree, div));
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::weakenSuperfluousCanceling(const LARGE& div, const std::vector<int>& pos) {
  assert(div > 1);
  assert(!isTautology());
  [[maybe_unused]] LARGE quot = aux::ceildiv(degree, div);
  LARGE rem = (degree - 1) % div;
  for (int i = vars.size() - 1; i >= 0 && rem > 0; --i) {  // going back to front in case the coefficients are sorted
    Var v = vars[i];
    if (pos[v] == INF || coefs[v] == 0 || saturatedVar(v)) continue;
    SMALL r = static_cast<SMALL>(static_cast<LARGE>(aux::abs(coefs[v])) % div);
    if (r > 0 && r <= rem) {
      rem -= r;
      weakenVar(r, v);
    }
  }
  assert(quot == aux::ceildiv(degree, div));
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::divideByGCD() {
  assert(isSortedInDecreasingCoefOrder());
  assert(hasNoZeroes());
  if (vars.empty()) return false;
  SMALL _gcd = aux::abs(coefs[vars.back()]);
  if (_gcd == 1) return false;
  for (Var v : vars) {
    if (saturatedVar(v)) continue;
    _gcd = aux::gcd(_gcd, aux::abs(coefs[v]));
    if (_gcd == 1) return false;
  }
  assert(_gcd > 1);
  divideRoundUp(_gcd);
  return true;
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::divideTo(double limit, const aux::predicate<Lit>& toWeaken) {
  LARGE maxVal = getCutoffVal();
  if (maxVal <= static_cast<LARGE>(limit)) {
    return false;
  }
  LARGE div = aux::ceildiv(maxVal, static_cast<LARGE>(limit));  // maxVal / div =< limit
  assert(div > 1);
  weakenDivideRound(div, toWeaken);  // TODO: weakenDivideRoundOrdered?
  return true;
}

// @return: highest decision level that does not make the constraint inconsistent
// @return: whether or not the constraint is asserting at that level
template <typename SMALL, typename LARGE>
std::pair<int, bool> ConstrExp<SMALL, LARGE>::getAssertionStatus(const IntMap<int>& level, const std::vector<int>& pos,
                                                                 LitVec& litsByPos) const {
  assert(hasNoZeroes());
  assert(isSortedInDecreasingCoefOrder());
  assert(hasNoUnits(level));

  if (vars.empty() && degree > 0) return {-1, false};
  if (vars.empty() && degree <= 0) return {0, false};

  if (isClause()) {
    // just find the highest level
    int lvl1 = 0;
    int lvl2 = -1;
    for (Var v : vars) {
      const int lvl3 = level[-getLit(v)];
      if (lvl3 > lvl1) {
        lvl2 = lvl1;
        lvl1 = lvl3;
      } else if (lvl3 > lvl2) {
        lvl2 = lvl3;
      }
    }
    if (lvl2 == lvl1 && lvl2 != INF) {
      // apparently non-propagating. In this case, just jump back on level
      // works for falsified clauses, safe for non-falsified clauses
      assert(lvl1 != 0);  // no unit literals
      return {lvl1 - 1, false};
    }
    return {lvl2, lvl2 != INF};
  }

  // calculate slack at level 0
  LARGE slack = -degree;
  for (Var v : vars) slack += aux::abs(coefs[v]);
  if (slack < 0) return {-1, false};

  // create useful datastructures
  litsByPos.clear();
  for (Var v : vars) {
    Lit l = getLit(v);
    assert(l != 0);
    if (isFalse(level, l)) litsByPos.push_back(-l);
  }
  boost::sort::pdqsort(litsByPos.begin(), litsByPos.end(),
                       [&](Lit l1, Lit l2) { return pos[toVar(l1)] < pos[toVar(l2)]; });

  // calculate earliest propagating decision level by decreasing slack one decision level at a time
  auto posIt = litsByPos.cbegin();
  auto coefIt = vars.cbegin();
  int assertionLevel = 0;
  while (true) {
    while (posIt != litsByPos.cend() && level[*posIt] <= assertionLevel) {
      slack -= aux::abs(coefs[aux::abs(*posIt)]);
      ++posIt;
    }
    if (slack < 0) return {assertionLevel - 1, false};  // not asserting, but earliest non-conflicting level

    // skip all known variables until we get to an unknown one (NOTE: literals can only go from unknown to known)
    while (coefIt != vars.cend() && (assertionLevel >= level[*coefIt] || assertionLevel >= level[-*coefIt])) ++coefIt;

    if (coefIt == vars.cend()) return {INF, false};
    if (slack < aux::abs(coefs[*coefIt])) return {assertionLevel, true};
    if (posIt == litsByPos.cend()) return {INF, false};
    assertionLevel = level[*posIt];
  }
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::falsifiedBy(const IntSet& assumptions) const {
  if (degree <= 0) return false;
  // weaken all literals that are not falsified assumptions
  LARGE weakenedDegree = degree;
  for (Var v : vars) {
    if (!assumptions.has(-getLit(v))) {
      weakenedDegree -= aux::abs(coefs[v]);
      if (weakenedDegree <= 0) return false;
    }
  }
  return weakenedDegree > 0;
}

// @post: preserves order after removeZeroes()
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::weakenNonImplied(const IntMap<int>& level, const LARGE& slack) {
  int weakenings = 0;
  for (Var v : vars) {
    if (coefs[v] != 0 && aux::abs(coefs[v]) <= slack && !falsified(level, v)) {
      weaken(v);
      ++weakenings;
    }
  }
  global.stats.NWEAKENEDNONIMPLIED.z += weakenings;
}

// @post: preserves order after removeZeroes()
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::weakenNonImplying(const IntMap<int>& level, const SMALL& propCoef, LARGE& slack) {
  assert(hasNoZeroes());
  assert(isSortedInDecreasingCoefOrder());
  const SMALL orig_slk = static_cast<SMALL>(aux::max<LARGE>(static_cast<LARGE>(limitAbs<SMALL, LARGE>()), slack));
  int weakenings = 0;
  for (Var v : vars | std::views::reverse) {
    const SMALL cf = aux::abs(coefs[v]);
    if (slack + cf >= propCoef || cf > orig_slk) break;
    if (falsified(level, v)) {
      slack += cf;
      weaken(v);
      ++weakenings;
    }
  }
  global.stats.NWEAKENEDNONIMPLYING.z += weakenings;
}

// @post: preserves order after removeZeroes()
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::heuristicWeakening(const IntMap<int>& level, const std::vector<int>& pos) {
  assert(hasNoZeroes());
  assert(isSortedInDecreasingCoefOrder());
  const SMALL smallestCf = aux::abs(coefs[vars.back()]);
  if (aux::abs(coefs[vars[0]]) == smallestCf) return;
  LARGE slk = getSlack(level);
  if (slk < smallestCf) return;  // only literals less than or equal to slack will be weakened
  Var v_prop = -1;
  for (int i = vars.size() - 1; i >= 0; --i) {
    Var v = vars[i];
    if (isUnknown(pos, v) && aux::abs(coefs[v]) > slk) {
      v_prop = v;
      break;
    }
  }
  if (v_prop == -1) return;  // no propagation, no idea what to weaken
  weakenNonImplied(level, slk);
  if (global.options.weakenNonImplying) {
    removeZeroes();
    weakenNonImplying(level, aux::abs(coefs[v_prop]), slk);
  }
  assert(slk < aux::abs(coefs[v_prop]));
  assert(slk == getSlack(level));
}

template <typename SMALL, typename LARGE>
LARGE ConstrExp<SMALL, LARGE>::absCoeffSum() const {
  LARGE result = 0;
  for (Var v : vars) result += aux::abs(coefs[v]);
  return result;
}

template <typename SMALL, typename LARGE>
std::pair<LARGE, LARGE> ConstrExp<SMALL, LARGE>::getLhsExtrema() const {
  LARGE lb = -getRhs();
  LARGE ub = -getRhs();
  for (Var v : vars) {
    if (coefs[v] < 0) lb += coefs[v];
    if (coefs[v] > 0) ub += coefs[v];
  }
  return {lb, ub};
}

// @post: preserves order of vars
template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::simplifyToCardinality(bool equivalencePreserving, int cardDegree) {
  assert(isSortedInDecreasingCoefOrder());
  assert(hasNoZeroes());
  assert(!equivalencePreserving || isSaturated());

  if (vars.empty() || aux::abs(coefs[vars[0]]) == 1) return false;
  if (cardDegree <= 0) {
    saturate(true, true);
    return false;
  }
  assert(cardDegree <= nVars());

  if (equivalencePreserving) {
    LARGE smallCoefSum = 0;
    for (int i = 1; i <= cardDegree; ++i) {
      smallCoefSum += aux::abs(coefs[vars[nVars() - i]]);
    }
    if (smallCoefSum < degree) return false;
    // else, we have an equivalent cardinality constraint
  }

  if (cardDegree == 1) {
    simplifyToClause();
    return true;
  }
  SMALL cardCoef = aux::abs(coefs[vars[cardDegree - 1]]);
  for (int i = 0; i < cardDegree - 1; ++i) {
    Var v = vars[i];
    assert(aux::abs(coefs[v]) >= cardCoef);
    const SMALL m = aux::abs(coefs[v]) - cardCoef;
    if (m != 0) weakenVar(m, v);
  }
  assert(cardCoef == getLargestCoef());
  LARGE cardSum = static_cast<LARGE>(cardDegree - 1) * cardCoef;
  assert(degree > cardSum);
  while (nVars() > cardDegree && (degree - aux::abs(coefs[vars.back()]) > cardSum)) {
    weakenLast();
  }
  assert(cardSum + cardCoef >= degree);
  divideRoundUp(cardCoef);
  assert(isCardinality());
  assert(degree == cardDegree);

  return true;
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::isCardinality() const {
  return std::all_of(vars.cbegin(), vars.cend(), [&](Var v) { return aux::abs(coefs[v]) <= 1; });
}

template <typename SMALL, typename LARGE>
int ConstrExp<SMALL, LARGE>::getCardinalityDegree() const {
  assert(isSortedInDecreasingCoefOrder());
  assert(hasNoZeroes());
  if (vars.empty()) return degree > 0;
  if (isClause()) return 1;
  if (aux::abs(coefs[vars[0]]) == 1) return static_cast<int>(degree);
  LARGE coefsum = -degree;
  int i = 0;
  for (; i < (int)vars.size() && coefsum < 0; ++i) {
    coefsum += aux::abs(coefs[vars[i]]);
  }
  return i;
}

template <typename SMALL, typename LARGE>
int ConstrExp<SMALL, LARGE>::getMaxStrengthCardinalityDegree(std::vector<int>& cardPoints) const {
  if (vars.empty() == 0) return degree > 0;
  if (isClause()) return 1;
  if (aux::abs(coefs[vars[0]]) == 1) return static_cast<int>(degree);
  getCardinalityPoints(cardPoints);
  int bestCardDegree = 0;
  double bestStrength = 0;
  for (int i = 0; i < (int)cardPoints.size(); ++i) {
    double strength = (cardPoints.size() - i) / (double)(cardPoints[i] + 1);
    if (bestStrength < strength) {
      bestStrength = strength;
      bestCardDegree = cardPoints.size() - i;
    }
  }

  assert(bestCardDegree > 0);
  assert(bestStrength > 0);
  assert(bestStrength <= 1);
  return bestCardDegree;
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::getCardinalityPoints(std::vector<int>& cardPoints) const {
  assert(isSortedInDecreasingCoefOrder());
  assert(hasNoZeroes());
  LARGE coefsum = 0;
  int cardDegree = 0;
  for (; cardDegree < (int)vars.size() && coefsum < degree; ++cardDegree) {
    coefsum += aux::abs(coefs[vars[cardDegree]]);
  }
  cardPoints.clear();
  cardPoints.reserve(cardDegree);

  LARGE weakenedDegree = degree;
  int varsLeft = nVars();
  coefsum -= aux::abs(coefs[vars[cardDegree - 1]]);
  while (weakenedDegree > 0 && cardDegree > 0 && varsLeft > 0) {
    --varsLeft;
    weakenedDegree -= aux::abs(coefs[vars[varsLeft]]);
    assert(index[vars[varsLeft]] == varsLeft);
    if (weakenedDegree <= coefsum) {
      --cardDegree;
      coefsum -= aux::abs(coefs[vars[cardDegree - 1]]);
      cardPoints.push_back(varsLeft);
    }
  }
}

// @pre: sorted in *IN*creasing coef order, so that we can pop zero coefficient literals
template <typename SMALL, typename LARGE>
int ConstrExp<SMALL, LARGE>::getCardinalityDegreeWithZeroes() {
  LARGE coefsum = -degree;
  int carddegree = 0;
  int i = vars.size() - 1;
  for (; i >= 0 && coefsum < 0; --i) {
    if (coefs[vars[i]] != 0) {
      coefsum += aux::abs(coefs[vars[i]]);
      ++carddegree;
    }
  }
  ++i;
  [[maybe_unused]] int newsize = i + carddegree;
  int j = i;
  for (; i < (int)vars.size(); ++i) {
    Var v = vars[i];
    if (coefs[v] != 0) {
      index[v] = j;
      vars[j++] = v;
    } else {
      index[v] = -1;
    }
  }
  vars.resize(j);
  assert(newsize == (int)vars.size());
  return carddegree;
}

// @post: preserves order of vars
template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::simplifyToClause() {
  assert(isSortedInDecreasingCoefOrder());
  assert(hasNoZeroes());
  assert(!isTautology());
  while (!vars.empty() && aux::abs(coefs[vars.back()]) < degree) {
    weakenLast();
  }
  if (!vars.empty()) divideRoundUp(aux::abs(coefs[vars[0]]));
  assert(vars.empty() || degree <= 1);
  assert(isClause());
  assert(hasNoZeroes());
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::isClause() const {
  return degree == 1;
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::simplifyToUnit(const IntMap<int>& level, const std::vector<int>& pos, Var v_unit) {
  removeUnitsAndZeroes(level, pos);
  assert(getLit(v_unit) != 0);
  for (Var v : vars) {
    if (v != v_unit) weaken(v);
  }
  removeZeroes();
  saturate(true, false);
  assert(degree > 0);
  divideRoundUp(std::max<LARGE>(aux::abs(coefs[v_unit]), degree));
  assert(isUnitConstraint());
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::strengthen() {
  if (!global.options.proofAssumps || global.options.subsetSum.get() <= 0 || symbBound.isValid() || isTautology())
    return;
  assert(isSaturated());
  assert(isSortedInDecreasingCoefOrder());
  assert(hasNoZeroes());
  assert(!vars.empty());

  LARGE total = absCoeffSum();
  if (total <= degree) return;  // either an inconsistency or all literals will be propagated to units

  // cardinalities and saturated constraints are not liftable
  const SMALL& largest = getCoef(vars[0]);
  if (largest == 1 || largest == degree) return;

  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

  if (total < std::numeric_limits<int32_t>::max()) {
    int32_t total32 = static_cast<int32_t>(total);  // we also know all coefficients and degree fit in 32 bits
    if (std::ssize(vars) * static_cast<int64_t>(total32 - degree) <= global.options.subsetSum.get()) {
      std::vector<int32_t>& cfs = tmpintvec;
      cfs.clear();
      cfs.reserve(vars.size());
      for (Var v : vars) {
        cfs.emplace_back(static_cast<int32_t>(aux::abs(coefs[v])));
      }

      std::vector<std::pair<int32_t, int32_t>>& sums = tmpintpairvec;
      int32_t target = static_cast<int32_t>(degree);
      auto [newdegree, hasLast] = subsetsum_dp(global, cfs, target, total32, sums);
      if (newdegree > degree) {
        rhs += newdegree - degree;
        degree = newdegree;
        global.stats.NLIFTDEGREE.z += 1;
        global.logger.logAssumption(*this, global.options.proofAssumps.operator bool());
      }

      bool foundSuperfluous = false;
      while (!hasLast && !cfs.empty()) {
        int32_t smallest = cfs.back();
        assert(aux::abs(coefs[vars.back()]) == smallest);
        if (smallest == degree) break;  // target will be 0
        cfs.pop_back();
        total32 -= smallest;
        target = static_cast<int32_t>(degree) - smallest;
        auto [newdegree, hl] = subsetsum_dp(global, cfs, target, total32, sums);
        hasLast = hl;
        SMALL& last = coefs.at(vars.back());
        if (newdegree >= target + smallest) {
          foundSuperfluous = true;
          global.stats.NSUPERFLUOUS.z += 1;
          if (last < 0) rhs -= last;
          last = 0;
          popLast();
        } else {
          if (newdegree > target) {
            foundSuperfluous = true;
            global.stats.NSUPERFLUOUSPART.z += 1;
            const int32_t diff = newdegree - target;
            if (last > 0) {
              last -= diff;
            } else {
              assert(last < 0);
              rhs += diff;
              last += diff;
            }
          }
          break;
        }
      }
      if (foundSuperfluous) {
        global.logger.logAssumption(*this, global.options.proofAssumps.operator bool());
      }
      assert(hasRhsDegreeInvariant());

      global.stats.SUBSETSUMTIME.z +=
          std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start).count();
      return;
    }
  }

  int64_t steps = 150;  // estimated overhead of alternative lift degree routine
  uint32_t i = 0;
  while (i < vars.size()) {
    const SMALL v = aux::abs(coefs[vars[i]]);
    ++i;
    uint32_t multiple = 2;  // NOTE: only one new variable will already double the work
    while (i < vars.size() && v == aux::abs(coefs[vars[i]])) {
      ++i;
      ++multiple;
    }
    steps *= multiple;
    if (steps > global.options.subsetSum.get()) {
      global.stats.SUBSETSUMTIME.z +=
          std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start).count();
      return;
    }
  }

  std::vector<SMALL>& cfs = tmpvec;
  cfs.clear();
  cfs.reserve(vars.size());
  for (Var v : vars) {
    cfs.emplace_back(aux::abs(coefs[v]));
  }

  unordered_map<LARGE, SMALL, aux::hsh<LARGE>>& sums = tmpmap;
  std::vector<std::pair<LARGE, SMALL>>& stack = tmppairvec;
  auto [newdegree, hasLast] = subsetsum_set(global, cfs, degree, total, sums, stack);

  if (newdegree > degree) {
    rhs += newdegree - degree;
    degree = newdegree;
    global.stats.NLIFTDEGREE.z += 1;
    global.logger.logAssumption(*this, global.options.proofAssumps.operator bool());
  }

  bool foundSuperfluous = false;
  LARGE target = degree;
  while (!hasLast && !cfs.empty()) {
    const SMALL smallest = std::move(cfs.back());
    assert(aux::abs(coefs[vars.back()]) == smallest);
    if (smallest == degree) break;  // target will be 0
    cfs.pop_back();
    target = degree - smallest;
    total -= smallest;
    auto [newdegree, hl] = subsetsum_set(global, cfs, target, total, sums, stack);
    hasLast = hl;
    SMALL& last = coefs.at(vars.back());
    if (newdegree >= target + smallest) {
      foundSuperfluous = true;
      global.stats.NSUPERFLUOUS.z += 1;
      if (last < 0) rhs -= last;
      last = 0;
      popLast();
    } else {
      if (newdegree > target) {
        foundSuperfluous = true;
        global.stats.NSUPERFLUOUSPART.z += 1;
        const SMALL diff = static_cast<SMALL>(newdegree - target);
        if (coefs[vars.back()] > 0) {
          coefs[vars.back()] -= diff;
        } else {
          assert(coefs[vars.back()] < 0);
          rhs += diff;
          coefs[vars.back()] += diff;
        }
      }
      break;
    }
  }
  if (foundSuperfluous) {
    global.logger.logAssumption(*this, global.options.proofAssumps.operator bool());
  }
  assert(hasRhsDegreeInvariant());

  global.stats.SUBSETSUMTIME.z +=
      std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start).count();
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::liftDegreeSymbolic(const bigint& lastUpperBound, const bigint& lastLowerBound) {
  if (!symbBound.isValid()) return;
  const bigint newDeg = symbBound.getDegree(lastUpperBound, lastLowerBound);
  if (newDeg > degree) {
    ++global.stats.NSYMBBOUND;
    if (newDeg > limitRhs<SMALL, LARGE>()) {
      assert((limitRhs<SMALL, LARGE>() > degree));
      degree = static_cast<LARGE>(limitRhs<SMALL, LARGE>());
    } else {
      degree = static_cast<LARGE>(newDeg);
    }
    rhs = calcRhs();
  }
  // TODO: proof logging
}

template <typename SMALL, typename LARGE>
bool ConstrExp<SMALL, LARGE>::isSortedInDecreasingCoefOrder() const {
  if (vars.size() <= 1) return true;
  SMALL first = aux::abs(coefs[vars[0]]);
  SMALL second = 0;
  for (int i = 1; i < std::ssize(vars); ++i) {
    second = aux::abs(coefs[vars[i]]);
    if (first < second) return false;
    first = std::move(second);
  }
  return true;
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::sortInDecreasingCoefOrder(const std::function<bool(Var, Var)>& tiebreaker) {
  if (vars.size() <= 1) return;
  boost::sort::pdqsort(vars.begin(), vars.end(), [&](Var v1, Var v2) {
    const SMALL res = aux::abs(coefs[v1]) - aux::abs(coefs[v2]);
    return res > 0 || (res == 0 && tiebreaker(v1, v2));
  });
  for (int i = 0; i < std::ssize(vars); ++i) index[vars[i]] = i;
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::sortWithCoefTiebreaker(const std::function<int(Var, Var)>& comp) {
  if (vars.size() <= 1) return;
  boost::sort::pdqsort(vars.begin(), vars.end(), [&](Var v1, Var v2) {
    const int res = comp(v1, v2);
    return res > 0 || (res == 0 && aux::abs(coefs[v1]) > aux::abs(coefs[v2]));
  });
  for (int i = 0; i < std::ssize(vars); ++i) index[vars[i]] = i;
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::toStreamAsOPBlhs(std::ostream& o, bool withConstant) const {
  VarVec vs = vars;
  boost::sort::pdqsort(vs.begin(), vs.end(), [](Var v1, Var v2) { return v1 < v2; });
  for (Var v : vs) {
    Lit l = getLit(v);
    if (l == 0) continue;
    o << std::pair<SMALL, Lit>{getCoef(l), l} << " ";
  }
  if (withConstant && degree != 0) {
    o << "-" << degree << " 1 ";
  }
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::toStreamAsOPB(std::ostream& o) const {
  toStreamAsOPBlhs(o, false);
  o << ">= " << degree << " ;";
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::toStreamWithAssignment(std::ostream& o, const IntMap<int>& level,
                                                     const std::vector<int>& pos) const {
  VarVec vs = vars;
  boost::sort::pdqsort(vs.begin(), vs.end(), [](Var v1, Var v2) { return v1 < v2; });
  for (Var v : vs) {
    Lit l = getLit(v);
    if (l == 0) continue;
    o << getCoef(l) << "x" << l
      << (isUnknown(pos, l) ? "u"
                            : (isFalse(level, l) ? "f" + std::to_string(level[-l]) : "t" + std::to_string(level[l])))
      << " ";
  }
  o << ">= " << degree << " (" << getSlack(level) << ")";
}

template <typename SMALL, typename LARGE>
void ConstrExp<SMALL, LARGE>::toStreamPure(std::ostream& o) const {
  VarVec vs = vars;
  for (Var v : vs) {
    Lit l = getLit(v);
    o << (l == 0 ? std::pair<SMALL, Lit>{0, v} : std::pair<SMALL, Lit>{getCoef(l), l}) << " ";
  }
  std::cout << ">= " << degree << " (" << rhs << ")";
}

size_t ConstrExpSuper::calculateLbd(auto lits_, const IntMap<int>& level) {
  std::array<int, MAXLBD> levels{};  // initial value is all zeroes
  int n = 0;
  for (Lit l : lits_) {
    const int lvl = level[-l];
    if (lvl == 0 || lvl == INF) continue;
    levels[n] = lvl;  // add the level
    ++n;
    for (int32_t i = 0; i < n - 1; ++i) {
      if (levels[i] == lvl) {
        --n;
        levels[n] = 0;  // remove the level if it is found before
        break;
      }
    }
    if (n == MAXLBD) return MAXLBD;
  }
  return aux::max(n, 1);  // avoid returning 0
}

template <typename SMALL, typename LARGE>
unsigned int ConstrExp<SMALL, LARGE>::resolveWith(const std::span<const Lit>& data, unsigned int deg, ID id, Lit toProp,
                                                  const Solver& solver, const SymbolicBound* sb) {
  const IntMap<int>& level = solver.getLevel();
  const std::vector<int>& pos = solver.getPos();
  const int decisionLvl = solver.decisionLevel();

  assert(getCoef(-toProp) > 0);
  assert(hasNoZeroes());
  assert(sb == nullptr || sb->isValid());
  assert(isTrue(level, toProp));
  global.stats.NADDEDLITERALS += data.size();

  if (getDegree() == 1 && deg == 1) {
    // NOTE: no canceling literals for clausal resolution
    assert(std::ranges::count_if(data, [&](Lit l) { return hasLit(-l); }) == 1);
    symbBound.reset();
    tmpSlack = -1;
    tmpPrevSlack -= 2;  // takes resolving literal into account
    // resolving clauses with clauses can be done efficiently
    for (Lit l : data) {
      assert(coefs[toVar(l)] == 0 || coefs[toVar(l)] == aux::sgn(l) || l == toProp);
      assert(!isUnit(level, l));
      tmpPrevSlack += (level[-l] >= decisionLvl && !hasLit(l));
      if (!isUnit(level, -l)) {
        Var v = toVar(l);
        SMALL& c = coefs[v];
        if (c == 0) {
          assert(!used(v));
          rhs -= (l < 0);
          c = aux::sgn(l);
          index[v] = vars.size();
          vars.push_back(v);
        }
      }
    }
    rhs += (toProp > 0);
    remove(toVar(toProp));
    assert(isClause());  // clausal resolution yields new clauses

    if (global.logger.isActive()) {
      proofBuffer << id << " + s ";
      for (Lit l : data) {
        if (isUnit(level, -l)) {
          Logger::proofWeakenFalseUnit(proofBuffer, global.logger.getUnitID(l, pos), -1);
        }
      }
    }
    ++global.stats.NSATURATESTEPS;
  } else {
    LARGE oldDegree = getDegree();
    SMALL largestCF = 0;
    const SMALL cmult = getCoef(-toProp);
    assert(cmult >= 1);
    if (global.logger.isActive()) {
      Logger::proofMult(proofBuffer << id << " ", cmult) << "+ ";
      for (Lit l : data) {
        if (isUnit(level, l)) {
          Logger::proofWeaken(proofBuffer, l, -cmult);
        } else if (isUnit(level, -l)) {
          Logger::proofWeakenFalseUnit(proofBuffer, global.logger.getUnitID(l, pos), -cmult);
        }
      }
    }

    if (symbBound.isValid() && sb != nullptr) {
      symbBound.add(*sb, cmult);
    } else if (!symbBound.isValid() && sb != nullptr) {
      symbBound = *sb;
      symbBound.multiply(cmult);
      symbBound.addOffset(getDegree());
    } else if (symbBound.isValid() && sb == nullptr) {
      symbBound.addOffset(cmult * deg);
    }

    addRhs(cmult * deg);
    tmpSlack -= cmult * deg;
    tmpPrevSlack -= cmult * deg;
    for (Lit l : data) {
      // TODO: merge below two ifs
      if (!isFalse(level, l)) {
        tmpSlack += cmult;
        if (!isTrue(level, l)) {  // it is unknown
          const SMALL cf = getCoef(-l);
          if (cf > 0) tmpSlack -= aux::min(cf, cmult);
        }
      }
      if (level[-l] >= decisionLvl) {
        tmpPrevSlack += cmult;
        if (level[l] >= decisionLvl) {
          const SMALL cf = getCoef(-l);
          if (cf > 0) {
            tmpPrevSlack -= aux::min(cf, cmult);
            tmpPrevLargestCf = aux::max(tmpPrevLargestCf, aux::abs(cmult - cf));
          } else {
            tmpPrevLargestCf = aux::max(tmpPrevLargestCf, aux::abs(cmult + getCoef(l)));
          }
        }
      }
      if (isUnit(level, -l)) {
        continue;
      }
      if (isUnit(level, l)) {
        addRhs(-cmult);
        symbBound.addOffset(-cmult);
        continue;
      }
      Var v = toVar(l);
      SMALL cf = cmult;
      if (l < 0) {
        rhs -= cmult;
        cf = -cmult;
      }
      add(v, cf, true, true);
      largestCF = std::max(largestCF, aux::abs(coefs[v]));
    }
    assert(hasRhsDegreeInvariant());
    assert(getDegree() > 0);
    if (oldDegree <= getDegree()) {
      bool resetTmpPrevious = false;
      if (largestCF > getDegree()) {
        resetTmpPrevious = true;
        largestCF = static_cast<SMALL>(degree);
        const SMALL& smallDeg = largestCF;
        global.stats.NSATURATESTEPS += data.size();
        if (global.logger.isActive()) proofBuffer << "s ";
        if (!global.options.symbDegNoSat) symbBound.reset();
        for (Lit l : data) {
          Var v = toVar(l);
          if (coefs[v] < -smallDeg) {
            rhs -= coefs[v] + smallDeg;
            symbBound.addOffset(smallDeg + coefs[v]);
            coefs[v] = -smallDeg;
          } else {
            if (coefs[v] > smallDeg) {
              symbBound.addOffset(smallDeg - coefs[v]);
            }
            coefs[v] = std::min(coefs[v], smallDeg);
          }
        }
      }
      if (fixOverflow(level, decisionLvl, global.options.bitsOverflow.get(), global.options.bitsReduced.get(),
                      largestCF, 0) |
          resetTmpPrevious) {
        setTmpPrevious(level, decisionLvl);  // TODO: optimize in case largestCF is sufficiently large
      }
    } else {
      if (saturateAndFixOverflow(level, decisionLvl, global.options.bitsOverflow.get(),
                                 global.options.bitsReduced.get(), 0, false)) {
        setTmpPrevious(level, decisionLvl);
      }
    }
  }

  assert(getCoef(-toProp) == 0);
  assert(hasNegativeSlack(level));
  assert(hasCorrectTmpSlack(level));
  assert(hasCorrectTmpPrevious(level, decisionLvl));

  return calculateLbd(data, level);
}

//@post: variable vector vars is not changed, but coefs[toVar(toSubsume)] may become 0
template <typename SMALL, typename LARGE>
unsigned int ConstrExp<SMALL, LARGE>::subsumeWith(const std::span<const Lit>& data, unsigned int deg, ID id,
                                                  Lit toSubsume, const Solver& solver, IntSet& saturatedLits) {
  const IntMap<int>& level = solver.getLevel();
  const std::vector<int>& pos = solver.getPos();
  assert(isSaturated());
  assert(getCoef(-toSubsume) > 0);
  global.stats.NADDEDLITERALS += data.size();

  int weakenedDeg = deg;
  assert(weakenedDeg > 0);
  for (Lit l : data) {
    if (l != toSubsume && !isUnit(level, -l) && !saturatedLits.has(l)) {
      --weakenedDeg;
      if (weakenedDeg <= 0) {
        return 0;
      }
    }
  }
  assert(weakenedDeg > 0);
  SMALL& cf = coefs[toVar(toSubsume)];
  const SMALL mult = aux::abs(cf);
  if (cf < 0) {
    rhs -= cf;
  }
  cf = 0;
  saturatedLits.remove(-toSubsume);
  ++global.stats.NSUBSUMESTEPS;

  if (global.logger.isActive()) {
    proofBuffer << id << " ";
    for (Lit l : data) {
      if (isUnit(level, l)) {
        Logger::proofWeaken(proofBuffer, l, -1);
      } else if (isUnit(level, -l)) {
        Logger::proofWeakenFalseUnit(proofBuffer, global.logger.getUnitID(l, pos), -1);
      }
    }
    for (Lit l : data) {
      if (l != toSubsume && !isUnit(level, -l) && !isUnit(level, l) && !saturatedLits.has(l)) {
        Logger::proofWeaken(proofBuffer, l, -1);
      }
    }
    // saturate, multiply, divide, add, saturate
    Logger::proofMult(proofBuffer, mult) << "+ s ";
  }
  symbBound.reset();  // almost always some form of saturation going on

  return calculateLbd(data | std::views::filter([&](Lit l) { return l == toSubsume || saturatedLits.has(l); }), level);
}

template <typename SMALL, typename LARGE>
unsigned int ConstrExp<SMALL, LARGE>::resolveWith(const Lit* lits, const int* cfs, unsigned int size,
                                                  const int64_t& degr, ID id, Origin o, Lit l, const Solver& solver,
                                                  const SymbolicBound* sb) {
  return genericResolve(lits, cfs, size, degr, id, o, l, solver.getLevel(), solver.getPos(), solver.decisionLevel(),
                        solver.decisionPos(), sb);
}
template <typename SMALL, typename LARGE>
unsigned int ConstrExp<SMALL, LARGE>::resolveWith(const Lit* lits, const int64_t* cfs, unsigned int size,
                                                  const int128& degr, ID id, Origin o, Lit l, const Solver& solver,
                                                  const SymbolicBound* sb) {
  return genericResolve(lits, cfs, size, degr, id, o, l, solver.getLevel(), solver.getPos(), solver.decisionLevel(),
                        solver.decisionPos(), sb);
}
template <typename SMALL, typename LARGE>
unsigned int ConstrExp<SMALL, LARGE>::resolveWith(const Lit* lits, const int128* cfs, unsigned int size,
                                                  const int128& degr, ID id, Origin o, Lit l, const Solver& solver,
                                                  const SymbolicBound* sb) {
  return genericResolve(lits, cfs, size, degr, id, o, l, solver.getLevel(), solver.getPos(), solver.decisionLevel(),
                        solver.decisionPos(), sb);
}
template <typename SMALL, typename LARGE>
unsigned int ConstrExp<SMALL, LARGE>::resolveWith(const Lit* lits, const int128* cfs, unsigned int size,
                                                  const int256& degr, ID id, Origin o, Lit l, const Solver& solver,
                                                  const SymbolicBound* sb) {
  return genericResolve(lits, cfs, size, degr, id, o, l, solver.getLevel(), solver.getPos(), solver.decisionLevel(),
                        solver.decisionPos(), sb);
}
template <typename SMALL, typename LARGE>
unsigned int ConstrExp<SMALL, LARGE>::resolveWith(const Lit* lits, const bigint* cfs, unsigned int size,
                                                  const bigint& degr, ID id, Origin o, Lit l, const Solver& solver,
                                                  const SymbolicBound* sb) {
  return genericResolve(lits, cfs, size, degr, id, o, l, solver.getLevel(), solver.getPos(), solver.decisionLevel(),
                        solver.decisionPos(), sb);
}

template <typename SMALL, typename LARGE>
unsigned int ConstrExp<SMALL, LARGE>::subsumeWith(const Lit* lits, const int* cfs, unsigned int size,
                                                  const int64_t& degr, ID id, Lit l, const Solver& solver,
                                                  IntSet& saturatedLits) {
  return genericSubsume(lits, cfs, size, degr, id, l, solver.getLevel(), solver.getPos(), saturatedLits);
}
template <typename SMALL, typename LARGE>
unsigned int ConstrExp<SMALL, LARGE>::subsumeWith(const Lit* lits, const int64_t* cfs, unsigned int size,
                                                  const int128& degr, ID id, Lit l, const Solver& solver,
                                                  IntSet& saturatedLits) {
  return genericSubsume(lits, cfs, size, degr, id, l, solver.getLevel(), solver.getPos(), saturatedLits);
}
template <typename SMALL, typename LARGE>
unsigned int ConstrExp<SMALL, LARGE>::subsumeWith(const Lit* lits, const int128* cfs, unsigned int size,
                                                  const int128& degr, ID id, Lit l, const Solver& solver,
                                                  IntSet& saturatedLits) {
  return genericSubsume(lits, cfs, size, degr, id, l, solver.getLevel(), solver.getPos(), saturatedLits);
}
template <typename SMALL, typename LARGE>
unsigned int ConstrExp<SMALL, LARGE>::subsumeWith(const Lit* lits, const int128* cfs, unsigned int size,
                                                  const int256& degr, ID id, Lit l, const Solver& solver,
                                                  IntSet& saturatedLits) {
  return genericSubsume(lits, cfs, size, degr, id, l, solver.getLevel(), solver.getPos(), saturatedLits);
}
template <typename SMALL, typename LARGE>
unsigned int ConstrExp<SMALL, LARGE>::subsumeWith(const Lit* lits, const bigint* cfs, unsigned int size,
                                                  const bigint& degr, ID id, Lit l, const Solver& solver,
                                                  IntSet& saturatedLits) {
  return genericSubsume(lits, cfs, size, degr, id, l, solver.getLevel(), solver.getPos(), saturatedLits);
}

template struct ConstrExp<int, int64_t>;
template struct ConstrExp<int64_t, int128>;
template struct ConstrExp<int128, int128>;
template struct ConstrExp<int128, int256>;
template struct ConstrExp<bigint, bigint>;

}  // namespace xct
