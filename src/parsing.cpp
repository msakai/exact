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

#include "parsing.hpp"
#include <boost/algorithm/string/replace.hpp>
#include <boost/tokenizer.hpp>
#include "Solver.hpp"
#include "interface/IntProg.hpp"

#if WITHCOINUTILS
#include "coin/CoinLpIO.hpp"
#include "coin/CoinMpsIO.hpp"
#endif

namespace xct::parsing {

// TODO: check efficiency of parsing with a .opb-file that takes long to parse, according to experiments

bigint read_bigint(const std::string& s, int64_t start, int64_t end) {
  assert(start >= 0);
  assert(end <= std::ssize(s));
  while (end > start && iswspace(s[end - 1])) --end;
  while (start < end && iswspace(s[start])) ++start;
  start += start < std::ssize(s) && s[start] == '+';
  return aux::sto<bigint>(s.substr(start, end - start));
}

void file_read(IntProg& intprog) {
  if (intprog.global.options.formulaName.empty()) {
    if (intprog.global.options.verbosity.get() > 0) {
      std::cout << "c No filename given, reading from standard input, expected format is "
                << intprog.global.options.fileFormat.get() << std::endl;
    }
  } else {
    std::string::size_type dotidx = intprog.global.options.formulaName.rfind('.');
    if (dotidx != std::string::npos) {
      std::string ext = intprog.global.options.formulaName.substr(dotidx + 1);
      if (intprog.global.options.fileFormat.valid(ext)) {
        intprog.global.options.fileFormat.parse(ext);
      }
    }
    if (intprog.global.options.verbosity.get() > 0) {
      std::cout << "c Reading input file, expected format is " << intprog.global.options.fileFormat.get() << std::endl;
    }
  }
  if (intprog.global.options.fileFormat.is("mps")) {
    mps_read(intprog.global.options.formulaName, intprog);
  } else if (intprog.global.options.fileFormat.is("lp")) {
    lp_read(intprog.global.options.formulaName, intprog);
  } else {
    if (intprog.global.options.formulaName.empty()) {
      if (intprog.global.options.fileFormat.is("opb") || intprog.global.options.fileFormat.is("wbo")) {
        opb_read(std::cin, intprog);
      } else if (intprog.global.options.fileFormat.is("cnf")) {
        cnf_read(std::cin, intprog);
      } else if (intprog.global.options.fileFormat.is("wcnf")) {
        wcnf_read(std::cin, intprog);
      } else {
        assert(false);
      }
    } else {
      std::ifstream fin(intprog.global.options.formulaName);
      if (!fin) {
        throw InvalidArgument("Could not open " + intprog.global.options.formulaName);
      }
      if (intprog.global.options.fileFormat.is("opb") || intprog.global.options.fileFormat.is("wbo")) {
        opb_read(fin, intprog);
      } else if (intprog.global.options.fileFormat.is("cnf")) {
        cnf_read(fin, intprog);
      } else if (intprog.global.options.fileFormat.is("wcnf")) {
        wcnf_read(fin, intprog);
      } else {
        assert(false);
      }
      fin.close();
    }
  }
  intprog.global.logger.logComment("INPUT FORMULA ABOVE - AUXILIARY AXIOMS BELOW");
}

void read_solution_hints(IntProg& intprog, const std::string& filename) {
  std::ifstream fin(filename);
  if (!fin) {
    throw InvalidArgument("Could not open initial-solution file: " + filename);
  }
  const int verb = intprog.global.options.verbosity.get();
  std::vector<std::pair<IntVar*, bigint>> hints;
  std::string line;
  int64_t lineno = 0;
  int64_t skipped = 0;
  while (std::getline(fin, line)) {
    ++lineno;
    size_t a = 0;
    while (a < line.size() && std::isspace(static_cast<unsigned char>(line[a]))) ++a;
    if (a == line.size()) continue;
    if (line[a] == 'c' || line[a] == '*') continue;
    size_t b = a;
    while (b < line.size() && !std::isspace(static_cast<unsigned char>(line[b]))) ++b;
    std::string name = line.substr(a, b - a);
    size_t c = b;
    while (c < line.size() && std::isspace(static_cast<unsigned char>(line[c]))) ++c;
    size_t d = c;
    while (d < line.size() && !std::isspace(static_cast<unsigned char>(line[d]))) ++d;
    if (c == d) {
      if (verb > 0) {
        std::cout << "c WARNING initial-solution line " << lineno << ": expected '<name> <value>', skipping" << std::endl;
      }
      ++skipped;
      continue;
    }
    size_t e = d;
    while (e < line.size() && std::isspace(static_cast<unsigned char>(line[e]))) ++e;
    if (e != line.size()) {
      if (verb > 0) {
        std::cout << "c WARNING initial-solution line " << lineno << ": extra tokens after value, skipping" << std::endl;
      }
      ++skipped;
      continue;
    }
    IntVar* iv = intprog.getVarFor(name);
    if (!iv) {
      if (verb > 0) {
        std::cout << "c WARNING initial-solution line " << lineno << ": unknown variable '" << name << "', skipping"
                  << std::endl;
      }
      ++skipped;
      continue;
    }
    bigint val;
    try {
      val = aux::sto<bigint>(line.substr(c, d - c));
    } catch (const std::exception&) {
      if (verb > 0) {
        std::cout << "c WARNING initial-solution line " << lineno << ": could not parse value for '" << name
                  << "', skipping" << std::endl;
      }
      ++skipped;
      continue;
    }
    if (val < iv->lowerBound || val > iv->upperBound) {
      if (verb > 0) {
        std::cout << "c WARNING initial-solution line " << lineno << ": value " << val << " for '" << name
                  << "' is outside [" << iv->lowerBound << "," << iv->upperBound << "], skipping" << std::endl;
      }
      ++skipped;
      continue;
    }
    hints.emplace_back(iv, val);
  }
  fin.close();
  intprog.setSolutionHints(hints);
  if (verb > 0) {
    std::cout << "c loaded " << hints.size() << " initial-solution hints from " << filename;
    if (skipped > 0) std::cout << " (" << skipped << " line(s) skipped)";
    std::cout << std::endl;
  }
}

IntVar* indexedBoolVar(IntProg& intprog, const std::string& name) {
  if (IntVar* res = intprog.getVarFor(name); res) return res;
  return intprog.addVar(name, 0, 1, Encoding::ORDER, true);
}

// @post: asConjunction ==  true: result is equivalent to the conjunction in input
// @post: asConjunction == false: result == false implies the disjunction in input
Var reify(bool asConjunction, Solver& solver, LitVec& input, unordered_map<LitVec, Var, aux::IntVecHash>& auxiliaries) {
  assert(input.size() > 1);
  unordered_set<Lit> unique_lits(input.begin(), input.end());
  input.clear();
  input.insert(input.begin(), unique_lits.begin(), unique_lits.end());
  Var& aux = auxiliaries[input];
  if (aux != 0) return aux;
  aux = solver.addVar(true);  // increases n to n+1
  if (asConjunction) {
    for (Lit& l : input) {  // reverse implication as binary clauses
      solver.addBinaryConstraint(-aux, l, Origin::FORMULA);
      l = -l;
    }
  }
  input.emplace_back(aux);  // implication
  solver.addClauseConstraint(input, Origin::FORMULA);
  return aux;
}

void opb_read(std::istream& in, IntProg& intprog) {
  intprog.getSolver().ignoreLastObjective();
  std::string line;
  getline(in, line);
  const std::string vartoken = "#variable=";
  size_t idx = line.find(vartoken);
  if (idx == std::string::npos) {
    throw InvalidArgument("First line did not contain " + vartoken + ":\n" + line);
  }
  idx += vartoken.size();
  while (idx < line.size() && iswspace(line[idx])) ++idx;
  int64_t nvars = aux::sto<int64_t>(line.substr(idx, line.find(' ', idx) - idx));
  if (nvars < 1) throw InvalidArgument("Parsed number of variables is zero or less " + aux::str(nvars) + ":\n" + line);
  if (nvars >= INF) {
    throw InvalidArgument("Parsed number of variables is larger than Exact's limit of " + aux::str(INF - 1) + ":\n" +
                          line);
  }
  intprog.getSolver().setNbVars(nvars, true);
  intprog.setInputVarLimit();

  unordered_map<LitVec, Var, aux::IntVecHash> auxiliaries;
  ConstrSimpleArb constr;
  constr.orig = Origin::FORMULA;
  LitVec subTerms;
  int64_t lineNr = -1;
  bool wbo = false;
  std::optional<bigint> topcost;
  IntTermVec obj;
  std::optional<bigint> weight;
  for (std::string line; getline(in, line);) {
    ++lineNr;
    if (line.empty() || line[0] == '*') continue;
    for (char& c : line)
      if (c == ';') c = ' ';
    if (line[0] == 's') {
      assert(line.substr(0, 5) == "soft:");
      wbo = true;
      line = line.substr(5);
      while (!line.empty() && line.back() == ' ') line.pop_back();
      if (!line.empty()) topcost = read_bigint(line, 0, std::ssize(line));
      continue;
    }
    weight = std::nullopt;
    constr.rhs = 0;
    bool isInequality = false;
    bool opt_line = line[0] == 'm';
    if (opt_line) {
      assert(line.substr(0, 4) == "min:");
      line = line.substr(4);
      if (std::all_of(line.begin(), line.end(), isspace)) {
        std::cout << "c WARNING objective function is empty" << std::endl;
        // TODO: global warning routine that also checks for verbosity
      }
    } else {
      size_t eqpos = line.find('=');
      if (eqpos == std::string::npos || eqpos == 0) {
        throw InvalidArgument("Incorrect constraint at line " + std::to_string(lineNr) + ":\n" + line);
      }
      constr.rhs += read_bigint(line, eqpos + 1, std::ssize(line));
      isInequality = line[eqpos - 1] == '>';
      line = line.substr(0, eqpos - isInequality);
    }
    if (line[0] == '[') {
      size_t closingbracket = line.find(']');
      assert(closingbracket != std::string::npos);
      weight = read_bigint(line, 1, closingbracket);
      line = line.substr(closingbracket + 1);
    }

    std::istringstream is(line);

    std::vector<std::string> tokens;
    std::string tmp;
    while (is >> tmp) tokens.push_back(tmp);
    constr.terms.clear();
    for (int64_t i = 0; i < std::ssize(tokens); ++i) {
      const std::string& coef = tokens[i];
      assert(coef.find('x') == std::string::npos);
      constr.terms.emplace_back(read_bigint(coef, 0, std::ssize(coef)), 0);
      subTerms.clear();
      ++i;
      for (; i < std::ssize(tokens) && tokens[i].find('x') != std::string::npos; ++i) {
        if (tokens[i][0] == '~') {
          subTerms.push_back(-std::stoi(tokens[i].c_str() + 2));
        } else {
          subTerms.push_back(std::stoi(tokens[i].c_str() + 1));
        }
      }
      --i;
      constr.terms.back().l =
          subTerms.size() == 1 ? subTerms[0] : reify(true, intprog.getSolver(), subTerms, auxiliaries);
    }

    if (opt_line) {
      assert(obj.empty());
      obj.resize(constr.size());
      constr.toNormalFormVar();
      for (int64_t i = 0; i < std::ssize(constr); ++i) {
        assert(constr.terms[i].l > 0);
        obj[i].v = indexedBoolVar(intprog, std::to_string(constr.terms[i].l));
        obj[i].c = constr.terms[i].c;
      }
      intprog.setObjective(obj, true, -constr.rhs);
    } else {
      if (weight) {
        assert(wbo);
        assert(weight > 0);
        Var aux = intprog.getSolver().addVar(true);
        obj.emplace_back(weight.value(), indexedBoolVar(intprog, std::to_string(aux)));
        // if aux is false, constraint should be true;
        constr.toNormalFormLit();
        constr.terms.emplace_back(constr.rhs, aux);
        intprog.getSolver().addConstraint(constr);
        if (!isInequality) {
          constr.terms.pop_back();
          constr.flip();
          constr.toNormalFormLit();
          constr.terms.emplace_back(constr.rhs, aux);
          intprog.getSolver().addConstraint(constr);
        }
      } else {
        intprog.getSolver().addConstraint(constr);
        if (!isInequality) {
          constr.flip();
          intprog.getSolver().addConstraint(constr);
        }
      }
    }
  }
  if (wbo) {
    intprog.setObjective(obj, true, 0);
    if (topcost) {
      constr.terms.clear();
      for (const IntTerm& it : obj) {
        assert(it.v->encodingVars.size() == 1);
        constr.terms.emplace_back(it.c, it.v->encodingVars[0]);
      }
      constr.rhs = topcost.value() - 1;  // strict limit
      constr.flip();
      intprog.getSolver().addConstraint(constr);
    }
  }
}

void wcnf_read(std::istream& in, IntProg& intprog) {
  std::vector<LitVec> inputs;
  char dummy;
  IntTermVec obj_terms;
  bigint obj_offset = 0;
  // NOTE: there are annoying edge cases where two clauses share the same line, or a clause is split on two lines.
  // the following rewrite fixes this.
  std::string all = "";
  for (std::string line; getline(in, line);) {
    if (line.empty() || line[0] == 'c') continue;
    all += boost::replace_all_copy(" " + line, " 0", " #");
  }
  boost::tokenizer<boost::char_separator<char>> lines(all, boost::char_separator<char>("#"));
  for (const std::string& line : lines) {
    std::istringstream is(line);
    bigint weight = 0;
    if (line.find('h') != std::string::npos) {
      is >> dummy;
    } else {
      is >> weight;
      if (weight <= 0) throw InvalidArgument("Non-positive clause weight: " + line);
    }
    inputs.emplace_back();
    LitVec& input = inputs.back();
    obj_terms.push_back({weight, nullptr});
    Lit l = 0;
    while (is >> l) {
      input.emplace_back(l);
      intprog.getSolver().setNbVars(toVar(l), true);
    }
    if (weight == 0) {  // hard clause
      intprog.getSolver().addClauseConstraint(input, Origin::FORMULA);
      inputs.pop_back();
      obj_terms.pop_back();
    }
  }
  intprog.setInputVarLimit();
  // NOTE: to avoid using original maxsat variable indices for auxiliary variables,
  // introduce auxiliary variables *after* parsing all the original maxsat variables
  assert(inputs.size() == obj_terms.size());
  unordered_map<LitVec, Var, aux::IntVecHash> auxiliaries;
  for (int64_t i = inputs.size() - 1; i >= 0; --i) {
    quit::checkInterrupt(intprog.global);
    assert(i + 1 == std::ssize(inputs));  // each processed input is popped
    LitVec& input = inputs[i];
    if (input.size() == 1) {  // no need to introduce auxiliary variable
      Lit ll = input[0];
      obj_terms[i].v = indexedBoolVar(intprog, std::to_string(toVar(ll)));
      if (ll > 0) {
        const bigint& weight = obj_terms[i].c;
        obj_offset += weight;
        obj_terms[i].c = -weight;
      }
    } else {
      Var aux = reify(false, intprog.getSolver(), input, auxiliaries);
      obj_terms[i].v = indexedBoolVar(intprog, std::to_string(aux));
    }
    inputs.pop_back();
  }
  intprog.setObjective(obj_terms, true, obj_offset);
}

void cnf_read(std::istream& in, IntProg& intprog) {
  intprog.getSolver().ignoreLastObjective();
  LitVec input;
  // NOTE: there are annoying edge cases where two clauses share the same line, or a clause is split on two lines.
  // the following rewrite fixes this.
  std::string all = "";
  for (std::string line; getline(in, line);) {
    if (line.empty() || line[0] == 'c' || line[0] == 'p') continue;
    all += boost::replace_all_copy(" " + line, " 0", " #");
  }
  boost::tokenizer<boost::char_separator<char>> lines(all, boost::char_separator<char>("#"));
  for (const std::string& line : lines) {
    quit::checkInterrupt(intprog.global);
    std::istringstream is(line);
    input.clear();
    Lit l = 0;
    while (is >> l) {
      intprog.getSolver().setNbVars(toVar(l), true);
      input.emplace_back(l);
    }
    intprog.getSolver().addClauseConstraint(input, Origin::FORMULA);
  }
}

#if WITHCOINUTILS
ratio parseCoinUtilsFloat(double x, bool& firstFloat) {
  if (firstFloat && std::floor(x) != x) {
    std::cout << "c WARNING floating point values may not be exactly represented by double" << std::endl;
    firstFloat = false;
  }
  return static_cast<ratio>(x);
}

template <typename T>
void coinutils_read(T& coinutils, IntProg& intprog, bool wasMaximization) {
  // Variables
  for (int c = 0; c < coinutils.getNumCols(); ++c) {
    quit::checkInterrupt(intprog.global);
    const std::string varname = coinutils.columnName(c);
    if (!coinutils.isInteger(c)) {
      if (intprog.global.options.intContinuous) {
        std::cout << "c WARNING continuous variable " << varname << " treated as integer" << std::endl;
      } else {
        throw InvalidArgument("No support for continuous variables: " + varname);
      }
    }
    double lower = coinutils.getColLower()[c];
    if (aux::abs(lower) == coinutils.getInfinity()) {
      if (intprog.global.options.intUnbounded) {
        lower = -intprog.global.options.intDefaultBound.get();
        std::cout << "c WARNING default lower bound " << lower << " for unbounded variable " << varname << std::endl;
      } else {
        throw InvalidArgument("No support for unbounded variables: " + varname);
      }
    }
    double upper = coinutils.getColUpper()[c];
    if (aux::abs(upper) == coinutils.getInfinity()) {
      if (intprog.global.options.intUnbounded) {
        upper = intprog.global.options.intDefaultBound.get();
        std::cout << "c WARNING default upper bound " << upper << " for unbounded variable " << varname << std::endl;
      } else {
        throw InvalidArgument("No support for unbounded variables: " + varname);
      }
    }
    if (std::ceil(upper) < std::floor(upper)) {
      std::cout << "c Conflicting bound on integer variable" << std::endl;
      intprog.addConstraint(IntConstraint{{}, {}, 1});  // 0>=1 should trigger UNSAT
    }
    [[maybe_unused]] IntVar* iv =
        intprog.addVar(varname, static_cast<bigint>(std::ceil(lower)), static_cast<bigint>(std::floor(upper)),
                       opt2enc(intprog.global.options.intEncoding.get()));
  }

  std::vector<ratio> ratcoefs;
  std::vector<bigint> coefs;
  std::vector<IntVar*> vars;
  bool firstFloat = true;

  // Objective
  for (int c = 0; c < coinutils.getNumCols(); ++c) {
    double objcoef = coinutils.getObjCoefficients()[c];
    if (objcoef == 0) continue;
    ratcoefs.emplace_back(parseCoinUtilsFloat(objcoef, firstFloat));
    vars.emplace_back(intprog.getVarFor(coinutils.columnName(c)));
  }
  ratcoefs.emplace_back(parseCoinUtilsFloat(coinutils.objectiveOffset(), firstFloat));
  bigint& cdenom = intprog.obj_denominator;
  cdenom = aux::commonDenominator(ratcoefs);
  for (const ratio& r : ratcoefs) {
    coefs.emplace_back(r * cdenom);
  }
  bigint offset = coefs.back();
  coefs.pop_back();
  if (wasMaximization) {
    cdenom = -cdenom;
    offset = -offset;
  }
  // NOTE: CoinUtils seems to flip the coefficients but not the offset when turning a maximization problem into a
  // minimization one...
  intprog.setObjective(IntConstraint::zip(coefs, vars), true, offset);

  // Constraints
  const CoinPackedMatrix* cpm = coinutils.getMatrixByRow();
  if (cpm == nullptr) throw InvalidArgument("CoinUtils parsing failed.");
  for (int r = 0; r < coinutils.getNumRows(); ++r) {
    quit::checkInterrupt(intprog.global);
    char rowSense = coinutils.getRowSense()[r];
    if (rowSense == 'N') continue;  // free constraint

    ratcoefs.clear();
    coefs.clear();
    vars.clear();
    for (int c = cpm->getVectorFirst(r); c < cpm->getVectorLast(r); ++c) {
      double coef = cpm->getElements()[c];
      if (coef == 0) continue;
      ratcoefs.emplace_back(parseCoinUtilsFloat(coef, firstFloat));
      vars.emplace_back(intprog.getVarFor(coinutils.columnName(cpm->getIndices()[c])));
    }
    bool useLB = rowSense == 'G' || rowSense == 'E' || rowSense == 'R';
    ratcoefs.emplace_back(useLB ? parseCoinUtilsFloat(coinutils.getRowLower()[r], firstFloat) : 0);
    bool useUB = rowSense == 'L' || rowSense == 'E' || rowSense == 'R';
    ratcoefs.emplace_back(useUB ? parseCoinUtilsFloat(coinutils.getRowUpper()[r], firstFloat) : 0);
    const bigint cdenom = aux::commonDenominator(ratcoefs);
    for (const ratio& rat : ratcoefs) {
      coefs.emplace_back(rat * cdenom);
    }
    bigint ub = coefs.back();
    coefs.pop_back();
    bigint lb = coefs.back();
    coefs.pop_back();
    intprog.addConstraint({IntConstraint::zip(coefs, vars), aux::option(useLB, lb), aux::option(useUB, ub)});
  }
}

template void coinutils_read<CoinMpsIO>(CoinMpsIO& coinutils, IntProg& intprog, bool wasMaximization);
template void coinutils_read<CoinLpIO>(CoinLpIO& coinutils, IntProg& intprog, bool wasMaximization);

void mps_read(const std::string& filename, IntProg& intprog) {
  CoinMpsIO coinutils;
  coinutils.messageHandler()->setLogLevel(0);
  if (filename == "") {
    coinutils.readMps("-");  // NOTE: filename "-" reads from stdin
  } else {
    coinutils.readMps(filename.c_str(), "");
  }
  coinutils_read<CoinMpsIO>(coinutils, intprog, false);
}

void lp_read(const std::string& filename, IntProg& intprog) {
  CoinLpIO coinutils;
  coinutils.messageHandler()->setLogLevel(0);
  if (filename == "") {
    coinutils.readLp(stdin);
  } else {
    coinutils.readLp(filename.c_str());
  }
  coinutils_read<CoinLpIO>(coinutils, intprog, coinutils.wasMaximization());
}

#else
void mps_read([[maybe_unused]] const std::string& filename, [[maybe_unused]] IntProg& intprog) {
  throw InvalidArgument("Please compile with CoinUtils to parse MPS format.");
}

void lp_read([[maybe_unused]] const std::string& filename, [[maybe_unused]] IntProg& intprog) {
  throw InvalidArgument("Please compile with CoinUtils to parse LP format.");
}
#endif

}  // namespace xct::parsing
