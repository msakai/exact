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

#pragma once

#include "../typedefs.hpp"

namespace xct {

template <typename T>
class IntMap {
  std::vector<T> _int2type;
  typename std::vector<T>::iterator int2type;

 public:
  T& operator[](int index) {
    assert(std::abs(index) <= (int)_int2type.size() / 2);
    return int2type[index];
  }

  const T& operator[](int index) const {
    assert(std::abs(index) <= (int)_int2type.size() / 2);
    return int2type[index];
  }

  void resize(int size, const T& init) {  // should always be called before use, as int2type is not set otherwise
    assert(size >= 0);
    int64_t oldsize = -1;  // NOTE: oldsize can be -1, which is useful in for loops below
    int64_t newsize = 0;
    if (!_int2type.empty()) {
      assert(_int2type.size() % 2 == 1);
      oldsize = (std::ssize(_int2type) - 1) / 2;
      newsize = oldsize;
    }
    if (oldsize >= size) return;
    while (newsize < size) {
      newsize = newsize * resize_factor + 1;
    }
    _int2type.resize(2 * newsize + 1);
    int2type = _int2type.begin() + newsize;
    int64_t i = _int2type.size() - 1;
    for (; i > newsize + oldsize; --i) _int2type[i] = init;
    for (; i >= newsize - oldsize; --i) _int2type[i] = std::move(_int2type[i - newsize + oldsize]);
    for (; i >= 0; --i) _int2type[i] = init;
  }

  typename std::vector<T>::iterator begin() { return _int2type.begin(); }
  typename std::vector<T>::const_iterator begin() const { return _int2type.begin(); }
  typename std::vector<T>::iterator end() { return _int2type.end(); }
  typename std::vector<T>::const_iterator end() const { return _int2type.end(); }

  size_t reserved() const { return _int2type.size(); }
};

template <typename T>
std::ostream& operator<<(std::ostream& o, const IntMap<T>& im) {
  if (im.reserved() > 0) {
    o << "0:" << im[0] << ", ";
  }
  for (int64_t i = 1; i <= im.reserved() / 2; ++i) {
    o << -i << ":" << im[-i] << "|" << i << ":" << im[i] << ", ";
  }
  return o;
}

}  // namespace xct
