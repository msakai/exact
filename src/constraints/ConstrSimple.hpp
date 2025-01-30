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

#include <iostream>
#include <sstream>
#include <vector>
#include "../auxiliary.hpp"
#include "../typedefs.hpp"

namespace xct {

struct ConstrExpSuper;
class ConstrExpPools;

struct ConstrSimpleSuper {
  Origin orig = Origin::UNKNOWN;

  virtual ~ConstrSimpleSuper() = default;

  virtual CeSuper toExpanded(ConstrExpPools& cePools) const = 0;
};

template <typename CF, typename DG>
struct ConstrSimple final : public ConstrSimpleSuper {
  std::vector<Term<CF>> terms;
  DG rhs;
  std::string proofLine;

  explicit ConstrSimple(const std::vector<Term<CF>>& t = {}, const DG& r = 0, const Origin& o = Origin::UNKNOWN,
                        const std::string& p = (std::to_string(ID_Trivial) + " "))
      : terms(t), rhs(r), proofLine(p) {
    orig = o;
  }

  CeSuper toExpanded(ConstrExpPools& cePools) const override;
  unsigned int size() const { return terms.size(); }

  void toNormalFormLit();
  void toNormalFormVar();
  void flip();
  void reset();

  void toStreamAsOPB(std::ostream& o) const;
};

template <typename CF, typename DG>
std::ostream& operator<<(std::ostream& o, const ConstrSimple<CF, DG>& sc) {
  for (const Term<CF>& t : sc.terms) o << "+ " << t << " ";
  return o << ">= " << sc.rhs;
}

}  // namespace xct
