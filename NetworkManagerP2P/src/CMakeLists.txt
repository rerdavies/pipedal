cmake_minimum_required(VERSION 3.19.0)


# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)# apt get lbsdbus-c++-dev

find_package(sdbus-c++ REQUIRED)

include(FindPkgConfig)
 
pkg_check_modules(SYSTEMD "systemd")
if(!SYSTEMD_FOUND)
  message(ERROR "libsystemd-dev package not installed.")
else()
  message(STATUS "SYSTEMD_LIBRARIES: ${SYSTEMD_LIBRARIES}")
  message(STATUS "SYSTEMD_INCLUDE_DIRS: ${SYSTEMD_INCLUDE_DIRS}")
endif()


set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")

if (CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "Debug build")

    set (CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -DDEBUG" )
endif()


message(STATUS "NMPIPEDAL CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")


# Use the sdbus-c++ target in SDBusCpp namespace
add_executable(pipedal_nm_p2pd
  Install.cpp Install.hpp
  NMP2pSettings.cpp NMP2pSettings.hpp
  SignalHandler.cpp SignalHandler.hpp
  WpaInterfaces.hpp WpaInterfaces.cpp
   NetworkManagerInterfaces.hpp NetworkManagerInterfaces.cpp
   NMManager.hpp
   NMManager.cpp
   P2PManager.hpp P2PManager.cpp
   DBusEvent.hpp DBusEvent.cpp
   DBusDispatcher.hpp DBusDispatcher.cpp
   DBusLog.hpp DBusLog.cpp
   DBusVariantHelper.hpp DBusVariantHelper.cpp
   ss.hpp
   dbus/hpp/org.freedesktop.NetworkManager.hpp
   main.cpp
   CommandLineParser.hpp
   Sudo.hpp Sudo.cpp
 )
target_link_libraries(pipedal_nm_p2pd PRIVATE PiPedalCommon SDBusCpp::sdbus-c++ systemd )

target_compile_definitions(pipedal_nm_p2pd PRIVATE USE_NETWORK_MANAGER)


