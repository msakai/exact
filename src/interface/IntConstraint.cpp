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
  return o << x.name << "[" << x.lowerBound << "," << x.upperBound << "]";
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
  bool first = true;
  for (const std::string& s : terms) {
    if (!first) o << " ";
    first = false;
    o << s;
  }
}

std::ostream& operator<<(std::ostream& o, const IntConstraint& x) {
  if (x.upperBound.has_value()) o << x.upperBound.value() << " >= ";
  x.lhs2str(o);
  if (x.lowerBound.has_value()) o << " >= " << x.lowerBound.value();
  return o;
}

IntVar::IntVar(const std::string& n, const bigint& lb, const bigint& ub, Encoding e, const VarVec& encvars, int64_t i)
    : name(n), lowerBound(lb), upperBound(ub), encoding(e), encodingVars(encvars), id(i) {
  assert(lb <= ub);
  assert(encoding == Encoding::ORDER || getRange() > 1);
}

bigint IntVar::getRange() const { return upperBound - lowerBound; }  // TODO: Boolean range is 1?
bool IntVar::isBoolean() const { return lowerBound == 0 && upperBound == 1; }
bool IntVar::isConstant() const { return lowerBound == upperBound; }

bigint IntVar::getValue(const LitVec& sol) const {
  bigint val = lowerBound;
  if (encoding == Encoding::LOG) {
    bigint base = 1;
    for (Var v : encodingVars) {
      assert(v != 0);
      assert(v < (int)sol.size());
      assert(toVar(sol[v]) == v);
      if (sol[v] > 0) val += base;
      base *= 2;
    }
  } else if (encoding == Encoding::ORDER) {
    int sum = 0;
    for (Var v : encodingVars) {
      assert(v < (int)sol.size());
      assert(toVar(sol[v]) == v);
      sum += sol[v] > 0;
    }
    val += sum;
  } else {
    assert(encoding == Encoding::ONEHOT);
    int ith = 0;
    for (Var v : encodingVars) {
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
  const VarVec& enc = encodingVars;
  LitVec res;
  res.reserve(enc.size());
  if (encoding == Encoding::LOG) {
    bigint value = val - lowerBound;
    assert(value >= 0);
    for (Var v : enc) {
      res.push_back(value % 2 == 0 ? -v : v);
      value /= 2;
    }
    assert(value == 0);
    return res;
  }
  assert(val - lowerBound <= encodingVars.size());
  int val_int = static_cast<int>(val - lowerBound);
  if (encoding == Encoding::ONEHOT) {
    for (int i = 0; i < (int)enc.size(); ++i) {
      res.push_back(i == val_int ? enc[i] : -enc[i]);
    }
    return res;
  }
  assert(encoding == Encoding::ORDER);
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

void IntConstraint::normalize() {
  bigint offset = 0;
  for (IntTerm& it : lhs) {
    if (it.v->isConstant()) {
      offset += it.c * it.v->lowerBound;
      it.c = 0;
    }
  }
  if (lowerBound) {
    lowerBound = lowerBound.value() - offset;
  }
  if (upperBound) {
    upperBound = upperBound.value() - offset;
  }
  std::ranges::sort(lhs, [](const IntTerm& x, const IntTerm& y) { return x.v->id < y.v->id; });
  for (uint32_t i = 0; i < std::ssize(lhs) - 1; ++i) {
    if (lhs[i].v == lhs[i + 1].v) {
      // remove duplicates
      lhs[i + 1].c += lhs[i].c;
      lhs[i].c = 0;
    }
  }
  std::erase_if(lhs, [](const IntTerm& x) { return x.c == 0; });
  if (lhs.size() == 0) return;
  if (lhs[0].c < 0) {
    invert();
    std::swap(lowerBound, upperBound);
  }
  bigint gcd = aux::abs(lhs[0].c);
  for (const IntTerm& it : lhs) {
    gcd = std::min(gcd, aux::abs(it.c));
  }
  for (const IntTerm& it : lhs) {
    if (gcd == 1) return;
    gcd = aux::gcd(gcd, aux::abs(it.c));
  }
  if (gcd == 1) return;
  assert(gcd > 1);
  for (IntTerm& it : lhs) {
    it.c /= gcd;
  }
  if (lowerBound) {
    lowerBound = aux::ceildiv_safe(lowerBound.value(), gcd);
  }
  if (upperBound) {
    upperBound = aux::floordiv_safe(upperBound.value(), gcd);
  }
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
    if (t.v->lowerBound != 0) input->addRhs(-t.c * t.v->lowerBound);
    if (t.v->encoding == Encoding::LOG) {
      assert(!t.v->encodingVars.empty());
      bigint base = 1;
      for (const Var v : t.v->encodingVars) {
        input->addLhs(base * t.c, v);
        base *= 2;
      }
    } else if (t.v->encoding == Encoding::ORDER) {
      assert(t.v->isConstant() || !t.v->encodingVars.empty());
      for (const Var v : t.v->encodingVars) {
        input->addLhs(t.c, v);
      }
    } else {
      assert(t.v->encoding == Encoding::ONEHOT);
      assert(!t.v->encodingVars.empty());
      int ith = 0;
      for (const Var v : t.v->encodingVars) {
        input->addLhs(ith * t.c, v);
        ++ith;
      }
    }
  }
  if (!useLowerBound) input->invert();
}

template <typename T>
void encode_num(const T& num, std::string& result) {
  T val = aux::abs(num);
  while (val > 0) {
    result.push_back(static_cast<char>(val % 252));
    val /= 252;
  }
}

void encode_itv(const IntTermVec& itv, std::string& out) {
  for (const IntTerm& it : itv) {
    if (it.c == 1) {
      out.push_back(CHAR_ONE);
      encode_num(it.v->id, out);
      continue;
    }
    if (it.c == -1) {
      out.push_back(CHAR_MIN_ONE);
      encode_num(it.v->id, out);
      continue;
    }
    out.push_back(CHAR_PLUS);
    encode_num(it.v->id, out);
    out.push_back(it.c >= 0 ? CHAR_PLUS : CHAR_MINUS);
    encode_num(it.c, out);
  }
}

std::string IntConstraint::encode() const {
  std::string result;
  if (lowerBound) {
    result.push_back(lowerBound.value() >= 0 ? CHAR_PLUS : CHAR_MINUS);
    encode_num(lowerBound.value(), result);
  } else {
    result.push_back(CHAR_MIN_ONE);
  }
  assert(result[0] == CHAR_PLUS || result[0] == CHAR_MIN_ONE || result[0] == CHAR_MINUS);
  if (upperBound) {
    result.push_back(upperBound.value() >= 0 ? CHAR_PLUS : CHAR_MINUS);
    encode_num(upperBound.value(), result);
  } else {
    result.push_back(CHAR_MIN_ONE);
  }
  encode_itv(lhs, result);
  return result;
}

template <typename T>
T decode_num(const std::string& code, size_t& i, bool positive) {
  ++i;
  T result = 0;
  T basis = 1;
  while (i < code.size() && static_cast<uint8_t>(code[i]) < 252) {
    result += basis * static_cast<uint8_t>(code[i]);
    basis *= 252;
    ++i;
  }
  return positive ? result : -result;
}

void decode_itv(const std::string& code, const std::vector<IntVar*>& ivs, size_t start, IntTermVec& out) {
  out.clear();
  size_t i = start;
  while (i < code.size()) {
    const char& signal = code[i];
    IntVar* iv = ivs[decode_num<int64_t>(code, i, true)];
    if (signal == CHAR_ONE) {
      out.push_back({1, iv});
    } else if (signal == CHAR_MIN_ONE) {
      out.push_back({-1, iv});
    } else {
      assert(signal == CHAR_PLUS);
      assert(code[i] == CHAR_PLUS || code[i] == CHAR_MINUS);
      out.push_back({decode_num<bigint>(code, i, code[i] == CHAR_PLUS), iv});
    }
  }
}

void IntConstraint::decode(const std::string& code, const std::vector<IntVar*>& ivs) {
  lowerBound = std::nullopt;
  upperBound = std::nullopt;
  size_t i = 0;
  assert(i < code.size());
  if (code[i] != CHAR_MIN_ONE) {
    assert(code[i] == CHAR_PLUS || code[i] == CHAR_MINUS);
    lowerBound = decode_num<bigint>(code, i, code[i] == CHAR_PLUS);
  } else {
    ++i;
  }
  assert(i < code.size());
  if (code[i] != CHAR_MIN_ONE) {
    assert(code[i] == CHAR_PLUS || code[i] == CHAR_MINUS);
    upperBound = decode_num<bigint>(code, i, code[i] == CHAR_PLUS);
  } else {
    ++i;
  }
  decode_itv(code, ivs, i, lhs);
}

}  // namespace xct
