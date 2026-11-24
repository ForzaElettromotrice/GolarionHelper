add_library(golarion_model STATIC
    src/character/spell_cast_result.cpp
    src/effect/effect_to_json.cpp
    src/effect/collection_effect_to_json.cpp
    src/effect/casting_rule_effect_to_json.cpp
    src/equipment/item_definition_json.cpp
    src/equipment/item_catalog.cpp
    src/character/enhancements.cpp
    src/effect/collection_effect.cpp
    src/effect/collection_effect_json.cpp
    src/effect/casting_rule_effect.cpp
    src/effect/casting_rule_effect_json.cpp
    src/character/character_class.cpp
    src/character/character_class_type.cpp
    src/character/character_class_archetypes.cpp
    src/character/classes.cpp
    src/character/favored_classes.cpp
    src/character/class_definition_manager.cpp
    src/character/class_definition_json.cpp
    src/character/class_option_catalog_json.cpp
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
    src/character/race.cpp
    src/character/race_definition_json.cpp
    src/character/race_definition_manager.cpp
    src/character/feat_definition_json.cpp
    src/character/feat_catalog.cpp
    src/character/feat_definition_manager.cpp
    src/character/feat_prerequisite_registry.cpp
    src/character/feats.cpp
    src/character/languages.cpp
    src/character/spell_definition.cpp
    src/character/spell_repertoire.cpp
    src/character/spellcasting.cpp
    src/character/spellcastings.cpp
    src/character/granted_spells.cpp
    src/character/prepared_spellcasting.cpp
    src/character/spontaneous_spellcasting.cpp
    src/character/flexible_spellcasting.cpp
    src/character/spell_preparation_cost.cpp
    src/character/spell_preparation_rules.cpp
    src/character/metamagic.cpp
    src/character/spell_definition_json.cpp
    src/character/spell_catalog.cpp
    src/character/spell_definition_manager.cpp
    src/effect/effect_compiler.cpp
    src/effect/effect_json.cpp
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
    src/equipment/item_usage_pool.cpp
    src/equipment/item_definition_manager.cpp
    src/equipment/money.cpp
    src/resource/modifier.cpp
    src/resource/requirement.cpp
    src/resource/modifier_set.cpp
    src/resource/expression_parser.cpp
    src/resource/contribution.cpp
    src/resource/contribution_set.cpp
    src/resource/resource_manager.cpp
    src/resource/usage_pool.cpp
    src/util/string_utils.cpp
    src/catalog/catalog_reader.cpp
    src/persistence/json.cpp
    src/persistence/spellcastings_json.cpp
)

include(GNUInstallDirs)
set(catalog_build_directory "${PROJECT_BINARY_DIR}/catalogs")
if(GOLARION_TARGET STREQUAL "golarion_helper")
    set(catalog_build_directory "${PROJECT_BINARY_DIR}/frontend/app/catalogs")
endif()
if(WIN32)
    set(catalog_install_directory "${CMAKE_INSTALL_BINDIR}/catalogs")
else()
    set(catalog_install_directory "${CMAKE_INSTALL_DATADIR}/golarion-helper/catalogs")
endif()
target_compile_definitions(golarion_model PRIVATE GOLARION_BUILD_CATALOG_DIRECTORY="${catalog_build_directory}")

function(configure_catalog directory header accessor entries)
    set(catalog_paths "")
    foreach(file IN LISTS ARGN)
        string(APPEND catalog_paths "        \"${file}\",\n")
        configure_file("resources/${file}" "${catalog_build_directory}/${file}" COPYONLY)
        if(GOLARION_TARGET STREQUAL "golarion_helper")
            get_filename_component(folder "${file}" DIRECTORY)
            install(FILES "resources/${file}" DESTINATION "${catalog_install_directory}/${folder}" COMPONENT Runtime)
        endif()
    endforeach()
    file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/golarion/${directory}/${header}.hpp"
        CONTENT [=[
#pragma once

#include "golarion/catalog/catalog_reader.hpp"

namespace golarion::catalog
{
    inline nlohmann::json @accessor@()
    {
        return readCatalog({
@catalog_paths@        }, CatalogEntries::@entries@);
    }
}
]=] @ONLY)
endfunction()

configure_catalog(character class_definition_catalog loadClassDefinitionCatalog Array
    classes/core.json
    classes/advanced.json
    classes/alternative.json
    classes/hybrid.json
    classes/occult.json
    classes/unchained.json
)

