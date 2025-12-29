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

#include "Options.hpp"

namespace xct {

VoidOption::VoidOption(const std::string& n, const std::string& d) : Option{n, d} {}

void VoidOption::printUsage(int colwidth) const {
  std::cout << " --" << name;
  for (int64_t i = std::ssize(name) + 3; i < colwidth; ++i) std::cout << " ";
  std::cout << description << "\n";
}

void VoidOption::parse([[maybe_unused]] const std::string& v) {
  assert(v.empty());
  val = true;
}

BoolOption::BoolOption(const std::string& n, const std::string& d, bool v) : Option{n, d}, val(v) {}

void BoolOption::set(bool v) { val = v; }

void BoolOption::printUsage(int colwidth) const {
  std::stringstream output;
  output << " --" << name << "=" << val << " ";
  std::cout << output.str();
  for (int i = 0; i < colwidth - (int)output.str().size(); ++i) std::cout << " ";
  std::cout << description << " (0 or 1)\n";
}

void BoolOption::parse(const std::string& v) {
  try {
    int x = std::stoi(v);
    if (x == 0 || x == 1) {
      val = x;
    } else {
      throw InvalidArgument("Invalid value for " + name + ": " + v + ".\nCheck usage with --help option.");
    }
  } catch (const std::invalid_argument&) {
    throw InvalidArgument("Invalid value for " + name + ": " + v + ".\nCheck usage with --help option.");
  }
}

EnumOption::EnumOption(const std::string& n, const std::string& d, const std::string& v,
                       const std::vector<std::string>& vs)
    : Option{n, d}, val(v), values(vs) {}

void EnumOption::printUsage(int colwidth) const {
  std::stringstream output;
  output << " --" << name << "=" << val << " ";
  std::cout << output.str();
  for (int i = 0; i < colwidth - (int)output.str().size(); ++i) std::cout << " ";
  std::cout << description << " (";
  for (int i = 0; i < (int)values.size(); ++i) {
    if (i > 0) std::cout << ", ";
    std::cout << values[i];
  }
  std::cout << ")\n";
}

bool EnumOption::valid(const std::string& v) const {
  return std::find(std::begin(values), std::end(values), v) != std::end(values);
}

void EnumOption::parse(const std::string& v) {
  if (!valid(v)) {
    throw InvalidArgument("Invalid value for " + name + ": " + v + ".\nCheck usage with --help option.");
  }
  val = v;
}

bool EnumOption::is(const std::string& v) const {
  assert(valid(v));
  return val == v;
}

const std::string& EnumOption::get() const { return val; }

Options::Options() : pureClausalConstraints(true) {
  for (Option* opt : options) name2opt[opt->name] = opt;
}

void Options::parseOption(const std::string& option, const std::string& value) {
  std::string opt = option;
  std::ranges::replace(opt, '_', '-');
  if (name2opt.count(opt) == 0) {
    throw InvalidArgument("Unknown option: " + opt + ".\nCheck usage with --help");
  }
  name2opt[opt]->parse(value);
}

void Options::parseCommandLine(int argc, char** argv) {
  if (argc == 0) return;
  unordered_map<std::string, std::string> opt_val;
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg.substr(0, 2) != "--") {
      formulaName = arg;
    } else {
      size_t eqindex = std::min(arg.size(), arg.find('='));
      parseOption(arg.substr(2, eqindex - 2), arg.substr(std::min(arg.size(), eqindex + 1)));
    }
  }

  if (help) {
    usage(argv[0]);
    throw EarlyTermination();
  } else if (copyright) {
    licenses::printUsed();
    throw EarlyTermination();
  } else if (licenseInfo.get() != "") {
    licenses::printLicense(licenseInfo.get());
    throw EarlyTermination();
  }
}

void Options::usage(const char* name) {
  std::cout << "Welcome to Exact!\n\n";
  std::cout << "Source code: https://gitlab.com/nonfiction-software/exact\n";
  std::cout << "branch       " EXPANDED(GIT_BRANCH) "\n";
  std::cout << "commit       " EXPANDED(GIT_COMMIT_HASH) "\n";
  std::cout << "\n";
  std::cout << "Usage: " << name << " [OPTIONS] instancefile\n";
  std::cout << "or     cat instancefile | " << name << " [OPTIONS]\n";
  std::cout << "\n";
  std::cout << "Options:\n";
  for (Option* opt : options) opt->printUsage(24);
}

void Options::setClausalConstraints(bool isClause) { pureClausalConstraints = pureClausalConstraints && isClause; }
bool Options::hasOnlyClausalConstraints() const { return pureClausalConstraints; }

int Options::getBitsOverflow() const {
  return pureClausalConstraints ? limitBitConfl<int, int64_t>() : bitsOverflow.get();
}
int Options::getBitsReduced() const {
  return pureClausalConstraints ? limitBitConfl<int, int64_t>() : bitsReduced.get();
}
int Options::getBitsLearned() const { return pureClausalConstraints ? limitBit<int, int64_t>() : bitsLearned.get(); }

}  // namespace xct