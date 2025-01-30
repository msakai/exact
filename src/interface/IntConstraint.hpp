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

#pragma once

#include "Solver.hpp"
#include "typedefs.hpp"

namespace xct {
enum class Encoding { ORDER, LOG, ONEHOT };
Encoding opt2enc(const std::string& opt);

struct IntVar {
  const std::string name;
  const bigint lowerBound;
  const bigint upperBound;

  const Encoding encoding;
  const VarVec encodingVars;

  const int64_t id;

  explicit IntVar(const std::string& n, const bigint& lb, const bigint& ub, Encoding e, const VarVec& encvars,
                  int64_t id);

  [[nodiscard]] bigint getRange() const;
  [[nodiscard]] bool isBoolean() const;
  [[nodiscard]] bool isConstant() const;

  [[nodiscard]] bigint getValue(const LitVec& sol) const;

  [[nodiscard]] LitVec val2lits(const bigint& val) const;
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
  bool operator==(const IntTerm&) const = default;
};
std::ostream& operator<<(std::ostream& o, const IntTerm& x);
using IntTermVec = std::vector<IntTerm>;
}  // namespace xct

// template <>
// struct std::hash<xct::IntVar*> {
//   size_t operator()(xct::IntVar* iv) const noexcept;
// };
//
// template <>
// struct std::hash<xct::IntTerm> {
//   size_t operator()(const xct::IntTerm& it) const noexcept;
// };
//
// template <>
// struct std::hash<xct::IntTermVec> {
//   size_t operator()(const xct::IntTermVec& itv) const noexcept;
// };

namespace xct {

constexpr char CHAR_PLUS = static_cast<char>(255);
constexpr char CHAR_MINUS = static_cast<char>(254);
constexpr char CHAR_ONE = static_cast<char>(253);
constexpr char CHAR_MIN_ONE = static_cast<char>(252);

void encode_itv(const IntTermVec& itv, std::string& out);
void decode_itv(const std::string& code, const std::vector<IntVar*>& ivs, size_t start, IntTermVec& out);

struct IntConstraint {
  IntTermVec lhs = {};
  std::optional<bigint> lowerBound = 0;
  std::optional<bigint> upperBound = std::nullopt;

  bool operator==(const IntConstraint&) const = default;

  static IntTermVec zip(const std::vector<bigint>& coefs, const std::vector<IntVar*>& vars);

  [[nodiscard]] bigint getRange() const;
  [[nodiscard]] int64_t size() const;

  void invert();
  void normalize();

  void toConstrExp(CeArb&, bool useLowerBound) const;
  std::string encode() const;
  void decode(const std::string& code, const std::vector<IntVar*>& ivs);

  void lhs2str(std::ostream& o) const;
};
std::ostream& operator<<(std::ostream& o, const IntConstraint& x);

}  // namespace xct