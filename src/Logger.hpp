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

#include "Stats.hpp"
#include "datastructures/IntMap.hpp"
#include "typedefs.hpp"
#if WITHZLIB
#include "external/gzstream/gzstream.h"
// To reduce dependencies, we could also switch to the Boost version:
// https://stackoverflow.com/questions/624250/how-do-i-read-write-gzipped-files-in-c
#endif  // WITHZLIB

namespace xct {

class Logger {
  std::ofstream formula_out;
  std::ofstream proof_out;
#if WITHZLIB
  ogzstream proof_out_zip;
  ogzstream formula_out_zip;
  bool proof_is_zip = false;
#endif  // WITHZLIB

  std::vector<ID> unitIDs;
  [[maybe_unused]] const Stats& stats;
  bool active;

  std::ostream& proofStream();

  std::stringstream formula_constr;
  std::stringstream formula_obj;

 public:
  ID last_formID;
  ID last_proofID;

  explicit Logger(const Stats&);

  void activate(const std::string& proof_log_name, bool zip);
  bool isActive() const;
  void flush();
  void logComment([[maybe_unused]] const std::string& comment);

  ID logInput(const CeSuper& ce);
  void logObjective(const CeSuper& ce);
  ID logAssumption(const CeSuper& ce, bool allowed);
  ID logAssumption(ConstrExpSuper& ce, bool allowed);
  ID logProofLine(const CeSuper& ce);
  ID logProofLineWithInfo(const CeSuper& ce, [[maybe_unused]] const std::string& info);
  ID logUnsat(const CeSuper& ce, const IntMap<int>& level, const std::vector<int>& position);
  void logUnit(const CeSuper& ce);
  ID logRUP(Lit l, Lit ll);
  ID logImpliedUnit(Lit implying, Lit implied);
  ID logBottomUp(const CeSuper& ce);
  ID logUpperBound(const CeSuper& ce, const LitVec& lastSol);
  ID logPure(const CeSuper& ce);
  ID logDomBreaker(const CeSuper& ce);  // second lit is the witness
  ID logAtMostOne(const ConstrSimple32& c, const CeSuper& ce);
  ID logResolvent(ID id1, ID id2);
  std::pair<ID, ID> logEquality(Lit a, Lit b, ID aImpReprA, ID reprAImplA, ID bImpReprB, ID reprBImplB, Lit reprA,
                                Lit reprB);
  void logDeletion(ID id);
  void logAsCore(ID id);

  ID getUnitID(Lit l, const std::vector<int>& pos) const;
  int getNbUnitIDs() const;

 public:
  template <typename T>
  static std::ostream& proofMult(std::ostream& o, const T& m) {
    assert(m > 0);
    if (m != 1) o << m << " * ";
    return o;
  }
  template <typename T>
  static std::ostream& proofDiv(std::ostream& o, const T& d) {
    assert(d > 0);
    if (d != 1) o << d << " d ";
    return o;
  }
  template <typename T>
  static std::ostream& proofWeaken(std::ostream& o, Lit l, const T& m) {
    if (m == 0) return o;
    if ((m < 0) != (l < 0)) {
      o << "~";
    }
    return proofMult(o << "x" << toVar(l) << " ", aux::abs(m)) << "+ ";
  }
  template <typename T>
  static std::ostream& proofWeakenFalseUnit(std::ostream& o, ID id, const T& m) {
    assert(m < 0);
    return proofMult(o << id << " ", -m) << "+ ";
  }
  template <typename T>
  static std::ostream& proofIncreaseDegree(std::ostream& o, Lit l, const T& m) {
    if (m == 0) return o;
    if ((m < 0) != (l < 0)) {
      o << "~";
    }
    return proofMult(o << "x" << toVar(l) << " ", aux::abs(m)) << "+ ";
  }
};

}  // namespace xct
