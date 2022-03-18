set(VERBOSE true)
cmake_minimum_required(VERSION 3.16.0)

set (CMAKE_INSTALL_PREFIX "/usr/")

include(FindPkgConfig)


#  apt install libsdbus-c++-dev
# apt install libsdbus-c++-bin
find_package(sdbus-c++ REQUIRED)





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


pkg_check_modules(SYSTEMD "systemd")
if(!SYSTEMD_FOUND)
  message(STATUS "libsystemd-dev package not installed.")
endif()


pkg_check_modules(LILV_0 "lilv-0")
if(!LILV_0_FOUND)
  message(ERROR "liblilv-dev package not installed.")
else()
  message(STATUS "LILV_0_LIBRARIES: ${LILV_0_LIBRARIES}")
  message(STATUS "LILV_0_INCLUDE_DIRS: ${LILV_0_INCLUDE_DIRS}")
endif()


# Use LV2 headers from the /usr/lib directory.
set (LV2DEV_INCLUDE_DIRS /usr/lib)


pkg_check_modules(JACK "jack")
if(!JACK_FOUND)
  message(ERROR "jack package not installed.")
else()
  message(STATUS "JACK_LIBRARIES: ${JACK_LIBRARIES}")
  message(STATUS "JACK_INCLUDE_DIRS: ${JACK_INCLUDE_DIRS}")
endif()


# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set (USE_SANITIZE False)
if (CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "Debug build")

    set (CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wno-psabi -O0 -DDEBUG -Werror" )
    if (USE_SANITIZE) 
        set (CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-fsanitize=address -static-libasan " )
    endif()
elseif(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
    message(STATUS "RelWithgDebInfo build")
    set (CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wno-psabi -Werror" )
else()
    message(STATUS "Release build")
    set (CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wno-psabi -Werror" )
endif()


# message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
# set(CMAKE_ENABLE_EXPORTS 1)

message (STATUS "Cxx flags: ${CMAKE_CXX_FLAGS}"  )

add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY -DBOOST_BIND_GLOBAL_PLACEHOLDERS -DONBOARDING)

# build Bluez dbus proxies
# requires apt install libsdbus-c++-bin
add_custom_command(
    OUTPUT ${PROJECT_SOURCE_DIR}/src/dbus/bluez_proxy.h  ${PROJECT_SOURCE_DIR}/src/dbus/bluez_adaptor.h
    DEPENDS ${PROJECT_SOURCE_DIR}/src/dbus/bluez.xml
    COMMAND sdbus-c++-xml2cpp ${PROJECT_SOURCE_DIR}/src/dbus/bluez.xml 
        --proxy=${PROJECT_SOURCE_DIR}/src/dbus/bluez_proxy.h 
        --adaptor=${PROJECT_SOURCE_DIR}/src/dbus/bluez_adaptor.h 
    
    COMMENT "Generating D-Bus bindings for bluez"
)
 

set (PIPEDAL_SOURCES
    StdErrorCapture.hpp StdErrorCapture.cpp
    Ipv6Helpers.cpp Ipv6Helpers.hpp
    PluginPreset.cpp PluginPreset.hpp
    CpuGovernor.cpp CpuGovernor.hpp
    GovernorSettings.cpp GovernorSettings.hpp
    SysExec.cpp SysExec.hpp
    BeastServer.cpp BeastServer.hpp HtmlHelper.cpp HtmlHelper.hpp pch.h Uri.cpp Uri.hpp
    WifiConfigSettings.hpp WifiConfigSettings.cpp
    RequestHandler.hpp json.cpp json.hpp Scratch.cpp Lv2Host.hpp Lv2Host.cpp
    PluginType.hpp PluginType.cpp
    Lv2Log.hpp Lv2Log.cpp
    PiPedalSocket.hpp PiPedalSocket.cpp
    PiPedalVersion.hpp PiPedalVersion.cpp
    PiPedalModel.hpp PiPedalModel.cpp 
    PedalBoard.hpp PedalBoard.cpp
    Presets.hpp Presets.cpp
    Storage.hpp Storage.cpp
    Banks.hpp Banks.cpp
    JackHost.hpp JackHost.cpp
    JackConfiguration.hpp JackConfiguration.cpp
    defer.hpp
    Lv2Effect.cpp Lv2Effect.hpp
    Lv2PedalBoard.cpp Lv2PedalBoard.hpp
    BufferPool.hpp
    SplitEffect.hpp SplitEffect.cpp
    RingBufferReader.hpp
    MapFeature.hpp MapFeature.cpp
    LogFeature.hpp LogFeature.cpp
    Worker.hpp Worker.cpp
    OptionsFeature.hpp OptionsFeature.cpp
    VuUpdate.hpp VuUpdate.cpp
    Units.hpp Units.cpp
    RingBuffer.hpp
    PiPedalConfiguration.hpp PiPedalConfiguration.cpp
    Shutdown.hpp
    CommandLineParser.hpp
    Lv2SystemdLogger.hpp Lv2SystemdLogger.cpp
    JackServerSettings.hpp JackServerSettings.cpp
    ShutdownClient.hpp ShutdownClient.cpp
    MidiBinding.hpp MidiBinding.cpp
    PiPedalMath.hpp
    Locale.hpp Locale.cpp
    Lv2EventBufferWriter.hpp Lv2EventBufferWriter.cpp
    IpSubnet.hpp
    WifiChannels.hpp
    WifiChannels.cpp
    RegDb.cpp RegDb.hpp
    PiPedalAlsa.hpp PiPedalAlsa.cpp
    InheritPriorityMutex.hpp InheritPriorityMutex.cpp
)

configure_file(config.hpp.in config.hpp)

#################################
add_executable(pipedald ${PIPEDAL_SOURCES}     
    asan_options.cpp  # disable leak checking for sanitize=address.
    main.cpp 
    )

include_directories( ${pipedald_SOURCE_DIR}/. ../build/src) 

target_precompile_headers(pipedald PRIVATE pch.h)


target_include_directories(pipedald PRIVATE
    ${LV2DEV_INCLUDE_DIRS}
   ${JACK_INCLUDE_DIRS} ${LILV_0_INCLUDE_DIRS}  ${LIBNL3_INCLUDE_DIRS} ${LVDEV_INCLUDE_DIRS}
    )

target_link_libraries(pipedald PRIVATE pthread atomic stdc++fs asound
    ${LILV_0_LIBRARIES} ${JACK_LIBRARIES} ${LIBNL3_LIBRARIES} systemd SDBusCpp::sdbus-c++
    )

#################################
add_executable(pipedaltest ${PIPEDAL_SOURCES} testMain.cpp
    dbus/bluez_proxy.h dbus/bluez_adaptor.h
    dbus/bluez_test.cpp
    jsonTest.cpp
    WifiChannelsTest.cpp
    PiPedalAlsaTest.cpp

    Lv2HostLeakTest.cpp


    SystemConfigFile.hpp SystemConfigFile.cpp
    SystemConfigFileTest.cpp
    BeastServerTest.cpp
    MemDebug.cpp
    MemDebug.hpp
    )

configure_file(config.hpp.in config.hpp)
include_directories( ${pipedald_SOURCE_DIR}/. ../build/src) 

target_precompile_headers(pipedaltest PRIVATE pch.h)


target_include_directories(pipedaltest PRIVATE
    .
   ${LV2DEV_INCLUDE_DIRS}
   ${JACK_INCLUDE_DIRS} ${LILV_0_INCLUDE_DIRS}  ${LIBNL3_INCLUDE_DIRS}
    )

target_link_libraries(pipedaltest PRIVATE pthread atomic stdc++fs asound
    ${LILV_0_LIBRARIES} ${JACK_LIBRARIES} ${LIBNL3_LIBRARIES} systemd SDBusCpp::sdbus-c++
    )


# Build machine tests. Run tests that are not dependent on hardware.
add_test(NAME BuildTest COMMAND pipedaltest "[Build]")

# Developer tests. Run tests that only succeed on a Raspberry Pi with attached UBS Audio.
add_test(NAME DevTest COMMAND pipedaltest "[Dev]")

#################################


add_executable(pipedalconfig json.cpp json.hpp 
    HtmlHelper.cpp HtmlHelper.hpp
    CommandLineParser.hpp
    WriteTemplateFile.hpp WriteTemplateFile.cpp
    PiPedalException.hpp 
    ConfigMain.cpp
    WifiConfigSettings.cpp WifiConfigSettings.hpp 
    JackServerSettings.hpp JackServerSettings.cpp
    SetWifiConfig.hpp SetWifiConfig.cpp
    SystemConfigFile.hpp SystemConfigFile.cpp
    SysExec.hpp SysExec.cpp
    asan_options.cpp
    Lv2Log.cpp Lv2Log.hpp
    )

target_link_libraries(pipedalconfig PRIVATE pthread atomic stdc++fs
    )

add_executable(pipedaladmind ShutdownMain.cpp CommandLineParser.hpp
    JackServerSettings.hpp JackServerSettings.cpp
    json.hpp json.cpp HtmlHelper.hpp HtmlHelper.cpp Lv2Log.hpp Lv2Log.cpp
    Lv2SystemdLogger.hpp Lv2SystemdLogger.cpp
    WifiConfigSettings.cpp WifiConfigSettings.hpp 
    SetWifiConfig.hpp SetWifiConfig.cpp
    SystemConfigFile.hpp SystemConfigFile.cpp    
    SysExec.hpp SysExec.cpp
    CpuGovernor.cpp CpuGovernor.hpp
    asan_options.cpp
    )
target_link_libraries(pipedaladmind PRIVATE pthread atomic stdc++fs systemd)


add_executable(processcopyrights copyrightMain.cpp
    CommandLineParser.hpp 
    PiPedalException.hpp 
    HtmlHelper.hpp HtmlHelper.cpp
    asan_options.cpp    )
target_link_libraries(processcopyrights PRIVATE stdc++fs)

set (REACT_BUILD_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../build/react/build/)
set (REACT_NOTICES_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../build/src/notices.txt)
set (DEBIAN_COPYRIGHT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../debian/copyright)


# generate Copyright section of settings page.
# warning: there may be multiple versions. Pick the latest.
if(EXISTS "/usr/share/doc/libboost1.74-dev")
   set (BOOST_COPYRIGHT_DIR "libboost1.74-dev")
elseif(EXISTS "/usr/share/doc/libboost1.71-dev")
   set (BOOST_COPYRIGHT_DIR "libboost1.71-dev")
elseif(EXISTS "/usr/share/doc/libboost1.67-dev")
   set (BOOST_COPYRIGHT_DIR "libboost1.67-dev")
else()
    message(ERROR "Boost libary version has changed. Please update me.")
endif() 


add_custom_command(OUTPUT ${REACT_NOTICES_FILE} 
    COMMAND "$<TARGET_FILE:processcopyrights>"
        --output ${REACT_NOTICES_FILE} 
        --projectCopyright ${DEBIAN_COPYRIGHT_FILE} 
        liblilv-0-0 ${BOOST_COPYRIGHT_DIR} libnl-3-dev libnl-3-dev  lv2-dev
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${DEBIAN_COPYRIGHT_FILE} "$<TARGET_FILE:processcopyrights>"
    COMMENT "Updating copyright notices."
)

add_custom_target (
    CopyrightBuild ALL
    DEPENDS ${REACT_NOTICES_FILE} 
)


install (TARGETS pipedalconfig pipedald pipedaladmind DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
   EXPORT pipedalTargets)


