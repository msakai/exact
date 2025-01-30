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

TEST_SUITE_BEGIN("IntProg inference tests");

constexpr double timeouttime = 0.0000001;

TEST_CASE("toOptimum") {
  Options opts;
  for (int ii = 0; ii < 2; ++ii) {
    opts.optCoreguided.set(ii);
    for (double x : {0.0, 0.5, 1.0}) {
      opts.optRatio.set(x);
      IntProg intprog(opts);
      std::vector<IntVar*> vars;
      vars.reserve(5);
      for (const auto& s : {"a", "b", "c", "d", "e"}) {
        vars.push_back(intprog.addVar(s, 0, 1, Encoding::ORDER));
      }

      auto [state, obj, optcore] = intprog.toOptimum(intprog.getObjective(), true, {false, 0});
      CHECK(state == SolveState::SAT);
      CHECK(obj == 0);
      CHECK(optcore->empty());

      intprog.setObjective(IntConstraint::zip({1, 1, 2, 3, 5}, vars));
      auto [state1, obj1, optcore1] = intprog.toOptimum(intprog.getObjective(), true, {false, 0});
      CHECK(state1 == SolveState::SAT);
      CHECK(obj1 == 0);
      CHECK(optcore1->empty());

      intprog.addConstraint({IntConstraint::zip({1, 2, 3, 4, 5}, vars), 6});
      auto [state0, obj0, optcore0] = intprog.toOptimum(intprog.getObjective(), true, {false, 0});
      CHECK(state0 == SolveState::SAT);
      CHECK(obj0 == 4);
      CHECK(optcore0->empty());

      intprog.setAssumptions(std::vector<std::pair<IntVar*, bigint>>{{vars[4], true}});
      auto [state2, obj2, optcore2] = intprog.toOptimum(intprog.getObjective(), false, {false, 0});
      CHECK(state2 == SolveState::SAT);
      CHECK(obj2 == 6);
      CHECK(optcore2->size() == 1);
      CHECK(optcore2->contains(vars[4]));
      intprog.fix(vars[4], 1);
      intprog.addConstraint({IntConstraint::zip({1, 1, 2, 3, 5}, vars), std::nullopt, 5});
      state = intprog.getOptim()->runFull(false, 0);
      CHECK(state == SolveState::UNSAT);
    }
  }
}

TEST_CASE("toOptimum advanced") {
  Options opts;
  for (int ii = 0; ii < 2; ++ii) {
    opts.optCoreguided.set(ii);
    for (double x : {0.0, 0.5, 1.0}) {
      opts.optRatio.set(x);
      IntProg intprog(opts);
      std::vector<IntVar*> vars;
      vars.reserve(6);
      for (const auto& s : {"a", "b", "c", "d", "e", "x"}) {
        vars.push_back(intprog.addVar(s, 0, 1, Encoding::ORDER));
      }

      intprog.setAssumptions(std::vector<std::pair<IntVar*, bigint>>{{vars[5], false}});
      auto [state, obj, optcore] = intprog.toOptimum(intprog.getObjective(), true, {false, 0});
      CHECK(state == SolveState::SAT);
      CHECK(obj == 0);
      CHECK(optcore->empty());

      intprog.setObjective(IntConstraint::zip({1, 1, 2, 3, 5, -10}, vars));
      auto [state0, obj0, optcore0] = intprog.toOptimum(intprog.getObjective(), true, {false, 0});
      CHECK(state0 == SolveState::SAT);
      CHECK(obj0 == 0);
      CHECK(optcore0->size() == 1);
      CHECK(optcore0->contains(vars[5]));

      intprog.addConstraint({IntConstraint::zip({1, 2, 3, 4, 5, 10}, vars), 6});
      auto [state1, obj1, optcore1] = intprog.toOptimum(intprog.getObjective(), true, {false, 0});
      CHECK(state1 == SolveState::SAT);
      CHECK(obj1 == 4);
      CHECK(optcore1->size() == 1);
      CHECK(optcore1->contains(vars[5]));

      intprog.setAssumptions(std::vector<std::pair<IntVar*, bigint>>{{vars[4], true}});
      auto [state2, obj2, optcore2] = intprog.toOptimum(intprog.getObjective(), true, {false, 0});
      CHECK(state2 == SolveState::SAT);
      CHECK(obj2 == 6);
      CHECK(optcore2->size() == 2);
      CHECK(optcore2->contains(vars[5]));
      CHECK(optcore2->contains(vars[4]));

      intprog.setObjective(IntConstraint::zip({-1, -1, -1, -1, -1, -1}, vars));
      auto [state3, obj3, optcore3] = intprog.toOptimum(intprog.getObjective(), true, {false, 0});
      CHECK(state3 == SolveState::SAT);
      CHECK(obj3 == -5);
      CHECK(optcore3->size() == 1);
      CHECK(optcore3->contains(vars[5]));

      intprog.clearAssumptions();
      auto [state4, obj4, optcore4] = intprog.toOptimum(intprog.getObjective(), true, {false, 0});
      CHECK(state4 == SolveState::SAT);
      CHECK(obj4 == -6);
      CHECK(optcore4->empty());

      auto [state5, obj5, optcore5] = intprog.toOptimum(intprog.getObjective(), true, {true, timeouttime});
      CHECK(state5 == SolveState::TIMEOUT);
      CHECK(obj5 == 0);
      CHECK(optcore5->empty());

      std::vector<std::pair<IntVar*, bigint>> assumps;
      for (int64_t i = 2; i < 6; ++i) assumps.push_back({vars[i], true});
      for (int64_t i = 2; i < 6; ++i) assumps.push_back({vars[i], false});
      intprog.setAssumptions(assumps);
      auto [state6, obj6, optcore6] = intprog.toOptimum(intprog.getObjective(), true, {false, 0});
      CHECK(state6 == SolveState::INCONSISTENT);
      CHECK(obj6 == 0);
      CHECK(optcore6->size() == 4);

      intprog.clearAssumptions({vars[2]});
      intprog.addConstraint({IntConstraint::zip({1, 2, 3, 4, 5, 10}, vars), std::nullopt, 5});
      auto [state7, obj7, optcore7] = intprog.toOptimum(intprog.getObjective(), true, {false, 0});
      CHECK(state7 == SolveState::UNSAT);
      CHECK(obj7 == 0);
      CHECK(optcore7->empty());
    }
  }
}

