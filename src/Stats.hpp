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
#include "typedefs.hpp"

namespace xct {

struct Stat {
  StatNum z;
  std::string name;
};

std::ostream& operator<<(std::ostream& o, const Stat& stat);

void operator++(Stat& stat);
void operator--(Stat& stat);
template <typename IN>
Stat& operator+=(Stat& stat, const IN& rhs) {
  stat.z += static_cast<StatNum>(rhs);
  return stat;
}
template <typename IN>
Stat& operator-=(Stat& stat, const IN& rhs) {
  stat.z -= static_cast<StatNum>(rhs);
  return stat;
}
template <typename IN>
StatNum operator+(const Stat& stat, const IN& in) {
  return stat.z + static_cast<StatNum>(in);
}
template <typename IN>
StatNum operator+(const IN& in, const Stat& stat) {
  return static_cast<StatNum>(in) + stat.z;
}
StatNum operator+(const Stat& x, const Stat& y);
template <typename IN>
StatNum operator-(const Stat& stat, const IN& in) {
  return stat.z - static_cast<StatNum>(in);
}
template <typename IN>
StatNum operator-(const IN& in, const Stat& stat) {
  return static_cast<StatNum>(in) - stat.z;
}
StatNum operator-(const Stat& x, const Stat& y);
template <typename IN>
StatNum operator*(const Stat& stat, const IN& in) {
  return stat.z * static_cast<StatNum>(in);
}
template <typename IN>
StatNum operator*(const IN& in, const Stat& stat) {
  return static_cast<StatNum>(in) * stat.z;
}
StatNum operator*(const Stat& x, const Stat& y);
template <typename IN>
StatNum operator/(const Stat& stat, const IN& in) {
  return stat.z / static_cast<StatNum>(in);
}
template <typename IN>
StatNum operator/(const IN& in, const Stat& stat) {
  return static_cast<StatNum>(in) / stat.z;
}
StatNum operator/(const Stat& x, const Stat& y);
template <typename IN>
bool operator==(const Stat& stat, const IN& in) {
  return stat.z == static_cast<StatNum>(in);
}
template <typename IN>
bool operator==(const IN& in, const Stat& stat) {
  return static_cast<StatNum>(in) == stat.z;
}
bool operator==(const Stat& x, const Stat& y);
template <typename IN>
bool operator>(const Stat& stat, const IN& in) {
  return stat.z > static_cast<StatNum>(in);
}
template <typename IN>
bool operator>(const IN& in, const Stat& stat) {
  return static_cast<StatNum>(in) > stat.z;
}
bool operator>(const Stat& x, const Stat& y);
template <typename IN>
bool operator<(const Stat& stat, const IN& in) {
  return stat.z < static_cast<StatNum>(in);
}
template <typename IN>
bool operator<(const IN& in, const Stat& stat) {
  return static_cast<StatNum>(in) < stat.z;
}
bool operator<(const Stat& x, const Stat& y);
template <typename IN>
bool operator<=(const Stat& stat, const IN& in) {
  return stat.z <= static_cast<StatNum>(in);
}
template <typename IN>
bool operator<=(const IN& in, const Stat& stat) {
  return static_cast<StatNum>(in) <= stat.z;
}
bool operator<=(const Stat& x, const Stat& y);
template <typename IN>
bool operator>=(const Stat& stat, const IN& in) {
  return stat.z >= static_cast<StatNum>(in);
}
template <typename IN>
bool operator>=(const IN& in, const Stat& stat) {
  return static_cast<StatNum>(in) >= stat.z;
}
bool operator>=(const Stat& x, const Stat& y);

struct Stats {
  Stat NTRAILPOPS{0, "trail pops"};
  Stat NWATCHLOOKUPS{0, "watch lookups"};
  Stat NWATCHLOOKUPSBJ{0, "watch backjump lookups"};
  Stat NWATCHCHECKS{0, "watch checks"};
  Stat NPROPCHECKS{0, "propagation checks"};
  Stat NBLOCKINGSUCCESS{0, "blocking literal success"};
  Stat NBLOCKINGFAILS{0, "blocking literal fails"};

