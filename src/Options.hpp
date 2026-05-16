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
#include "quit.hpp"
#include "typedefs.hpp"
#include "used_licenses/licenses.hpp"

namespace xct {

class Option {
 public:
  const std::string name;
  const std::string description;

  Option(const std::string& n, const std::string& d) : name(n), description(d) {}

  virtual void printUsage(int colwidth) const = 0;
  virtual void parse(const std::string& v) = 0;
};

class VoidOption : public Option {
  bool val = false;

 public:
  VoidOption(const std::string& n, const std::string& d);

  explicit operator bool() const { return val; }

  void printUsage(int colwidth) const override;
  void parse([[maybe_unused]] const std::string& v) override;
};

class BoolOption : public Option {
  bool val = false;

 public:
  BoolOption(const std::string& n, const std::string& d, bool v);

  explicit operator bool() const { return val; }
  void set(bool v);

  void printUsage(int colwidth) const override;
  void parse(const std::string& v) override;
};

template <typename T>
class ValOption : public Option {
  T val;
  std::string checkDescription;
  std::function<bool(const T&)> check;

 public:
  ValOption(const std::string& n, const std::string& d, const T& v, const std::string& cd,
            const std::function<bool(const T&)>& c)
      : Option{n, d}, val(v), checkDescription(cd), check(c) {}

  const T& get() const { return val; }
  void set(const T& v) { val = v; }

  void printUsage(int colwidth) const override {
    std::stringstream output;
    output << " --" << name << "=" << val << " ";
    std::cout << output.str();
    for (int i = 0; i < colwidth - (int)output.str().size(); ++i) std::cout << " ";
    std::cout << description << " (" << checkDescription << ")\n";
  }

  void parse(const std::string& v) override {
    try {
      set(aux::sto<T>(v));
    } catch (const std::invalid_argument&) {
      throw InvalidArgument("Invalid value for " + name + ": " + v + ".\nCheck usage with --help option.");
    }
    if (!check(val)) {
      throw InvalidArgument("Invalid value for " + name + ": " + v + ".\nCheck usage with --help option.");
    }
  }
};

class EnumOption : public Option {
  std::string val;
  std::vector<std::string> values;

 public:
  EnumOption(const std::string& n, const std::string& d, const std::string& v, const std::vector<std::string>& vs);

  void printUsage(int colwidth) const override;

  [[nodiscard]] bool valid(const std::string& v) const;

  void parse(const std::string& v) override;

