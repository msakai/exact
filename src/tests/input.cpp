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

#include "../external/doctest/doctest.h"
#include "interface/IntProg.hpp"

using namespace xct;

TEST_SUITE_BEGIN("IntProg input constraints");

TEST_CASE("multiplication simple") {
  Options opts;
  IntProg intprog(opts);
  std::vector<IntVar*> vars;
  vars.reserve(5);
  vars.push_back(intprog.addVar("a", 0, 2, Encoding::LOG));
  vars.push_back(intprog.addVar("b", 0, 2, Encoding::LOG));
  IntVar* rhs = intprog.addVar("z", -10, 10, Encoding::LOG);

  intprog.addMultiplication(vars, rhs, rhs);

  auto [state, cnt] = intprog.count(vars, true);
  CHECK_EQ(state, SolveState::SAT);
  CHECK_EQ(cnt, 9);
  auto propres = intprog.propagate({rhs}, true);
  CHECK_EQ(propres.state, SolveState::SAT);
  CHECK_EQ(propres.val, std::vector<std::pair<bigint, bigint>>{{0, 4}});
}

TEST_CASE("multiplication") {
  Options opts;
  IntProg intprog(opts, true);
  std::vector<IntVar*> vars;
  vars.reserve(5);
  vars.push_back(intprog.addVar("a", -3, 4, Encoding::LOG));
  vars.push_back(intprog.addVar("b", -2, 5, Encoding::ORDER));
  vars.push_back(intprog.addVar("c", -1, 6, Encoding::ONEHOT));
  vars.push_back(intprog.addVar("d", 0, 1, Encoding::ORDER));
  vars.push_back(intprog.addVar("e", 2, 2, Encoding::ONEHOT));

  IntVar* z = intprog.addVar("z", -1000, 1000, Encoding::LOG);

  intprog.addMultiplication(vars, z, z);

  auto [state, cnt] = intprog.count(vars, true);
  CHECK_EQ(state, SolveState::SAT);
  CHECK_EQ(cnt, 1024);
  auto propres = intprog.propagate({z}, true);
  CHECK_EQ(propres.state, SolveState::SAT);
  CHECK_EQ(propres.val, std::vector<std::pair<bigint, bigint>>{{-180, 240}});

  std::stringstream ss;
  intprog.printInput(ss);
  CHECK_EQ(ss.str(), "OBJ MIN \nz[-1000,1000] =< 1*a[-3,4]*b[-2,5]*c[-1,6]*d[0,1]*e[2,2] =< z[-1000,1000]\n");

  // Auxiliary variables are only created when needed
  int64_t internal_nvars = intprog.getSolver().getNbVars();
  intprog.addMultiplication(vars, z, z);
  CHECK_EQ(intprog.getSolver().getNbVars(), internal_nvars);
}

TEST_CASE("multiplication edge cases") {
  Options opts;
  IntProg intprog(opts);

  IntVar* a = intprog.addVar("a", -2, 2, Encoding::ONEHOT);
  IntVar* y = intprog.addVar("y", -10, 10, Encoding::LOG);
  IntVar* z = intprog.addVar("z", -10, 10, Encoding::ORDER);
  IntVar* q = intprog.addVar("q", -10, 10, Encoding::LOG);
  IntVar* r = intprog.addVar("r", -10, 10, Encoding::ORDER);

  intprog.addMultiplication({}, q, r);
  intprog.addMultiplication({a}, y, z);

  auto propres = intprog.propagate({a, q, r, y, z}, true);
  CHECK_EQ(propres.state, SolveState::SAT);
  CHECK_EQ(propres.val, std::vector<std::pair<bigint, bigint>>{{-2, 2}, {-10, 1}, {1, 10}, {-10, 2}, {-2, 10}});
}