  Stat NADDEDLITERALS{0, "literal additions"};
  Stat NSATURATESTEPS{0, "saturation steps"};
  Stat NUNKNOWNROUNDEDUP{0, "unknown literals rounded up"};

  Stat NCONFL{0, "conflicts"};
  Stat NDECIDE{0, "decisions"};
  Stat NPROP{0, "propagations"};
  Stat NPROPCLAUSE{0, "clausal propagations"};
  Stat NPROPCARD{0, "cardinality propagations"};
  Stat NPROPWATCH{0, "watched propagations"};
  Stat NPROPCOUNTING{0, "counting propagations"};
  Stat NRESOLVESTEPS{0, "resolve steps"};
  Stat NSUBSUMESTEPS{0, "self-subsumptions"};

  Stat EXTERNLENGTHSUM{0, "input length sum"};
  Stat EXTERNDEGREESUM{0, "input degree sum"};
  Stat LEARNEDLENGTHSUM{0, "learned length sum"};
  Stat LEARNEDDEGREESUM{0, "learned degree sum"};
  Stat LEARNEDLBDSUM{0, "learned LBD sum"};

  Stat NUNITS{0, "unit literals derived"};
  Stat NPURELITS{0, "pure literals"};
  Stat NSATISFIEDSREMOVED{0, "constraints satisfied at root"};
  Stat NCONSREADDED{0, "constraints simplified during database reduction"};

  Stat NPROBINGLITS{0, "unit lits due to probing"};
  Stat NPROBINGEQS{0, "equalities due to probing"};
  Stat NPROBINGIMPLS{0, "implications added due to probing"};
  Stat NPROBINGIMPLMEM{0, "max implications in memory due to probing"};
  Stat NPROBINGS{0, "probing calls"};
  Stat PROBETIME{0, "probing inprocessing time"};
  Stat ATMOSTONES{0, "detected at-most-ones"};
  Stat ATMOSTONETIME{0, "at-most-one detection time"};
  Stat ATMOSTONEDETTIME{0, "at-most-one detection time det"};
  Stat NATMOSTONEUNITS{0, "units derived during at-most-one detection"};

  Stat PARSETIME{0, "parse time"};
  Stat SOLVETIMETOPDOWN{0, "top-down time"};
  Stat DETTIMETOPDOWN{0, "top-down time det"};
  Stat SOLVETIMEBOTTOMUP{0, "bottom-up solve time"};
  Stat DETTIMEBOTTOMUP{0, "bottom-up solve time det"};
  Stat CATIME{0, "conflict analysis time"};
  Stat MINTIME{0, "learned minimize time"};
  Stat PROPTIME{0, "propagation time"};

  Stat NCLAUSESEXTERN{0, "input clauses"};
  Stat NCARDINALITIESEXTERN{0, "input cardinalities"};
  Stat NGENERALSEXTERN{0, "input general constraints"};
  Stat NCLAUSESLEARNED{0, "learned clauses"};
  Stat NCARDINALITIESLEARNED{0, "learned cardinalities"};
  Stat NGENERALSLEARNED{0, "learned general constraints"};

  Stat NSMALL{0, "small coef constraints"};
  Stat NLARGE{0, "large coef constraints"};
  Stat NARB{0, "arbitrary coef constraints"};

  Stat NCLEANUP{0, "inprocessing phases"};
  Stat NRESTARTS{0, "restarts"};
  Stat NCORES{0, "cores"};
  Stat NSOLS{0, "solutions"};
  Stat NGCD{0, "gcd simplifications"};
  Stat NCARDDETECT{0, "detected cardinalities"};
  Stat NWEAKENEDNONIMPLYING{0, "weakened non-implying"};
  Stat NWEAKENEDNONIMPLIED{0, "weakened non-implied"};
  Stat NMULTWEAKENEDREASON{0, "multiply-weakens on reason"};
  Stat NMULTWEAKENEDCONFLICT{0, "multiply-weakens on conflict"};
  Stat NMULTWEAKENEDDIRECT{0, "direct multiply-weakens"};
  Stat NMULTWEAKENEDINDIRECT{0, "indirect multiply-weakens"};
  Stat NSYMBBOUND{0, "symbolic bound improvements"};
  Stat NSYMBBOUNDADDED{0, "added constraints with symbolic bounds"};
  Stat NORIGVARS{0, "original variables"};
  Stat NAUXVARS{0, "auxiliary variables"};