  [[nodiscard]] bool is(const std::string& v) const;
  [[nodiscard]] const std::string& get() const;
};

struct Options {
  VoidOption help{"help", "Print this help message"};
  VoidOption copyright{"copyright", "Print copyright information"};
  EnumOption licenseInfo{"license",
                         "Print the license text of the given license",
                         "",
                         {
                             "AGPLv3",
                             "Boost",
#if WITHCOINUTILS
                             "EPL",
#endif
                             "MIT",
                             "RS",
#if WITHSOPLEX
                             "ZIB Apache 2",
#endif
                         }};
  ValOption<int64_t> randomSeed{"seed", "Seed for the pseudo-random number generator", 1, "1 =< int",
                                [](int64_t x) -> bool { return 1 <= x; }};
  VoidOption noSolve{"onlyparse", "Quit after parsing file"};
  EnumOption fileFormat{"format",
                        "File format (overridden by corresponding file extension)",
                        "opb",
                        {"opb", "wbo", "cnf", "wcnf", "mps", "lp"}};
  ValOption<std::string> writeOpb{"write-opb",
                                  "Filename to store OPB of the parsed problem, disabled if left unspecified", "",
                                  "/path/to/file", [](const std::string&) -> bool { return true; }};
  ValOption<std::string> writeLp{"write-lp", "Filename to store LP of the parsed problem, disabled if left unspecified",
                                 "", "/path/to/file", [](const std::string&) -> bool { return true; }};
  BoolOption uniformOut{"print-uniform", "Use a default output style for all file formats", true};
  VoidOption printSol{"print-sol", "Print the solution if found (style can be uniform or non-uniform)"};
  VoidOption printUnits{"print-units", "Print unit literals"};
  BoolOption logFixedLits{"log-fixed-lits",
                          "Stream literals fixed at decision level 0 to stdout (as 'c fixed <lit>' lines) "
                          "for use by hybrid solvers",
                          false};
  VoidOption printCsvData{"print-csv", "Print statistics in a comma-separated value format"};
  ValOption<int32_t> verbosity{"verbosity", "Verbosity of the output", 0, "0 =< int",
                               [](const int32_t& x) -> bool { return x >= 0; }};
  ValOption<std::string> proofLog{"proof-log",
                                  "Filename for the proof logs, proof logging disabled if left unspecified", "",
                                  "/path/to/file", [](const std::string&) -> bool { return true; }};
  BoolOption proofZip{"proof-zip", "Generate proof file in ZIP format. This can alleviate space and IO constraints.",
                      false};
  BoolOption proofAssumps{"proof-assumptions",
                          "Allow advanced solving techniques that generate incomplete proofs with "
                          "assumption rules. Disabling may reduce performance.",
                          true};
  ValOption<double> timeout{"timeout", "Timeout in seconds, 0 is infinite ", 0, "0 =< float",
                            [](double x) -> bool { return 0 <= x; }};
  ValOption<int64_t> timeoutDet{"timeout-det", "Deterministic timeout, 0 is infinite ", 0, "0 =< int",
                                [](int64_t x) -> bool { return 0 <= x; }};
  ValOption<double> lubyBase{"luby-base", "Base of the Luby restart sequence", 2, "1 =< float",
                             [](double x) -> bool { return 1 <= x; }};
  ValOption<int32_t> lubyMult{"luby-mult", "Multiplier of the Luby restart sequence", 100, "1 =< int",
                              [](const int32_t& x) -> bool { return x >= 1; }};
  EnumOption varConstraint{
      "var-constraint", "Constraint to bump variable activity with", "conflict", {"conflict", "learned", "both"}};
  ValOption<double> varWeight{
      "var-weight", "Activity weight for latest conflict variables - 0 = fixed activity, 0.5 = ACIDS, 1 = VMTF.", 0.99,
      "0 =< float =< 1", [](const double& x) -> bool { return 0 <= x && x <= 1; }};
  BoolOption varAssump{"var-assump", "Activity tracks assumption literals", true};
  BoolOption varObjective{"var-objective", "Initialize heuristic to optimize the objective variables first", false};
  BoolOption varRandom{"var-random", "Random variable order heuristic", false};
  BoolOption varSol{"var-sol", "Use last solution as phase", true};
  EnumOption varPolarity{"var-polarity", "Variable polarity", "phase", {"phase", "inverse", "random"}};
  ValOption<int64_t> dbBase{"db-base", "Initial number of conflicts at which database cleaning is performed.", 2000,
                            "1 =< int", [](const int64_t& x) -> bool { return x >= 1; }};
  ValOption<double> dbExp{"db-exp",
                          "Exponent of the growth of the learned constraint database and the inprocessing intervals, "
                          "with log(#conflicts) as base",
                          1.1, "0 =< float", [](const double& x) -> bool { return 0 <= x; }};
  ValOption<double> dbScale{"db-scale", "Multiplier of the learned constraint database and inprocessing intervals", 500,
                            "0 < float", [](const double& x) -> bool { return 0 < x; }};
  EnumOption dbCleaningPriority{"db-cleaning",
                                "Heuristic to decide which constraints get cleaned from the store",
                                "combo",
                                {"random", "lbd", "activity", "combo"}};
  ValOption<double> dbWeight{"db-weight", "Weight of the most recent activity in weighted activity average", 0.9,
                             "0 < float <= 1", [](const double& x) -> bool { return 0 < x && x <= 1; }};
  BoolOption dbAssump{"db-assump", "LBD tracks assumption literals", true};
  ValOption<double> lpTimeRatio{
      "lp", "Ratio of time spent in LP calls (0 means no LP solving, 1 means no limit on LP solver)",
#if WITHSOPLEX
      0.1, "0 =< float <= 1", [](const double& x) -> bool { return 1 >= x && x >= 0; }
#else
      0, "0", [](const double& x) -> bool { return x == 0; }
#endif
  };
  ValOption<int32_t> lpPivotBudget{"lp-budget", "Initial LP call pivot budget", 2000, "1 =< int",
                                   [](const int32_t& x) -> bool { return x >= 1; }};
  ValOption<double> lpIntolerance{"lp-intolerance", "Intolerance for floating point artifacts", 1e-6, "0 < float",
                                  [](const double& x) -> bool { return x > 1; }};
  BoolOption lpLearnDuals{"lp-learnduals", "Learn dual constraints from optimal LP", true};
  BoolOption lpGomoryCuts{"lp-cut-gomory", "Generate Gomory cuts", false};
  BoolOption lpLearnedCuts{"lp-cut-learned", "Use learned constraints as cuts", false};
  ValOption<int32_t> lpGomoryCutLimit{"lp-cut-gomlim",
                                      "Max number of tableau rows considered for Gomory cuts in a single round", 100,
                                      "1 =< int", [](const int32_t& x) -> bool { return 1 <= x; }};
  ValOption<double> lpMaxCutCos{
      "lp-cut-maxcos",
      "Upper bound on cosine of angle between cuts added in one round, higher means cuts can be more parallel", 0.1,
      "0 =< float =< 1", [](const double& x) -> bool { return 0 <= x && x <= 1; }};
  BoolOption multWeaken{"ca-multweaken", "Multiply and weaken instead of division when possible.", true};
  BoolOption multBeforeDiv{"ca-multiply", "Multiply reason with the asserting literal's conflict coefficient", true};
  EnumOption division{"ca-division",
                      "Division method to round the reason to non-positive slack",
                      "mindiv",
                      {"rto", "slack+1", "mindiv"}};
  BoolOption weakenNonImplying{"ca-weaken-nonimplying",
                               "Weaken non-implying falsified literals from learned constraints", false};
  BoolOption learnedMin{"ca-min", "Minimize learned constraints through generalized self-subsumption.", true};
  BoolOption caCancelingUnkns{"ca-cancelingunknowns", "Exploit canceling unknowns", false};
  ValOption<int64_t> subsetSum{"ca-subsetsum",
                               "Use subset sum calculations to lift the degree and remove superfluous literals when "
                               "the estimated cost is at most this value (0 disables)",
                               static_cast<int32_t>(1000000), "0 =< 1e9",
                               [](const int64_t& x) -> bool { return x >= 0 && x <= 1e9; }};
  BoolOption skipResolution{"ca-skipresolution", "Skip resolving a literal when the conflict slack is sufficiently low",
                            true};
  ValOption<int32_t> bitsOverflow{
      "bits-overflow",
      "Bit width of maximum coefficient during conflict analysis calculations (0 is unlimited, "
      "unlimited or greater than 62 may use slower arbitrary precision implementations)",
      limitBitConfl<int128, int128>(), "0 =< int", [](const int32_t& x) -> bool { return x >= 0; }};
  ValOption<int32_t> bitsReduced{"bits-reduced",
                                 "Bit width of maximum coefficient after reduction when exceeding bits-overflow (0 is "
                                 "unlimited, 1 reduces to cardinalities)",
                                 limitBit<int, int64_t>(), "0 =< int", [](const int32_t& x) -> bool { return x >= 0; }};
  ValOption<int32_t> bitsLearned{
      "bits-learned",
      "Bit width of maximum coefficient for learned constraints (0 is unlimited, 1 reduces to cardinalities)",
      limitBit<int, int64_t>(), "0 =< int", [](const int32_t& x) -> bool { return x >= 0; }};
  ValOption<float> optRatio{"opt-ratio", "Ratio of bottom-up optimization time (0 means top-down, 1 fully bottom-up)",
                            0.5, "0 =< float =< 1", [](const double& x) -> bool { return x >= 0 && x <= 1; }};
  BoolOption optCoreguided{"opt-coreguided", "Core-guided bottom up optimization instead of a basic approach", true};
  BoolOption optReuseCores{"opt-reusecores", "Reuse cores during core-guided bottom up optimization", true};
  ValOption<int32_t> optStratification{
      "opt-stratification",
      "Stratification during core-guided optimization will ignore the smallest literals that together amount to at "
      "most 1/x of the optimality gap (0 means no stratification)",
      50, "0 or int > 1", [](const int32_t& x) -> bool { return x > 1 || x == 0; }};
  ValOption<int32_t> optPrecision{"opt-precision",
                                  "Precision of bottom-up optimization. Each core will improve the optimality gap by "
                                  "at least a factor of 1/x (0 guarantees the minimum improvement of 1)",
                                  50, "0 or int > 1", [](const int32_t& x) -> bool { return x > 1 || x == 0; }};
  EnumOption intEncoding{"int-encoding", "Encoding of integer variables", "log", {"log", "order", "onehot"}};
  BoolOption intContinuous{"int-continuous",
                           "Accept continuous variables by treating them as integer variables. This restricts the "
                           "problem and may yield UNSAT when no integer solution exists.",
                           false};
  BoolOption intUnbounded{"int-unbounded",
                          "Accept unbounded integer variables by imposing default bounds. This restricts the problem "
                          "and may yield UNSAT when no solution within the bounds exists.",
                          true};
  ValOption<double> intDefaultBound{"int-defbound", "Default bound used for unbounded integer variables",
                                    limitAbs<int, int64_t>(), "0 < double",
                                    [](const double& x) -> bool { return x > 0; }};
  BoolOption pureLits{"inp-purelits", "Propagate pure literals", false};
  ValOption<int32_t> domBreakLim{
      "inp-dombreaklim",
      "Maximum limit of queried constraints for dominance breaking (0 means no dominance breaking, -1 is unlimited)", 0,
      "-1 =< int", [](const int32_t& x) -> bool { return x >= -1; }};
  BoolOption inpProbing{"inp-probing", "Perform probing", true};
  ValOption<double> inpAMO{"inp-atmostone",
                           "Ratio of time spent detecting at-most-ones (0 means none, 1 means unlimited)", 0.1,
                           "0 =< float <= 1", [](const double& x) -> bool { return 1 >= x && x >= 0; }};
  ValOption<DetTime> basetime{"inp-basetime", "Initial deterministic time allotted to presolve techniques", 1,
                              "0 =< float", [](const DetTime& x) -> bool { return x >= 0; }};
  ValOption<int> liftDegreeSymbolic{"inp-liftsymb", "Use symbolic bounds to lift the degree of a constraint", 2,
                                    "0 =< int =< 2", [](const int& x) -> bool { return x >= 0 && x <= 2; }};
  BoolOption symbDegNoSat{"inp-liftsymbsat",
                          "If true, saturation weakens symbolic bounds, else saturation invalidates symbolic bounds",
                          true};
  BoolOption test{"test", "Activate experimental option", false};

