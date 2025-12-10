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

#include "auxiliary.hpp"

std::ostream& operator<<(std::ostream& o, enum SolveState state) {
  switch (state) {
    case SolveState::UNSAT:
      o << "UNSAT";
      break;
    case SolveState::INCONSISTENT:
      o << "INCONSISTENT";
      break;
    case SolveState::INPROCESSED:
      o << "INPROCESSED";
      break;
    case SolveState::SAT:
      o << "SAT";
      break;
    case SolveState::TIMEOUT:
      o << "TIMEOUT";
      break;
    default:
      assert(false);
  }
  return o;
}

namespace xct::aux {

std::ostream& cout = std::cout;  // to easily find debugging prints

bigint commonDenominator(const std::vector<ratio>& ratios) {
  bigint cdenom = 1;
  for (const ratio& r : ratios) {
    cdenom = lcm(cdenom, boost::multiprecision::denominator(r));
  }
  return cdenom;
}

bool contains(const std::string& s, char c) { return s.find(c) != std::string::npos; }

namespace rng {

uint32_t seed = 1;

uint32_t xorshift32() {
  /* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
  seed ^= seed << 13;
  seed ^= seed >> 17;
  seed ^= seed << 5;
  return seed;
}

}  // namespace rng

int32_t getRand(int32_t min, int32_t max) {
  assert(rng::seed != 0);
  // based on https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction
  assert(min < max);
  return (((uint64_t)rng::xorshift32() * (uint64_t)(max - min + 1)) >> 32) + min;
}

uint64_t shift_hash(uint64_t x) {
  // based on
  // https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key/12996028#12996028
  x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
  x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
  return x ^ (x >> 31);
}
uint64_t seed_hash(uint64_t x, uint64_t seed) { return seed ^ (x + 0x9e3779b9 + (seed << 6) + (seed >> 2)); }

template <>
uint64_t hash(const __int128& el) {
  const uint64_t seed = shift_hash(static_cast<uint64_t>(el));
  return seed_hash(shift_hash(static_cast<uint64_t>(el >> 64)), seed);
}
template <>
uint64_t hash(const boost::multiprecision::int128_t& el) {
  const uint64_t seed = shift_hash(static_cast<uint64_t>(el));
  return seed_hash(shift_hash(static_cast<uint64_t>(el >> 64)), seed);
}
template <>
uint64_t hash(const boost::multiprecision::int256_t& el) {
  uint64_t seed = shift_hash(static_cast<uint64_t>(el));
  seed = seed_hash(shift_hash(static_cast<uint64_t>(el >> 64)), seed);
  seed = seed_hash(shift_hash(static_cast<uint64_t>(el >> 128)), seed);
  seed = seed_hash(shift_hash(static_cast<uint64_t>(el >> 192)), seed);
  return seed;
}
template <>
uint64_t hash(const boost::multiprecision::cpp_int& el) {
  uint64_t seed = el >= 0;
  boost::multiprecision::cpp_int x = aux::abs(el);
  while (x > 0) {
    seed = seed_hash(shift_hash(static_cast<uint64_t>(x)), seed);
    x >>= 64;
  }
  return seed;
}

void* align_alloc(size_t alignment, size_t size) {
#if UNIXLIKE
  return std::aligned_alloc(alignment, size);
#else
  // MSVC alternative
  return _aligned_malloc(size, alignment);
#endif
}

void align_free(void* ptr) {
#if UNIXLIKE
  std::free(ptr);
#else
  // MSVC alternative
  _aligned_free(ptr);
#endif
}

}  // namespace xct::aux
