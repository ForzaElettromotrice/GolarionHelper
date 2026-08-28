#include "golarion/character/ability.hpp"
#include "golarion/character/character_sheet.hpp"
#include "golarion/persistence/json.hpp"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{
    template<typename Function>
    bool throwsInvalidArgument(Function function)
    {
        try
        {
            function();
            return false;
        }
        catch (const std::invalid_argument &)
        {
            return true;
        }
    }
}

int main()
{
    using namespace golarion;

    CharacterSheet sheet;
    sheet.setName("Seelah");
    sheet.setPlayerName("Giocatore");
    sheet.setAlignment(Alignment::LawfulGood);
    sheet.setDeity("Iomedae");
    sheet.setHomeland("Cheliax");
    sheet.setGender("Donna");
    sheet.setAge(27);
    sheet.setHeightCentimeters(178);
    sheet.setWeightGrams(72000);
    sheet.setHair("Neri");
    sheet.setEyes("Marroni");
    sheet.setAppearance("Armatura splendente");
    sheet.setAbilityBaseValue(AbilityType::Strength, 16);
    sheet.setAbilityBaseValue(AbilityType::Dexterity, 9);
    sheet.setAbilityBaseValue(AbilityType::Charisma, 14);
    assert(sheet.toView().hitPoints.temporary.total == 0);
    sheet.damage(4, DamageLethality::Lethal);
    sheet.heal(4);
    assert(sheet.toView().hitPoints.current == 0);
    sheet.setAbilityBaseValue(AbilityType::Constitution, 14);
    assert(sheet.toView().hitPoints.current == 2);
    sheet.setSkillRanks(SkillType::Acrobatics, 2);
    sheet.setSkillSpecializationRanks(SkillType::Craft, "alchemy", 3);
    sheet.addSkillSpecialization(SkillType::Craft, "clockwork", "Meccanismi");
    sheet.setSkillSpecializationRanks(SkillType::Craft, "clockwork", 1);
    assert(throwsInvalidArgument([&]
    {
        sheet.setAbilityBaseValue(AbilityType::Constitution, 0);
    }));
    assert(throwsInvalidArgument([&]
    {
        sheet.setSkillRanks(SkillType::Craft, 1);
    }));
    assert(throwsInvalidArgument([&]
    {
        sheet.addSkillSpecialization(SkillType::Craft, "alchemy", "Alchimia");
    }));

    CharacterSheetView view = sheet.toView();
    assert(view.identity.name == "Seelah");
    assert(view.identity.playerName == "Giocatore");
    assert(view.identity.alignment == Alignment::LawfulGood);
    assert(view.identity.deity == "Iomedae");
    assert(view.identity.homeland == "Cheliax");
    assert(view.identity.gender == "Donna");
    assert(view.identity.age == 27);
    assert(view.identity.heightCentimeters == 178);
    assert(view.identity.weightGrams == 72000);
    assert(view.identity.hair == "Neri");
    assert(view.identity.eyes == "Marroni");
    assert(view.identity.appearance == "Armatura splendente");
    assert(view.actions.categories.size() == 5);
    assert(view.reminders.messages.empty());
    assert(view.abilities.size() == 6);
    assert(view.abilities[0].type == AbilityType::Strength);
    assert(view.abilities[0].baseValue == 16);
    assert(view.abilities[0].totalValue == 16);
    assert(view.baseAttackBonus.total == 0);
    assert(view.baseAttackBonus.contributions.contributions.empty());
    assert(view.strikes.strikes.empty());
    assert(view.attackRoutines.routines.size() == 3);
    assert(view.attackRoutines.routines[0].id == StandardAttackRoutineId);
    assert(view.attackRoutines.routines[1].id == FullAttackRoutineId);
    assert(view.attackRoutines.routines[2].id == TwoWeaponFightingRoutineId);
    assert(view.attacks.attacks.empty());
    assert(throwsInvalidArgument([&]
    {
        sheet.createAttack("attack", "Attacco", "missingRoutine");
    }));
    view = sheet.toView(StrikeCalculationContext{.activeConditions = {"Contro giganti"}});
    assert((view.strikes.activeConditions == std::vector<std::string>{"Contro giganti"}));
    assert(view.combatManeuvers.baseAttackBonus == 0);
    assert(view.combatManeuvers.maneuvers.size() == 10);
    assert(view.combatManeuvers.maneuvers[0].bonus.abilityOptions[0].totalValue == 3);
    assert(view.combatManeuvers.maneuvers[0].defense.totalValue == 12);
    assert(view.hitPoints.baseMax == 0);
    assert(view.hitPoints.max == 2);
    assert(view.hitPoints.current == 2);
    assert(view.hitPoints.temporary.total == 0);
    assert(view.hitPoints.nonLethal == 0);
    assert(view.initiative.abilityOptions.size() == 1);
    assert(view.initiative.abilityOptions[0].abilityType == AbilityType::Dexterity);
    assert(view.initiative.abilityOptions[0].totalValue == -1);
    assert(!view.armorClass.maximumDexterityBonus.has_value());
    assert(view.armorClass.maximumDexterityLimits.empty());
    assert(!view.armorClass.abilityBonusSuppressed);
    assert(view.armorClass.abilitySuppressions.empty());
    assert(view.armorClass.abilityOptions.size() == 1);
    assert(!view.armorClass.abilityOptions[0].replacementId.has_value());
    assert(view.armorClass.abilityOptions[0].abilityType == AbilityType::Dexterity);
    assert(view.armorClass.abilityOptions[0].abilityModifier == -1);
    assert(view.armorClass.abilityOptions[0].values.size() == 3);
    assert(view.armorClass.abilityOptions[0].values[0].totalValue == 9);
    assert(view.armorClass.abilityOptions[0].values[1].totalValue == 9);
    assert(view.armorClass.abilityOptions[0].values[2].totalValue == 9);
    assert(view.savingThrows.savingThrows.size() == 3);
    assert(view.savingThrows.savingThrows[0].abilityOptions[0].totalValue == 2);
    assert(view.savingThrows.savingThrows[1].abilityOptions[0].abilityType == AbilityType::Dexterity);
    assert(view.savingThrows.savingThrows[1].abilityOptions[0].totalValue == -1);
    assert(view.skills.skills.size() == 99);
    assert(view.movement.grants.empty());
    assert(view.size.baseCategory == SizeCategory::Medium);
    assert(!view.size.replacementCategory.has_value());
    assert(view.size.effectiveCategory == SizeCategory::Medium);
    assert(view.size.attackModifier == 0);
    assert(view.carryingCapacity.strength == 16);
    assert(view.carryingCapacity.effectiveStrength == 16);
    assert(view.carryingCapacity.bodyType == CarryingBodyType::Biped);
    assert(!view.carryingCapacity.bodyTypeBase.has_value());
    assert(view.carryingCapacity.size == SizeCategory::Medium);
    assert(view.carryingCapacity.lightLoadMaxGrams == 38000);
    assert(view.carryingCapacity.mediumLoadMaxGrams == 76500);
    assert(view.carryingCapacity.heavyLoadMaxGrams == 115000);
    assert(view.encumbrance.totalWeightGrams == 0);
    assert(view.encumbrance.weights.size() == 1);
    assert(view.encumbrance.weights[0].id == InventoryWeightId);
    assert(view.encumbrance.weights[0].source == "Inventario");
    assert(view.encumbrance.weights[0].grams == 0);
    assert(view.encumbrance.category == LoadCategory::Light);
    assert(view.encumbrance.lightLoadMaxGrams == 38000);
    assert(view.encumbrance.effects.runMultiplierPenalty == 0);
    assert(!view.encumbrance.effects.preventsRunning);
    assert(view.inventory.totalCarriedWeightGrams == 0);
    assert(view.inventory.items.empty());
    assert(view.inventory.containers.size() == 1);
    assert(view.inventory.containers[0].id == MainContainerId);
    assert(view.inventory.equipmentSlots.size() == 14);
    assert(view.specialDefenses.energyResistances.empty());
    assert(view.specialDefenses.damageReductions.empty());
    assert(view.specialDefenses.damageReductionCombinations.empty());
    assert(view.specialDefenses.immunities.empty());
    assert(view.specialDefenses.spellResistances.empty());
    assert(view.specialDefenses.fastHealing.empty());
    assert(view.specialDefenses.regeneration.empty());
    assert(view.conditions.conditions.size() == 30);
    assert(std::ranges::any_of(view.conditions.conditions, [](const ConditionView &condition)
    {
        return condition.id == "stable" && condition.name == "Stabilizzato";
    }));
    assert(std::ranges::any_of(view.conditions.conditions, [](const ConditionView &condition)
    {
        return condition.id == "stunned" && condition.name == "Stordito";
    }));
    assert(std::ranges::none_of(view.conditions.conditions, [](const ConditionView &condition)
    {
        return condition.id == "broken";
    }));
    assert(throwsInvalidArgument([&]
    {
        sheet.removeSkillSpecialization(SkillType::Craft, "alchemy");
    }));

    sheet.removeSkillSpecialization(SkillType::Craft, "clockwork");
    assert(throwsInvalidArgument([&]
    {
        sheet.setSkillSpecializationRanks(SkillType::Craft, "clockwork", 1);
    }));

    sheet.addSkillSpecialization(SkillType::Craft, "clockwork", "Meccanismi");
    sheet.setSkillSpecializationRanks(SkillType::Craft, "clockwork", 4);
    sheet.addCondition(ConditionEntry(ConditionEntryDefinition{
        .id = "spell.sickened",
        .conditionId = "sickened",
        .source = "Incantesimo"
    }));
    view = sheet.toView();
    const auto sickened = std::ranges::find_if(view.conditions.conditions, [](const ConditionView &condition)
    {
        return condition.id == "sickened";
    });
    assert(sickened != view.conditions.conditions.end());
    assert(sickened->effectiveSeverity == 1);
    assert(sickened->entries.size() == 1);
    assert(view.initiative.abilityOptions[0].totalValue == -3);
    assert(view.savingThrows.savingThrows[0].abilityOptions[0].totalValue == 0);
    assert(view.combatManeuvers.maneuvers[0].bonus.abilityOptions[0].totalValue == 1);

    CharacterSheetSaveData saveData = sheet.toSaveData();
    assert(saveData.formatVersion == 22);
    assert(saveData.identity.name == "Seelah");
    assert(saveData.identity.playerName == "Giocatore");
    assert(saveData.identity.alignment == Alignment::LawfulGood);
    assert(saveData.identity.deity == "Iomedae");
    assert(saveData.identity.homeland == "Cheliax");
    assert(saveData.identity.gender == "Donna");
    assert(saveData.identity.age == 27);
    assert(saveData.identity.heightCentimeters == 178);
    assert(saveData.identity.weightGrams == 72000);
    assert(saveData.identity.hair == "Neri");
    assert(saveData.identity.eyes == "Marroni");
    assert(saveData.identity.appearance == "Armatura splendente");
    assert(saveData.abilities.size() == 6);
    assert(saveData.abilities[0].type == AbilityType::Strength);
    assert(saveData.abilities[0].baseValue == 16);
    assert(saveData.abilities[1].type == AbilityType::Dexterity);
    assert(saveData.abilities[1].baseValue == 9);
    assert(saveData.hitPoints.baseMax == 0);
    assert(saveData.hitPoints.damageTaken == 0);
    assert(saveData.hitPoints.temporary.pools.empty());
    assert(saveData.hitPoints.nonLethal == 0);
    assert(saveData.skills.skills.size() == 99);
    assert(saveData.attacks.attacks.empty());
    assert(saveData.conditions.manualEntries.size() == 1);
    assert(saveData.conditions.manualEntries[0].id == "spell.sickened");
    assert(saveData.inventory.containers.empty());
    assert(saveData.inventory.items.empty());
    const std::filesystem::path savePath = "character_sheet_test_save.json";
    sheet.save(savePath);
    CharacterSheet loadedSheet = CharacterSheet::load(savePath);
    CharacterSheetView loadedView = loadedSheet.toView();
    assert(loadedView.identity.name == "Seelah");
    assert(loadedView.identity.playerName == "Giocatore");
    assert(loadedView.identity.alignment == Alignment::LawfulGood);
    assert(loadedView.identity.deity == "Iomedae");
    assert(loadedView.identity.homeland == "Cheliax");
    assert(loadedView.identity.gender == "Donna");
    assert(loadedView.identity.age == 27);
    assert(loadedView.identity.heightCentimeters == 178);
    assert(loadedView.identity.weightGrams == 72000);
    assert(loadedView.identity.hair == "Neri");
    assert(loadedView.identity.eyes == "Marroni");
    assert(loadedView.identity.appearance == "Armatura splendente");
    assert(loadedView.abilities[0].baseValue == 16);
    assert(loadedView.abilities[0].totalValue == 16);
    assert(loadedView.strikes.strikes.empty());
    assert(loadedView.attacks.attacks.empty());
    assert(loadedView.combatManeuvers.maneuvers.size() == 10);
    assert(loadedView.combatManeuvers.maneuvers[0].bonus.abilityOptions[0].totalValue == 1);
    assert(loadedView.combatManeuvers.maneuvers[0].defense.totalValue == 12);
    assert(loadedView.hitPoints.max == 2);
    assert(loadedView.hitPoints.current == 2);
    assert(loadedView.initiative.abilityOptions.size() == 1);
    assert(loadedView.initiative.abilityOptions[0].abilityType == AbilityType::Dexterity);
    assert(loadedView.initiative.abilityOptions[0].totalValue == -3);
    assert(!loadedView.armorClass.maximumDexterityBonus.has_value());
    assert(loadedView.armorClass.abilityOptions.size() == 1);
    assert(loadedView.armorClass.abilityOptions[0].abilityType == AbilityType::Dexterity);
    assert(loadedView.armorClass.abilityOptions[0].values[0].totalValue == 9);
    assert(loadedView.savingThrows.savingThrows[0].abilityOptions[0].totalValue == 0);
    assert(loadedView.savingThrows.savingThrows[1].abilityOptions[0].abilityType == AbilityType::Dexterity);
    assert(loadedView.savingThrows.savingThrows[1].abilityOptions[0].totalValue == -3);
    assert(loadedView.skills.skills.size() == 99);
    assert(loadedView.movement.grants.empty());
    assert(loadedView.carryingCapacity.effectiveStrength == 16);
    assert(loadedView.carryingCapacity.bodyType == CarryingBodyType::Biped);
    assert(loadedView.carryingCapacity.size == SizeCategory::Medium);
    assert(loadedView.carryingCapacity.heavyLoadMaxGrams == 115000);
    assert(loadedView.encumbrance.totalWeightGrams == 0);
    assert(loadedView.encumbrance.category == LoadCategory::Light);
    assert(loadedView.inventory.items.empty());
    assert(loadedView.inventory.containers.size() == 1);
    assert(loadedView.conditions.conditions.size() == 30);
    const auto loadedClockwork = std::ranges::find_if(loadedView.skills.skills, [](const SkillView &skillView)
    {
        return skillView.resourceName == "skill.craft.clockwork";
    });
    assert(loadedClockwork != loadedView.skills.skills.end());
    assert(loadedClockwork->custom);
    assert(loadedClockwork->ranks == 4);
    assert(!loadedClockwork->classSkill);
    assert(loadedClockwork->classSkillGrants.empty());
    assert(loadedClockwork->abilityOptions.size() == 1);
    assert(loadedClockwork->abilityOptions[0].abilityType == AbilityType::Intelligence);
    assert(loadedClockwork->abilityOptions[0].totalValue == 2);
    const auto loadedSickened = std::ranges::find_if(loadedView.conditions.conditions, [](const ConditionView &condition)
    {
        return condition.id == "sickened";
    });
    assert(loadedSickened != loadedView.conditions.conditions.end());
    assert(loadedSickened->effectiveSeverity == 1);
    assert(loadedSickened->entries.size() == 1);
    assert(loadedSickened->entries[0].origin == ConditionEntryOrigin::Manual);

    loadedSheet.removeCondition("spell.sickened");
    loadedView = loadedSheet.toView();
    assert(loadedView.initiative.abilityOptions[0].totalValue == -1);
    assert(loadedView.savingThrows.savingThrows[0].abilityOptions[0].totalValue == 2);
    assert(loadedView.combatManeuvers.maneuvers[0].bonus.abilityOptions[0].totalValue == 3);

    loadedSheet.addCondition(ConditionEntry(ConditionEntryDefinition{
        .id = "table.stable",
        .conditionId = "stable",
        .source = "Punti ferita"
    }));
    loadedView = loadedSheet.toView();
    assert(loadedView.abilities[1].totalValue == 0);
    const auto unconscious = std::ranges::find_if(loadedView.conditions.conditions, [](const ConditionView &condition)
    {
        return condition.id == "unconscious";
    });
    const auto helpless = std::ranges::find_if(loadedView.conditions.conditions, [](const ConditionView &condition)
    {
        return condition.id == "helpless";
    });
    assert(unconscious != loadedView.conditions.conditions.end());
    assert(helpless != loadedView.conditions.conditions.end());
    assert(unconscious->effectiveSeverity == 1);
    assert(helpless->effectiveSeverity == 1);
    loadedSheet.removeCondition("table.stable");
    assert(loadedSheet.toView().abilities[1].totalValue == 9);
    std::filesystem::remove(savePath);

    saveData.inventory = InventorySaveData{
        .containers = {
            InventoryContainerSaveData{
                .id = "home",
                .name = "Casa",
                .maximumContentsWeightGrams = std::nullopt,
                .maximumContentsVolumeMilliliters = std::nullopt,
                .acceptedItems = std::nullopt,
                .quantityLimits = {},
                .ignoresContentsWeight = false,
                .ignoresContentsVolume = false,
                .allowsPossessionEffects = false
            }
        },
        .items = {
            InventoryItemSaveData{
                .id = "rope.saved",
                .itemDefinitionId = "hempRope15m",
                .quantity = 2,
                .containerId = "item.backpack.saved",
                .equipped = false
            },
            InventoryItemSaveData{
                .id = "backpack.saved",
                .itemDefinitionId = "commonBackpack",
                .quantity = 1,
                .containerId = "worn",
                .equipped = false
            },
            InventoryItemSaveData{
                .id = "cloak.saved",
                .itemDefinitionId = "cloakOfResistance1",
                .quantity = 1,
                .containerId = "worn",
                .equipped = true
            }
        }
    };
    const std::filesystem::path inventorySavePath = "character_sheet_inventory_test_save.json";
    persistence::saveToFile(saveData, inventorySavePath);
    CharacterSheet inventoryLoadedSheet = CharacterSheet::load(inventorySavePath);
    const CharacterSheetView inventoryLoadedView = inventoryLoadedSheet.toView();
    assert(inventoryLoadedView.inventory.totalCarriedWeightGrams == 11500);
    assert(inventoryLoadedView.inventory.items.size() == 3);
    assert(inventoryLoadedView.inventory.containers.size() == 3);
    const auto savedRope = std::ranges::find(inventoryLoadedView.inventory.items, "rope.saved", &InventoryItemView::id);
    assert(savedRope != inventoryLoadedView.inventory.items.end());
    assert(savedRope->containerId == "item.backpack.saved");
    const auto savedCloak = std::ranges::find(inventoryLoadedView.inventory.items, "cloak.saved", &InventoryItemView::id);
    assert(savedCloak != inventoryLoadedView.inventory.items.end());
    assert(savedCloak->equipped);
    assert(savedCloak->containerId == MainContainerId);
    assert(inventoryLoadedView.encumbrance.totalWeightGrams == 11500);
    const InventorySaveData restoredInventory = inventoryLoadedSheet.toSaveData().inventory;
    assert(restoredInventory.containers.size() == 1);
    assert(restoredInventory.items.size() == 3);
    std::filesystem::remove(inventorySavePath);

    sheet.removeSkillSpecialization(SkillType::Craft, "clockwork");
    sheet.removeCondition("spell.sickened");

    CharacterSheet conditionSmokeSheet;
    const CharacterSheetView conditionDefinitions = conditionSmokeSheet.toView();
    std::vector<std::string> conditionIds;
    conditionIds.reserve(conditionDefinitions.conditions.conditions.size());
    for (const ConditionView &condition : conditionDefinitions.conditions.conditions)
    {
        conditionIds.push_back(condition.id);
        const std::string entryId = "smoke." + condition.id;
        conditionSmokeSheet.addCondition(ConditionEntry(ConditionEntryDefinition{
            .id = entryId,
            .conditionId = condition.id,
            .source = "Test",
            .parameter = condition.id == "bleeding" ? std::optional<std::string>("1 PF") : std::nullopt
        }));
        const CharacterSheetView activeConditionView = conditionSmokeSheet.toView();
        const auto activeCondition = std::ranges::find_if(activeConditionView.conditions.conditions, [&condition](const ConditionView &candidate)
        {
            return candidate.id == condition.id;
        });
        assert(activeCondition != activeConditionView.conditions.conditions.end());
        assert(activeCondition->effectiveSeverity >= 1);
        conditionSmokeSheet.removeCondition(entryId);
    }
    assert((conditionIds == std::vector<std::string>{
        "bleeding",
        "blinded",
        "confused",
        "cowering",
        "dazed",
        "dazzled",
        "dead",
        "deafened",
        "disabled",
        "dying",
        "entangled",
        "fascinated",
        "fatigue",
        "fear",
        "flatFooted",
        "grappled",
        "helpless",
        "incorporeal",
        "invisible",
        "nauseated",
        "negativeLevels",
        "paralyzed",
        "petrified",
        "pinned",
        "prone",
        "sickened",
        "stable",
        "staggered",
        "stunned",
        "unconscious"
    }));

    assert(throwsInvalidArgument([&]
    {
        conditionSmokeSheet.addCondition(ConditionEntry(ConditionEntryDefinition{
            .id = "bleeding.withoutDamage",
            .conditionId = "bleeding",
            .source = "Test"
        }));
    }));
    assert(throwsInvalidArgument([&]
    {
        conditionSmokeSheet.addCondition(ConditionEntry(ConditionEntryDefinition{
            .id = "sickened.withDamage",
            .conditionId = "sickened",
            .source = "Test",
            .parameter = "1 PF"
        }));
    }));
    conditionSmokeSheet.addCondition(ConditionEntry(ConditionEntryDefinition{
        .id = "critical.bleeding",
        .conditionId = "bleeding",
        .source = "Critico Sanguinante",
        .parameter = "2d6 PF"
    }));
    const CharacterSheetView bleedingView = conditionSmokeSheet.toView();
    const auto bleeding = std::ranges::find_if(bleedingView.conditions.conditions, [](const ConditionView &condition)
    {
        return condition.id == "bleeding";
    });
    assert(bleeding != bleedingView.conditions.conditions.end());
    assert(bleeding->entryParameterName == "Danno per turno");
    assert(bleeding->entries.size() == 1);
    assert(bleeding->entries[0].parameter == "2d6 PF");
    const CharacterSheetSaveData bleedingSaveData = conditionSmokeSheet.toSaveData();
    assert(bleedingSaveData.conditions.manualEntries.size() == 1);
    assert(bleedingSaveData.conditions.manualEntries[0].parameter == "2d6 PF");
    const std::filesystem::path bleedingSavePath = "character_sheet_bleeding_test_save.json";
    conditionSmokeSheet.save(bleedingSavePath);
    CharacterSheet loadedBleedingSheet = CharacterSheet::load(bleedingSavePath);
    const CharacterSheetView loadedBleedingView = loadedBleedingSheet.toView();
    const auto loadedBleeding = std::ranges::find_if(loadedBleedingView.conditions.conditions, [](const ConditionView &condition)
    {
        return condition.id == "bleeding";
    });
    assert(loadedBleeding != loadedBleedingView.conditions.conditions.end());
    assert(loadedBleeding->entries[0].parameter == "2d6 PF");
    assert(loadedBleedingView.reminders.messages.size() == 1);
    assert(loadedBleedingView.reminders.messages[0].find("2d6 PF") != std::string::npos);
    std::filesystem::remove(bleedingSavePath);
    conditionSmokeSheet.removeCondition("critical.bleeding");

    conditionSmokeSheet.addCondition(ConditionEntry(ConditionEntryDefinition{
        .id = "smoke.fear.maximum",
        .conditionId = "fear",
        .source = "Test",
        .severity = 3
    }));
    conditionSmokeSheet.addCondition(ConditionEntry(ConditionEntryDefinition{
        .id = "smoke.fatigue.exhausted",
        .conditionId = "fatigue",
        .source = "Test",
        .severity = 2
    }));
    const CharacterSheetView maximumStageView = conditionSmokeSheet.toView();
    const auto fear = std::ranges::find_if(maximumStageView.conditions.conditions, [](const ConditionView &condition)
    {
        return condition.id == "fear";
    });
    const auto fatigue = std::ranges::find_if(maximumStageView.conditions.conditions, [](const ConditionView &condition)
    {
        return condition.id == "fatigue";
    });
    assert(fear != maximumStageView.conditions.conditions.end());
    assert(fatigue != maximumStageView.conditions.conditions.end());
    assert(fear->effectiveSeverity == 3);
    assert(fatigue->effectiveSeverity == 2);
    assert(maximumStageView.abilities[0].totalValue == 4);
    assert(maximumStageView.abilities[1].totalValue == 4);
    conditionSmokeSheet.removeCondition("smoke.fear.maximum");
    conditionSmokeSheet.removeCondition("smoke.fatigue.exhausted");
    assert(conditionSmokeSheet.toView().abilities[0].totalValue == 10);
    assert(conditionSmokeSheet.toView().abilities[1].totalValue == 10);

    return 0;
}
