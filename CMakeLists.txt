cmake_minimum_required(VERSION 3.16.0)
project(pipedal 
    VERSION 1.5.99
    DESCRIPTION "PiPedal Guitar Effect Pedal For Raspberry Pi"
    HOMEPAGE_URL "https://rerdavies.github.io/pipedal"
)

execute_process( COMMAND dpkg --print-architecture COMMAND tr -d '\n' OUTPUT_VARIABLE DEBIAN_ARCHITECTURE )

set (DISPLAY_VERSION "PiPedal v1.5.99-Beta")
set (PACKAGE_ARCHITECTURE ${DEBIAN_ARCHITECTURE})
set (CMAKE_INSTALL_PREFIX "/usr/")

set (PIPEDAL_EXCLUDE_TESTS false)
include(CTest)
enable_testing()

add_subdirectory("modules")

add_subdirectory("PiPedalCommon")

add_subdirectory("vite")

add_subdirectory("src")


set (VITE_BUILD_DIRECTORY ${PROJECT_SOURCE_DIR}/vite/dist/)

# install

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
install (FILES ${PROJECT_SOURCE_DIR}/build/src/notices.txt
    DESTINATION /etc/pipedal/react/var/)

install (
    DIRECTORY ${PROJECT_SOURCE_DIR}/src/templates 
    DESTINATION /etc/pipedal/config/
)

install (
    DIRECTORY ${PROJECT_SOURCE_DIR}/default_presets 
    DESTINATION /etc/pipedal/config
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

# Accept either libzip4 or libzip5 to satisfy libzip 4 linkage (because Ubuntu nonsense)

set(CPACK_DEBIAN_PACKAGE_DEPENDS "libicu-dev ,lv2-dev, ffmpeg, authbind, curl, gpg, alsa-base| pipewire, alsa-utils")
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
