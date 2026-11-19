cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(GolarionPlay LANGUAGES CXX)
    include(CTest)
endif()

find_package(nlohmann_json 3.12 REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL 3.0 REQUIRED COMPONENTS Crypto)

add_library(golarion_play STATIC
    src/admission.cpp
    src/client_connection.cpp
    src/hosting.cpp
    src/identity_storage.cpp
    src/local_commands.cpp
    src/participants.cpp
    src/permission_commands.cpp
    src/permission_manager.cpp
    src/protocol.cpp
    src/protocol_manager.cpp
    src/resource_manager.cpp
    src/session.cpp
    src/session_code.cpp
    src/user_token.cpp
    src/user_tokens.cpp
)

target_compile_features(golarion_play PUBLIC cxx_std_20)
set_target_properties(golarion_play PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(golarion_play PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(golarion_play PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(golarion_play PRIVATE Threads::Threads OpenSSL::Crypto)

if(WIN32)
    target_link_libraries(golarion_play PRIVATE ws2_32)
endif()

if(MSVC)
    target_compile_options(golarion_play PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(golarion_play PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(golarion_play_host tools/host.cpp)
add_executable(golarion_play_client tools/client.cpp)
target_link_libraries(golarion_play_host PRIVATE golarion_play)
target_link_libraries(golarion_play_client PRIVATE golarion_play)
if(WIN32)
    target_link_libraries(golarion_play_host PRIVATE shell32)
endif()
if(MSVC)
    target_compile_options(golarion_play_host PRIVATE /utf-8)
    target_compile_options(golarion_play_client PRIVATE /utf-8)
endif()

if(BUILD_TESTING)
    add_executable(play_participants_test tests/participants_test.cpp)
    target_include_directories(play_participants_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(play_participants_test PRIVATE golarion_play)
    add_test(NAME play_participants_test COMMAND play_participants_test)
    set_tests_properties(play_participants_test PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} TIMEOUT 40)

    add_executable(play_events_test tests/events_test.cpp)
    target_include_directories(play_events_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(play_events_test PRIVATE golarion_play)
    add_test(NAME play_events_test COMMAND play_events_test)
    set_tests_properties(play_events_test PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} TIMEOUT 40)

    add_executable(play_permission_commands_test tests/permission_commands_test.cpp)
    target_include_directories(play_permission_commands_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(play_permission_commands_test PRIVATE golarion_play)
    add_test(NAME play_permission_commands_test COMMAND play_permission_commands_test)
    set_tests_properties(play_permission_commands_test PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} TIMEOUT 30)

    add_executable(play_permission_set_test tests/permission_set_test.cpp)
    target_link_libraries(play_permission_set_test PRIVATE golarion_play)
    add_test(NAME play_permission_set_test COMMAND play_permission_set_test)

    add_executable(play_permission_manager_test tests/permission_manager_test.cpp)
    target_link_libraries(play_permission_manager_test PRIVATE golarion_play)
    add_test(NAME play_permission_manager_test COMMAND play_permission_manager_test)
    set_tests_properties(play_permission_manager_test PROPERTIES TIMEOUT 15)

    add_executable(play_send_command_test tests/send_command_test.cpp)
    target_include_directories(play_send_command_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(play_send_command_test PRIVATE golarion_play)
    add_test(NAME play_send_command_test COMMAND play_send_command_test)
    set_tests_properties(play_send_command_test PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} TIMEOUT 30)

    add_executable(play_client_test tests/client_test.cpp)
    target_include_directories(play_client_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(play_client_test PRIVATE golarion_play)
    add_test(NAME play_client_test COMMAND play_client_test)
    set_tests_properties(play_client_test PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} TIMEOUT 40)

    add_executable(play_admission_test tests/admission_test.cpp)
    target_link_libraries(play_admission_test PRIVATE golarion_play)
    add_test(NAME play_admission_test COMMAND play_admission_test)
    set_tests_properties(play_admission_test PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} TIMEOUT 40)

    add_executable(play_user_token_test tests/user_token_test.cpp)
    target_link_libraries(play_user_token_test PRIVATE golarion_play)
    add_test(NAME play_user_token_test COMMAND play_user_token_test)

    add_executable(play_user_tokens_test tests/user_tokens_test.cpp)
    target_include_directories(play_user_tokens_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(play_user_tokens_test PRIVATE golarion_play)
    add_test(NAME play_user_tokens_test COMMAND play_user_tokens_test)

    add_executable(play_identity_persistence_test tests/identity_persistence_test.cpp)
    target_link_libraries(play_identity_persistence_test PRIVATE golarion_play)
    add_test(NAME play_identity_persistence_test COMMAND play_identity_persistence_test)
    set_tests_properties(play_identity_persistence_test PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

    add_executable(play_session_code_test tests/session_code_test.cpp)
    target_link_libraries(play_session_code_test PRIVATE golarion_play)
    add_test(NAME play_session_code_test COMMAND play_session_code_test)

    add_executable(play_hosting_test tests/hosting_test.cpp)
    target_link_libraries(play_hosting_test PRIVATE golarion_play)
    add_test(NAME play_hosting_test COMMAND play_hosting_test)
    set_tests_properties(play_hosting_test PROPERTIES TIMEOUT 20)

    add_executable(play_json_protocol_test tests/json_protocol_test.cpp)
    target_include_directories(play_json_protocol_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(play_json_protocol_test PRIVATE golarion_play)
    add_test(NAME play_json_protocol_test COMMAND play_json_protocol_test)

    add_executable(play_session_test tests/session_test.cpp)
    target_link_libraries(play_session_test PRIVATE golarion_play)
    add_test(NAME play_session_test COMMAND play_session_test)

    add_executable(play_protocol_manager_test tests/protocol_manager_test.cpp)
    target_link_libraries(play_protocol_manager_test PRIVATE golarion_play)
    add_test(NAME play_protocol_manager_test COMMAND play_protocol_manager_test)
    set_tests_properties(play_protocol_manager_test PROPERTIES TIMEOUT 20)

    add_executable(play_resource_manager_test tests/resource_manager_test.cpp)
    target_link_libraries(play_resource_manager_test PRIVATE golarion_play)
    add_test(NAME play_resource_manager_test COMMAND play_resource_manager_test)
endif()
