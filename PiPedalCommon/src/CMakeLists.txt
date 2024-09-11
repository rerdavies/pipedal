cmake_minimum_required(VERSION 3.19.0)


# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)# apt get lbsdbus-c++-dev

find_package(sdbus-c++ REQUIRED)

include(FindPkgConfig)
 
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")

if (CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "Debug build")

    set (CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -O0 -DDEBUG" )
endif()

# Can't get the pkg_check to work.
# pkg_check_modules(LIBNL3 "nl-genl-3")
# if(!LIBNL3_FOUND)
#   message(ERROR "libnl-genl-3-dev package not installed.")
# else()
#   message(STATUS "LIBNL3 libraries: ${LIBNL3_LIBRARIES}")
#   message(STATUS "LBNL3 includes: ${LIBNL3_INCLUDE_DIRS}")
# endif()

# nlgenl-3 library.
execute_process(COMMAND ls /usr/include/libnl3/netlink/netlink.h RESULT_VARIABLE LNL3_MISSING OUTPUT_QUIET ERROR_QUIET)
if(LNL3_MISSING)
    message(ERROR " Need to: sudo apt install libnl-3-dev libnl-genl-3-dev ")
endif()
set(LIBNL3_INCLUDE_DIRS /usr/include/libnl3)
set(LIBNL3_LIBRARIES nl-3 nl-genl-3)



message(STATUS "NMPIPEDAL CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")


# Use the sdbus-c++ target in SDBusCpp namespace
add_library(PiPedalCommon STATIC
  include/dbus/org.freedesktop.NetworkManager.Device.Statistics.hpp
  include/dbus/org.freedesktop.NetworkManager.Settings.Connection.hpp
  include/dbus/org.freedesktop.NetworkManager.IP6Config.hpp
  include/dbus/org.freedesktop.NetworkManager.Connection.Active.hpp
  include/dbus/org.freedesktop.NetworkManager.DnsManager.hpp
  include/dbus/org.freedesktop.NetworkManager.IP4Config.hpp
  include/dbus/org.freedesktop.NetworkManager.Settings.hpp
  include/dbus/org.freedesktop.NetworkManager.Device.hpp
  include/dbus/org.freedesktop.NetworkManager.DHCP6Config.hpp
  include/dbus/org.freedesktop.NetworkManager.AccessPoint.hpp
  include/dbus/org.freedesktop.NetworkManager.Device.WifiP2P.hpp
  include/dbus/org.freedesktop.NetworkManager.WifiP2PPeer.hpp  
  include/dbus/org.freedesktop.NetworkManager.hpp
  include/dbus/org.freedesktop.NetworkManager.Device.Wireless.hpp
  include/dbus/org.freedesktop.NetworkManager.DHCP4Config.hpp

  NetworkManagerInterfaces.cpp
  include/NetworkManagerInterfaces.hpp

  DBusEvent.cpp
  DBusVariantHelper.cpp
  DBusLog.cpp
  DBusDispatcher.cpp
  include/DBusDispatcher.hpp
  include/DBusEvent.hpp
  include/DBusLog.hpp
  include/DBusVariantHelper.hpp

  ofstream_synced.cpp include/ofstream_synced.hpp
  ConfigSerializer.cpp include/ConfigSerializer.hpp
  WifiRegs.cpp include/WifiRegs.hpp
  WifiRegulations.cpp include/WifiRegulations.hpp
  WifiDirectConfigSettings.cpp include/WifiDirectConfigSettings.hpp
  WifiConfigSettings.cpp include/WifiConfigSettings.hpp
  ServiceConfiguration.cpp include/ServiceConfiguration.hpp
  WriteTemplateFile.cpp include/WriteTemplateFile.hpp
  #NetLinkChannelInfo.cpp include/NetLinkChannelInfo.hpp
  ChannelInfo.cpp include/ChannelInfo.hpp
  HtmlHelper.cpp include/HtmlHelper.hpp
  RegDb.cpp include/RegDb.hpp
  ConfigUtil.cpp include/ConfigUtil.hpp
  SysExec.cpp include/SysExec.hpp
  util.cpp include/util.hpp
  json.cpp include/json.hpp

  json_variant.cpp include/json_variant.hpp
  include/autoptr_vector.h

  include/P2pConfigFiles.hpp

  ss.hpp
)
target_include_directories(PiPedalCommon PUBLIC "include" ${LIBNL3_INCLUDE_DIRS})
target_link_libraries(PiPedalCommon PUBLIC ${LIBNL3_LIBRARIES} SDBusCpp::sdbus-c++ )



