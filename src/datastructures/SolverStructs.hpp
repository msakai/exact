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

#include <ostream>
#include "../typedefs.hpp"
#include "IntMap.hpp"

namespace xct {

struct CRef {
  uint32_t ofs = std::numeric_limits<uint32_t>::max();
  bool operator==(CRef const& o) const { return ofs == o.ofs; }
  bool operator!=(CRef const& o) const { return ofs != o.ofs; }
  bool operator<(CRef const& o) const { return ofs < o.ofs; }
};
inline std::ostream& operator<<(std::ostream& o, const CRef& c) { return o << c.ofs; }

const CRef CRef_Undef = {std::numeric_limits<uint32_t>::max()};
inline bool isValid(CRef cr) { return cr != CRef_Undef; };  // TODO: remove

// TODO: make below methods part of a Solver object that's passed around
inline bool isTrue(const IntMap<int>& level, Lit l) { return level[l] != INF; }
inline bool isFalse(const IntMap<int>& level, Lit l) { return level[-l] != INF; }
inline bool isUnit(const IntMap<int>& level, Lit l) { return level[l] == 0; }
inline bool isUnknown(const std::vector<int>& pos, Lit l) { return pos[toVar(l)] == INF; }
inline bool isKnown(const std::vector<int>& pos, Lit l) { return pos[toVar(l)] != INF; }
// NOTE: below assumes isKnown(position,l)
inline bool isDecided(const std::vector<CRef>& reasons, Var v) { return reasons[v] == CRef_Undef; }
inline bool isPropagated(const std::vector<CRef>& reasons, Lit l) { return !isDecided(reasons, toVar(l)); }

struct Watch {
  CRef cref;
  /**
   * 0<=idx<INF: index of watched literal for Watched32 propagation
   * 2*INF<=idx<3*INF: index of watched literal for Watched propagation
   * 3*INF<=idx<4*INF: index of watched literal for Cardinality propagation
   * idx==4*INF: Clause
   * idx==4*INF+1: Binary
   **/
  uint32_t idx;
  Lit blocking;
  bool operator==(const Watch& other) const { return other.cref == cref && other.idx == idx; }
};

// ----------------------------------------------------------------------
// Memory. Maximum supported size of learnt constraint database is 64GiB.

struct ConstraintAllocator {
  // NOTE: no uint64_t as this allows constraint reference CRef to be only 32 bit
  std::byte* memory = nullptr;
  uint32_t cap = 0;
  uint32_t at = 0;
  uint32_t wasted = 0;  // for GC

  void capacity(int64_t min_cap);

  template <typename C>
  C* alloc(int nTerms) {
    int64_t newAt = at;
    newAt += C::getMemSize(nTerms);
    capacity(newAt);
    assert(newAt <= 0xffffffff);
    C* res = (C*)(memory + maxAlign * at);
    assert(size_t(res) % maxAlign == 0);
    at = newAt;
    return res;
  }

  Constr& operator[](CRef cr) const;

  void cleanup();
};

class OutOfMemoryException : public std::exception {};

std::byte* xrealloc(std::byte* ptr, size_t oldsize, size_t newsize);

}  // namespace xct

template <>
struct std::hash<xct::CRef> {
  size_t operator()(xct::CRef const& cr) const noexcept { return cr.ofs; }
};