TEST_CASE("constant vars") {
  Options opts;
  IntProg intprog(opts);

  IntVar* a = intprog.addVar("a", 0, 3, Encoding::LOG);
  IntVar* b = intprog.addVar("b", 2, 2, Encoding::LOG);
  IntVar* c = intprog.addVar("c", 0, 0, Encoding::LOG);
  IntVar* d = intprog.addVar("d", 0, 1, Encoding::LOG);
  IntVar* e = intprog.addVar("e", -10, 10, Encoding::LOG);

  intprog.addMultiplication({b, d}, a, a);

  auto propres = intprog.propagate({a, b, c, d, e}, true);
  CHECK_EQ(propres.state, SolveState::SAT);
  CHECK_EQ(propres.val, std::vector<std::pair<bigint, bigint>>{{0, 2}, {2, 2}, {0, 0}, {0, 1}, {-10, 10}});

  intprog.addMultiplication({b, c, d, e}, a, a);

  auto propres2 = intprog.propagate({a, b, c, d, e}, true);
  CHECK_EQ(propres2.state, SolveState::SAT);
  CHECK_EQ(propres2.val, std::vector<std::pair<bigint, bigint>>{{0, 0}, {2, 2}, {0, 0}, {0, 0}, {-10, 10}});
  // NOTE: d must be 0 because the first constraint says a=b*d and a=0 because of the second constraint
}

TEST_CASE("implication constraints for reification") {
  Options opts;
  IntProg intprog(opts);

  IntVar* f = intprog.addVar("f", 0, 7);
  IntVar* g = intprog.addVar("g", 0, 7);
  IntVar* p = intprog.addVar("p");
  IntVar* q = intprog.addVar("q");
  IntVar* r = intprog.addVar("r");
  IntVar* s = intprog.addVar("s");
  IntVar* t = intprog.addVar("t");
  IntVar* u = intprog.addVar("u");
  IntVar* rr = intprog.addVar("rr");
  IntVar* ss = intprog.addVar("ss");

  const Solver& solver = intprog.getSolver();

  // p <=> f >= 2
  IntConstraint ic1 = IntConstraint{{{1, f}}, 2, std::nullopt};
  intprog.addReification(p, true, ic1);
  CHECK_EQ(solver.getNbConstraints(), 2);  // adds no implications
  // q <=> f >= 6
  IntConstraint ic2 = IntConstraint{{{1, f}}, 6, std::nullopt};
  intprog.addReification(q, true, ic2);
  CHECK_EQ(solver.getNbConstraints(), 5);  // adds q => p

  // ~r => 2f =< 1
  // r <= f >= 1
  IntConstraint ic3 = IntConstraint{{{2, f}}, std::nullopt, 1};
  intprog.addRightReification(r, false, ic3);
  CHECK_EQ(solver.getNbConstraints(), 7);  // adds p => r
  // ~s <= -2f >= -9
  // s => f >= 5
  IntConstraint ic4 = IntConstraint{{{-2, f}}, -9, std::nullopt};
  intprog.addLeftReification(s, false, ic4);
  CHECK_EQ(solver.getNbConstraints(), 10);  // adds s => p and s => r

  // ~rr => 2g <= 9
  // rr <= g >= 5
  IntConstraint ic5 = IntConstraint{{{2, g}}, std::nullopt, 9};
  intprog.addRightReification(rr, false, ic5);
  CHECK_EQ(solver.getNbConstraints(), 11);  // adds no implications
  // ~ss <= -2g >= -9
  // ss => g >= 5
  IntConstraint ic6 = IntConstraint{{{-2, g}}, -9, std::nullopt};
  intprog.addLeftReification(ss, false, ic6);
  CHECK_EQ(solver.getNbConstraints(), 13);  // adds ss => rr

  // t <=> f <= 3
  // ~t <=> f >= 4
  IntConstraint ic7 = IntConstraint{{{1, f}}, std::nullopt, 3};
  intprog.addReification(t, true, ic7);
  CHECK_EQ(solver.getNbConstraints(), 19);  // adds ~t => p and q => ~t and s => ~t and ~t => r

  // u => 5f =< 25
  // ~u <= f >= 6
  IntConstraint ic8 = IntConstraint{{{5, f}}, std::nullopt, 25};
  intprog.addRightReification(u, true, ic8);
  CHECK_EQ(solver.getNbConstraints(), 21);  // adds q => ~u

  std::stringstream strs;
  for (auto c : solver.getRawConstraints()) {
    strs << solver.getCA()[c] << std::endl;
  }
  const std::string constraints = strs.str();
  for (auto t : {
           "1x-8 1x7 >= 1",     // q => p
           "1x-7 1x9 >= 1",     // p => r
           "1x-10 1x7 >= 1",    // s => p
           "1x-10 1x9 >= 1",    // s => r
           "1x11 1x7 >= 1",     // ~t => p
           "1x-8 1x-11 >= 1",   // q => ~t
           "1x-10 1x-11 >= 1",  // s => ~t
           "1x11 1x9 >= 1",     // ~t => r
           "1x-8 1x-12 >= 1",   // q => ~u
           "1x-14 1x13 >= 1",   // ss => rr
       }) {
    if (constraints.find(t) == std::string::npos) {
      aux::cout << "missing: " << t << std::endl;
      aux::cout << constraints << std::endl;
    }
    CHECK(constraints.find(t) != std::string::npos);
  }
}

