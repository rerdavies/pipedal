cmake_minimum_required(VERSION 3.16.0)
project(pipedal 
    VERSION 1.1.31
    DESCRIPTION "PiPedal Guitar Effect Pedal For Raspberry Pi"
    HOMEPAGE_URL "https://rerdavies.github.io/pipedal"
)
set (DISPLAY_VERSION "v1.1.31")

set (CMAKE_INSTALL_PREFIX "/usr/")

include(CTest)
enable_testing()

# Frameworks
# catch

add_subdirectory("submodules/pipedal_p2pd")

add_subdirectory("react")

add_subdirectory("src")

install (TARGETS pipedal_p2pd DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
   )
 
# select LV2 source directory for the current build architecture

message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
set(LV2_SOURCE_DIRECTORY ${PROJECT_SOURCE_DIR}/lv2/${CMAKE_SYSTEM_PROCESSOR}/ToobAmp.lv2)

if (NOT EXISTS ${LV2_SOURCE_DIRECTORY})
     message(FATAL_ERROR "Lv2 Binaries for architecture '${CMAKE_SYSTEM_PROCESSOR}' have not been provisioned. See lv2/Readme.md")
endif()
# add_subdirectory("test")

# add_test(NAME Test COMMAND piddletest)

set (REACT_BUILD_DIRECTORY ${PROJECT_SOURCE_DIR}/build/react/build/)

# install

install(
    DIRECTORY config/ DESTINATION /etc/pipedal/config
)
install(   
     FILES ${PROJECT_SOURCE_DIR}/debian/copyright DESTINATION /usr/share/doc/pipedal
)
install (
    DIRECTORY ${REACT_BUILD_DIRECTORY} DESTINATION /etc/pipedal/react
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

install (DIRECTORY ${LV2_SOURCE_DIRECTORY} 
       DESTINATION /usr/lib/lv2
)
install(CODE 
[[
  file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES ${CMAKE_INSTALL_PREFIX}/bin/pipedald ${CMAKE_INSTALL_PREFIX}/bin/pipedalconfig
    )
]]
)

# Should be done by install.sh instead of here so that we can build the package without root permissions.
# install(
#     CODE "execute_process(COMMAND ${CMAKE_INSTALL_PREFIX}/bin/pipedalconfig --install)"
# )


set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Robin Davies<rerdavies@gmail.com>") # required
set(CPACK_PACKAGE_VENDOR "Robin Davies")
set(CPACK_PACKAGE_DESCRIPTION "IoT guitar effect pedal for Raspberry Pi, with phone-friendly web interface.")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "IoT guitar effect for Raspberry Pi")
set(CPACK_DEBIAN_PACKAGE_SECTION sound)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
#set(CPACK_DEBIAN_PACKAGE_DEPENDS "jackd2, hostapd, dnsmasq" )
set(CPACK_DEBIAN_PACKAGE_DEPENDS "lv2-dev, hostapd, dnsmasq, authbind" )
#set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
set(CPACK_PACKAGING_INSTALL_PREFIX /usr)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set (CPACK_STRIP_FILES ON)


set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/debian/prerm")


include(CPack)
