add_library(golarion_model STATIC
    src/character/action.cpp
    src/character/canonical_actions.cpp
    src/character/ability.cpp
    src/character/armor_class.cpp
    src/character/attack.cpp
    src/character/attack_routine.cpp
    src/character/canonical_attack_routines.cpp
    src/character/strike.cpp
    src/character/attack_distance.cpp
    src/character/base_attack_bonus.cpp
    src/character/carrying_capacity.cpp
    src/character/character_identity.cpp
    src/character/character_sheet.cpp
    src/character/combat_maneuvers.cpp
    src/character/condition.cpp
    src/character/conditions/bleeding.cpp
    src/character/conditions/blinded.cpp
    src/character/conditions/confused.cpp
    src/character/conditions/cowering.cpp
    src/character/conditions/dazed.cpp
    src/character/conditions/dazzled.cpp
    src/character/conditions/dead.cpp
    src/character/conditions/deafened.cpp
    src/character/conditions/disabled.cpp
    src/character/conditions/dying.cpp
    src/character/conditions/entangled.cpp
    src/character/conditions/fascinated.cpp
    src/character/conditions/fatigue.cpp
    src/character/conditions/fear.cpp
    src/character/conditions/flat_footed.cpp
    src/character/conditions/grappled.cpp
    src/character/conditions/helpless.cpp
    src/character/conditions/incorporeal.cpp
    src/character/conditions/invisible.cpp
    src/character/conditions/nauseated.cpp
    src/character/conditions/negative_levels.cpp
    src/character/conditions/paralyzed.cpp
    src/character/conditions/petrified.cpp
    src/character/conditions/pinned.cpp
    src/character/conditions/prone.cpp
    src/character/conditions/register_conditions.cpp
    src/character/conditions/sickened.cpp
    src/character/conditions/stable.cpp
    src/character/conditions/staggered.cpp
    src/character/conditions/stunned.cpp
    src/character/conditions/unconscious.cpp
    src/character/damage.cpp
    src/character/encumbrance.cpp
    src/character/hit_points.cpp
    src/character/temporary_hit_points.cpp
    src/character/initiative.cpp
    src/character/movement.cpp
    src/character/reminder.cpp
    src/character/saving_throw.cpp
    src/character/saving_throws.cpp
    src/character/size.cpp
    src/character/skill.cpp
    src/character/skills.cpp
    src/character/special_defenses.cpp
    src/equipment/container.cpp
    src/equipment/coin.cpp
    src/equipment/equipment.cpp
    src/equipment/equipment_slot.cpp
    src/equipment/inventory.cpp
    src/equipment/item.cpp
    src/equipment/item_definition_manager.cpp
    src/equipment/money.cpp
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

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/resources/items.json" ITEM_DEFINITION_CATALOG_JSON)
configure_file(
    src/equipment/item_definition_catalog.hpp.in
    "${CMAKE_CURRENT_BINARY_DIR}/generated/golarion/equipment/item_definition_catalog.hpp"
    @ONLY
)

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
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

target_link_libraries(golarion_model PRIVATE golarion_persistence nlohmann_json::nlohmann_json)

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
    add_executable(action_test tests/character/action_test.cpp)
    target_link_libraries(action_test PRIVATE golarion_model)
    add_test(NAME action_test COMMAND action_test)

    add_executable(ability_test tests/character/ability_test.cpp)
    target_link_libraries(ability_test PRIVATE golarion_model)
    add_test(NAME ability_test COMMAND ability_test)

    add_executable(armor_class_test tests/character/armor_class_test.cpp)
    target_link_libraries(armor_class_test PRIVATE golarion_model)
    add_test(NAME armor_class_test COMMAND armor_class_test)

    add_executable(attack_routine_test tests/character/attack_routine_test.cpp)
    target_link_libraries(attack_routine_test PRIVATE golarion_model)
    add_test(NAME attack_routine_test COMMAND attack_routine_test)

    add_executable(attacks_test tests/character/attacks_test.cpp)
    target_link_libraries(attacks_test PRIVATE golarion_model)
    add_test(NAME attacks_test COMMAND attacks_test)

    add_executable(strike_test tests/character/strike_test.cpp)
    target_link_libraries(strike_test PRIVATE golarion_model)
    add_test(NAME strike_test COMMAND strike_test)

    add_executable(base_attack_bonus_test tests/character/base_attack_bonus_test.cpp)
    target_link_libraries(base_attack_bonus_test PRIVATE golarion_model)
    add_test(NAME base_attack_bonus_test COMMAND base_attack_bonus_test)

    add_executable(carrying_capacity_test tests/character/carrying_capacity_test.cpp)
    target_link_libraries(carrying_capacity_test PRIVATE golarion_model)
    add_test(NAME carrying_capacity_test COMMAND carrying_capacity_test)

    add_executable(canonical_action_inhibitions_test tests/character/canonical_action_inhibitions_test.cpp)
    target_link_libraries(canonical_action_inhibitions_test PRIVATE golarion_model)
    add_test(NAME canonical_action_inhibitions_test COMMAND canonical_action_inhibitions_test)

    add_executable(character_sheet_test tests/character/character_sheet_test.cpp)
    target_link_libraries(character_sheet_test PRIVATE golarion_model)
    add_test(NAME character_sheet_test COMMAND character_sheet_test)

    add_executable(character_identity_test tests/character/character_identity_test.cpp)
    target_link_libraries(character_identity_test PRIVATE golarion_model)
    add_test(NAME character_identity_test COMMAND character_identity_test)

    add_executable(combat_maneuvers_test tests/character/combat_maneuvers_test.cpp)
    target_link_libraries(combat_maneuvers_test PRIVATE golarion_model)
    add_test(NAME combat_maneuvers_test COMMAND combat_maneuvers_test)

    add_executable(condition_test tests/character/condition_test.cpp)
    target_link_libraries(condition_test PRIVATE golarion_model)
    add_test(NAME condition_test COMMAND condition_test)

    add_executable(condition_reminders_test tests/character/condition_reminders_test.cpp)
    target_link_libraries(condition_reminders_test PRIVATE golarion_model)
    add_test(NAME condition_reminders_test COMMAND condition_reminders_test)

    add_executable(damage_test tests/character/damage_test.cpp)
    target_link_libraries(damage_test PRIVATE golarion_model)
    add_test(NAME damage_test COMMAND damage_test)

    add_executable(encumbrance_test tests/character/encumbrance_test.cpp)
    target_link_libraries(encumbrance_test PRIVATE golarion_model)
    add_test(NAME encumbrance_test COMMAND encumbrance_test)

    add_executable(initiative_test tests/character/initiative_test.cpp)
    target_link_libraries(initiative_test PRIVATE golarion_model)
    add_test(NAME initiative_test COMMAND initiative_test)

    add_executable(movement_test tests/character/movement_test.cpp)
    target_link_libraries(movement_test PRIVATE golarion_model)
    add_test(NAME movement_test COMMAND movement_test)

    add_executable(reminder_test tests/character/reminder_test.cpp)
    target_link_libraries(reminder_test PRIVATE golarion_model)
    add_test(NAME reminder_test COMMAND reminder_test)

    add_executable(special_defenses_test tests/character/special_defenses_test.cpp)
    target_link_libraries(special_defenses_test PRIVATE golarion_model)
    add_test(NAME special_defenses_test COMMAND special_defenses_test)

    add_executable(hit_points_test tests/character/hit_points_test.cpp)
    target_link_libraries(hit_points_test PRIVATE golarion_model)
    add_test(NAME hit_points_test COMMAND hit_points_test)

    add_executable(hit_point_conditions_test tests/character/hit_point_conditions_test.cpp)
    target_link_libraries(hit_point_conditions_test PRIVATE golarion_model)
    add_test(NAME hit_point_conditions_test COMMAND hit_point_conditions_test)

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

    add_executable(size_test tests/character/size_test.cpp)
    target_link_libraries(size_test PRIVATE golarion_model)
    add_test(NAME size_test COMMAND size_test)

    add_executable(skills_test tests/character/skills_test.cpp)
    target_link_libraries(skills_test PRIVATE golarion_model)
    add_test(NAME skills_test COMMAND skills_test)

    add_executable(item_test tests/equipment/item_test.cpp)
    target_link_libraries(item_test PRIVATE golarion_model)
    add_test(NAME item_test COMMAND item_test)

    add_executable(item_definition_manager_test tests/equipment/item_definition_manager_test.cpp)
    target_link_libraries(item_definition_manager_test PRIVATE golarion_model)
    add_test(NAME item_definition_manager_test COMMAND item_definition_manager_test)

    add_executable(money_test tests/equipment/money_test.cpp)
    target_link_libraries(money_test PRIVATE golarion_model)
    add_test(NAME money_test COMMAND money_test)

    add_executable(container_test tests/equipment/container_test.cpp)
    target_link_libraries(container_test PRIVATE golarion_model)
    add_test(NAME container_test COMMAND container_test)

    add_executable(equipment_slot_test tests/equipment/equipment_slot_test.cpp)
    target_link_libraries(equipment_slot_test PRIVATE golarion_model)
    add_test(NAME equipment_slot_test COMMAND equipment_slot_test)

    add_executable(equipment_test tests/equipment/equipment_test.cpp)
    target_link_libraries(equipment_test PRIVATE golarion_model)
    add_test(NAME equipment_test COMMAND equipment_test)

    add_executable(inventory_test tests/equipment/inventory_test.cpp)
    target_link_libraries(inventory_test PRIVATE golarion_model)
    add_test(NAME inventory_test COMMAND inventory_test)

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
