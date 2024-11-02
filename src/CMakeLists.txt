set(VERBOSE true)
cmake_minimum_required(VERSION 3.16.0)

set (CMAKE_INSTALL_PREFIX "/usr/")

set (USE_PCH 1)

set (ENABLE_BACKTRACE 1)

set (USE_SANITIZE true)

set(CXX_STANDARD 20)

include(FindPkgConfig)

find_package(sdbus-c++ REQUIRED)


if(DEFINED ENV{GITHUB_ACTIONS})
  message(STATUS "Building in GitHub Actions environment")
  set(GITHUB_ACTIONS TRUE)
else()
  set(GITHUB_ACTIONS FALSE)
endif()
#################################################################
# ENABLE/DISABLE VST3 Support.
# Disabled, pending approval of Steinberg VST3 License.
# Do not enable unless you have non-GPL3 access to the Steinberg 
# SDK.
#
# Plus, there don't seem to be any VST3 plugins for linux, as it 
# turns out. :-/ Status: not completely implemented due to lack 
# of test targets. Deprecated, and non-functional.
#################################################################

set (ENABLE_VST3 0)


if (ENABLE_VST3) 
    set (VST3PATH  "../../vst3sdk")
    message(STATUS " VST3 Support Enabled")
    set (VST3_INCLUDES "${VST3PATH}")
    set (VST3_LIBRARIES vst3_lib dl)
    if (CMAKE_BUILD_TYPE MATCHES Debug)
        add_compile_definitions("DEVELOPMENT")
    else()
    add_compile_definitions("RELEASE")
    endif()
    add_compile_definitions("ENABLE_VST3" )
    #add_compile_definitions("_GLIBCXX_USE_CXX11_ABI")

else()
    message(STATUS " VST3 Support Disabled")
    set (VST3_INCLUDES "")
    set (VST3_LIBRARIES "")
endif()

########### VST 3 SUPPORT #############################################3





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

pkg_check_modules(ZIP "lilv-0")

# Use LV2 headers from the /usr/lib directory.
#set (LV2DEV_INCLUDE_DIRS /usr/lib)
set (LV2DEV_INCLUDE_DIRS ) # use lvt headers fro /usr/include


# Use submodules/websocketpp heders taken from websocketpp/develop branch, because debian-provided 
# version of websocketpp cannot compile under GCC 12.x.

set (WEBSOCKETPP_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/modules/websocketpp) 
message(STATUS "WEBSOCKETPP_INCLUDE_DIRS ${WEBSOCKETPP_INCLUDE_DIRS}" )
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