TEST_CASE("count") {
  Options opts;
  for (int ii = 0; ii < 2; ++ii) {
    opts.optCoreguided.set(ii);
    for (double x : {0.0, 0.5, 1.0}) {
      opts.optRatio.set(x);

      IntProg intprog(opts);
      std::vector<IntVar*> vars;
      vars.reserve(5);
      for (const auto& s : {"a", "b", "c", "d", "e"}) {
        vars.push_back(intprog.addVar(s, 0, 1, Encoding::ORDER));
      }

      CHECK(intprog.count(vars, true).val == 32);

      intprog.addConstraint({IntConstraint::zip({1, 2, 3, 4, 5}, vars), 6});
      CHECK(intprog.count(vars, true).val == 22);

      intprog.setObjective(IntConstraint::zip({1, 1, 2, 3, 5}, vars));

      CHECK(intprog.count(vars, true).val == 2);
      CHECK(intprog.count(vars, false).val == 2);
      CHECK(intprog.count(vars, false).val == 0);
    }
  }
}

TEST_CASE("count advanced") {
  Options opts;
  for (int ii = 0; ii < 2; ++ii) {
    opts.optCoreguided.set(ii);
    for (int i = 0; i < 2; ++i) {
      opts.optCoreguided.set(ii);
      for (double x : {0.0, 0.5, 1.0}) {
        opts.optRatio.set(x);
        IntProg intprog(opts);
        std::vector<IntVar*> vars;
        vars.reserve(5);
        for (const auto& s : {"a", "b", "c", "d", "e"}) {
          vars.push_back(intprog.addVar(s, 0, 1, Encoding::ORDER));
        }

        intprog.setAssumptions(std::vector<std::pair<IntVar*, bigint>>{{vars[4], false}});
        auto [state0, count0] = intprog.count(vars, true);
        CHECK(state0 == SolveState::SAT);
        CHECK(count0 == 16);

        intprog.addConstraint({IntConstraint::zip({1, 2, 3, 4, 5}, vars), 6});
        auto [state1, count1] = intprog.count(vars, true);
        CHECK(state1 == SolveState::SAT);
        CHECK(count1 == 7);

        intprog.setObjective(IntConstraint::zip({1, 1, 2, 3, 5}, vars));
        auto [state2, count2] = intprog.count(vars, true);
        CHECK(state2 == SolveState::SAT);
        CHECK(count2 == 2);

        intprog.setObjective(IntConstraint::zip({-1, -1, -2, -3, -5}, vars));
        auto [state3, count3] = intprog.count(vars, true, {true, timeouttime});
        CHECK(state3 == SolveState::TIMEOUT);
        CHECK(count3 == 0);

        auto [state4, count4] = intprog.count(vars, true);
        CHECK(state4 == SolveState::SAT);
        CHECK(count4 == 1);

        intprog.setAssumptions(std::vector<std::pair<IntVar*, bigint>>{{vars[3], false}, {vars[1], false}});
        auto [state5, count5] = intprog.count(vars, true);
        CHECK(state5 == SolveState::INCONSISTENT);
        CHECK(count5 == 0);

        intprog.clearAssumptions();
        intprog.addConstraint({IntConstraint::zip({1, 2, 3, 4, 5}, vars), std::nullopt, 5});
        auto [state6, count6] = intprog.count(vars, true);
        CHECK(state6 == SolveState::UNSAT);
        CHECK(count6 == 0);
      }
    }
  }
}

