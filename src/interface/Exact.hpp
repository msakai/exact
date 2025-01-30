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

#include <pybind11/pybind11.h>
#include <string>
#include <vector>
#include "auxiliary.hpp"
#include "interface/IntProg.hpp"

class Exact {
  xct::IntProg intprog;

  xct::IntVar* getVariable(const std::string& name) const;
  std::vector<xct::IntVar*> getVars(const std::vector<std::string>& names) const;
  template <typename T>
  std::vector<std::pair<xct::IntVar*, T>> getVars(const std::vector<std::pair<std::string, T>>& in) {
    return xct::aux::comprehension(in, [&](const std::pair<std::string, T>& pr) {
      return std::pair<xct::IntVar*, T>{getVariable(pr.first), pr.second};
    });
  }

 public:
  /**
   * Create an instance of the Exact solver.
   *
   * @param: list of pairs of option names and values for the option, both encoded as a string. Run with --help or look
   * at Options.hpp to find the possible options. Options without an argument are activated regardless of any value.
   */
  Exact(const std::vector<std::pair<std::string, std::string>>& options = {});

  /**
   * Add a bounded integer variable.
   *
   * @param name: name of the variable
   * @param lb: lower bound
   * @param ub: upper bound
   * @param encoding: "log" (default), "order" or "onehot"
   */
  void addVariable(const std::string& name, const bigint& lb = 0, const bigint& ub = 1,
                   const std::string& encoding = "");

  /**
   * Returns a list of variables added to the solver.
   *
   * @return the list of variables
   */
  std::vector<std::string> getVariables() const;

  /**
   * Add a linear constraint.
   *
   * @param terms: terms of the linear constraint, each term is represented as a coefficient-variable pair
   * @param useLB: whether or not the constraint is lower bounded
   * @param lb: the lower bound
   * @param useUB: whether or not the constraint is upper bounded
   * @param ub: the upper bound
   */
  void addConstraint(const std::vector<std::pair<bigint, std::string>>& terms, bool useLB = false, const bigint& lb = 0,
                     bool useUB = false, const bigint& ub = 0);

  /**
   * Add a reification of a linear constraint, where the constraint holds iff the head variable takes the given sign
   * value.
   *
   * @param head: Boolean variable that should take the given sign iff the constraint holds
   * @param sign: value for the head variable
   * @param terms: terms of the linear constraint, each term is represented as a coefficient-variable pair
   * @param lb: lower bound of the constraint (a straightforward conversion exists if the constraint is upper bounded)
   */
  void addReification(const std::string& head, bool sign, const std::vector<std::pair<bigint, std::string>>& terms,
                      const bigint& lb);

  /**
   * Add a reification of a linear constraint, where the constraint holds if the head variable takes the given sign
   * value.
   *
   * @param head: Boolean variable that, if it takes the given sign value, makes the constraint hold
   * @param sign: value for the head variable
   * @param terms: terms of the linear constraint, each term is represented as a coefficient-variable pair
   * @param lb: lower bound of the constraint (a straightforward conversion exists if the constraint is upper bounded)
   */
  void addRightReification(const std::string& head, bool sign, const std::vector<std::pair<bigint, std::string>>& terms,
                           const bigint& lb);

  /**
   * Add a reification of a linear constraint, where the head variable must take the given sign value if the constraint
   * holds.
   *
   * @param head: Boolean variable that must take the given sign if the constraint holds
   * @param sign: value for the head variable
   * @param terms: terms of the linear constraint, each term is represented as a coefficient-variable pair
   * @param lb: lower bound of the constraint (a straightforward conversion exists if the constraint is upper bounded)
   */
  void addLeftReification(const std::string& head, bool sign, const std::vector<std::pair<bigint, std::string>>& terms,
                          const bigint& lb);

  /**
   * Add a multiplicative constraint, where the lower and upper bound variables constrain a product of factors.
   * Use both the lower and upper bound to post a multiplicative equality constraint in the form of a*b*...*y = z.
   *
   * @param factors: the factors to be multiplied.
   * @param useLB: whether or not the constraint is lower bounded
   * @param lb: the lower bound variable
   * @param useUB: whether or not the constraint is upper bounded
   * @param ub: the upper bound variable
   */
  void addMultiplication(const std::vector<std::string>& factors, bool useLB = false, const std::string& lb = "",
                         bool useUB = false, const std::string& ub = "");