configure_catalog(character class_option_catalog loadClassOptionCatalog Objects
    class-options/advanced.json
    class-options/alternative.json
    class-options/barbarian.json
    class-options/core-remaining.json
    class-options/divine.json
    class-options/hybrid.json
    class-options/occult.json
    class-options/unchained.json
    class-options/wizard.json
)

configure_catalog(character archetype_definition_catalog loadArchetypeDefinitionCatalog Array
    archetypes/core-early.json
    archetypes/core-advanced.json
    archetypes/other.json
)

configure_catalog(character spell_definition_catalog loadSpellDefinitionCatalog Array
    spells.json
    spells/level-0-1.json
    spells/level-2.json
    spells/level-3.json
    spells/level-4.json
    spells/level-5.json
    spells/level-6.json
    spells/level-7.json
    spells/level-8.json
    spells/level-9.json
)

configure_catalog(character race_definition_catalog loadRaceDefinitionCatalog Array
    races.json
    races/core.json
    races/other.json
    races/rare.json
    races/uncommon.json
)

configure_catalog(character feat_definition_catalog loadFeatDefinitionCatalog Array
    feats.json
    feats/general.json
    feats/combat.json
    feats/other.json
)

configure_catalog(equipment item_definition_catalog loadItemDefinitionCatalog Array
    items.json
)

