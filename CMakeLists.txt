# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)
set (CMAKE_CXX_STANDARD 17)
set(MODULE_NAME "WebBrowser2")
project (${MODULE_NAME})

# Include sub-projects.
add_subdirectory ("WebBrowser2")

target_include_directories(${MODULE_NAME} PUBLIC "include")

