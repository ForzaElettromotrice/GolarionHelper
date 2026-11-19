cmake_minimum_required(VERSION 3.25)

project(GolarionHelper VERSION 1.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

add_subdirectory(core)

option(GOLARION_BUILD_FRONTEND "Build the Qt Quick sheet panel preview" OFF)
if(GOLARION_BUILD_FRONTEND)
    add_subdirectory(frontend)
endif()

