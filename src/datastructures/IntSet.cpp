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

#include "IntSet.hpp"

namespace xct {

bool IntSet::check() const {
  for (int i = 0; i < (int)index.reserved() / 2; ++i) {
    assert(index[i] == _unused_() || i == keys[index[i]]);
    assert(index[-i] == _unused_() || -i == keys[index[-i]]);
  }
  for (int i = 0; i < (int)keys.size(); ++i) assert(index[keys[i]] == i);
  return true;
}

IntSet::IntSet(int size, const std::vector<int>& ints) {
  resize(size);
  for (int i : ints) add(i);
}
IntSet::IntSet(const IntSet& other) {
  keys = other.keys;
  index = other.index;
}
IntSet& IntSet::operator=(const IntSet& other) {
  if (&other == this) return *this;
  clear();
  for (int k : other.getKeys()) {
    add(k);
  }
  return *this;
}

const IntMap<int>& IntSet::getIndex() const { return index; }

void IntSet::resize(int size) { index.resize(size, _unused_()); }
size_t IntSet::size() const { return keys.size(); }
bool IntSet::isEmpty() const { return size() == 0; }

void IntSet::clear() {
  //    assert(check());  // test
  for (int k : keys) index[k] = _unused_();
  keys.clear();
}

const std::vector<int>& IntSet::getKeys() const { return keys; }
std::vector<int>& IntSet::getKeysMutable() { return keys; }

bool IntSet::has(int key) const {
  return index.reserved() > (unsigned int)2 * std::abs(key) && index[key] != _unused_();
}

void IntSet::add(int key) {
  if (index.reserved() <= (unsigned int)2 * std::abs(key)) resize(std::abs(key));
  if (index[key] != _unused_()) return;
  assert(!aux::contains(keys, key));
  index[key] = static_cast<int>(keys.size());
  keys.push_back(key);
}

void IntSet::remove(int key) {
  if (!has(key)) return;
  int idx = index[key];
  index[keys.back()] = idx;
  plf::single_reorderase(keys, keys.begin() + idx);
  index[key] = _unused_();
  assert(!has(key));
}

std::ostream& operator<<(std::ostream& o, const IntSet& s) {
  for (int k : s.getKeys()) {
    if (s.has(k)) {
      o << k << " ";
    }
  }
  return o;
}

IntSetPool::~IntSetPool() {
  for (IntSet* is : intsets) delete is;
}

IntSet& IntSetPool::take() {
  assert(intsets.size() < 5);  // Sanity check that no large amounts of IntSets are created
  if (availables.size() == 0) {
    intsets.emplace_back(new IntSet());
    availables.push_back(intsets.back());
  }
  IntSet* result = availables.back();
  availables.pop_back();
  assert(result->isEmpty());
  return *result;
}

void IntSetPool::release(IntSet& is) {
  assert(std::any_of(intsets.cbegin(), intsets.cend(), [&](IntSet* i) { return i == &is; }));
  assert(std::none_of(availables.cbegin(), availables.cend(), [&](IntSet* i) { return i == &is; }));
  is.clear();
  availables.push_back(&is);
}

}  // namespace xct
