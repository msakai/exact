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

#include "ConstrExpPools.hpp"
#include "../Global.hpp"
#include "ConstrExp.hpp"

namespace xct {

ConstrExpPools::ConstrExpPools(Global& g) : ce32s(g), ce64s(g), ce96s(g), ce128s(g), ceArbs(g) {}

void ConstrExpPools::resize(size_t newn) {
  ce32s.resize(newn);
  ce64s.resize(newn);
  ce96s.resize(newn);
  ce128s.resize(newn);
  ceArbs.resize(newn);
}

template <>
Ce32 ConstrExpPools::take<int, int64_t>() {
  return ce32s.take();
}
template <>
Ce64 ConstrExpPools::take<int64_t, int128>() {
  return ce64s.take();
}
template <>
Ce96 ConstrExpPools::take<int128, int128>() {
  return ce96s.take();
}
template <>
Ce128 ConstrExpPools::take<int128, int256>() {
  return ce128s.take();
}
template <>
CeArb ConstrExpPools::take<bigint, bigint>() {
  return ceArbs.take();
}

Ce32 ConstrExpPools::take32() { return take<int, int64_t>(); }
Ce64 ConstrExpPools::take64() { return take<int64_t, int128>(); }
Ce96 ConstrExpPools::take96() { return take<int128, int128>(); }
Ce128 ConstrExpPools::take128() { return take<int128, int256>(); }
CeArb ConstrExpPools::takeArb() { return take<bigint, bigint>(); }

template <typename SMALL, typename LARGE>
ConstrExpPool<SMALL, LARGE>::ConstrExpPool(Global& g) : n(0), global(g) {}

template <typename SMALL, typename LARGE>
void ConstrExpPool<SMALL, LARGE>::resize(size_t newn) {
  assert(n <= INF);
  n = newn;
  for (CePtr<SMALL, LARGE>& ce : ces) ce->resize(n);
}

template <typename SMALL, typename LARGE>
CePtr<SMALL, LARGE> ConstrExpPool<SMALL, LARGE>::take() {
  for (int64_t i = std::ssize(ces) - 1; i >= 0; --i) {
    if (ces[i].use_count() == 1) {
      ces[i]->reset(false);
      if (i == std::ssize(ces) - 1) return ces[i];
      std::swap(ces[i], ces[i + 1]);  // slowly move free CePtr to the back
      return ces[i + 1];
    }
  }
  assert(ces.size() < 20);  // Sanity check that no large amounts of ConstrExps are created
  CePtr<SMALL, LARGE> fresh = std::make_shared<ConstrExp<SMALL, LARGE>>(global);
  fresh->resize(n);
  assert(fresh->isReset());
  ces.push_back(fresh);
  return fresh;
}

template class ConstrExpPool<int, int64_t>;
template class ConstrExpPool<int64_t, int128>;
template class ConstrExpPool<int128, int128>;
template class ConstrExpPool<int128, int256>;
template class ConstrExpPool<bigint, bigint>;

}  // namespace xct