  Stat NCONSFORMULA{0, "formula constraints"};
  Stat NCONSLEARNED{0, "learned constraints"};
  Stat NCONSBOUND{0, "bound constraints"};
  Stat NCONSCOREGUIDED{0, "core-guided constraints"};
  Stat NCONSDOMBREAKER{0, "dominance breaking constraints"};
  Stat NCONSREDUCED{0, "reduced constraints"};
  Stat NENCFORMULA{0, "encountered formula constraints"};
  Stat NENCDOMBREAKER{0, "encountered dominance breaking constraints"};
  Stat NENCLEARNED{0, "encountered learned constraints"};
  Stat NENCBOUND{0, "encountered bound constraints"};
  Stat NENCCOREGUIDED{0, "encountered core-guided constraints"};
  Stat NENCREDUCED{0, "encountered reduced constraints"};
  Stat NENCDETECTEDAMO{0, "encountered detected at-most-ones"};
  Stat NENCEQ{0, "encountered detected equalities"};
  Stat NENCIMPL{0, "encountered detected implications"};

  Stat LPSOLVETIME{0, "LP solve time"};
  Stat LPTOTALTIME{0, "LP total time"};
  Stat LPDETTIME{0, "LP total time det"};

  Stat NLPADDEDROWS{0, "LP constraints added"};
  Stat NLPDELETEDROWS{0, "LP constraints removed"};
  Stat NLPPIVOTS{0, "LP pivots"};
  Stat NLPOPERATIONS{0, "LP approximate operations"};
  Stat NLPADDEDLITERALS{0, "LP literal additions"};
  Stat NLPNOPIVOT{0, "LP no pivot count"};
  Stat NLPRESETBASIS{0, "LP basis resets"};
  Stat NLPCALLS{0, "LP calls"};
  Stat NLPOPTIMAL{0, "LP optimalities"};
  Stat NLPINFEAS{0, "LP infeasibilities"};
  Stat NLPFARKAS{0, "LP Farkas constraints"};
  Stat NLPDUAL{0, "LP dual constraints"};
  Stat NLPCYCLING{0, "LP cycling count"};
  Stat NLPNOPRIMAL{0, "LP no primal count"};
  Stat NLPNODUAL{0, "LP no dual count"};
  Stat NLPNOFARKAS{0, "LP no farkas count"};
  Stat NLPSINGULAR{0, "LP singular count"};
  Stat NLPOTHER{0, "LP other issue count"};
  Stat NLPGOMORYCUTS{0, "LP Gomory cuts"};
  Stat NLPLEARNEDCUTS{0, "LP learned cuts"};
  Stat NLPDELETEDCUTS{0, "LP deleted cuts"};
  Stat NLPENCGOMORY{0, "LP encountered Gomory constraints"};
  Stat NLPENCFARKAS{0, "LP encountered Farkas constraints"};
  Stat NLPENCDUAL{0, "LP encountered dual constraints"};
  Stat LPOBJ{std::numeric_limits<StatNum>::quiet_NaN(), "LP relaxation objective"};

  Stat NCGUNITCORES{0, "CG unit cores"};
  Stat NCGNONCLAUSALCORES{0, "CG non-clausal cores"};

  // derived statistics
  Stat CPUTIME{0, "cpu time"};
  Stat SOLVETIME{0, "solve time"};
  Stat DETTIME{0, "solve time det"};
  Stat OPTTIME{0, "optimization time"};
  Stat CLEANUPTIME{0, "constraint cleanup time"};
  Stat INPROCESSTIME{0, "inprocessing time"};
  Stat GCTIME{0, "garbage collection time"};
  Stat LEARNTIME{0, "constraint learning time"};
  Stat HEURTIME{0, "time spent in activity heuristic"};