  /**
   * Fix the value of a variable.
   *
   * Fixing the variable to different values will lead to unsatisfiability.
   *
   * @param var: the variable to be fixed.
   * @param val: the value the variable is fixed to
   */
  void fix(const std::string& var, const bigint& val);

  /**
   * Set assumptions for given variables under which a(n optimal) solution is to be found. These assumptions enforce
   * that the given variables can take only the given values, overriding any previous assumed restrictions on these
   * variables. Assumptions for other variables are left untouched.
   *
   * If no solution under the given assumptions exists, a subset of the assumption variables will form a "core" which
   * can be accessed via getLastCore().
   *
   * @param varvals: list of variables and the corresponding (list of) value(s) to allow via assumptions
   * @pre: the given values are within the bounds of the variable
   * @pre: the set of possible values is not empty
   * @pre: if the number of distinct possible values is larger than one and smaller than the range of the variable, then
   * the variable uses a one-hot encoding. As a consequence, for Boolean variables the encoding does not matter.
   */
  void setAssumptions(const std::vector<std::pair<std::string, bigint>>& varvals);
  void setAssumptions(const std::vector<std::pair<std::string, std::vector<bigint>>>& varvals);

  /**
   * Clears any previous assumptions.
   */
  void clearAssumptions();

  /**
   * Clears any previous assumptions for the given variables.
   *
   * @param vars: the variables to clear the assumptions for.
   */
  void clearAssumptions(const std::vector<std::string>& vars);

  /**
   * Check whether a given variable has any assumed restrictions in the possible values it can take.
   *
   * @param var: the variable to check
   * @return: true if the variable has assumed restrictions, false if not
   */
  bool hasAssumption(const std::string& var) const;

  /**
   * Get the possible values allowed by the currently set assumptions for a given variable.
   *
   * This method is mainly meant for diagnostic purposes and is not very efficient.
   *
   * @param var: the variable under inspection
   * @return: the values of the variable that are allowed by the currently set assumptions
   */
  std::vector<pybind11::int_> getAssumption(const std::string& var) const;

  /**
   * Set solution hints for a list of variables. These hints guide the solver to prefer a solution with those values.
   * Internally, this is done by the search heuristic trying to assign the hinted value when making a search decision.
   *
   * @param hints: hints as variable-value pairs
   * @pre: the given values are within the bounds of their corresponding variable
   */
  void setSolutionHints(const std::vector<std::pair<std::string, bigint>>& hints);

  /**
   * Clears solution hints for the given variables
   *
   * @param vars: the variables to clear the hint for.
   */
  void clearSolutionHints(const std::vector<std::string>& vars);

  /**
   * Pass an objective function to be minimized.
   *
   * This function can be called multiple times, replacing the previous objective with the new one.
   * Any constraints implied by the previous objective are *not* removed. If this is desired, add an auxiliary variable
   * to the objective that can disable objective bounds. E.g., if the objective is x+2y+3z, then adding the auxiliary
   * variable a with coefficient 6 and assuming it to true will allow the invalidation of any objective bound later on
   * by fixing it to false.
   * I.e., any bound on x+2y+3z+6a with a assumed to true will have at least 6 as upper bound, and fixing a to false
   * will satisfy any such upper bound, in effect disabling the upper bound constraints.
   *
   * @param terms: terms of the objective, each term is represented as a coefficient-variable pair
   * @param minimize: whether to minimize the objective (otherwise maximizes)
   * @param offset: constant value added to the objective
   */
  void setObjective(const std::vector<std::pair<bigint, std::string>>& terms, bool minimize = true,
                    const bigint& offset = 0);

  /**
   * Start / continue the search.
   *
   * @return: a status string taking the following values:
   * - "UNSAT": an unsatisfiable constraint has been derived. No (more) solutions exist. The search process is
   * finished and all future calls will return this value.
   * - "SAT": a solution consistent with the assumptions and the constraints has been found. The search process can be
   * continued, but to avoid finding the same solution over and over again, change the set of assumptions or add a
   * constraint invalidating this solution via boundObjByLastSol().
   * - "INCONSISTENT": no solutions consistent with the assumptions exist and a core has been constructed, which can be
   * accessed via getLastCore(). The search process can be continued, but to avoid finding the same core over and over
   * again, change the set of assumptions.
   * - "TIMEOUT": the timeout was reached. Solving can be resumed with a later call.
   * - "PAUSED": the search process just finished an inprocessing phase and was paused. Control is passed to the caller
   * to, e.g., change assumptions, add constraints, or do nothing. The search process can simply be continued by another
   * call to runOnce().
   */
  std::string runOnce(double timeout = 0);

