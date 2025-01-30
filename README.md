# Exact

[Exact](https://gitlab.com/nonfiction-software/exact) solves decision and optimization problems formulated as integer linear programs. Under the hood, it converts integer variables to binary (0-1) variables and applies highly efficient propagation routines and strong cutting-planes / pseudo-Boolean conflict analysis.

Exact is a fork of [RoundingSat](https://gitlab.com/MIAOresearch/roundingsat) and improves upon its predecessor in reliability, performance and ease-of-use. 
In particular, Exact supports integer variables, reified linear constraints, multiplication constraints, and propagation and count inferences. 
The name "Exact" reflects that the answers are fully sound, as approximate and floating-point calculations only occur in heuristic parts of the algorithm.
As such, Exact can soundly be used for verification and theorem proving, where its envisioned ability to emit machine-checkable certificates of optimality and unsatisfiability should prove useful.

## Features

- **Native conflict analysis** over binary linear constraints, constructing full-blown cutting planes proofs.
- Highly efficient **watched propagation** routines.
- Fine-grained employment of **arbitrary precision** calculations - only when needed.
- Hybrid linear (top-down) and **core-guided** (bottom-up) optimization.
- Optional integration with the **SoPlex LP solver**.
- Core solver also compiles on **macOS** and **Windows**.
- **Python** interface with assumption solving and reuse of solver state (Linux only for now).
- Generation of **certificates** of optimality and unsatisfiability that can be automatically verified by [VeriPB](https://gitlab.com/MIAOresearch/software/VeriPB).
- Excellent **performance**, as showcased in [2024's PB competition](https://www.cril.univ-artois.fr/PB24/results/results.php?idev=108). 


## Python interface

### PyPI package

The easiest way is to use Exact's Python interfaces is on an **x86_64** machine with **Windows** or **Linux**. In that case, install this precompiled [PyPi package](https://pypi.org/project/exact), e.g., by running `pip install exact`.

### Compile your own Python package

To use the Exact Python interface with optimal binaries for your machine (and the option to include SoPlex in the binary), compile as a shared library and install it with your package manager.
E.g., on Linux systems, running `pip install .` in Exact's root directory should do the trick. On Windows, uncomment the build options below `# FOR WINDOWS` and comment out those for `# FOR LINUX`.
Make sure to have the Boost libraries installed (see dependencies).

### Documentation

The header file [`Exact.hpp`](https://gitlab.com/nonfiction-software/exact/-/blob/main/src/Exact.hpp) contains the C++ methods exposed to Python via [Pybind11](https://pybind11.readthedocs.io) as well as their description. 
This is probably the best place to start to learn about Exact's Python interface.

Next, [`python_examples`](https://gitlab.com/nonfiction-software/exact/-/blob/main/python_examples) contains instructive examples.
Of particular interest is the [`knapsack tutorial`](https://gitlab.com/nonfiction-software/exact/-/blob/main/python_examples/knapsack_tutorial), which is fully commented, starts simple, and ends with some of Exact's advanced features.


## Command line usage

Exact takes as command line input an integer linear program and outputs a(n optimal) solution or reports that none exists.
Either pipe the program

    cat test/instances/opb/opt/stein15.opb | build/Exact

or pass the file as a parameter

    build/Exact test/instances/opb/opt/stein15.opb

Use the flag `--help` to display a list of runtime parameters.

Exact supports five input formats (described in more detail in [`InputFormats.md`](https://gitlab.com/nonfiction-software/exact/-/blob/main/InputFormats.md)):
- `.opb` pseudo-Boolean decision and optimization (equivalent to 0-1 integer linear programming)
- `.wbo` weighted pseudo-Boolean optimization (0-1 integer linear programming with weighted soft constraints)
- `.cnf` DIMACS Conjunctive Normal Form (CNF)
- `.wcnf` Weighted Conjunctive Normal Form (WCNF)
- `.mps` Mathematical Programming System (MPS) via the optional CoinUtils library
- `.lp` Linear Program (LP) via the optional CoinUtils library

Note that `.mps` and `.lp` allow rational variables, which are not supported by Exact.
Additionally, these formats permit floating point values, which may lead to [tricky](https://gitlab.com/nonfiction-software/exact/-/issues/11) [issues](https://gitlab.com/nonfiction-software/exact/-/issues/12).
Rewrite constraints with fractional values to integral ones by multiplying with the lowest common multiple of the denominators. 

By default, Exact decides on the format based on the filename extension, but this can be overridden with the `--format` option.


## Compilation from source

In the root directory of Exact:

    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make
	
Replace `make` by `cmake --build .` on Windows. For more builds, similar build directories can be created.

For installing system-wide or to the `CMAKE_INSTALL_PREFIX` root, use `make install` (on Linux).

## Dependencies

- A recent C++20 compiler (GCC, Clang or MSVC should do)
- [Boost](https://www.boost.org) library, minimal version 1.65.
  On a Debian/Ubuntu system, install with `sudo apt install libboost-dev`. On Windows, follow the instructions on [boost.org](https://www.boost.org/doc/libs/1_85_0/more/getting_started/windows.html)
- Optionally: [CoinUtils](https://github.com/coin-or/CoinUtils) library to parse MPS and LP file formats.
  Use CMake option `-Dcoinutils=ON` after [installing the library](https://github.com/coin-or/CoinUtils#binaries).
- Optionally: [SoPlex](https://soplex.zib.de) LP solver (see below).


## SoPlex (on Linux)

Exact supports an integration with the LP solver [SoPlex](https://soplex.zib.de) to improve its search routine.
For this, checkout SoPlex from its [git repository](https://github.com/scipopt/soplex) as a submodule, compile it in some separate directory, and configure the right CMake options when compiling Exact.

By default, the following commands in Exact's root directory should work with a freshly checked out repository:
```
    git submodule init
    git submodule update

    mkdir soplex_build
    cd soplex_build
    cmake ../soplex -DBUILD_TESTING="0" -DSANITIZE_UNDEFINED="0" -DCMAKE_BUILD_TYPE="Release" -DBOOST="0" -DGMP="0" -DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS="0" -DZLIB="0"
    make -j 8

    cd ../build_debug
    cmake .. -DCMAKE_BUILD_TYPE="Release" -Dsoplex="ON"
    make -j 8
```
The CMake options `soplex_src` and `soplex_build` allow to look for SoPlex in a different location.


## License

Exact is licensed under the [AGPLv3](https://www.gnu.org/licenses/agpl-3.0.en.html). If this would hinder your intended usage, please contact @JoD.


## Benchmarks

The current set of benchmarks which is used to assess performance is available [here](https://gitlab.com/nonfiction-software/exact-benchmarks).


## Citations

If you use Exact, please **star and cite this repository** and cite the RoundingSat origin paper (which focuses on cutting planes conflict analysis):  
**[EN18]** J. Elffers, J. Nordström. Divide and Conquer: Towards Faster Pseudo-Boolean Solving. *IJCAI 2018*

Please cite any of the following papers if they are relevant.

Integration with SoPlex:  
**[DGN20]** J. Devriendt, A. Gleixner, J. Nordström. Learn to Relax: Integrating 0-1 Integer Linear Programming with Pseudo-Boolean Conflict-Driven Search. *CPAIOR 2020 / Constraints journal*

Watched propagation:  
**[D20]** J. Devriendt. Watched Propagation for 0-1 Integer Linear Constraints. *CP 2020*

Core-guided optimization:  
**[DGDNS21]** J. Devriendt, S. Gocht, E. Demirović, J. Nordström, P. J. Stuckey. Cutting to the Core of Pseudo-Boolean Optimization: Combining Core-Guided Search with Cutting Planes Reasoning. *AAAI 2021*

## Industrial use cases
After considering Gurobi and OR-tools to extract minimal unsatisfiable subsets for a workforce allocation problem, [a big European airplane manufacturer decided Exact was the way to go!](https://freuder.wordpress.com/wp-content/uploads/2024/08/trustworthy_allocation_pthg24.pdf) ([mirror](https://gitlab.com/nonfiction-software/exact/-/blob/main/industrial_use_cases/trustworthy_allocation_pthg24.pdf))
