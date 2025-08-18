cmake_minimum_required(VERSION 3.16.0)

add_subdirectory("SQLiteCpp")

set(SDBUSCPP_BUILD_CODEGEN off)
set(SDBUSCPP_INSTALL off)
set(SDBUSCPP_BUILD_DOCS off)
set(SDBUSCPP_BUILD_DOXYGEN_DOCS off)
set(SDBUSCPP_BUILD_TESTS off)
add_subdirectory("sdbus-cpp")


option(BUILD_TOOLS "Build tools in the src directory (zipcmp, zipmerge, ziptool)" OFF)
option(BUILD_REGRESS "Build regression tests" OFF)
option(BUILD_OSSFUZZ "Build fuzzers for ossfuzz" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)
option(BUILD_DOC "Build documentation" OFF)
add_subdirectory("libzip")
