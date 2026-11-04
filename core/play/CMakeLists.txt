cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(GolarionPlay LANGUAGES CXX)
    include(CTest)
endif()

add_library(golarion_play STATIC
    src/game.cpp
    src/network.cpp
    src/protocol.cpp
)

target_compile_features(golarion_play PUBLIC cxx_std_20)
set_target_properties(golarion_play PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(golarion_play PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(WIN32)
    target_link_libraries(golarion_play PRIVATE ws2_32)
endif()

if(MSVC)
    target_compile_options(golarion_play PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(golarion_play PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(golarion_play_host tools/host.cpp)
target_link_libraries(golarion_play_host PRIVATE golarion_play)
add_executable(golarion_play_client tools/client.cpp)
target_link_libraries(golarion_play_client PRIVATE golarion_play)
if(MSVC)
    target_compile_options(golarion_play_host PRIVATE /utf-8)
    target_compile_options(golarion_play_client PRIVATE /utf-8)
endif()

if(BUILD_TESTING)
    add_executable(play_game_test tests/game_test.cpp)
    target_link_libraries(play_game_test PRIVATE golarion_play)
    add_test(NAME play_game_test COMMAND play_game_test)

    add_executable(play_protocol_test tests/protocol_test.cpp)
    target_include_directories(play_protocol_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(play_protocol_test PRIVATE golarion_play)
    add_test(NAME play_protocol_test COMMAND play_protocol_test)

    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(NAME play_network_process_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/network_process_test.py $<TARGET_FILE:golarion_play_host> $<TARGET_FILE:golarion_play_client>)
        set_tests_properties(play_network_process_test PROPERTIES TIMEOUT 25)
    endif()
endif()
