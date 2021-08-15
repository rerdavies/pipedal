cmake_minimum_required(VERSION 3.19.0)
project(piddletest VERSION 0.1.0)


# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set (CMAKE_CXX_FLAGS "-Wno-psabi -fsanitize=address -static-libasan")

SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")


include(CTest)
enable_testing()




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

add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
add_executable(piddletest main.cpp pch.h UriTest.cpp JsonTest.cpp


    ../src/Uri.cpp ../src/Uri.hpp ../src/HtmlHelper.cpp ../src/HtmlHelper.hpp BeastServerTest.cpp 
    ../src/BeastServer.cpp ../src/BeastServer.hpp ../src/json.hpp ../src/json.cpp
    ../src/Lv2Host.cpp ../src/Lv2Host.hpp
    ../src/Lv2Log.hpp ../src/Lv2Log.cpp
    ../src/PluginType.hpp ../src/PluginType.cpp
    ../src/Lv2Effect.hpp ../src/Lv2Effect.cpp
    ../src/Lv2PedalBoard.hpp ../src/Lv2PedalBoard.cpp

    ../src/MapFeature.hpp ../src/MapFeature.cpp
    ../src/LogFeature.hpp ../src/LogFeature.cpp
    ../src/Worker.hpp ../src/Worker.cpp
    ../src/OptionsFeature.hpp ../src/OptionsFeature.cpp
    ../src/asan_options.cpp

    ../src/Units.hpp ../src/Units.cpp


    Lv2Test.cpp

    )
target_precompile_headers(piddletest PRIVATE pch.h
     )
target_link_libraries(piddletest PUBLIC 
    pthread atomic
    ${LILV_0_LIBRARIES}
    ${JACK_LIBRARIES}
    ${SYSTEMD_LIBRARIES}
    )

target_include_directories(piddletest PRIVATE
    ${LILV_0_INCLUDE_DIRS} ${JACK_INCLUDE_DIRS} ${SYSTEMD_INCLUDE_DIRS}
    "../src"
    )

#target_link_libraries(Piddle boost_system)

