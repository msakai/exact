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

/***********************************************************************
Copyright (c) 2014-2020, Jan Elffers
Copyright (c) 2019-2020, Jo Devriendt
Copyright (c) 2020, Stephan Gocht

Parts of the code were copied or adapted from MiniSat.

MiniSAT -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
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
***********************************************************************/

#include "licenses.hpp"
#include <cassert>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <vector>
#include "COPYING.hpp"
#include "EPL.hpp"
#include "MIT.hpp"
#include "boost.hpp"
#include "roundingsat.hpp"
#include "zib_apache.hpp"

namespace licenses {

struct Codebase {
  std::string shortName;
  std::string info;
  std::string licenseName;
};

std::vector<Codebase> usedCodebases = {
    {"Exact", "The source code of Exact", "AGPLv3, MIT"},
    {"RoundingSat", "The source code of RoundingSat", "RS, MIT"},
    {"Boost", "Boost library", "Boost"},
#if WITHCOINUTILS
    {"Coin-OR Utils", "Coin-OR Utils library", "EPL"},
#endif
#if WITHSOPLEX
    {"SoPlex", "SoPlex LP solver", "Apache 2.0"},
#endif
};

void printLicense(const std::string& lic) {
  std::unordered_map<std::string, const char*> lic2text;
  lic2text.insert({
      {"AGPLv3", COPYING},
      {"Boost", boost},
#if WITHCOINUTILS
      {"EPL", EPL},
#endif
      {"MIT", MIT},
      {"RS", roundingsat},
#if WITHSOPLEX
      {"ZIB Apache 2", zib_apache},
#endif
  });

  assert(lic2text.find(lic) != lic2text.end());
  std::cout << lic2text.find(lic)->second << std::endl;
}

void printUsed() {
  std::cout << "The following codebases are used in this binary." << std::endl;
  std::cout << std::setw(20) << "Codebase" << std::setw(15) << "License(s)" << "   " << "Information" << std::endl;
  for (Codebase& cb : usedCodebases) {
    std::cout << std::setw(20) << cb.shortName << std::setw(15) << cb.licenseName << "   " << cb.info << std::endl;
  }
  std::cout << "Note that the license that applies to this binary depends on the tools used.\n"
               "Use --license=[license name] to display the corresponding license text."
            << std::endl;
}
}  // namespace licenses