  Stat EXTERNLENGTHAVG{0, "input length average"};
  Stat EXTERNDEGREEAVG{0, "input degree average"};
  Stat LEARNEDLENGTHAVG{0, "learned length average"};
  Stat LEARNEDDEGREEAVG{0, "learned degree average"};
  Stat LEARNEDLBDAVG{0, "learned LBD average"};

  Stat LASTLB{std::numeric_limits<StatNum>::quiet_NaN(), "best lower bound"};
  Stat LASTUB{std::numeric_limits<StatNum>::quiet_NaN(), "best upper bound"};

  Stat SUBSETSUMTIME{0, "time spent in subsetsum optimizations"};
  Stat NLIFTDEGREE{0, "lifted degrees"};
  Stat NSUPERFLUOUS{0, "superfluous literals"};
  Stat NSUPERFLUOUSPART{0, "partially superfluous literals"};

  std::chrono::steady_clock::time_point startTime;
  std::chrono::steady_clock::time_point runStartTime;

  void setDerivedStats(const StatNum& lowerbound, const StatNum& upperbound) {
    DETTIME.z = getDetTime();
    CPUTIME.z = getTime();
    SOLVETIME.z = getRunTime();
    OPTTIME.z = SOLVETIME - getSolveTime();
    LPDETTIME.z = getLpDetTime();

    StatNum nonLearneds = NCLAUSESEXTERN + NCARDINALITIESEXTERN + NGENERALSEXTERN;
    EXTERNLENGTHAVG.z = (nonLearneds == 0 ? 0 : EXTERNLENGTHSUM / nonLearneds);
    EXTERNDEGREEAVG.z = (nonLearneds == 0 ? 0 : EXTERNDEGREESUM / nonLearneds);
    StatNum learneds = NCLAUSESLEARNED + NCARDINALITIESLEARNED + NGENERALSLEARNED;
    LEARNEDLENGTHAVG.z = (learneds == 0 ? 0 : LEARNEDLENGTHSUM / learneds);
    LEARNEDDEGREEAVG.z = (learneds == 0 ? 0 : LEARNEDDEGREESUM / learneds);
    LEARNEDLBDAVG.z = (learneds == 0 ? 0 : LEARNEDLBDSUM / learneds);

    LASTLB.z = lowerbound;
    LASTUB.z = upperbound;
  }

