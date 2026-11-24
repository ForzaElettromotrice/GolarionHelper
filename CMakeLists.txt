cmake_minimum_required(VERSION 3.25)

project(GolarionHelper VERSION 1.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GOLARION_TARGET "golarion_helper" CACHE STRING "Component to configure and build")
set(golarion_targets golarion_helper golarion_model golarion_play golarion_sheet_ui)
set_property(CACHE GOLARION_TARGET PROPERTY STRINGS ${golarion_targets})
if(NOT GOLARION_TARGET IN_LIST golarion_targets)
    message(FATAL_ERROR "Unknown GOLARION_TARGET '${GOLARION_TARGET}'. Choose one of: ${golarion_targets}")
endif()

option(BUILD_TESTING "Build tests for the configured components" OFF)
option(GOLARION_ALLOW_DOWNLOADS "Allow downloading missing project dependencies" ON)
include(CTest)

if(NOT GOLARION_TARGET STREQUAL "golarion_sheet_ui")
    include(cmake/JsonDependency.cmake)
    add_subdirectory(core)
endif()
if(GOLARION_TARGET STREQUAL "golarion_helper" OR GOLARION_TARGET STREQUAL "golarion_sheet_ui")
    add_subdirectory(frontend)
endif()

if(CMAKE_HOST_WIN32 AND WIN32 AND GOLARION_TARGET STREQUAL "golarion_helper")
    include(cmake/WindowsInstaller.cmake)
endif()