TEST_CASE("normalize constraints") {
  Options opts;
  IntProg intprog(opts);

  IntVar* f = intprog.addVar("f", 0, 7);
  IntVar* g = intprog.addVar("g", 0, 7);
  IntVar* h = intprog.addVar("h", 3, 3);
  IntVar* i = intprog.addVar("i", -4, -4);
  IntVar* r = intprog.addVar("r");

  // 10 >= 4i + 1f + 3h + 4r - 2g - 3f >= -5
  IntConstraint ic{{{4, i}, {1, f}, {-2, g}, {3, h}, {4, r}, {-3, f}}, -5, 10};
  ic.normalize();
  // normalized to
  // 10 >= -2f - 7 + 4r - 2g >= -5
  // 17 >= -2f + 4r - 2g >= 2
  // 8 >= -f + 2r - g >= 1
  // -1 >= f - 2r + g >= -8
  IntConstraint normalized{{{1, f}, {1, g}, {-2, r}}, -8, -1};

  CHECK(ic == normalized);
  ic.normalize();
  CHECK(ic == normalized);  // normalization is idempotent
}

TEST_CASE("encode constraints") {
  Options opts;
  IntProg intprog(opts);

  IntVar* f = intprog.addVar("f", 0, 7);
  IntVar* g = intprog.addVar("g", 0, 7);
  IntVar* h = intprog.addVar("h", 3, 3);
  IntVar* i = intprog.addVar("i", -4, -4);
  IntVar* r = intprog.addVar("r");
  IntVar* s = intprog.addVar("s");
  IntVar* t = intprog.addVar("t");

  // 10 >= 4i + 1f + 3h + 4r - 2g - 3f >= -5
  IntConstraint ic{{{4, i}, {1, f}, {-2, g}, {3, h}, {4, r}, {-3, f}}, -5, 10};
  std::string encoding = ic.encode();
  IntConstraint ic2;
  ic2.decode(encoding, intprog.getVariables());
  CHECK_EQ(ic, ic2);

  ic.normalize();
  encoding = ic.encode();
  IntConstraint ic3;
  ic3.decode(encoding, intprog.getVariables());
  CHECK_EQ(ic, ic3);

  IntConstraint ic4;
  IntConstraint ic5;
  encoding = ic4.encode();
  ic5.decode(encoding, intprog.getVariables());
  CHECK_EQ(ic4, ic5);

  // r + ~s + t >= 1
  IntConstraint ic6{{{1, r}, {-1, s}, {1, t}}, 0};
  encoding = ic6.encode();
  IntConstraint ic7;
  ic7.decode(encoding, intprog.getVariables());
  CHECK_EQ(ic6, ic7);

  ic6.invert();
  encoding = ic6.encode();
  IntConstraint ic8;
  ic8.decode(encoding, intprog.getVariables());
  CHECK_EQ(ic6, ic8);
}