TEST_CASE("intersect") {
  Options opts;
  for (int ii = 0; ii < 2; ++ii) {
    opts.optCoreguided.set(ii);
    for (double x : {0.0, 0.5, 1.0}) {
      opts.optRatio.set(x);
      IntProg intprog(opts);
      std::vector<IntVar*> vars;
      vars.reserve(5);
      for (const auto& s : {"a", "b", "c", "d", "e"}) {
        vars.push_back(intprog.addVar(s, 0, 1, Encoding::ORDER));
      }

      auto [solvestate, invalidator] = intprog.getSolIntersection(vars, true);
      CHECK_EQ(solvestate, SolveState::SAT);
      CHECK_EQ(aux::str(invalidator), ">= 1 ;");

      intprog.setObjective(IntConstraint::zip({-1, -4, -3, -5, -5}, vars));
      auto [solvestate0, invalidator0] = intprog.getSolIntersection(vars, true);
      CHECK_EQ(solvestate0, SolveState::SAT);
      CHECK_NE(invalidator0, nullptr);
      CHECK_EQ(aux::str(invalidator0), "+1 ~x1 +1 ~x2 +1 ~x3 +1 ~x4 +1 ~x5 >= 1 ;");

      intprog.addConstraint({IntConstraint::zip({2, 3, 4, 6, 6}, vars), std::nullopt, 10});
      auto [solvestate1, invalidator1] = intprog.getSolIntersection(vars, true);
      CHECK_EQ(solvestate1, SolveState::SAT);
      CHECK_NE(invalidator1, nullptr);
      CHECK_EQ(aux::str(invalidator1), "+1 x1 +1 ~x2 +1 x3 >= 1 ;");

      SolveState res = intprog.getOptim()->runFull(false, 0);
      CHECK_EQ(res, SolveState::SAT);

      auto [solvestate2, invalidator2] = intprog.getSolIntersection(vars, false);
      CHECK_EQ(aux::str(invalidator2), "+1 x1 +1 ~x2 +1 x3 >= 1 ;");

      auto [solvestate3, invalidator3] = intprog.getSolIntersection(vars, false);
      CHECK_EQ(invalidator3, nullptr);
    }
  }
}

