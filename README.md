# Kapish
A Compact Graph-based C++ Deep Learning Library

# Features
- DAG with variables, constants and operators
- Automatic back-propagation
- Header-only implementation
- CPU only (GPU pending)

# Contents
- bin - project scripts
- include - library headers
- samples - sample files
- unittest - unit tests

# Dependencies
- [Rapidjson](https://github.com/bowfin/rapidjson.git)
- [eigen3](https://github.com/OPM/eigen3)
- [ViennaCL](https://github.com/viennacl/viennacl-dev)

The unit tests for the library are built with CMake. 
Dependency on eigen3 and ViennaCL is satisfied via system include headers.

# Running
- bin/build - builds the unit tests
- bin/unittest - runs the unit tests

NOTE: It is a work in progress. Not for production.
