set(VERBOSE true)
cmake_minimum_required(VERSION 3.19.0)


include(FindPkgConfig)


pkg_check_modules(SYSTEMD "systemd")
if(!SYSTEMD_FOUND)
  message(ERROR "libsystemd-dev package not installed.")
else()
  message(STATUS "SYSTEMD_LIBRARIES: ${SYSTEMD_LIBRARIES}")
  message(STATUS "SYSTEMD_INCLUDE_DIRS: ${SYSTEMD_INCLUDE_DIRS}")
endif()


pkg_check_modules(LILV_0 "lilv-0")
if(!LILV_0_FOUND)
  message(ERROR "lilv-0 package not installed.")
else()
  message(STATUS "LILV_0_LIBRARIES: ${LILV_0_LIBRARIES}")
  message(STATUS "LILV_0_INCLUDE_DIRS: ${LILV_0_INCLUDE_DIRS}")
endif()

pkg_check_modules(JACK "jack")
if(!JACK_FOUND)
  message(ERROR "jack package not installed.")
else()
  message(STATUS "JACK_LIBRARIES: ${JACK_LIBRARIES}")
  message(STATUS "JACK_INCLUDE_DIRS: ${JACK_INCLUDE_DIRS}")
endif()


# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "Debug build")

    set (CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wno-psabi -fsanitize=address -static-libasan -O0 -DDEBUG" )
elseif(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
    message(STATUS "RelWithgDebInfo build")
    set (CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wno-psabi" )
else()
    message(STATUS "Release build")
    set (CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wno-psabi" )
endif()


# message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
# set(CMAKE_ENABLE_EXPORTS 1)

message (STATUS "Cxx flags: ${CMAKE_CXX_FLAGS}"  )

add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)

add_executable(pipedald BeastServer.cpp BeastServer.hpp HtmlHelper.cpp HtmlHelper.hpp pch.h Uri.cpp Uri.hpp
    RequestHandler.hpp main.cpp json.cpp json.hpp Scratch.cpp Lv2Host.hpp Lv2Host.cpp
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
    asan_options.cpp
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
    )

configure_file(config.hpp.in config.hpp)
include_directories( ${PiPedal_SOURCE_DIR}/. ../build/src) 

target_precompile_headers(pipedald PRIVATE pch.h)


target_include_directories(pipedald PRIVATE
   ${JACK_INCLUDE_DIRS} ${LILV_0_INCLUDE_DIRS} ${SYSTEMD_INCLUDE_DIRS}
    )

target_link_libraries(pipedald PRIVATE pthread atomic stdc++fs
    ${LILV_0_LIBRARIES} ${JACK_LIBRARIES} ${SYSTEMD_LIBRARIES} systemd
    )
#target_link_libraries(pipedald boost_system)


add_executable(pipedalconfig json.cpp json.hpp 
    HtmlHelper.cpp HtmlHelper.hpp
    CommandLineParser.hpp
    WriteTemplateFile.hpp WriteTemplateFile.cpp
    PiPedalException.hpp 
    ConfigMain.cpp
)

target_link_libraries(pipedalconfig PRIVATE pthread atomic stdc++fs
    )

add_executable(pipedalshutdownd ShutdownMain.cpp CommandLineParser.hpp
    JackServerSettings.hpp JackServerSettings.cpp
    json.hpp json.cpp HtmlHelper.hpp HtmlHelper.cpp Lv2Log.hpp Lv2Log.cpp
    Lv2SystemdLogger.hpp Lv2SystemdLogger.cpp
    )
target_link_libraries(pipedalshutdownd PRIVATE pthread atomic stdc++fs systemd)


install (TARGETS pipedalconfig pipedald pipedalshutdownd DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
   EXPORT pipedalTargets)