TEST_CASE("intersect advanced") {
  Options opts;
  for (int ii = 0; ii < 2; ++ii) {
    opts.optCoreguided.set(ii);
    for (double x : {0.0, 0.5, 1.0}) {
      opts.optRatio.set(x);
      IntProg intprog(opts);
      std::vector<IntVar*> vars;
      vars.reserve(5);
      for (const auto& s : {"a", "b", "c", "d", "e"}) {
        vars.push_back(intprog.addVar(s, 0, 1, Encoding::ORDER));
      }
      intprog.setAssumptions(std::vector<std::pair<IntVar*, bigint>>{{vars[0], false}});
      auto [state0, invalidator0] = intprog.getSolIntersection(vars, true);
      CHECK(state0 == SolveState::SAT);
      CHECK(aux::str(invalidator0) == "+1 x1 >= 1 ;");

      intprog.addConstraint({IntConstraint::zip({2, 3, 4, 6, 6}, vars), std::nullopt, 10});
      auto [state1, invalidator1] = intprog.getSolIntersection(vars, true);
      CHECK(state1 == SolveState::SAT);
      CHECK(aux::str(invalidator1) == "+1 x1 >= 1 ;");

      intprog.setObjective(IntConstraint::zip({-1, -4, -3, -5, -5}, vars));
      auto [state2, invalidator2] = intprog.getSolIntersection(vars, true);
      CHECK(state2 == SolveState::SAT);
      CHECK(aux::str(invalidator2) == "+1 x1 +1 ~x2 +1 x3 >= 1 ;");

      auto [state3, invalidator3] = intprog.getSolIntersection(vars, true, {true, timeouttime});
      CHECK(state3 == SolveState::TIMEOUT);
      CHECK(invalidator3 == nullptr);

      std::vector<IntVar*> vars2 = vars;
      vars2.resize(2);
      auto [state4, invalidator4] = intprog.getSolIntersection(vars2, true);
      CHECK(state4 == SolveState::SAT);
      CHECK(aux::str(invalidator4) == "+1 x1 +1 ~x2 >= 1 ;");

      intprog.setAssumptions(std::vector<std::pair<IntVar*, bigint>>{{vars[3], true}, {vars[4], true}});
      auto [state5, invalidator5] = intprog.getSolIntersection(vars2, true);
      CHECK(state5 == SolveState::INCONSISTENT);
      CHECK(invalidator5 == nullptr);

      intprog.clearAssumptions({vars[3], vars[4]});
      intprog.fix(vars[4], 1);
      intprog.fix(vars[3], 1);
      auto [state6, invalidator6] = intprog.getSolIntersection(vars2, true);
      CHECK(state6 == SolveState::UNSAT);
      CHECK(invalidator6 == nullptr);
    }
  }
}

