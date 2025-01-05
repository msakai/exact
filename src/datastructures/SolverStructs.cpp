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

#include "SolverStructs.hpp"

namespace xct {

void ConstraintAllocator::capacity(int64_t min_cap) {
  if (cap >= min_cap) return;
  if (min_cap > 0xffffffff) {
    throw OutOfMemoryException();  // throws when exceeding 64 GiB, as memory is measured in maxAlign bytes (128 bits)
  }

  int64_t newcap = cap;
  size_t oldsize = maxAlign * cap;
  while (newcap < min_cap) {
    // NOTE: Add 1 and multiply by a factor of 7/4
    newcap += (newcap >> 1) + (newcap >> 2) + 1;
  }
  // if the new cap exceeds 2^32-1 (the largest feasible 32-bit value), shrink to that value.
  cap = newcap > 0xffffffff ? 0xffffffff : newcap;
  assert(cap > 0);
  assert(cap >= min_cap);
  memory = xrealloc(memory, oldsize, maxAlign * cap);
}

Constr& ConstraintAllocator::operator[](CRef cr) const { return (Constr&)*(memory + maxAlign * cr.ofs); }

void ConstraintAllocator::cleanup() { aux::align_free(memory); }

std::byte* xrealloc(std::byte* ptr, size_t oldsize, size_t newsize) {
  // copy to a larger memory block
  // not the most efficient, but no better option right now:
  // https://stackoverflow.com/questions/64884745/is-there-a-linux-equivalent-of-aligned-realloc
  std::byte* mem = static_cast<std::byte*>(aux::align_alloc(maxAlign, newsize));
  if (mem == nullptr) throw OutOfMemoryException();
  std::memcpy(mem, ptr, oldsize);
  aux::align_free(ptr);
  return mem;
}

}  // namespace xct
