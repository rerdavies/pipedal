cmake_minimum_required(VERSION 3.16.0)

# Prevent installation of SQLite built components. (we use the .a library files directly)

add_subdirectory("SQLiteCpp" EXCLUDE_FROM_ALL)


set(SDBUSCPP_BUILD_CODEGEN off)
set(SDBUSCPP_INSTALL off)
set(SDBUSCPP_BUILD_DOCS off)
set(SDBUSCPP_BUILD_DOXYGEN_DOCS off)
set(SDBUSCPP_BUILD_TESTS off)
add_subdirectory("sdbus-cpp")


option(ENABLE_COMMONCRYPTO "Enable use of CommonCrypto" OFF)
option(ENABLE_GNUTLS "Enable use of GnuTLS" OFF)
option(ENABLE_MBEDTLS "Enable use of mbed TLS" OFF)
option(ENABLE_OPENSSL "Enable use of OpenSSL" OFF)
option(ENABLE_WINDOWS_CRYPTO "Enable use of Windows cryptography libraries" OFF)


option(BUILD_TOOLS "Build tools in the src directory (zipcmp, zipmerge, ziptool)" OFF)
option(BUILD_REGRESS "Build regression tests" OFF)
option(BUILD_OSSFUZZ "Build fuzzers for ossfuzz" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)
option(BUILD_DOC "Build documentation" OFF)
add_subdirectory("libzip")