  /**
   * Start / continue the search until an optimal solution or inconsistency is found.
   *
   * @ param optimize: whether to optimize for the given objective. If optimize is true, SAT answers will be handled
   * by adding an objective bound constraint, until UNSAT is reached, in which case the last found solution
   * (if it exists) is the optimal one. If optimize is false, control will be handed back to the caller when a solution
   * is found, without an objective bound constraint being added.
   *
   * CRUCIALLY, IF OPTIMIZE IS SET TO TRUE, THE NUMBER OF SOLUTIONS AFTER A CALL TO RUNFULL MAY BE LESS THAN BEFORE.
   *
   * @param timeout: a (rough) timeout limit in seconds. The solver state is still valid after hitting timeout. It may
   * happen that an internal routine exceeds timeout without returning for a while, but it should return eventually. A
   * value of 0 disables the timeout.
   *
   * @return: a status string taking the following values:
   * - "UNSAT": an unsatisfiable constraint has been derived, perhaps by proving that the objective is optimal. No
   * (more) solutions exist. The search process is finished and all future calls will return this value.
   * - "SAT": a solution consistent with the assumptions and the constraints has been found. The search process can be
   * continued, but to avoid finding the same solution over and over again, change the set of assumptions or add a
   * constraint invalidating this solution via boundObjByLastSol().
   * - "INCONSISTENT": no solutions consistent with the assumptions exist and a core has been constructed, which can be
   * accessed via getLastCore(). The search process can be continued, but to avoid finding the same core over and over
   * again, change the set of assumptions.
   * - "TIMEOUT": the timeout was reached. Solving can be resumed with a later call.
   */
  std::string runFull(bool optimize, double timeout = 0);

  /**
   * Check whether a solution has been found.
   *
   * @return: whether a solution has been found.
   */
  bool hasSolution() const;

  /**
   * Get the values assigned to the given variables in the last solution.
   *
   * @param vars: the added variables for which the solution values should be returned.
   * @return: the solution values to the variables. An empty list if no solution was found yet.
   */
  std::vector<pybind11::int_> getLastSolutionFor(const std::vector<std::string>& vars) const;

  /**
   * The subset of assumption variables in the core. Their assumed values imply inconsistency under the constraints.
   * When UNSAT is reached, the last core will be empty.
   *
   * @return: the variables in the core.
   */
  std::vector<std::string> getLastCore();

  /**
   * Calculate a Minimal Unsatisfiable Subset of variables whose assumed values imply inconsistency under the
   * constraints.
   *
   * @param timeout: a (rough) timeout limit in seconds. The solver state is still valid after hitting timeout. It may
   * happen that an internal routine exceeds timeout without returning for a while, but it should return eventually. A
   * value of 0 disables the timeout.
   *
   * @return: a status string and a set of variable names. The status string can be any of:
   * - "UNSAT": the problem is unsatisfiable and the MUS will be empty. The search process is finished and all future
   * calls will return this value.
   * - "INCONSISTENT": the problem is inconsistent with the assumptions and the MUS will be non-empty.
   * - "TIMEOUT": the timeout was reached. Solving can be resumed with a later call.
   * - "SAT": the problem is satisfiable and no MUS exists.
   */
  std::pair<std::string, std::vector<std::string>> extractMUS(double timeout = 0);

  /**
   * Add an upper bound to the objective function based on the objective value of the last found solution.
   */
  void boundObjByLastSol();

  /**
   * Add a constraint enforcing the exclusion of the last solution.
   */
  void invalidateLastSol();

  /**
   * Add a constraint enforcing the exclusion of the subset of the assignments in the last solution over a set of
   * variables.
   *
   * This can be useful in case a small number of variables determines the rest of the variables in each solution.
   *
   * @param vars: the variables for the sub-solution.
   */
  void invalidateLastSol(const std::vector<std::string>& vars);

  /**
   * Get the best known value so far of the objective function.
   *
   * @return: the best known value
   */
  pybind11::int_ getBestSoFar() const;

  /**
   * Get the best known value so far of the dual bound of the objective function.
   *
   * @return: the dual bound value
   */
  pybind11::int_ getDualBound() const;