if (!ENABLE_VST3)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wno-psabi -DARCHITECTURE=${CPACK_SYSTEM_NAME}")
else()
    set (CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wno-multichar -Wno-psabi")
endif()

EXECUTE_PROCESS( COMMAND dpkg --print-architecture COMMAND tr -d '\n' OUTPUT_VARIABLE DEBIAN_ARCHITECTURE )
message(STATUS DEBIAN_ARCHITECTURE="${DEBIAN_ARCHITECTURE}")

add_compile_definitions(DEBIAN_ARCHITECTURE="${DEBIAN_ARCHITECTURE}")

if (CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "Debug build")

    set (CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -O0 -DDEBUG -D_GLIBCXX_DEBUG" )
    if (USE_SANITIZE) 
        set (CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fsanitize=address -static-libasan  " )
    endif()
elseif(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
    message(STATUS "RelWithgDebInfo build")
    set (CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -DNDEBUG" )
else()
    message(STATUS "Release build")
    set (CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -DNDEBUG" )
endif()

if (ENABLE_BACKTRACE)
    set (CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -rdynamic")
    if (${DEBIAN_ARCHITECTURE} MATCHES arm64)
        set (CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -funwind-tables")
    endif()
endif()





# message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
# set(CMAKE_ENABLE_EXPORTS 1)

message (STATUS "Cxx flags: ${CMAKE_CXX_FLAGS}"  )

add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY -DBOOST_BIND_GLOBAL_PLACEHOLDERS -DONBOARDING)

 
if (VST3PATH)
set (VST3_SOURCES
    vst3/Vst3Effect.hpp Vst3Effect.cpp
    vst3/Vst3Host.hpp Vst3Host.cpp
    Vst3MidiToEvent.hpp
    vst3/Vst3RtStream.hpp Vst3RtStream.cpp
    )
else()
   set (VST3_SOURCES)
endif()

set (PIPEDAL_SOURCES
    ModFileTypes.cpp ModFileTypes.hpp
    PatchPropertyWriter.hpp
    PresetBundle.cpp PresetBundle.hpp
    RealtimeMidiEventType.hpp
    DBusToLv2Log.cpp DBusToLv2Log.hpp
    HotspotManager.cpp HotspotManager.hpp
    UpdateResults.cpp UpdateResults.hpp
    UpdaterSecurity.hpp
    Updater.cpp Updater.hpp UpdaterStatus.hpp
    GithubResponseHeaders.hpp
    Lv2PluginChangeMonitor.cpp Lv2PluginChangeMonitor.hpp
    WebServerConfig.cpp WebServerConfig.hpp
    Locale.hpp Locale.cpp
    Finally.hpp
    ZipFile.cpp ZipFile.hpp
    TemporaryFile.cpp TemporaryFile.hpp     
    FilePropertyDirectoryTree.cpp FilePropertyDirectoryTree.hpp 
    FileEntry.cpp FileEntry.hpp
    atom_object.hpp atom_object.cpp
    FileBrowserFiles.h
    FileBrowserFilesFeature.hpp FileBrowserFilesFeature.cpp
    MimeTypes.cpp MimeTypes.hpp
    inverting_mutex.hpp
    DbDezipper.hpp DbDezipper.cpp
    WebServerLog.hpp
    WifiChannelSelectors.cpp WifiChannelSelectors.hpp
    
    PiPedalUI.hpp
    PiPedalUI.cpp
    MapPathFeature.hpp
    MapPathFeature.cpp
    IHost.hpp
    StateInterface.hpp
    StateInterface.cpp
    AtomConverter.hpp
    AtomConverter.cpp
    AtomBuffer.hpp

    AutoLilvNode.hpp
    AutoLilvNode.cpp
    RtInversionGuard.hpp
    CpuUse.hpp CpuUse.cpp 
    AvahiService.cpp AvahiService.hpp
    StdErrorCapture.hpp StdErrorCapture.cpp
    Ipv6Helpers.cpp Ipv6Helpers.hpp
    PluginPreset.cpp PluginPreset.hpp
    CpuGovernor.cpp CpuGovernor.hpp
    GovernorSettings.cpp GovernorSettings.hpp
    WebServer.cpp WebServer.hpp pch.h Uri.cpp Uri.hpp

    RequestHandler.hpp 
    Scratch.cpp PluginHost.hpp PluginHost.cpp
    PluginType.hpp PluginType.cpp
    PiPedalSocket.hpp PiPedalSocket.cpp
    PiPedalVersion.hpp PiPedalVersion.cpp
    PiPedalModel.hpp PiPedalModel.cpp 
    Pedalboard.hpp Pedalboard.cpp
    Presets.hpp Presets.cpp
    Storage.hpp Storage.cpp
    Banks.hpp Banks.cpp
    AudioHost.hpp AudioHost.cpp
    JackConfiguration.hpp JackConfiguration.cpp
    defer.hpp
    Lv2Effect.cpp Lv2Effect.hpp
    Lv2Pedalboard.cpp Lv2Pedalboard.hpp
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
    AdminClient.hpp AdminClient.cpp
    MidiBinding.hpp MidiBinding.cpp
    PiPedalMath.hpp
    Lv2EventBufferWriter.hpp Lv2EventBufferWriter.cpp
    IpSubnet.hpp
    PiPedalAlsa.hpp PiPedalAlsa.cpp
    InheritPriorityMutex.hpp InheritPriorityMutex.cpp
    UnixSocket.cpp UnixSocket.hpp

    JackDriver.cpp JackDriver.hpp
    AlsaDriver.cpp AlsaDriver.hpp
    DummyAudioDriver.cpp DummyAudioDriver.hpp
    AudioDriver.hpp
    AudioConfig.hpp

    ${VST3_SOURCES}
)


configure_file(config.hpp.in config.hpp)
include_directories( 
    ${pipedald_SOURCE_DIR}/. 
    ../build/src
    ) 

set (PIPEDAL_INCLUDES     
    ${LV2DEV_INCLUDE_DIRS}
    ${JACK_INCLUDE_DIRS} ${LILV_0_INCLUDE_DIRS}  
    ${VST3_INCLUDES}
    ${WEBSOCKETPP_INCLUDE_DIRS}    
    .
)

set(PIPEDAL_LIBS libpipedald zip
    PiPedalCommon
    icui18n
    icuuc
    icudata
    pthread atomic stdc++fs asound avahi-common avahi-client systemd
    ${VST3_LIBRARIES}
    ${LILV_0_LIBRARIES} 
     # ${JACK_LIBRARIES} - pending delete for JACK support.
      )



##########################
 
add_library(libpipedald STATIC ${PIPEDAL_SOURCES})

target_include_directories(libpipedald PRIVATE ${PIPEDAL_INCLUDES}
    ) 

target_link_libraries(libpipedald PUBLIC PiPedalCommon)

if(${USE_PCH})
  target_precompile_headers(libpipedald PRIVATE pch.h)
endif()


#################################
add_executable(pipedald 
    asan_options.cpp  # disable leak checking for sanitize=address.
    main.cpp 
    )
target_include_directories(pipedald PRIVATE ${PIPEDAL_INCLUDES})

target_compile_definitions(pipedald PRIVATE "ENABLE_BACKTRACE=${ENABLE_BACKTRACE}")


target_link_libraries(pipedald PRIVATE PiPedalCommon
   
    ${PIPEDAL_LIBS}
    )

#################################
add_executable(hotspotManagerTest 
    hotspotManagerTestMain.cpp
    HotspotManager.cpp HotspotManager.hpp)

target_link_libraries(hotspotManagerTest PRIVATE ${PIPEDAL_LIBS})

set_target_properties(hotspotManagerTest PROPERTIES EXCLUDE_FROM_ALL true)



add_executable(pipedaltest testMain.cpp

    InvertingMutexTest.cpp
    jsonTest.cpp
    UpdaterTest.cpp
    
    utilTest.cpp
    
    AlsaDriverTest.cpp
    AvahiServiceTest.cpp
    WifiChannelsTest.cpp
    PiPedalAlsaTest.cpp
    UnixSocketTest.cpp

    Lv2HostLeakTest.cpp


    SystemConfigFile.hpp SystemConfigFile.cpp
    SystemConfigFileTest.cpp
    WebServerTest.cpp
    MemDebug.cpp
    MemDebug.hpp
    )
target_link_libraries(pipedaltest PRIVATE ${PIPEDAL_LIBS})
target_include_directories(pipedaltest PRIVATE ${PIPEDAL_INCLUDES}
) 

set_target_properties(pipedaltest PROPERTIES EXCLUDE_FROM_ALL true)

if (NOT GITHUB_ACTIONS) # Google perftools are not available on Github action servers.



    add_executable(pipedalProfilePlugin
        profilePluginMain.cpp ArmPerformanceCounters.hpp
        )
    target_link_libraries(pipedalProfilePlugin PRIVATE profiler  ${PIPEDAL_LIBS}) 

    target_include_directories(pipedalProfilePlugin PRIVATE ${PIPEDAL_INCLUDES}
        )
    if (${DEBIAN_ARCHITECTURE} MATCHES arm64)
        target_compile_definitions(pipedalProfilePlugin PRIVATE "PIPEDAL_AARCH64")
     endif()
     


endif()
add_executable(jsonTest
     testMain.cpp
     jsonTest.cpp
     MapFeature.cpp
     MapFeature.hpp
     AtomConverter.hpp
     AtomConverter.cpp
     AtomConverterTest.cpp
     AtomBuffer.hpp
     Promise.hpp
     PromiseTest.cpp
)
target_link_libraries(jsonTest PRIVATE PiPedalCommon)
target_include_directories(jsonTest PRIVATE ${PIPEDAL_INCLUDES}
    ) 

set_target_properties(jsonTest PROPERTIES EXCLUDE_FROM_ALL true)


target_link_libraries(jsonTest PRIVATE pthread)


add_test(NAME jsonTest COMMAND jsonTest)




include_directories( ${pipedald_SOURCE_DIR}/. ../build/src) 

if(${USE_PCH})
target_precompile_headers(pipedaltest PRIVATE pch.h)
endif()


target_include_directories(pipedaltest PRIVATE ${PIPEDAL_INCLUDES}
    )

target_link_libraries(pipedaltest PRIVATE ${PIPEDAL_LIBS} 
    )


#########################################################
#  VST3 Source.
#
#  VST3 support is optional, because it poses unfortunate 
#  licensing problems.

#  There's no easy way to get the VST SDK to build without
#  GCC c++11 linkage, which is not compatible with  pipedal's
#  use of C++20 features (strictly a link time breakage).
#
#  So pipedal rebuilds the VST3 SDK using pipedal-compatible
#  compile flags.
#
#  The current build supports only Linux builds. Refer to the 
#  original VST3 SDK build for details on how to build 
#  for other platforms.
#
#########################################################
if(ENABLE_VST3)

    set (VST3_SRCPATH "${VST3PATH}/public.sdk/source")

    set (VST3_BASEPATH "${VST3PATH}/base")
    set (VST3_PLUGININTERFACES_BASE_PATH "${VST3PATH}/pluginterfaces/base")

    set(vst3_sources


        ${VST3_SRCPATH}/vst/vstpresetfile.cpp
        ${VST3_SRCPATH}/vst/vstpresetfile.h

        # base files.


        ${VST3_BASEPATH}/source/baseiids.cpp
        ${VST3_BASEPATH}/source/classfactoryhelpers.h
        ${VST3_BASEPATH}/source/fbuffer.cpp
        ${VST3_BASEPATH}/source/fbuffer.h
        ${VST3_BASEPATH}/source/fcleanup.h
        ${VST3_BASEPATH}/source/fcommandline.h
        ${VST3_BASEPATH}/source/fdebug.cpp
        ${VST3_BASEPATH}/source/fdebug.h
        ${VST3_BASEPATH}/source/fdynlib.cpp
        ${VST3_BASEPATH}/source/fdynlib.h
        ${VST3_BASEPATH}/source/fobject.cpp
        ${VST3_BASEPATH}/source/fobject.h
        ${VST3_BASEPATH}/source/fstreamer.cpp
        ${VST3_BASEPATH}/source/fstreamer.h
        ${VST3_BASEPATH}/source/fstring.cpp
        ${VST3_BASEPATH}/source/fstring.h
        ${VST3_BASEPATH}/source/timer.cpp
        ${VST3_BASEPATH}/source/timer.h
        ${VST3_BASEPATH}/source/updatehandler.cpp
        ${VST3_BASEPATH}/source/updatehandler.h
    
        ${VST3_BASEPATH}/thread/include/fcondition.h
        ${VST3_BASEPATH}/thread/include/flock.h
        ${VST3_BASEPATH}/thread/source/fcondition.cpp
        ${VST3_BASEPATH}/thread/source/flock.cpp
    


        # sdk_common files.
        ${VST3_SRCPATH}/common/commoniids.cpp
        ${VST3_SRCPATH}/common/openurl.cpp
        ${VST3_SRCPATH}/common/openurl.h
        ${VST3_SRCPATH}/common/systemclipboard.h
        ${VST3_SRCPATH}/common/systemclipboard_win32.cpp
        ${VST3_SRCPATH}/common/threadchecker_linux.cpp
        ${VST3_SRCPATH}/common/threadchecker_win32.cpp
        ${VST3_SRCPATH}/common/threadchecker.h
        ${VST3_SRCPATH}/common/memorystream.cpp
        ${VST3_SRCPATH}/common/memorystream.h

        # pluginterfaces
        ${VST3_PLUGININTERFACES_BASE_PATH}/conststringtable.cpp
        ${VST3_PLUGININTERFACES_BASE_PATH}/conststringtable.h
        ${VST3_PLUGININTERFACES_BASE_PATH}/coreiids.cpp
        ${VST3_PLUGININTERFACES_BASE_PATH}/falignpop.h
        ${VST3_PLUGININTERFACES_BASE_PATH}/falignpush.h
        ${VST3_PLUGININTERFACES_BASE_PATH}/fplatform.h
        ${VST3_PLUGININTERFACES_BASE_PATH}/fstrdefs.h
        ${VST3_PLUGININTERFACES_BASE_PATH}/ftypes.h
        ${VST3_PLUGININTERFACES_BASE_PATH}/funknown.cpp
        ${VST3_PLUGININTERFACES_BASE_PATH}/funknown.h
        ${VST3_PLUGININTERFACES_BASE_PATH}/funknownimpl.h
        ${VST3_PLUGININTERFACES_BASE_PATH}/futils.h
        ${VST3_PLUGININTERFACES_BASE_PATH}/fvariant.h
        ${VST3_PLUGININTERFACES_BASE_PATH}/geoconstants.h
        ${VST3_PLUGININTERFACES_BASE_PATH}/ibstream.h
        ${VST3_PLUGININTERFACES_BASE_PATH}/icloneable.h
        ${VST3_PLUGININTERFACES_BASE_PATH}/ierrorcontext.h
        ${VST3_PLUGININTERFACES_BASE_PATH}/ipersistent.h
        ${VST3_PLUGININTERFACES_BASE_PATH}/ipluginbase.h
        ${VST3_PLUGININTERFACES_BASE_PATH}/istringresult.h
        ${VST3_PLUGININTERFACES_BASE_PATH}/iupdatehandler.h
        ${VST3_PLUGININTERFACES_BASE_PATH}/keycodes.h
        ${VST3_PLUGININTERFACES_BASE_PATH}/pluginbasefwd.h
        ${VST3_PLUGININTERFACES_BASE_PATH}/smartpointer.h
        ${VST3_PLUGININTERFACES_BASE_PATH}/typesizecheck.h
        ${VST3_PLUGININTERFACES_BASE_PATH}/ucolorspec.h
        ${VST3_PLUGININTERFACES_BASE_PATH}/ustring.cpp
        ${VST3_PLUGININTERFACES_BASE_PATH}/ustring.h
    

        # sdk files.
        # ${VST3_SRCPATH}/main/moduleinit.cpp
        # ${VST3_SRCPATH}/main/moduleinit.h
        ${VST3_SRCPATH}/vst/utility/audiobuffers.h
        ${VST3_SRCPATH}/vst/utility/processcontextrequirements.h
        ${VST3_SRCPATH}/vst/utility/processdataslicer.h
        ${VST3_SRCPATH}/vst/utility/ringbuffer.h
        ${VST3_SRCPATH}/vst/utility/rttransfer.h
        ${VST3_SRCPATH}/vst/utility/sampleaccurate.h
        ${VST3_SRCPATH}/vst/utility/stringconvert.cpp
        ${VST3_SRCPATH}/vst/utility/stringconvert.h
        ${VST3_SRCPATH}/vst/utility/testing.cpp
        ${VST3_SRCPATH}/vst/utility/testing.h
        ${VST3_SRCPATH}/vst/utility/vst2persistence.h
        ${VST3_SRCPATH}/vst/utility/vst2persistence.cpp
        ${VST3_SRCPATH}/vst/vstaudioeffect.cpp
        ${VST3_SRCPATH}/vst/vstaudioeffect.h
        ${VST3_SRCPATH}/vst/vstbus.cpp
        ${VST3_SRCPATH}/vst/vstbus.h
        ${VST3_SRCPATH}/vst/vstbypassprocessor.h
        ${VST3_SRCPATH}/vst/vstcomponent.cpp
        ${VST3_SRCPATH}/vst/vstcomponent.h
        ${VST3_SRCPATH}/vst/vstcomponentbase.cpp
        ${VST3_SRCPATH}/vst/vstcomponentbase.h
        ${VST3_SRCPATH}/vst/vsteditcontroller.cpp
        ${VST3_SRCPATH}/vst/vsteditcontroller.h
        ${VST3_SRCPATH}/vst/vsteventshelper.h
        ${VST3_SRCPATH}/vst/vsthelpers.h
        ${VST3_SRCPATH}/vst/vstinitiids.cpp
        ${VST3_SRCPATH}/vst/vstnoteexpressiontypes.cpp
        ${VST3_SRCPATH}/vst/vstnoteexpressiontypes.h
        ${VST3_SRCPATH}/vst/vstparameters.cpp
        ${VST3_SRCPATH}/vst/vstparameters.h
        ${VST3_SRCPATH}/vst/vstrepresentation.cpp
        ${VST3_SRCPATH}/vst/vstrepresentation.h

        # sdk_host.a files.

        ${VST3_SRCPATH}/vst/hosting/connectionproxy.cpp
        ${VST3_SRCPATH}/vst/hosting/connectionproxy.h
        ${VST3_SRCPATH}/vst/hosting/eventlist.cpp
        ${VST3_SRCPATH}/vst/hosting/eventlist.h
        ${VST3_SRCPATH}/vst/hosting/hostclasses.cpp
        ${VST3_SRCPATH}/vst/hosting/hostclasses.h
        ${VST3_SRCPATH}/vst/hosting/module.cpp
        ${VST3_SRCPATH}/vst/hosting/module.h
        ${VST3_SRCPATH}/vst/hosting/parameterchanges.cpp
        ${VST3_SRCPATH}/vst/hosting/parameterchanges.h 
        ${VST3_SRCPATH}/vst/hosting/pluginterfacesupport.cpp
        ${VST3_SRCPATH}/vst/hosting/pluginterfacesupport.h
        ${VST3_SRCPATH}/vst/hosting/plugprovider.cpp
        ${VST3_SRCPATH}/vst/hosting/plugprovider.h
        ${VST3_SRCPATH}/vst/hosting/processdata.cpp
        ${VST3_SRCPATH}/vst/hosting/processdata.h
        ${VST3_SRCPATH}/vst/utility/optional.h
        ${VST3_SRCPATH}/vst/utility/stringconvert.cpp
        ${VST3_SRCPATH}/vst/utility/stringconvert.h
        ${VST3_SRCPATH}/vst/utility/uid.h
        ${VST3_SRCPATH}/vst/vstinitiids.cpp

        ${VST3_SRCPATH}/vst/hosting/module_linux.cpp

    # source/audiohost.cpp
    # source/audiohost.h
    # source/media/audioclient.cpp
    # ource/media/audioclient.h
    # source/media/imediaserver.h
    # source/media/iparameterclient.h
    # #source/media/jack/jackclient.cpp
    # source/media/miditovst.h
    # source/platform/appinit.h
    # source/usediids.cpp
    )

    add_library(vst3_lib STATIC ${vst3_sources})

    target_include_directories(vst3_lib PRIVATE  ${VST3_INCLUDES}
    )

    
    # add_executable(vst3test Vst3test.cpp

    #     MemDebug.cpp
    #     MemDebug.hpp

    #     Vst3SdkRepro.cpp

    #     ${VST3_FILES}
    
    #     #vst3/Vst3PresetFile.hpp Vst3PresetFile.cpp
    #     asan_options.cpp
    # )

    target_include_directories(vst3test PRIVATE ${PIPEDAL_INCLUDES} 
        )

    
    target_link_libraries(vst3test PRIVATE ${PIPEDAL_LIBS} 
        vst3_lib dl pthread
        )

endif(ENABLE_VST3)




# Build machine tests. Run tests that are not dependent on hardware.
add_test(NAME BuildTest COMMAND pipedaltest "[Build]")

# Developer tests. Run tests that only succeed on a Raspberry Pi with attached UBS Audio.
add_test(NAME DevTest COMMAND pipedaltest "[Dev]")

#################################



add_executable(pipedalconfig 
    PrettyPrinter.hpp
    SetWifiConfig.cpp SetWifiConfig.hpp
    CommandLineParser.hpp
    PiPedalException.hpp 
    ConfigMain.cpp
    alsaCheck.cpp
    alsaCheck.hpp
    ModFileTypes.cpp ModFileTypes.hpp
    PiPedalConfiguration.hpp PiPedalConfiguration.cpp
    JackServerSettings.hpp JackServerSettings.cpp
    SystemConfigFile.hpp SystemConfigFile.cpp
    WifiChannelSelectors.cpp WifiChannelSelectors.hpp
    asan_options.cpp

    )

target_link_libraries(pipedalconfig PRIVATE PiPedalCommon pthread atomic uuid stdc++fs asound
    )

add_executable(pipedal_latency_test
    PrettyPrinter.hpp
    CommandLineParser.hpp
    PiLatencyMain.cpp
    PiPedalAlsa.hpp PiPedalAlsa.cpp
    asan_options.cpp
    AlsaDriver.cpp AlsaDriver.hpp
    DummyAudioDriver.cpp DummyAudioDriver.hpp
    JackConfiguration.hpp JackConfiguration.cpp
    JackServerSettings.hpp JackServerSettings.cpp
    CpuUse.hpp
    CpuUse.cpp
    )

target_link_libraries(pipedal_latency_test PRIVATE pthread  asound PiPedalCommon
    )
 

add_executable(pipedal_alsa_info
    alsaCheck.cpp alsaCheck.hpp
    alsaCheckMain.cpp    
)
target_link_libraries(pipedal_alsa_info PRIVATE asound)

add_executable(capturepresets
    CapturePresetsMain.cpp
    Storage.cpp
    Storage.hpp
    CommandLineParser.hpp
) 

target_include_directories(capturepresets PRIVATE ${PIPEDAL_INCLUDES})

target_link_libraries(capturepresets ${PIPEDAL_LIBS})

add_executable(makeRelease
    makeReleaseMain.cpp
    CommandLineParser.hpp
) 
target_include_directories(makeRelease PRIVATE ${PIPEDAL_INCLUDES})

target_link_libraries(makeRelease ${PIPEDAL_LIBS})





add_executable(pipedal_update
    UpdateMain.cpp 
    UpdateResults.cpp UpdateResults.hpp
    Lv2SystemdLogger.cpp Lv2SystemdLogger.hpp
    TemporaryFile.cpp TemporaryFile.hpp
    AdminInstallUpdate.cpp AdminInstallUpdate.hpp
)

target_link_libraries(pipedal_update PRIVATE  stdc++fs systemd PiPedalCommon )


add_executable(pipedaladmind AdminMain.cpp CommandLineParser.hpp
    UnixSocket.cpp UnixSocket.hpp

    SetWifiConfig.cpp SetWifiConfig.hpp
    JackServerSettings.hpp JackServerSettings.cpp
    Lv2SystemdLogger.hpp Lv2SystemdLogger.cpp

    SystemConfigFile.hpp SystemConfigFile.cpp    
    CpuGovernor.cpp CpuGovernor.hpp
    asan_options.cpp

    )
target_link_libraries(pipedaladmind PRIVATE PiPedalCommon pthread atomic stdc++fs systemd )

add_executable(processcopyrights copyrightMain.cpp
    CommandLineParser.hpp 
    PiPedalException.hpp 
    asan_options.cpp    )
target_link_libraries(processcopyrights PRIVATE PiPedalCommon stdc++fs)

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
        liblilv-0-0 ${BOOST_COPYRIGHT_DIR} lv2-dev libsdbus-c++-dev librsvg2-2 libpango-1.0-0 libx11-6 libxrandr2
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${DEBIAN_COPYRIGHT_FILE} "$<TARGET_FILE:processcopyrights>"
    COMMENT "Updating copyright notices."
)

add_custom_target (
    CopyrightBuild ALL
    DEPENDS ${REACT_NOTICES_FILE} 
)


install (TARGETS pipedalconfig pipedal_latency_test DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
   EXPORT pipedalTargets)

   install (TARGETS pipedald pipedaladmind pipedal_update DESTINATION ${CMAKE_INSTALL_PREFIX}/sbin
   EXPORT pipedalSbinTargets)


