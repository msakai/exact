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

#include "auxiliary.hpp"

namespace xct {

using ID = uint64_t;
constexpr ID ID_Undef = 0;
constexpr ID ID_Trivial = 1;  // represents constraint 0 >= 0
inline bool isValid(ID id) { return id != ID_Undef; }

constexpr unsigned int MAXLBD = 1e3;

using Var = int32_t;
using Lit = int32_t;
inline Var toVar(Lit l) { return std::abs(l); }
using VarVec = std::vector<Var>;
using LitVec = std::vector<Lit>;

constexpr int32_t resize_factor = 2;

constexpr int32_t INF =
    1e9 + 1;  // 1e9 < 30 bits is the maximum number of variables in the system, anything beyond is infinity
constexpr uint32_t UINF = INF;
constexpr uint32_t CLAUSE_IDX = 4 * UINF;
constexpr uint32_t BINARY_IDX = 4 * UINF + 1;
// NOTE: 31 bits is not possible due to the idx entry in the Watch struct
constexpr int64_t INFLPINT = 4e15 + 1;  // 4e15 < 52 bits, based on max int64_t range captured by double

template <typename CF, typename DG>
inline double limitAbs() {
  // static_assert(false);
  assert(false);
  return 0;
}
template <>
inline double limitAbs<bigint, bigint>() {
  return 0;
}
template <>
inline double limitAbs<int, int64_t>() {
  return 1e9;  // 2^29-2^30
}
template <>
inline double limitAbs<int64_t, int128>() {
  return 2e18;  // 2^60-2^61
}
template <>
inline double limitAbs<int128, int128>() {
  return 9e27;  // 2^92-2^93
}
template <>
inline double limitAbs<int128, int256>() {
  return 4e37;  // 2^124-2^125
}
template <>
inline double limitAbs<int256, bigint>() {
  return 1e76;  // 2^252-2^253
}

template <typename CF, typename DG>
bigint limitRhs() {
  return static_cast<bigint>(limitAbs<CF, DG>() * 1e9 * 2 + 1);
  // sufficiently high that any larger RHS value will lead to inconsistency of constraint
  // sufficiently small that degree or rhs can safely be derived still (difference is at most 1e9*limitAbs)
}

template <typename CF, typename DG>
inline int limitBit() {
  // static_assert(false);
  assert(false);
  return -1;
}
template <>
inline int limitBit<int, int64_t>() {
  return 29;
}
template <>
inline int limitBit<int64_t, int128>() {
  return 60;
}
template <>
inline int limitBit<int128, int128>() {
  return 92;
}
template <>
inline int limitBit<int128, int256>() {
  return 124;
}
template <>
inline int limitBit<int256, bigint>() {
  return 252;
}
template <>
inline int limitBit<bigint, bigint>() {
  return std::numeric_limits<int>::max();  // NOTE: limits numbers to 2^(2^31)-1, which should be plenty
}

template <typename CF, typename DG>
inline int limitBitConfl() {
  return limitBit<CF, DG>() / 2;
}

template <typename T>
bool fits([[maybe_unused]] const bigint& x) {
  return false;
}
template <>
inline bool fits<int>(const bigint& x) {
  return aux::abs(x) <= static_cast<bigint>(limitAbs<int, int64_t>());
}
template <>
inline bool fits<int64_t>(const bigint& x) {
  return aux::abs(x) <= static_cast<bigint>(limitAbs<int64_t, int128>());
}
template <>
inline bool fits<int128>(const bigint& x) {
  return aux::abs(x) <= static_cast<bigint>(limitAbs<int128, int256>());
}
template <>
inline bool fits<int256>(const bigint& x) {
  return aux::abs(x) <= static_cast<bigint>(limitAbs<int256, bigint>());
}
template <>
inline bool fits<bigint>([[maybe_unused]] const bigint& x) {
  return true;
}
template <typename T, typename S>
bool fitsIn([[maybe_unused]] const S& x) {
  return fits<T>(bigint(x));
}

template <typename T>
bool stillFits([[maybe_unused]] const T& x) {
  return false;
}
template <>
inline bool stillFits<int>(const int& x) {
  return aux::abs(x) <= limitAbs<int, int64_t>();
}
template <>
inline bool stillFits<int64_t>(const int64_t& x) {
  return aux::abs(x) <= limitAbs<int64_t, int128>();
}
template <>
inline bool stillFits<int128>(const int128& x) {
  return aux::abs(x) <= static_cast<int128>(limitAbs<int128, int256>());
}
template <>
inline bool stillFits<int256>(const int256& x) {
  return aux::abs(x) <= static_cast<int256>(limitAbs<int256, bigint>());
}
template <>
inline bool stillFits<bigint>([[maybe_unused]] const bigint& x) {
  return true;
}

using ActValV = long double;
using DetTime = long double;
using StatNum = long double;

// NOTE: max number of types is 32, as the type is stored with 5 bits in Constr
enum class Origin {
  UNKNOWN,      // uninitialized
  FORMULA,      // original input formula
  DOMBREAKER,   // dominance breaking
  INVALIDATOR,  // solution-invalidating constraint
  PURE,         // pure unit literal
  COREGUIDED,   // extension constraints from coreguided optimization
  BOTTOMUP,     // simple bottom-up optimization constraint
  REFORMBOUND,  // upper bound on the reformed objective function
  UPPERBOUND,   // upper bound on the objective function
  LOWERBOUND,   // lower bound on the objective function
  LEARNED,      // learned from regular conflict analysis
  FARKAS,       // LP solver infeasibility witness
  DUAL,         // LP solver feasibility dual constraint
  GOMORY,       // Gomory cut
  PROBING,      // probing unit literal
  DETECTEDAMO,  // detected cardinality constraint
  REDUCED,      // reduced constraint
  EQUALITY,     // equality enforcing constraint
  IMPLICATION,  // binary implication clause
};
inline std::ostream& operator<<(std::ostream& o, enum Origin orig) {
  switch (orig) {
    case (Origin::UNKNOWN):
      o << "UNKNOWN";
      break;
    case (Origin::FORMULA):
      o << "FORMULA";
      break;
    case (Origin::DOMBREAKER):
      o << "DOMBREAKER";
      break;
    case (Origin::INVALIDATOR):
      o << "INVALIDATOR";
      break;
    case (Origin::PURE):
      o << "PURE";
      break;
    case (Origin::COREGUIDED):
      o << "COREGUIDED";
      break;
    case (Origin::BOTTOMUP):
      o << "BOTTOMUP";
      break;
    case (Origin::REFORMBOUND):
      o << "REFORMBOUND";
      break;
    case (Origin::UPPERBOUND):
      o << "UPPERBOUND";
      break;
    case (Origin::LOWERBOUND):
      o << "LOWERBOUND";
      break;
    case (Origin::LEARNED):
      o << "LEARNED";
      break;
    case (Origin::FARKAS):
      o << "FARKAS";
      break;
    case (Origin::DUAL):
      o << "DUAL";
      break;
    case (Origin::GOMORY):
      o << "GOMORY";
      break;
    case (Origin::PROBING):
      o << "PROBING";
      break;
    case (Origin::DETECTEDAMO):
      o << "DETECTEDAMO";
      break;
    case (Origin::REDUCED):
      o << "REDUCED";
      break;
    case (Origin::EQUALITY):
      o << "EQUALITY";
      break;
    case (Origin::IMPLICATION):
      o << "IMPLICATION";
      break;
    default:
      assert(false);
  }
  return o;
}

inline bool isNonImplied(Origin o) {
  return o == Origin::FORMULA || o == Origin::DOMBREAKER || o == Origin::INVALIDATOR;
}
inline bool isBound(Origin o) { return o == Origin::UPPERBOUND || o == Origin::LOWERBOUND || o == Origin::REFORMBOUND; }
inline bool isExternal(Origin o) { return isBound(o) || o == Origin::COREGUIDED || o == Origin::BOTTOMUP; }
inline bool isInput(Origin o) { return o != Origin::UNKNOWN && o < Origin::LEARNED; }
inline bool isLearned(Origin o) { return o >= Origin::LEARNED; }

template <typename SMALL, typename LARGE>
struct ConstrExp;
using ConstrExp32 = ConstrExp<int, int64_t>;
using ConstrExp64 = ConstrExp<int64_t, int128>;
using ConstrExp96 = ConstrExp<int128, int128>;
using ConstrExp128 = ConstrExp<int128, int256>;
using ConstrExpArb = ConstrExp<bigint, bigint>;
struct ConstrExpSuper;

template <typename SMALL, typename LARGE>
using CePtr = std::shared_ptr<ConstrExp<SMALL, LARGE>>;
using Ce32 = std::shared_ptr<ConstrExp32>;
using Ce64 = std::shared_ptr<ConstrExp64>;
using Ce96 = std::shared_ptr<ConstrExp96>;
using Ce128 = std::shared_ptr<ConstrExp128>;
using CeArb = std::shared_ptr<ConstrExpArb>;
using CeSuper = std::shared_ptr<ConstrExpSuper>;
using CeNull = std::shared_ptr<ConstrExp32>;

template <typename CF, typename DG>
struct ConstrSimple;
using ConstrSimple32 = ConstrSimple<int, int64_t>;
using ConstrSimple64 = ConstrSimple<int64_t, int128>;
using ConstrSimple96 = ConstrSimple<int128, int128>;
using ConstrSimple128 = ConstrSimple<int128, int256>;
using ConstrSimpleArb = ConstrSimple<bigint, bigint>;
struct ConstrSimpleSuper;

struct Constr;
struct Clause;
struct Cardinality;

template <typename CF, typename DG>
struct Watched;
template <typename CF, typename DG>
struct Watched;
using Watched64 = Watched<int64_t, int128>;
using Watched96 = Watched<int128, int128>;
using Watched128 = Watched<int128, int256>;
using WatchedArb = Watched<bigint, bigint>;

template <typename CF>
struct Term {
  Term() : c(0), l(0) {}
  Term(const CF& x, Lit y) : c(x), l(y) {}
  ~Term() = default;
  CF c;
  Lit l;
};

using Term32 = Term<int>;
using Term64 = Term<int64_t>;
using Term128 = Term<int128>;
using TermArb = Term<bigint>;

template <typename CF>
std::ostream& operator<<(std::ostream& o, const Term<CF>& t) {
  return o << t.c << "x" << t.l;
}

template <typename CF>
std::ostream& operator<<(std::ostream& o, const std::pair<CF, Lit>& cl) {
  return o << (cl.first < 0 ? "" : "+") << cl.first << (cl.second < 0 ? " ~x" : " x") << toVar(cl.second);
}

class EarlyTermination : public std::exception {
 public:
  [[nodiscard]] const char* what() const noexcept override { return "Program terminated early."; }
};

class AsynchronousInterrupt : public std::exception {
 public:
  [[nodiscard]] const char* what() const noexcept override { return "Program interrupted by user."; }
};

class UnsatEncounter : public std::exception {
 public:
  //  UnsatEncounter() : std::exception() { assert(false); }
  [[nodiscard]] const char* what() const noexcept override {
    return "UNSAT state reached, this exception should have been caught.";
  }
};

class InvalidArgument : public std::invalid_argument {
 public:
  InvalidArgument(const std::string& what_arg) : invalid_argument(what_arg) {
    std::cerr << "Invalid argument error: " << what_arg << std::endl;
  }
};

}  // namespace xct
