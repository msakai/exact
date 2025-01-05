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

#include "IntConstraint.hpp"

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

void IntConstraint::lhs2str(std::ostream& o) const {
  std::vector<std::string> terms;
  terms.reserve(lhs.size());
  for (const IntTerm& t : lhs) {
    terms.push_back(aux::str(t));
  }
  std::sort(terms.begin(), terms.end());
  for (const std::string& s : terms) o << s << " ";
}

std::ostream& operator<<(std::ostream& o, const IntConstraint& x) {
  if (x.upperBound.has_value()) o << x.upperBound.value() << " >= ";
  x.lhs2str(o);
  if (x.lowerBound.has_value()) o << ">= " << x.lowerBound.value();
  return o;
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

LitVec IntVar::val2lits(const bigint& val) const {
  const VarVec& enc = getEncodingVars();
  LitVec res;
  res.reserve(enc.size());
  if (getEncoding() == Encoding::LOG) {
    bigint value = val - getLowerBound();
    assert(value >= 0);
    for (Var v : enc) {
      res.push_back(value % 2 == 0 ? -v : v);
      value /= 2;
    }
    assert(value == 0);
    return res;
  }
  assert(val - getLowerBound() <= getEncodingVars().size());
  int val_int = static_cast<int>(val - getLowerBound());
  if (getEncoding() == Encoding::ONEHOT) {
    for (int i = 0; i < (int)enc.size(); ++i) {
      res.push_back(i == val_int ? enc[i] : -enc[i]);
    }
    return res;
  }
  assert(getEncoding() == Encoding::ORDER);
  for (int i = 0; i < (int)enc.size(); ++i) {
    res.push_back(i < val_int ? enc[i] : -enc[i]);
  }
  return res;
}

IntTerm::IntTerm(const bigint& _c, IntVar* _v) : c(_c), v(_v) {}

IntTermVec IntConstraint::zip(const std::vector<bigint>& coefs, const std::vector<IntVar*>& vars) {
  assert(coefs.size() == vars.size());
  IntTermVec res;
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

}  // namespace xct