TEST_CASE("propagate") {
  Options opts;
  for (int ii = 0; ii < 2; ++ii) {
    opts.optCoreguided.set(ii);
    for (double x : {0.0, 0.5, 1.0}) {
      opts.optRatio.set(x);
      IntProg intprog(opts);
      std::vector<IntVar*> vars;
      vars.reserve(10);
      for (const auto& s : {"a", "b", "c", "d", "e"}) {
        vars.push_back(intprog.addVar(s, 0, 1, Encoding::ORDER));
      }
      for (const auto& s : {"k", "l"}) {
        vars.push_back(intprog.addVar(s, -1, 2, Encoding::ORDER));
      }
      for (const auto& s : {"m", "n"}) {
        vars.push_back(intprog.addVar(s, -1, 2, Encoding::LOG));
      }
      vars.push_back(intprog.addVar("o", -1, 2, Encoding::ONEHOT));

      auto propres = intprog.propagate(vars, true);
      CHECK(propres.state == SolveState::SAT);
      CHECK(propres.val == std::vector<std::pair<bigint, bigint>>{
                               {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {-1, 2}, {-1, 2}, {-1, 2}, {-1, 2}, {-1, 2}});

      intprog.addConstraint({IntConstraint::zip({1, -2, 3, -1, 2, -3, 1, -2, 3, 3}, vars), 7});
      propres = intprog.propagate(vars, true);
      CHECK(propres.state == SolveState::SAT);
      CHECK(propres.val == std::vector<std::pair<bigint, bigint>>{
                               {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {-1, 2}, {-1, 2}, {-1, 2}, {-1, 2}, {-1, 2}});

      intprog.setObjective(IntConstraint::zip({3, -4, 1, -2, 3, -4, 1, -2, 3, 3}, vars));
      propres = intprog.propagate(vars, true);
      CHECK(propres.state == SolveState::SAT);
      CHECK(propres.val == std::vector<std::pair<bigint, bigint>>{
                               {0, 0}, {1, 1}, {1, 1}, {1, 1}, {0, 0}, {2, 2}, {-1, 2}, {-1, 0}, {1, 2}, {1, 2}});

      propres = intprog.propagate(vars, false);
      CHECK(propres.state == SolveState::SAT);
      CHECK(propres.val == std::vector<std::pair<bigint, bigint>>{
                               {0, 0}, {1, 1}, {1, 1}, {1, 1}, {0, 0}, {2, 2}, {-1, 2}, {-1, 0}, {1, 2}, {1, 2}});

      propres = intprog.propagate(vars, true);
      CHECK(propres.state == SolveState::UNSAT);
      CHECK(propres.val == std::vector<std::pair<bigint, bigint>>{});
    }
  }
}

TEST_CASE("propagate advanced") {
  Options opts;
  for (int ii = 0; ii < 2; ++ii) {
    opts.optCoreguided.set(ii);
    for (double x : {0.0, 0.5, 1.0}) {
      opts.optRatio.set(x);
      IntProg intprog(opts);
      std::vector<IntVar*> vars;
      vars.reserve(10);
      for (const auto& s : {"a", "b", "c", "d", "e"}) {
        vars.push_back(intprog.addVar(s, 0, 1, Encoding::ORDER));
      }
      for (const auto& s : {"k", "l"}) {
        vars.push_back(intprog.addVar(s, -1, 2, Encoding::ORDER));
      }
      for (const auto& s : {"m", "n"}) {
        vars.push_back(intprog.addVar(s, -1, 2, Encoding::LOG));
      }
      vars.push_back(intprog.addVar("o", -1, 2, Encoding::ONEHOT));

      intprog.setAssumptions({{vars[7], std::vector<bigint>{1}}});
      auto propres = intprog.propagate(vars, true);
      CHECK(propres.state == SolveState::SAT);
      CHECK(propres.val == std::vector<std::pair<bigint, bigint>>{
                               {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {-1, 2}, {-1, 2}, {1, 1}, {-1, 2}, {-1, 2}});

      intprog.addConstraint({IntConstraint::zip({1, -2, 3, -1, 2, -3, 1, -2, 3, 3}, vars), 7});
      propres = intprog.propagate(vars, true);
      CHECK(propres.state == SolveState::SAT);
      CHECK(propres.val == std::vector<std::pair<bigint, bigint>>{
                               {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {-1, 2}, {-1, 2}, {1, 1}, {-1, 2}, {-1, 2}});

      intprog.setObjective(IntConstraint::zip({3, -4, 1, -2, 3, -4, 1, -2, 3, 3}, vars));
      propres = intprog.propagate(vars, true);
      CHECK(propres.state == SolveState::SAT);
      CHECK(propres.val == std::vector<std::pair<bigint, bigint>>{
                               {0, 0}, {1, 1}, {1, 1}, {0, 1}, {0, 1}, {1, 2}, {0, 2}, {1, 1}, {2, 2}, {2, 2}});

      propres = intprog.propagate(vars, false);
      CHECK(propres.state == SolveState::SAT);
      CHECK(propres.val == std::vector<std::pair<bigint, bigint>>{
                               {0, 0}, {1, 1}, {1, 1}, {0, 1}, {0, 1}, {1, 2}, {0, 2}, {1, 1}, {2, 2}, {2, 2}});

      propres = intprog.propagate(vars, true);
      CHECK(propres.state == SolveState::UNSAT);
      CHECK(propres.val == std::vector<std::pair<bigint, bigint>>{});
    }
  }
}

TEST_CASE("pruneDomains") {
  Options opts;
  for (int ii = 0; ii < 2; ++ii) {
    opts.optCoreguided.set(ii);
    for (double x : {0.0, 0.5, 1.0}) {
      opts.optRatio.set(x);
      IntProg intprog(opts);
      std::vector<IntVar*> vars;
      vars.reserve(10);
      for (const auto& s : {"a", "b", "c", "d", "e"}) {
        vars.push_back(intprog.addVar(s, 0, 1, Encoding::ORDER));
      }
      for (const auto& s : {"k", "l", "m", "n", "o"}) {
        vars.push_back(intprog.addVar(s, -1, 2, Encoding::ONEHOT));
      }

      auto propres = intprog.pruneDomains(vars, true);
      CHECK(propres.state == SolveState::SAT);
      CHECK(propres.val == std::vector<std::vector<bigint>>{{0, 1},
                                                            {0, 1},
                                                            {0, 1},
                                                            {0, 1},
                                                            {0, 1},
                                                            {-1, 0, 1, 2},
                                                            {-1, 0, 1, 2},
                                                            {-1, 0, 1, 2},
                                                            {-1, 0, 1, 2},
                                                            {-1, 0, 1, 2}});

      intprog.setObjective(IntConstraint::zip({3, -4, 1, -2, 3, -4, 1, -2, 3, 3}, vars));
      propres = intprog.pruneDomains(vars, true);
      CHECK(propres.state == SolveState::SAT);
      CHECK(propres.val == std::vector<std::vector<bigint>>{{0}, {1}, {0}, {1}, {0}, {2}, {-1}, {2}, {-1}, {-1}});

      intprog.addConstraint({IntConstraint::zip({1, -2, 3, -1, 2, -3, 1, -2, 3, 3}, vars), 7});
      propres = intprog.pruneDomains(vars, true);
      CHECK(propres.state == SolveState::SAT);
      CHECK(propres.val ==
            std::vector<std::vector<bigint>>{{0}, {1}, {1}, {1}, {0}, {2}, {-1, 1, 2}, {-1, 0}, {1, 2}, {1, 2}});

      propres = intprog.pruneDomains(vars, true);
      CHECK(propres.state == SolveState::SAT);
      CHECK(propres.val ==
            std::vector<std::vector<bigint>>{{0}, {1}, {1}, {1}, {0}, {2}, {-1, 1, 2}, {-1, 0}, {1, 2}, {1, 2}});

      propres = intprog.pruneDomains(vars, false);
      CHECK(propres.state == SolveState::SAT);
      CHECK(propres.val ==
            std::vector<std::vector<bigint>>{{0}, {1}, {1}, {1}, {0}, {2}, {-1, 1, 2}, {-1, 0}, {1, 2}, {1, 2}});

      propres = intprog.pruneDomains(vars, true);
      CHECK(propres.state == SolveState::UNSAT);
      CHECK(propres.val.empty());
    }
  }
}

TEST_CASE("pruneDomains advanced") {
  Options opts;
  for (int ii = 0; ii < 2; ++ii) {
    opts.optCoreguided.set(ii);
    for (double x : {0.0, 0.5, 1.0}) {
      opts.optRatio.set(x);
      IntProg intprog(opts);
      std::vector<IntVar*> vars;
      vars.reserve(10);
      for (const auto& s : {"a", "b", "c", "d", "e"}) {
        vars.push_back(intprog.addVar(s, 0, 1, Encoding::ORDER));
      }
      for (const auto& s : {"k", "l", "m", "n", "o"}) {
        vars.push_back(intprog.addVar(s, -1, 2, Encoding::ONEHOT));
      }

      intprog.setAssumptions({{vars[9], std::vector<bigint>{0}}});
      auto propres = intprog.pruneDomains(vars, true);
      CHECK(propres.state == SolveState::SAT);
      CHECK(
          propres.val ==
          std::vector<std::vector<bigint>>{
              {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {-1, 0, 1, 2}, {-1, 0, 1, 2}, {-1, 0, 1, 2}, {-1, 0, 1, 2}, {0}});

      intprog.setObjective(IntConstraint::zip({3, -4, 1, -2, 3, -4, 1, -2, 3, 3}, vars));
      propres = intprog.pruneDomains(vars, true);
      CHECK(propres.state == SolveState::SAT);
      CHECK(propres.val == std::vector<std::vector<bigint>>{{0}, {1}, {0}, {1}, {0}, {2}, {-1}, {2}, {-1}, {0}});

      intprog.addConstraint({IntConstraint::zip({1, -2, 3, -1, 2, -3, 1, -2, 3, 3}, vars), 7});
      propres = intprog.pruneDomains(vars, true);
      CHECK(propres.state == SolveState::SAT);
      CHECK(propres.val == std::vector<std::vector<bigint>>{{0}, {1}, {1}, {1}, {0}, {1}, {2}, {-1}, {2}, {0}});

      intprog.setAssumptions({{vars[9], std::vector<bigint>{0, 2}}});
      propres = intprog.pruneDomains(vars, true);
      CHECK(propres.state == SolveState::SAT);
      CHECK(propres.val ==
            std::vector<std::vector<bigint>>{{0}, {1}, {1}, {1}, {0}, {2}, {-1, 1, 2}, {-1, 0}, {1, 2}, {2}});

      intprog.setAssumptions({{vars[8], std::vector<bigint>{-1, 0}}});
      propres = intprog.pruneDomains(vars, false);
      CHECK(propres.state == SolveState::SAT);
      CHECK(propres.val == std::vector<std::vector<bigint>>{{0}, {1}, {1}, {1}, {0}, {1}, {2}, {-1}, {0}, {2}});
    }
  }
}

TEST_CASE("extract MUS") {
  Options opts;
  opts.lpTimeRatio.set(0);
  opts.inpProbing.set(0);
  opts.inpAMO.set(0);
  IntProg intprog(opts);
  std::vector<IntVar*> vars;
  vars.reserve(10);
  for (const auto& s : {"a", "b", "c", "d", "e", "f"}) {
    vars.push_back(intprog.addVar(s, 0, 1, Encoding::ORDER));
  }
  vars.push_back(intprog.addVar("x", -2, 2, Encoding::ORDER));
  vars.push_back(intprog.addVar("y", -2, 2, Encoding::ONEHOT));
  vars.push_back(intprog.addVar("z", -2, 2, Encoding::LOG));

  std::vector<IntVar*> abc = {vars[0], vars[1], vars[2]};
  std::vector<IntVar*> defxyz = {vars[3], vars[4], vars[5], vars[6], vars[7], vars[8]};

  intprog.addConstraint({IntConstraint::zip({1, 1, 1, 1, 1, 1}, defxyz), -5});
  intprog.addConstraint({{{1, vars[0]}, {1, vars[1]}, {1, vars[3]}}, 1});
  intprog.addConstraint({{{1, vars[1]}, {1, vars[2]}, {1, vars[4]}}, 1});
  intprog.addConstraint({{{1, vars[2]}, {1, vars[0]}, {1, vars[5]}}, 1});
  intprog.addConstraint({IntConstraint::zip({-1, -1, -1}, abc), -1});

  auto [state, mus] = intprog.extractMUS();
  CHECK(state == SolveState::SAT);
  CHECK(!mus);

  intprog.setAssumptions(std::vector<std::pair<IntVar*, bigint>>{
      {vars[3], 0}, {vars[4], 0}, {vars[5], 0}, {vars[6], -2}, {vars[7], -2}, {vars[8], -2}});
  SolveState res = intprog.getOptim()->runFull(false, 0);
  CHECK(res == SolveState::INCONSISTENT);

  Core core2 = intprog.getLastCore();
  CHECK(core2);
  CHECK(core2->size() == 6);

  auto [state1, mus1] = intprog.extractMUS();
  CHECK(state1 == SolveState::INCONSISTENT);
  CHECK(mus1);
  CHECK(mus1->size() == 3);

  auto [state2, mus2] = intprog.extractMUS({true, timeouttime});
  CHECK(state2 == SolveState::TIMEOUT);
  CHECK(!mus2);

  // check that state has not been altered
  CHECK(intprog.count(vars, true).val == 0);
  intprog.clearAssumptions();
  CHECK(intprog.count(vars, true).val == 1625);

  intprog.setAssumptions(std::vector<std::pair<IntVar*, bigint>>{
      {vars[3], 0}, {vars[4], 0}, {vars[5], 0}, {vars[6], -2}, {vars[7], -2}, {vars[8], -2}});
  intprog.addConstraint({IntConstraint::zip({1, 1, 1, 1, 1, 1}, defxyz), std::nullopt, -6});
  auto [state3, mus3] = intprog.extractMUS();
  CHECK(state3 == SolveState::UNSAT);
  CHECK(mus3);
  CHECK(mus3->size() == 0);
}

TEST_CASE("detailed count") {
  Options opts;
  for (int ii = 0; ii < 2; ++ii) {
    opts.optCoreguided.set(ii);
    for (double x : {0.0, 0.5, 1.0}) {
      opts.optRatio.set(x);

      IntProg intprog(opts);
      std::vector<IntVar*> vars;
      vars.reserve(5);
      for (const auto& s : {"a", "b", "c", "d", "e"}) {
        vars.push_back(intprog.addVar(s, -1, 2, Encoding::ORDER));
      }

      std::vector<IntVar*> vars2 = vars;
      vars2.pop_back();
      vars2.pop_back();

      auto res = intprog.count(vars, vars2, true).val;
      for (int64_t i = 0; i < std::ssize(vars2); ++i) {
        for (int64_t j = -1; j < 3; ++j) {
          CHECK(res[i][j] == 256);
        }
      }

      intprog.addConstraint({IntConstraint::zip({1, 1, 1, 1, 1}, vars), 9});
      auto res2 = intprog.count(vars, vars2, true).val;
      for (int64_t i = 0; i < std::ssize(vars2); ++i) {
        CHECK(res2[i].size() == 2);
        CHECK(res2[i][1] == 1);
        CHECK(res2[i][2] == 5);
      }
    }
  }
}

TEST_SUITE_END();
