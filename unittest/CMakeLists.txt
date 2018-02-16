#
# Deep Learning Unit Tests
#

# Show compiler stdout
set(CMAKE_VERBOSE_MAKEFILE ON)

# Define external source folders
get_filename_component(outerPath "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(DEPENDENCY_DIR "${outerPath}/depends" CACHE STRING "Path to Dependencies")

# Create unittest executable
add_executable (al-unittest main.cc utils.cc)

# Include dirs
include_directories(${AL_SOURCE_DIR}/include)
include_directories(${AL_SOURCE_DIR}/unittest)
include_directories(${DEPENDENCY_DIR}/rapidjson/include)

# Enable C++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Skip rpath settings
set(CMAKE_SKIP_RPATH TRUE)

# Link executable
list(APPEND AL_LIBS dl)
target_link_libraries(al-unittest ${AL_LIBS})
