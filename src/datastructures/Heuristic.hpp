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

#include "SolverStructs.hpp"

namespace xct {

struct ActNode {
  Var prev;
  Var next;
  ActValV activity;
};

class Heuristic {
  std::vector<std::pair<Lit, Lit>> phase;  // first lit is user-fixed phase, second is dynamic phase
  std::vector<ActNode> actList;
  Var nextDecision;

  int nVars() const;

 public:
  Heuristic();
  void resize(int n);

  void undoOne(Var v, Lit l);
  void setPhase(Var v, Lit l);
  void setFixedPhase(Var v, Lit l);

  ActValV getActivity(Var v) const;
  const std::vector<ActNode>& getActList() const;
  void randomize(const std::vector<int>& position);
  void bumpObjective(const CeArb& obj, const std::vector<int>& position);
  void vBumpActivity(VarVec& vars, const std::vector<int>& position, ActValV weightNew, int64_t nConfl);
  bool before(Var v1, Var v2) const;

  Lit pickBranchLit(const std::vector<int>& position, bool coreguided);
  Var nextInActOrder(Var v) const;
  Var firstInActOrder() const;
  void swapOrder(Var v1, Var v2);

  bool testActList(const std::vector<int>& position) const;
  void printActList(const std::vector<int>& position) const;
};

}  // namespace xct
