add_library(golarion_model STATIC
    src/character/ability.cpp
    src/character/armor_class.cpp
    src/character/attack.cpp
    src/character/attack_distance.cpp
    src/character/base_attack_bonus.cpp
    src/character/character_sheet.cpp
    src/character/combat_maneuvers.cpp
    src/character/damage.cpp
    src/character/hit_points.cpp
    src/character/temporary_hit_points.cpp
    src/character/initiative.cpp
    src/character/movement.cpp
    src/character/saving_throw.cpp
    src/character/saving_throws.cpp
    src/character/skill.cpp
    src/character/skills.cpp
    src/resource/modifier.cpp
    src/resource/requirement.cpp
    src/resource/modifier_set.cpp
    src/resource/expression_parser.cpp
    src/resource/contribution.cpp
    src/resource/contribution_set.cpp
    src/resource/resource_manager.cpp
    src/util/string_utils.cpp
)

find_package(nlohmann_json 3.12 REQUIRED)

add_library(golarion_persistence STATIC
    src/persistence/json.cpp
)

set_target_properties(golarion_persistence PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(golarion_persistence
    PRIVATE
        nlohmann_json::nlohmann_json
)

target_include_directories(golarion_persistence
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(golarion_model PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(golarion_model
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(golarion_model PRIVATE golarion_persistence)

add_library(golarion SHARED
    src/ffi/golarion.cpp
)

target_compile_definitions(golarion PRIVATE GH_BUILDING_LIBRARY)
target_link_libraries(golarion PRIVATE golarion_model)
target_include_directories(golarion
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(golarion_model PRIVATE /W4 /permissive-)
    target_compile_options(golarion_persistence PRIVATE /W4 /permissive-)
    target_compile_options(golarion PRIVATE /W4 /permissive-)
else()
    target_compile_options(golarion_model PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(golarion_persistence PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(golarion PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
    add_executable(ability_test tests/character/ability_test.cpp)
    target_link_libraries(ability_test PRIVATE golarion_model)
    add_test(NAME ability_test COMMAND ability_test)

    add_executable(armor_class_test tests/character/armor_class_test.cpp)
    target_link_libraries(armor_class_test PRIVATE golarion_model)
    add_test(NAME armor_class_test COMMAND armor_class_test)

    add_executable(attack_test tests/character/attack_test.cpp)
    target_link_libraries(attack_test PRIVATE golarion_model)
    add_test(NAME attack_test COMMAND attack_test)

    add_executable(base_attack_bonus_test tests/character/base_attack_bonus_test.cpp)
    target_link_libraries(base_attack_bonus_test PRIVATE golarion_model)
    add_test(NAME base_attack_bonus_test COMMAND base_attack_bonus_test)

    add_executable(character_sheet_test tests/character/character_sheet_test.cpp)
    target_link_libraries(character_sheet_test PRIVATE golarion_model)
    add_test(NAME character_sheet_test COMMAND character_sheet_test)

    add_executable(combat_maneuvers_test tests/character/combat_maneuvers_test.cpp)
    target_link_libraries(combat_maneuvers_test PRIVATE golarion_model)
    add_test(NAME combat_maneuvers_test COMMAND combat_maneuvers_test)

    add_executable(damage_test tests/character/damage_test.cpp)
    target_link_libraries(damage_test PRIVATE golarion_model)
    add_test(NAME damage_test COMMAND damage_test)

    add_executable(initiative_test tests/character/initiative_test.cpp)
    target_link_libraries(initiative_test PRIVATE golarion_model)
    add_test(NAME initiative_test COMMAND initiative_test)

    add_executable(movement_test tests/character/movement_test.cpp)
    target_link_libraries(movement_test PRIVATE golarion_model)
    add_test(NAME movement_test COMMAND movement_test)

    add_executable(hit_points_test tests/character/hit_points_test.cpp)
    target_link_libraries(hit_points_test PRIVATE golarion_model)
    add_test(NAME hit_points_test COMMAND hit_points_test)

    add_executable(temporary_hit_points_test tests/character/temporary_hit_points_test.cpp)
    target_link_libraries(temporary_hit_points_test PRIVATE golarion_model)
    add_test(NAME temporary_hit_points_test COMMAND temporary_hit_points_test)

    add_executable(game_duration_test tests/util/game_duration_test.cpp)
    target_link_libraries(game_duration_test PRIVATE golarion_model)
    add_test(NAME game_duration_test COMMAND game_duration_test)

    add_executable(skill_test tests/character/skill_test.cpp)
    target_link_libraries(skill_test PRIVATE golarion_model)
    add_test(NAME skill_test COMMAND skill_test)

    add_executable(saving_throw_test tests/character/saving_throw_test.cpp)
    target_link_libraries(saving_throw_test PRIVATE golarion_model)
    add_test(NAME saving_throw_test COMMAND saving_throw_test)

    add_executable(saving_throws_test tests/character/saving_throws_test.cpp)
    target_link_libraries(saving_throws_test PRIVATE golarion_model)
    add_test(NAME saving_throws_test COMMAND saving_throws_test)

    add_executable(skills_test tests/character/skills_test.cpp)
    target_link_libraries(skills_test PRIVATE golarion_model)
    add_test(NAME skills_test COMMAND skills_test)

    add_executable(golarion_smoke_test tests/ffi/golarion_smoke_test.cpp)
    target_link_libraries(golarion_smoke_test PRIVATE golarion)
    add_test(NAME golarion_smoke_test COMMAND golarion_smoke_test)

    add_executable(modifier_test tests/resource/modifier_test.cpp)
    target_link_libraries(modifier_test PRIVATE golarion_model)
    add_test(NAME modifier_test COMMAND modifier_test)

    add_executable(requirement_test tests/resource/requirement_test.cpp)
    target_link_libraries(requirement_test PRIVATE golarion_model)
    add_test(NAME requirement_test COMMAND requirement_test)

    add_executable(resource_manager_test tests/resource/resource_manager_test.cpp)
    target_link_libraries(resource_manager_test PRIVATE golarion_model)
    add_test(NAME resource_manager_test COMMAND resource_manager_test)

    add_executable(contribution_set_test tests/resource/contribution_set_test.cpp)
    target_link_libraries(contribution_set_test PRIVATE golarion_model)
    add_test(NAME contribution_set_test COMMAND contribution_set_test)

    add_executable(expression_parser_test tests/resource/expression_parser_test.cpp)
    target_link_libraries(expression_parser_test PRIVATE golarion_model)
    add_test(NAME expression_parser_test COMMAND expression_parser_test)

    add_executable(modifier_set_test tests/resource/modifier_set_test.cpp)
    target_link_libraries(modifier_set_test PRIVATE golarion_model)
    add_test(NAME modifier_set_test COMMAND modifier_set_test)

    add_executable(json_test tests/persistence/json_test.cpp)
    target_link_libraries(json_test PRIVATE golarion_persistence)
    add_test(NAME json_test COMMAND json_test)
endif()