TEST_CASE("implication constraints for reification 2") {
  Options opts;
  IntProg intprog(opts);

  IntVar* f = intprog.addVar("f", 0, 7);
  IntVar* g = intprog.addVar("g", 0, 7);
  IntVar* p = intprog.addVar("p");
  IntVar* q = intprog.addVar("q");
  IntVar* r = intprog.addVar("r");
  IntVar* s = intprog.addVar("s");
  IntVar* t = intprog.addVar("t");
  IntVar* u = intprog.addVar("u");
  IntVar* rr = intprog.addVar("rr");
  IntVar* ss = intprog.addVar("ss");
  IntVar* h = intprog.addVar("h", 0, 7);

  const Solver& solver = intprog.getSolver();

  IntTermVec first{{1, f}, {1, h}};

  // p <=> f >= 2
  IntConstraint ic1 = IntConstraint{first, 2, std::nullopt};
  intprog.addReification(p, true, ic1);
  CHECK_EQ(solver.getNbConstraints(), 2);  // adds no implications
  // q <=> f >= 6
  IntConstraint ic2 = IntConstraint{first, 6, std::nullopt};
  intprog.addReification(q, true, ic2);
  CHECK_EQ(solver.getNbConstraints(), 5);  // adds q => p

  // ~r => 2f =< 1
  // r <= f >= 1
  IntConstraint ic3 = IntConstraint{{{2, f}, {2, h}}, std::nullopt, 1};
  intprog.addRightReification(r, false, ic3);
  CHECK_EQ(solver.getNbConstraints(), 7);  // adds p => r
  // ~s <= -2f >= -9
  // s => f >= 5
  IntConstraint ic4 = IntConstraint{{{-2, f}, {-2, h}}, -9, std::nullopt};
  intprog.addLeftReification(s, false, ic4);
  CHECK_EQ(solver.getNbConstraints(), 10);  // adds s => p and s => r

  // ~rr => 2g <= 9
  // rr <= g >= 5
  IntConstraint ic5 = IntConstraint{{{2, g}, {2, h}}, std::nullopt, 9};
  intprog.addRightReification(rr, false, ic5);
  CHECK_EQ(solver.getNbConstraints(), 11);  // adds no implications
  // ~ss <= -2g >= -9
  // ss => g >= 5
  IntConstraint ic6 = IntConstraint{{{-2, g}, {-2, h}}, -9, std::nullopt};
  intprog.addLeftReification(ss, false, ic6);
  CHECK_EQ(solver.getNbConstraints(), 13);  // adds ss => rr

  // t <=> f <= 3
  // ~t <=> f >= 4
  IntConstraint ic7 = IntConstraint{first, std::nullopt, 3};
  intprog.addReification(t, true, ic7);
  CHECK_EQ(solver.getNbConstraints(), 19);  // adds ~t => p and q => ~t and s => ~t and ~t => r

  // u => 5f =< 25
  // ~u <= f >= 6
  IntConstraint ic8 = IntConstraint{{{5, f}, {5, h}}, std::nullopt, 25};
  intprog.addRightReification(u, true, ic8);
  CHECK_EQ(solver.getNbConstraints(), 21);  // adds q => ~u

  std::stringstream strs;
  for (auto c : solver.getRawConstraints()) {
    strs << solver.getCA()[c] << std::endl;
  }
  const std::string constraints = strs.str();
  for (auto t : {
           "1x-8 1x7 >= 1",     // q => p
           "1x-7 1x9 >= 1",     // p => r
           "1x-10 1x7 >= 1",    // s => p
           "1x-10 1x9 >= 1",    // s => r
           "1x11 1x7 >= 1",     // ~t => p
           "1x-8 1x-11 >= 1",   // q => ~t
           "1x-10 1x-11 >= 1",  // s => ~t
           "1x11 1x9 >= 1",     // ~t => r
           "1x-8 1x-12 >= 1",   // q => ~u
           "1x-14 1x13 >= 1",   // ss => rr
       }) {
    if (constraints.find(t) == std::string::npos) {
      aux::cout << "missing: " << t << std::endl;
      aux::cout << constraints << std::endl;
    }
    CHECK(constraints.find(t) != std::string::npos);
  }
}

TEST_SUITE_END();