set_target_properties(golarion_model PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(golarion_model
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

target_link_libraries(golarion_model PRIVATE nlohmann_json::nlohmann_json)

if(MSVC)
    target_compile_options(golarion_model PRIVATE /W4 /permissive-)
else()
    target_compile_options(golarion_model PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
    add_executable(sheet_notes_test tests/character/sheet_notes_test.cpp)
    add_executable(spell_cast_result_test tests/character/spell_cast_result_test.cpp)
    add_executable(human_catalog_test tests/character/human_catalog_test.cpp)
    add_executable(race_catalog_test tests/character/race_catalog_test.cpp)
    target_link_libraries(race_catalog_test PRIVATE golarion_model nlohmann_json::nlohmann_json)
    target_include_directories(race_catalog_test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
    add_test(NAME race_catalog_test COMMAND race_catalog_test)
    target_link_libraries(human_catalog_test PRIVATE golarion_model)
    add_test(NAME human_catalog_test COMMAND human_catalog_test)
    target_link_libraries(spell_cast_result_test PRIVATE golarion_model nlohmann_json::nlohmann_json)
    add_test(NAME spell_cast_result_test COMMAND spell_cast_result_test)
    target_link_libraries(sheet_notes_test PRIVATE golarion_model)
    add_test(NAME sheet_notes_test COMMAND sheet_notes_test)

    add_executable(advancement_gate_test tests/character/advancement_gate_test.cpp)
    target_link_libraries(advancement_gate_test PRIVATE golarion_model)
    add_test(NAME advancement_gate_test COMMAND advancement_gate_test)

    add_executable(pending_choices_test tests/character/pending_choices_test.cpp src/character/race_definition_manager.cpp)
    target_include_directories(pending_choices_test PRIVATE tests/fixtures/pending_choices)
    target_link_libraries(pending_choices_test PRIVATE golarion_model nlohmann_json::nlohmann_json)
    add_test(NAME pending_choices_test COMMAND pending_choices_test)

    add_executable(custom_feats_test tests/character/custom_feats_test.cpp)
    target_link_libraries(custom_feats_test PRIVATE golarion_model nlohmann_json::nlohmann_json)
    add_test(NAME custom_feats_test COMMAND custom_feats_test)

    add_executable(custom_spells_test tests/character/custom_spells_test.cpp)
    target_link_libraries(custom_spells_test PRIVATE golarion_model nlohmann_json::nlohmann_json)
    add_test(NAME custom_spells_test COMMAND custom_spells_test)

    add_executable(custom_items_test tests/equipment/custom_items_test.cpp)
    target_include_directories(custom_items_test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
    target_link_libraries(custom_items_test PRIVATE golarion_model nlohmann_json::nlohmann_json)
    add_test(NAME custom_items_test COMMAND custom_items_test)
    add_executable(enhancements_test tests/character/enhancements_test.cpp)
    target_link_libraries(enhancements_test PRIVATE golarion_model nlohmann_json::nlohmann_json)
    add_test(NAME enhancements_test COMMAND enhancements_test)
    add_executable(casting_rule_effect_test tests/effect/casting_rule_effect_test.cpp)
    target_link_libraries(casting_rule_effect_test PRIVATE golarion_model)
    add_test(NAME casting_rule_effect_test COMMAND casting_rule_effect_test)
    add_executable(collection_effect_test tests/effect/collection_effect_test.cpp)
    target_link_libraries(collection_effect_test PRIVATE golarion_model)
    add_test(NAME collection_effect_test COMMAND collection_effect_test)
    add_executable(backend_rules_test tests/character/backend_rules_test.cpp)
    target_link_libraries(backend_rules_test PRIVATE golarion_model)
    add_test(NAME backend_rules_test COMMAND backend_rules_test)
    add_executable(sheet_operations_test tests/character/sheet_operations_test.cpp)
    target_link_libraries(sheet_operations_test PRIVATE golarion_model)
    add_test(NAME sheet_operations_test COMMAND sheet_operations_test)
    add_executable(granted_spells_test tests/character/granted_spells_test.cpp src/equipment/item_definition_manager.cpp)
    target_include_directories(granted_spells_test PRIVATE tests/fixtures/item_usage)
    target_link_libraries(granted_spells_test PRIVATE golarion_model nlohmann_json::nlohmann_json)
    add_test(NAME granted_spells_test COMMAND granted_spells_test)

    # Compile the catalog loader against an isolated fixture, without adding test items to production.
    add_executable(item_usage_pool_test tests/equipment/item_usage_pool_test.cpp src/equipment/item_definition_manager.cpp)
    target_include_directories(item_usage_pool_test PRIVATE tests/fixtures/item_usage)
    target_link_libraries(item_usage_pool_test PRIVATE golarion_model nlohmann_json::nlohmann_json)
    add_test(NAME item_usage_pool_test COMMAND item_usage_pool_test)

    add_executable(spellcasting_persistence_test tests/character/spellcasting_persistence_test.cpp)
    target_link_libraries(spellcasting_persistence_test PRIVATE golarion_model nlohmann_json::nlohmann_json)
    add_test(NAME spellcasting_persistence_test COMMAND spellcasting_persistence_test)

    add_executable(metamagic_state_view_test tests/character/metamagic_state_view_test.cpp)
    target_link_libraries(metamagic_state_view_test PRIVATE golarion_model)
    add_test(NAME metamagic_state_view_test COMMAND metamagic_state_view_test)

    add_executable(spell_repertoire_test tests/character/spell_repertoire_test.cpp)
    target_link_libraries(spell_repertoire_test PRIVATE golarion_model)
    add_test(NAME spell_repertoire_test COMMAND spell_repertoire_test)

    add_executable(spellcastings_test tests/character/spellcastings_test.cpp)
    target_link_libraries(spellcastings_test PRIVATE golarion_model)
    add_test(NAME spellcastings_test COMMAND spellcastings_test)

    add_executable(flexible_spellcasting_test tests/character/flexible_spellcasting_test.cpp)
    target_link_libraries(flexible_spellcasting_test PRIVATE golarion_model)
    add_test(NAME flexible_spellcasting_test COMMAND flexible_spellcasting_test)

    add_executable(spontaneous_spellcasting_test tests/character/spontaneous_spellcasting_test.cpp)
    target_link_libraries(spontaneous_spellcasting_test PRIVATE golarion_model)
    add_test(NAME spontaneous_spellcasting_test COMMAND spontaneous_spellcasting_test)

    add_executable(prepared_metamagic_test tests/character/prepared_metamagic_test.cpp)
    target_link_libraries(prepared_metamagic_test PRIVATE golarion_model)
    add_test(NAME prepared_metamagic_test COMMAND prepared_metamagic_test)

    add_executable(metamagic_test tests/character/metamagic_test.cpp)
    target_link_libraries(metamagic_test PRIVATE golarion_model)
    add_test(NAME metamagic_test COMMAND metamagic_test)

    add_executable(spell_preparation_rules_test tests/character/spell_preparation_rules_test.cpp)
    target_link_libraries(spell_preparation_rules_test PRIVATE golarion_model)
    add_test(NAME spell_preparation_rules_test COMMAND spell_preparation_rules_test)

    add_executable(spell_preparation_cost_test tests/character/spell_preparation_cost_test.cpp)
    target_link_libraries(spell_preparation_cost_test PRIVATE golarion_model)
    add_test(NAME spell_preparation_cost_test COMMAND spell_preparation_cost_test)

    add_executable(prepared_spellcasting_test tests/character/prepared_spellcasting_test.cpp)
    target_link_libraries(prepared_spellcasting_test PRIVATE golarion_model)
    add_test(NAME prepared_spellcasting_test COMMAND prepared_spellcasting_test)

    add_executable(spellcasting_test tests/character/spellcasting_test.cpp)
    target_link_libraries(spellcasting_test PRIVATE golarion_model)
    add_test(NAME spellcasting_test COMMAND spellcasting_test)

    add_executable(spell_definition_manager_test tests/character/spell_definition_manager_test.cpp)
    target_link_libraries(spell_definition_manager_test PRIVATE golarion_model nlohmann_json::nlohmann_json)
    add_test(NAME spell_definition_manager_test COMMAND spell_definition_manager_test)

    add_executable(spell_catalog_test tests/character/spell_catalog_test.cpp)
    target_include_directories(spell_catalog_test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
    target_link_libraries(spell_catalog_test PRIVATE golarion_model nlohmann_json::nlohmann_json)
    add_test(NAME spell_catalog_test COMMAND spell_catalog_test)

    add_executable(languages_test tests/character/languages_test.cpp)
    target_link_libraries(languages_test PRIVATE golarion_model nlohmann_json::nlohmann_json)
    add_test(NAME languages_test COMMAND languages_test)

    add_executable(feats_test tests/character/feats_test.cpp)
    target_link_libraries(feats_test PRIVATE golarion_model nlohmann_json::nlohmann_json)
    add_test(NAME feats_test COMMAND feats_test)

    add_executable(feats_recheck_test tests/character/feats_recheck_test.cpp)
    target_link_libraries(feats_recheck_test PRIVATE golarion_model)
    add_test(NAME feats_recheck_test COMMAND feats_recheck_test)

    add_executable(feat_prerequisite_registry_test tests/character/feat_prerequisite_registry_test.cpp)
    target_link_libraries(feat_prerequisite_registry_test PRIVATE golarion_model)
    add_test(NAME feat_prerequisite_registry_test COMMAND feat_prerequisite_registry_test)

    add_executable(feat_definition_manager_test tests/character/feat_definition_manager_test.cpp)
    target_link_libraries(feat_definition_manager_test PRIVATE golarion_model nlohmann_json::nlohmann_json)
    add_test(NAME feat_definition_manager_test COMMAND feat_definition_manager_test)

    add_executable(feat_catalog_test tests/character/feat_catalog_test.cpp)
    target_include_directories(feat_catalog_test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
    target_link_libraries(feat_catalog_test PRIVATE golarion_model nlohmann_json::nlohmann_json)
    add_test(NAME feat_catalog_test COMMAND feat_catalog_test)

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

    add_executable(classes_persistence_test tests/character/classes_persistence_test.cpp)
    target_link_libraries(classes_persistence_test PRIVATE golarion_model)
    add_test(NAME classes_persistence_test COMMAND classes_persistence_test)

    add_executable(class_advancement_test tests/character/class_advancement_test.cpp)
    target_link_libraries(class_advancement_test PRIVATE golarion_model)
    add_test(NAME class_advancement_test COMMAND class_advancement_test)

    add_executable(favored_classes_test tests/character/favored_classes_test.cpp)
    target_link_libraries(favored_classes_test PRIVATE golarion_model)
    add_test(NAME favored_classes_test COMMAND favored_classes_test)

    add_executable(feat_slot_availability_test tests/character/feat_slot_availability_test.cpp)
    target_link_libraries(feat_slot_availability_test PRIVATE golarion_model)
    add_test(NAME feat_slot_availability_test COMMAND feat_slot_availability_test)

    add_executable(classes_test tests/character/classes_test.cpp)
    target_link_libraries(classes_test PRIVATE golarion_model)
    add_test(NAME classes_test COMMAND classes_test)

    add_executable(character_class_test tests/character/character_class_test.cpp)
    target_link_libraries(character_class_test PRIVATE golarion_model)
    add_test(NAME character_class_test COMMAND character_class_test)

    add_executable(archetypes_test tests/character/archetypes_test.cpp)
    target_link_libraries(archetypes_test PRIVATE golarion_model)
    add_test(NAME archetypes_test COMMAND archetypes_test)

    add_executable(archetype_definition_test tests/character/archetype_definition_test.cpp)
    target_link_libraries(archetype_definition_test PRIVATE golarion_model)
    add_test(NAME archetype_definition_test COMMAND archetype_definition_test)

    add_executable(class_definition_test tests/character/class_definition_test.cpp)
    target_link_libraries(class_definition_test PRIVATE golarion_model)
    add_test(NAME class_definition_test COMMAND class_definition_test)

    add_executable(class_definition_json_test tests/character/class_definition_json_test.cpp)
    target_link_libraries(class_definition_json_test PRIVATE golarion_model nlohmann_json::nlohmann_json)
    add_test(NAME class_definition_json_test COMMAND class_definition_json_test)

    add_executable(archetype_definition_json_test tests/character/archetype_definition_json_test.cpp)
    target_link_libraries(archetype_definition_json_test PRIVATE golarion_model nlohmann_json::nlohmann_json)
    add_test(NAME archetype_definition_json_test COMMAND archetype_definition_json_test)

    add_executable(class_option_catalog_json_test tests/character/class_option_catalog_json_test.cpp)
    target_link_libraries(class_option_catalog_json_test PRIVATE golarion_model nlohmann_json::nlohmann_json)
    add_test(NAME class_option_catalog_json_test COMMAND class_option_catalog_json_test)

    add_executable(class_catalog_test tests/character/class_catalog_test.cpp)
    target_include_directories(class_catalog_test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
    target_link_libraries(class_catalog_test PRIVATE golarion_model nlohmann_json::nlohmann_json)
    add_test(NAME class_catalog_test COMMAND class_catalog_test)

    add_executable(feat_definition_test tests/character/feat_definition_test.cpp)
    target_link_libraries(feat_definition_test PRIVATE golarion_model)
    add_test(NAME feat_definition_test COMMAND feat_definition_test)

    add_executable(spell_definition_test tests/character/spell_definition_test.cpp)
    target_link_libraries(spell_definition_test PRIVATE golarion_model)
    add_test(NAME spell_definition_test COMMAND spell_definition_test)

    add_executable(race_definition_test tests/character/race_definition_test.cpp)
    target_link_libraries(race_definition_test PRIVATE golarion_model)
    add_test(NAME race_definition_test COMMAND race_definition_test)

    add_executable(race_definition_manager_test tests/character/race_definition_manager_test.cpp)
    target_link_libraries(race_definition_manager_test PRIVATE golarion_model)
    add_test(NAME race_definition_manager_test COMMAND race_definition_manager_test)

    add_executable(race_test tests/character/race_test.cpp)
    target_link_libraries(race_test PRIVATE golarion_model)
    add_test(NAME race_test COMMAND race_test)

    add_executable(effect_compiler_test tests/effect/effect_compiler_test.cpp)
    target_link_libraries(effect_compiler_test PRIVATE golarion_model)
    add_test(NAME effect_compiler_test COMMAND effect_compiler_test)

    add_executable(effect_json_test tests/effect/effect_json_test.cpp)
    target_link_libraries(effect_json_test PRIVATE golarion_model)
    add_test(NAME effect_json_test COMMAND effect_json_test)

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

    add_executable(energy_vulnerability_test tests/character/energy_vulnerability_test.cpp)
    target_link_libraries(energy_vulnerability_test PRIVATE golarion_model)
    add_test(NAME energy_vulnerability_test COMMAND energy_vulnerability_test)

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
    add_executable(skill_rank_grant_test tests/character/skill_rank_grant_test.cpp)
    target_link_libraries(skill_rank_grant_test PRIVATE golarion_model)
    add_test(NAME skill_rank_grant_test COMMAND skill_rank_grant_test)

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
    add_executable(custom_class_skill_test tests/character/custom_class_skill_test.cpp)
    target_link_libraries(custom_class_skill_test PRIVATE golarion_model)
    add_test(NAME custom_class_skill_test COMMAND custom_class_skill_test)
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
    target_link_libraries(json_test PRIVATE golarion_model)
    add_test(NAME json_test COMMAND json_test)
endif()