  /**
   * Calculate the optimal value of the objective function *without* adding objective bound constraints. This way, the
   * search does not reach UNSAT by proving optimality, and the solver can be fully reused. A typical use case is to add
   * the constraint that the objective should take the optimal value after calling toOptimum() to restrict the search to
   * consider only optimal solutions in the next search phase.
   *
   * @param timeout: a (rough) timeout limit in seconds. The solver state is still valid after hitting timeout. It may
   * happen that an internal routine exceeds timeout without returning for a while, but it should return eventually. A
   * value of 0 disables the timeout.
   *
   * @return: a status string and an integer representing the optimal value. The status string can be any of:
   * - "UNSAT": the problem is unsatisfiable. No solution exists. The search process is finished and all future calls
   * will return this value.
   * - "INCONSISTENT": no solutions consistent with the assumptions exist and a core has been constructed, which can be
   * accessed via getLastCore().
   * - "TIMEOUT": the timeout was reached. Solving can be resumed with a later call.
   * - "SAT": the optimal value has been found. No objective bounding constraints were added and the search process can
   * be continued as is.
   */
  std::pair<std::string, pybind11::int_> toOptimum(double timeout = 0);

  /**
   * Under current assumptions, return implied lower and upper bound for variables in vars.
   *
   * @param vars: variables for which to calculate the implied bounds
   * @param timeout: a (rough) timeout limit in seconds. The solver state is still valid after hitting timeout. It may
   * happen that an internal routine exceeds timeout without returning for a while, but it should return eventually. A
   * value of 0 disables the timeout.
   *
   * @return: a status string and a list of pairs of bounds for each variable in vars. The status string can be any of:
   * - "UNSAT": the problem is unsatisfiable. No solution exists. The search process is finished and all future calls
   * will return this value.
   * - "INCONSISTENT": no solutions consistent with the assumptions exist and a core has been constructed, which can be
   * accessed via getLastCore().
   * - "TIMEOUT": the timeout was reached. Solving can be resumed with a later call.
   * - "SAT": the propagated bounds have been found.
   */
  std::pair<std::string, std::vector<std::pair<pybind11::int_, pybind11::int_>>> propagate(
      const std::vector<std::string>& vars, double timeout = 0);

  /**
   * Under previously set assumptions, derive domains for the given variables where all impossible values are pruned.
   *
   * @param vars: variables for which to calculate the pruned domains
   * @param timeout: a (rough) timeout limit in seconds. The solver state is still valid after hitting timeout. It may
   * happen that an internal routine exceeds timeout without returning for a while, but it should return eventually. A
   * value of 0 disables the timeout.
   *
   * @pre: all variables use the one-hot encoding or have a domain size of 2
   *
   * @return: a status string and a list of pruned domains for each variable in vars. The status string can be any of:
   * - "UNSAT": the problem is unsatisfiable. No solution exists. The search process is finished and all future calls
   * will return this value.
   * - "INCONSISTENT": no solutions consistent with the assumptions exist and a core has been constructed, which can be
   * accessed via getLastCore().
   * - "TIMEOUT": the timeout was reached. Solving can be resumed with a later call.
   * - "SAT": the pruned domains have been found.
   */
  std::pair<std::string, std::vector<std::vector<pybind11::int_>>> pruneDomains(const std::vector<std::string>& vars,
                                                                                double timeout = 0);

  /**
   * Under previously set assumptions, return number of different solutions projected to vars.
   *
   * @param vars: variables for which to calculate different solutions
   * @param timeout: a (rough) timeout limit in seconds. The solver state is still valid after hitting timeout. It may
   * happen that an internal routine exceeds timeout without returning for a while, but it should return eventually. A
   * value of 0 disables the timeout.
   *
   * @return: a status string and the total count. The status string can be any of:
   * - "UNSAT": the problem is unsatisfiable. No solution exists. The search process is finished and all future calls
   * will return this value.
   * - "INCONSISTENT": no solutions consistent with the assumptions exist and a core has been constructed, which can be
   * accessed via getLastCore().
   * - "TIMEOUT": the timeout was reached, and the solution count *so far* is returned.
   * Solving can be resumed with a later call.
   * - "SAT": the domains have been counted.
   */
  std::pair<std::string, int64_t> count(const std::vector<std::string>& vars, double timeout = 0);

  /**
   * Print variables given to Exact.
   */
  void printVariables() const;

  /**
   * Print objective and constraints given to Exact.
   */
  void printInput() const;

  /**
   * Print Exact's internal formula.
   */
  void printFormula();

  /**
   * Get Exact's internal statistics
   */
  std::vector<std::pair<std::string, double>> getStats();
};