  const std::vector<Stat*> statsToDisplay = {
      &CPUTIME,
      &PARSETIME,
      &SOLVETIME,
      &DETTIME,
      &OPTTIME,
      &SOLVETIMETOPDOWN,
      &DETTIMETOPDOWN,
      &SOLVETIMEBOTTOMUP,
      &DETTIMEBOTTOMUP,
      &CATIME,
      &MINTIME,
      &PROPTIME,
      &CLEANUPTIME,
      &INPROCESSTIME,
      &GCTIME,
      &LEARNTIME,
      &HEURTIME,
      &ATMOSTONETIME,
      &ATMOSTONEDETTIME,
      &NSYMBBOUND,
      &NSYMBBOUNDADDED,
#if WITHSOPLEX
      &LPSOLVETIME,
      &LPTOTALTIME,
      &LPDETTIME,
#endif  // WITHSOPLEX
      &NCORES,
      &NSOLS,
      &NPROP,
      &NDECIDE,
      &NCONFL,
      &NRESTARTS,
      &NCLEANUP,
      &NORIGVARS,
      &NAUXVARS,
      &NCLAUSESEXTERN,
      &NCARDINALITIESEXTERN,
      &NGENERALSEXTERN,
      &EXTERNLENGTHAVG,
      &EXTERNDEGREEAVG,
      &NCLAUSESLEARNED,
      &NCARDINALITIESLEARNED,
      &NGENERALSLEARNED,
      &LEARNEDLENGTHAVG,
      &LEARNEDDEGREEAVG,
      &LEARNEDLBDAVG,
      &NUNITS,
      &NPURELITS,
      &NSATISFIEDSREMOVED,
      &NCONSREADDED,
      &NSMALL,
      &NLARGE,
      &NARB,
      &NPROBINGS,
      &PROBETIME,
      &NPROBINGLITS,
      &NPROBINGEQS,
      &NPROBINGIMPLS,
      &NPROBINGIMPLMEM,
      &ATMOSTONES,
      &NATMOSTONEUNITS,
      &NRESOLVESTEPS,
      &NSUBSUMESTEPS,
      &NGCD,
      &NCARDDETECT,
      &NWEAKENEDNONIMPLIED,
      &NWEAKENEDNONIMPLYING,
      &NMULTWEAKENEDREASON,
      &NMULTWEAKENEDCONFLICT,
      &NMULTWEAKENEDDIRECT,
      &NMULTWEAKENEDINDIRECT,
      &NPROPCLAUSE,
      &NPROPCARD,
      &NPROPWATCH,
      &NPROPCOUNTING,
      &NWATCHLOOKUPS,
      &NWATCHLOOKUPSBJ,
      &NWATCHCHECKS,
      &NPROPCHECKS,
      &NBLOCKINGSUCCESS,
      &NBLOCKINGFAILS,
      &NADDEDLITERALS,
      &NSATURATESTEPS,
      &NUNKNOWNROUNDEDUP,
      &NTRAILPOPS,
      &NCONSFORMULA,
      &NCONSDOMBREAKER,
      &NCONSLEARNED,
      &NCONSBOUND,
      &NCONSCOREGUIDED,
      &NCONSREDUCED,
      &NENCFORMULA,
      &NENCDOMBREAKER,
      &NENCLEARNED,
      &NENCBOUND,
      &NENCCOREGUIDED,
      &NENCREDUCED,
      &NENCDETECTEDAMO,
      &NENCEQ,
      &NENCIMPL,
      &NCGUNITCORES,
      &NCGNONCLAUSALCORES,
      &LASTUB,
      &LASTLB,
#if WITHSOPLEX
      &LPOBJ,
      &NLPADDEDROWS,
      &NLPDELETEDROWS,
      &NLPPIVOTS,
      &NLPOPERATIONS,
      &NLPADDEDLITERALS,
      &NLPCALLS,
      &NLPOPTIMAL,
      &NLPNOPIVOT,
      &NLPINFEAS,
      &NLPFARKAS,
      &NLPDUAL,
      &NLPRESETBASIS,
      &NLPCYCLING,
      &NLPSINGULAR,
      &NLPNOPRIMAL,
      &NLPNODUAL,
      &NLPNOFARKAS,
      &NLPOTHER,
      &NLPGOMORYCUTS,
      &NLPLEARNEDCUTS,
      &NLPDELETEDCUTS,
      &NLPENCGOMORY,
      &NLPENCFARKAS,
      &NLPENCDUAL,
#endif  // WITHSOPLEX
      &SUBSETSUMTIME,
      &NLIFTDEGREE,
      &NSUPERFLUOUS,
      &NSUPERFLUOUSPART,
  };

  [[nodiscard]] StatNum getTime() const;
  [[nodiscard]] StatNum getRunTime() const;
  [[nodiscard]] StatNum getSolveTime() const;
  // NOTE: below linear relations were determined by regression tests on experimental data,
  // so that the deterministic time correlates as closely as possible with the cpu time in seconds
  [[nodiscard]] StatNum getLpDetTime() const;
  [[nodiscard]] StatNum getNonLpDetTime() const;
  [[nodiscard]] StatNum getDetTime() const;
  [[nodiscard]] int64_t getNConfl() const;

  void print(const StatNum& lowerbound, const StatNum& upperbound);
  void printCsvLine(const StatNum& lowerbound, const StatNum& upperbound);
  void printCsvHeader();
};

}  // namespace xct
