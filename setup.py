# To upload a package to PyPI, follow the instructions on https://packaging.python.org/en/latest/tutorials/packaging-projects
# For Windows, this is straightforward:
# adjust pyproject.toml to use Windows compile options.
# In Exact's root:
## py -m build
## py -m twine upload --repository pypi dist/Exact-2.0.0-cp312-cp312-win_amd64.whl -p <API TOKEN>

# For Linux, follow the ManyLinux approach on https://github.com/pypa/manylinux.
# Commands that worked previously:
# (Alma's Boost package does not work, so we install our own (following https://www.baeldung.com/linux/boost-install-on-ubuntu))
# In Exact's root:
## docker build -f docker_images/pypi_package -t manylinux_with_boost
## docker run -it --entrypoint bash manylinux_with_boost
## git clone https://gitlab.com/nonfiction-software/exact
## cd exact
## /opt/python/cp312-cp312/bin/python -m build
## auditwheel repair dist/Exact-2.0.0-cp312-cp312-linux_x86_64.whl
## /opt/python/cp312-cp312/bin/python -m twine upload --repository pypi wheelhouse/Exact-2.0.0-cp312-cp312-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl -p <API TOKEN>

# For OSX, only compilation from source for now (<path to venv binaries>/pip install .)
# Use https://github.com/kholia/OSX-KVM
## cd ~/workspace/_systems/OSX-KVM
## ./OpenCore-Boot.sh
# Installing OSX requires a fresh partition and takes a long time (downloading stuff?)
# When stuck in a shell screen, type exit, and boot from a BIOS-like environment.
# Repeatedly run the "install image" option until completion (up to 5 times!). Be patient
# Current image works though.

from pybind11.setup_helpers import Pybind11Extension
from setuptools import setup

ext_modules = [
    Pybind11Extension(
        "exact",
        [
            "src/constraints/Constr.cpp",
            "src/constraints/ConstrExp.cpp",
            "src/constraints/ConstrSimple.cpp",
            "src/constraints/ConstrExpPools.cpp",
            "src/propagation/LpSolver.cpp",
            "src/Solver.cpp",
            "src/IntProg.cpp",
            "src/datastructures/SolverStructs.cpp",
            "src/Logger.cpp",
            "src/datastructures/IntSet.cpp",
            "src/parsing.cpp",
            "src/datastructures/Heuristic.cpp",
            "src/Stats.cpp",
            "src/Options.cpp",
            "src/auxiliary.cpp",
            "src/Optimization.cpp",
            "src/quit.cpp",
            "src/propagation/Propagator.cpp",
            "src/propagation/Equalities.cpp",
            "src/propagation/Implications.cpp",
            "src/used_licenses/licenses.cpp",
            "src/used_licenses/zib_apache.cpp",
            "src/used_licenses/roundingsat.cpp",
            "src/used_licenses/MIT.cpp",
            "src/used_licenses/boost.cpp",
            "src/used_licenses/EPL.cpp",
            "src/used_licenses/COPYING.cpp",
            "src/Exact.cpp"
        ],
        # FOR WINDOWS
        # include_dirs=['C:\\Program Files\\boost\\boost_1_85_0'],
        # extra_compile_args=["/O2","/std:c++20"],
        # define_macros=[("UNIXLIKE",0),("ANKERLMAPS",1)]
        # FOR LINUX / OSX
        extra_compile_args=["-O3","-std=c++20"],
        define_macros=[("UNIXLIKE",1),("ANKERLMAPS",1)]
    ),
]

setup(ext_modules=ext_modules)