  const std::vector<Option*> options = {
      &help,
      &copyright,
      &licenseInfo,
      &randomSeed,
      &noSolve,
      &fileFormat,
      &writeOpb,
      &writeLp,
      &uniformOut,
      &printSol,
      &printUnits,
      &logFixedLits,
      &printCsvData,
      &verbosity,
      &timeout,
      &timeoutDet,
      &proofLog,
      &proofZip,
      &proofAssumps,
      &lubyBase,
      &lubyMult,
      &varConstraint,
      &varWeight,
      &varAssump,
      &varSol,
      &varObjective,
      &varRandom,
      &varPolarity,
      &dbBase,
      &dbExp,
      &dbScale,
      &dbCleaningPriority,
      &dbWeight,
      &dbAssump,
#if WITHSOPLEX
      &lpTimeRatio,
      &lpPivotBudget,
      &lpIntolerance,
      &lpLearnDuals,
      &lpGomoryCuts,
      &lpLearnedCuts,
      &lpGomoryCutLimit,
      &lpMaxCutCos,
#endif  // WITHSOPLEX
      &multWeaken,
      &multBeforeDiv,
      &division,
      &weakenNonImplying,
      &learnedMin,
      &caCancelingUnkns,
      &subsetSum,
      &skipResolution,
      &bitsOverflow,
      &bitsReduced,
      &bitsLearned,
      &optRatio,
      &optCoreguided,
      &optReuseCores,
      &optStratification,
      &optPrecision,
      &intEncoding,
      &intContinuous,
      &intUnbounded,
      &intDefaultBound,
      &pureLits,
      &domBreakLim,
      &inpProbing,
      &inpAMO,
      &basetime,
      &liftDegreeSymbolic,
      &symbDegNoSat,
      //      &test,
  };
  unordered_map<std::string, Option*> name2opt;

  std::string formulaName;

  Options();

  void parseOption(const std::string& option, const std::string& value);
  void parseCommandLine(int argc, char** argv);
  void usage(const char* name);

 private:
  bool pureClausalConstraints;

 public:
  void setClausalConstraints(bool isClause);
  bool hasOnlyClausalConstraints() const;

  int getBitsOverflow() const;
  int getBitsReduced() const;
  int getBitsLearned() const;
};

}  // namespace xct
