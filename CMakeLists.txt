cmake_minimum_required(VERSION 3.16.0)
project(pipedal 
    VERSION 2.0.108
    DESCRIPTION "PiPedal Guitar Effect Pedal For Raspberry Pi"
    HOMEPAGE_URL "https://rerdavies.github.io/pipedal"
)

set (DISPLAY_VERSION "PiPedal v2.0.108-Release")

option(PIPEDAL_DISABLE_COPYRIGHT_BUILD "Skip generation of copyright notices (use on non-Debian distros)" OFF)
option(PIPEDAL_EXCLUDE_TESTS "Exclude test targets from default build" OFF)

set(PIPEDAL_T3K_PUBLISHABLE_KEY "t3k_pub_bHrH8btdwXXTtxz5ryEU8sNLF-2TGRT9")


# Determine Debian-compatible architecture name (portable across distros)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    set(DEBIAN_ARCHITECTURE "amd64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(DEBIAN_ARCHITECTURE "arm64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7l")
    set(DEBIAN_ARCHITECTURE "armhf")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i686")
    set(DEBIAN_ARCHITECTURE "i386")
else()
    execute_process(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE UNAME_M)
    message(FATAL_ERROR "Cannot determine Debian architecture from CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR} (uname -m: ${UNAME_M})")
endif()
message(STATUS "DEBIAN_ARCHITECTURE=${DEBIAN_ARCHITECTURE}")

set (PACKAGE_ARCHITECTURE ${DEBIAN_ARCHITECTURE})
set (CMAKE_INSTALL_PREFIX "/usr/")


set (LIBZIP_DO_INSTALL OFF CACHE BOOL "Disable libzip install" FORCE)
set (SQLITECPP_INSTALL OFF CACHE BOOL "Disable SQLiteCpp install" FORCE)

set(SUBMODULE_INSTALL OFF CACHE BOOL "" FORCE)


include(CTest)
enable_testing()

add_subdirectory("modules")

add_subdirectory("PiPedalCommon")

add_subdirectory("vite")

add_subdirectory("src")


set (VITE_BUILD_DIRECTORY ${PROJECT_SOURCE_DIR}/vite/dist/)


# install


# Remove sqlite3.h from the package - SQLiteCpp's bundled sqlite3 installs it
# unconditionally but we don't want it in the binary .deb package.
install(CODE "file(REMOVE \"\${CMAKE_INSTALL_PREFIX}/include/sqlite3.h\")")


install(
    DIRECTORY config/ DESTINATION /etc/pipedal/config
)
install(   
     FILES ${PROJECT_SOURCE_DIR}/debian/copyright DESTINATION /usr/share/doc/pipedal
)
#install (
#    DIRECTORY ${PROJECT_SOURCE_DIR}/src/polkit-1/rules/10-pipedal-networkmanager.rule DESTINATION /var/lib/polkit-1/localauthority/
#)

install (
    FILES ${PROJECT_SOURCE_DIR}/src/polkit-1/rules/10-pipedal-networkmanager.rules DESTINATION /etc/polkit-1/rules.d/
)
install (
    DIRECTORY ${VITE_BUILD_DIRECTORY} DESTINATION /etc/pipedal/react
)
if(NOT PIPEDAL_DISABLE_COPYRIGHT_BUILD)
    install (FILES ${PROJECT_SOURCE_DIR}/build/src/notices.txt
        DESTINATION /etc/pipedal/react/var/)
endif()

install (
    DIRECTORY ${PROJECT_SOURCE_DIR}/src/templates 
    DESTINATION /etc/pipedal/config/
)

install (
    DIRECTORY ${PROJECT_SOURCE_DIR}/default_presets/presets 
    DESTINATION /etc/pipedal/config/default_presets
)

message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")


if ("${DEBIAN_ARCHITECTURE}" MATCHES "arm64")

    # select LV2 source directory for the current build architecture

    set(LV2_SOURCE_DIRECTORY ${PROJECT_SOURCE_DIR}/lv2/${CMAKE_SYSTEM_PROCESSOR}/ToobAmp.lv2)

    install (DIRECTORY ${LV2_SOURCE_DIRECTORY} 
        DESTINATION /usr/lib/lv2
    )
elseif("${DEBIAN_ARCHITECTURE}" MATCHES "amd64")

    add_custom_target(extract_toobamp_lv2_files ALL
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/lv2/x86_64/unpackArtifact.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Extracting x64 ToobAmp components"
    )

    set(LV2_SOURCE_DIRECTORY ${PROJECT_SOURCE_DIR}/build/lv2_x64/pkg/usr/lib/lv2/ToobAmp.lv2)

    install (DIRECTORY ${LV2_SOURCE_DIRECTORY} 
        DESTINATION /usr/lib/lv2
    )
else()
    message(FATAL_ERROR "Unknown/unsupported build architecture: ${DEBIAN_ARCHITECTURE}")
endif()


set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Robin E. R. Davies<rerdavies@gmail.com>") # required
set(CPACK_PACKAGE_VENDOR "Robin E. R. Davies")
set(CPACK_PACKAGE_DESCRIPTION "IoT guitar effect pedal for Raspberry Pi, with phone-friendly web interface.")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "IoT guitar effect pedal for Raspberry Pi")
set(CPACK_DEBIAN_PACKAGE_SECTION sound)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)



set(CPACK_DEBIAN_PACKAGE_DEPENDS "libicu-dev ,lv2-dev,iw, ffmpeg, authbind, curl, gpg, alsa-base| pipewire, alsa-utils")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${DEBIAN_ARCHITECTURE})
set(CPACK_PACKAGING_INSTALL_PREFIX /usr)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
if (CMAKE_BUILD_TYPE MATCHES Debug)
    set (CPACK_STRIP_FILES false)
else()
    set (CPACK_STRIP_FILES true)
endif()

set(CPACK_DEBIAN_PACKAGE_SIGN_ALGORITHM "detached")
set(CPACK_DEBIAN_PACKAGE_SIGN_TYPE "origin")

set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA 
   "${CMAKE_CURRENT_SOURCE_DIR}/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/debian/prerm"
   )



include(CPack)

message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")

