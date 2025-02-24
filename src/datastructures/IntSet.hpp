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

#include "IntMap.hpp"

namespace xct {

struct IntSet {  // TODO: template to int64_t, int128, ...?
 private:
  std::vector<int> keys;
  IntMap<int> index;
  static constexpr int _unused_() { return INF; }  // INF allows index to be used as level (e.g., for weakenDivideRound)

  [[nodiscard]] bool check() const;

 public:
  IntSet() = default;
  IntSet(int size, const std::vector<int>& ints);
  IntSet(const IntSet& other);
  IntSet& operator=(const IntSet& other);

  const IntMap<int>& getIndex() const;

  void resize(int size);
  [[nodiscard]] size_t size() const;
  [[nodiscard]] bool isEmpty() const;

  void clear();
  [[nodiscard]] const std::vector<int>& getKeys() const;
  [[nodiscard]] std::vector<int>& getKeysMutable();
  // NOTE: mutating the keys messes up internal data structure.
  // Only use when IntSet will be cleared afterwards.

  [[nodiscard]] bool has(int key) const;
  void add(int key);
  void remove(int key);
};

std::ostream& operator<<(std::ostream& o, const IntSet& s);

class IntSetPool {
  std::vector<IntSet*> intsets;
  std::vector<IntSet*> availables;

 public:
  ~IntSetPool();

  IntSet& take();
  void release(IntSet& intset);
};

}  // namespace xct
