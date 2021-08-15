cmake_minimum_required(VERSION 3.19.0)
project(pipedal VERSION 0.1.0)

include(CTest)
enable_testing()

# Frameworks
# catch

add_subdirectory("react")

add_subdirectory("src")
 

# add_subdirectory("test")

# add_test(NAME Test COMMAND piddletest)

set (REACT_BUILD_DIRECTORY ${PROJECT_SOURCE_DIR}/build/react/build/)


install(
    DIRECTORY config/ DESTINATION /etc/pipedal/config
)
install (
    DIRECTORY ${REACT_BUILD_DIRECTORY} DESTINATION /etc/pipedal/react
)

install (FILES  ${PROJECT_SOURCE_DIR}/src/template.service 
                ${PROJECT_SOURCE_DIR}/src/templateShutdown.service 
        DESTINATION /etc/pipedal )

install(CODE 
[[
  file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES ${CMAKE_INSTALL_PREFIX}/bin/pipedald ${CMAKE_INSTALL_PREFIX}/bin/pipedalconfig
    )
]]
)

install(
    CODE "execute_process(COMMAND ${CMAKE_INSTALL_PREFIX}/bin/pipedalconfig -install)"
)



set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
