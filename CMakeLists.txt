cmake_minimum_required(VERSION 3.6)

project(
    workflow-polaris
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_SKIP_RPATH TRUE)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

if(Workflow_DIR)
    find_package(Workflow REQUIRED CONFIG HINTS ${Workflow_DIR})
else()
    find_package(Workflow REQUIRED CONFIG)
endif()
message("Use workflow at " ${WORKFLOW_INCLUDE_DIR})

find_package(Yaml-cpp-static)
if(Yaml-cpp-static_FOUND)
    message("Use yaml-cpp static library")
else()
    find_package(Yaml-cpp REQUIRED)
    message("Use yaml-cpp dynamic library")
endif()

set(LIBRARY_NAME workflow-polaris)
add_library(${LIBRARY_NAME} STATIC
    src/PolarisClient.cc
    src/PolarisConfig.cc
    src/PolarisManager.cc
    src/PolarisPolicy.cc
    src/PolarisTask.cc
)

include_directories(
    ${WORKFLOW_INCLUDE_DIR}
    ${YAML_CPP_INCLUDE_DIR}
)

target_include_directories(${LIBRARY_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_subdirectory(example)

include(CMakePackageConfigHelpers)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(LIBRARIES_DIR ${CMAKE_CURRENT_BINARY_DIR})
configure_package_config_file(
    ${LIBRARY_NAME}-config.cmake.in
    ${PROJECT_SOURCE_DIR}/${LIBRARY_NAME}-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${LIBRARY_NAME}
    PATH_VARS INCLUDE_DIR LIBRARIES_DIR
)
