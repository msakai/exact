/**********************************************************************
This file is part of Exact.

Copyright (c) 2022-2024 Jo Devriendt, Nonfiction Software

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

#include <csignal>
#include "interface/IntProg.hpp"
#include "quit.hpp"

using namespace xct;

int main(int argc, char** argv) {
  signal(SIGINT, SIGINT_interrupt);
  signal(SIGTERM, SIGINT_interrupt);
#if UNIXLIKE
  signal(SIGXCPU, SIGINT_interrupt);
#endif

  try {
    Options opts;
    opts.pureLits.set(true);
    opts.domBreakLim.set(-1);
    opts.verbosity.set(1);
    opts.parseCommandLine(argc, argv);
    IntProg intprog(opts);

    try {
      intprog.runFromCmdLine();
      return quit::exit_SUCCESS(intprog);
    } catch (const AsynchronousInterrupt& ai) {
      std::cout << "c " << ai.what() << std::endl;
      return quit::exit_INDETERMINATE(intprog);
    } catch (const UnsatEncounter&) {
      return quit::exit_SUCCESS(intprog);
    } catch (const std::invalid_argument& ia) {
      return quit::exit_ERROR(ia.what());
    } catch (const std::bad_alloc&) {
      std::cout << "c OUT OF MEMORY" << std::endl;
      return quit::exit_INDETERMINATE(intprog);
    } catch (const OutOfMemoryException&) {
      std::cout << "c OUT OF MEMORY" << std::endl;
      return quit::exit_INDETERMINATE(intprog);
#if WITHSOPLEX
    } catch (const soplex::SPxMemoryException&) {
      std::cout << "c OUT OF MEMORY" << std::endl;
      return quit::exit_INDETERMINATE(intprog);
#endif
    }
  } catch (const EarlyTermination&) {
    return quit::exit_EARLY();
  } catch (const std::bad_alloc&) {
    std::cout << "c OUT OF MEMORY DURING PARSING" << std::endl;
    return quit::exit_EARLY();
  } catch (const OutOfMemoryException&) {
    std::cout << "c OUT OF MEMORY DURING PARSING" << std::endl;
    return quit::exit_EARLY();
#if WITHSOPLEX
  } catch (const soplex::SPxMemoryException&) {
    std::cout << "c OUT OF MEMORY DURING PARSING" << std::endl;
    return quit::exit_EARLY();
#endif
  } catch (const std::exception& e) {
    std::cout << "c UNKNOWN EXCEPTION: " << e.what() << std::endl;
    return quit::exit_EARLY();
  }
}