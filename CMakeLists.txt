#
# Master project
#

# CMakeLists files in this project can
# refer to the root source directory and root bnary directory of the project as
# respectively ${AL_SOURCE_DIR} and ${AL_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8.11)
project (MASTER)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DRELEASE")

# Recurse into the subdirectories. This does not actually cause another cmake
# executable to run. The same process will walk through the project's entire
# directory structure.
add_subdirectory (unittest)
