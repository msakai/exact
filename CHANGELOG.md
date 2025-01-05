### 2023-03-22 
commit: 8c77fcc4e28a88e27cc4cd9fe34701a76f822627  
changes:
- the meaning of the argument of `runFull(bool)` has been flipped: `False` now stops at satisfiability, while
  `True` keeps optimizing by adding objective bound constraints to improve the following solution. Additionally, 
  `runFull()` no longer adds objective bound constraints.
- rationale: it was very annoying to end up in an infinite loop or unexpected unsatisfiability when incorrectly setting 
  Exact's option to automatically add objective bound constraints. With this change, users can denote (and are forced
  to) whether they want to optimize or search for satisfiability with just one parameter. 

### 2023-04-08
commit: 3f6db7a4acb38cdf7e0b362780f4f9305958120c
- add SolveState::TIMEOUT (3) to the Python interface.
- add timeout parameter to propagated and pruneDomains from Python
  interface.
- SolveState::TIMEOUT is now returned by runFull when timeout is
  detected to be exceeded. In particularly hard problems, timeouts may
  take a while to be detected (as we only check the outer loop). The
  search state is still valid after timeout is reached.
- propagate and pruneDomains no longer throw an unsatEncounter exception when called on an unsatisfiable problem, but have a defined return value. 
- rationale:
It is useful to have some form of recoverable timeout from the Python interface, even though it is not as finegrained as the non-recoverable timeout from the commandline.

### 2023-07-19
commit 6e1fa28ee5cd8bd94f1192e92ff622f1fc1da862
- add a "setSolutionHints" and "clearSolutionHints" function to the Python interface.
- rationale: useful to search for solutions with certain characteristics, or to hot start an optimization problem.

### 2024-06-04
commit: 88ea45e627db1b564dcc9d7ec1aa528b9abe47b2  
*2.0.0 RELEASE*   
Lots of changes, most small, some big. Biggest new features:
- New Pybind11 Python bindings. Ditching CPPYY for something simpler and more mainstream
- All inferences are fully stateful (or not, depending on parameters) and play nice with assumptions and core-guided optimization
- New PyPI packages, also for Windows
- Total revamp of the Python interface
- Support for a multiplication constraint: `A = B * C * D * E * ... * Z`
- Option to disable assumption proof logging (`--proof-assumptions=0`)
- New conflict analysis optimizations
- Reduced memory footprint
- Proper unit tests for new code
- Lots of small fixes and optimizations

### 2024-06-05
*2.1.0*
commit: 2c6bee36dbeac27bc054d3d80264c73f3ac498cf
- Add multiplication constraint to the Python interface

### 2025-01-??
* 2.2.0*
commit: TODO
- Add redundant binary implications for reification constraints which should improve propagation speed and mimic integer hybrid order/log encoding.
