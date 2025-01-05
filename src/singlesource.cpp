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

/*
 * Had the following three issues with linktime optimization (gcc's '-flto'):
 * - clang did not work with it (missing '/usr/lib/LLVMgold.so')
 * - bug with C-style arrays:
 * https://stackoverflow.com/questions/73047957/gcc-with-lto-warning-argument-1-value-18-615-size-max-exceeds-maximum-ob
 * - infinite INPROCESSING loop
 *
 * Gave up, decided to go old-school with a single source file.
 */

// this must come first because of explicit template specialization rules
// clang-format off
#include "constraints/ConstrExpPools.cpp"
// clang-format on
#include "Logger.cpp"
#include "Optimization.cpp"
#include "Options.cpp"
#include "Solver.cpp"
#include "Stats.cpp"
#include "auxiliary.cpp"
#include "constraints/Constr.cpp"
#include "constraints/ConstrExp.cpp"
#include "constraints/ConstrSimple.cpp"
#include "datastructures/Heuristic.cpp"
#include "datastructures/IntSet.cpp"
#include "datastructures/SolverStructs.cpp"
#include "interface/IntConstraint.cpp"
#include "interface/IntProg.cpp"
#include "parsing.cpp"
#include "propagation/Equalities.cpp"
#include "propagation/Implications.cpp"
#include "propagation/LpSolver.cpp"
#include "propagation/Propagator.cpp"
#include "quit.cpp"
#include "used_licenses/COPYING.cpp"
#include "used_licenses/EPL.cpp"
#include "used_licenses/MIT.cpp"
#include "used_licenses/boost.cpp"
#include "used_licenses/licenses.cpp"
#include "used_licenses/roundingsat.cpp"
#include "used_licenses/zib_apache.cpp"
