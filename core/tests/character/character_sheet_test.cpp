#include "golarion/character/ability.hpp"
#include "golarion/character/character_sheet.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/modifier_group.hpp"
#include "golarion/resource/contribution_group.hpp"

#include <algorithm>
#include <cassert>
#include <filesystem>
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
    sheet.setAbilityBaseValue(AbilityType::Strength, 16);
    sheet.setAbilityBaseValue(AbilityType::Dexterity, 9);
    sheet.setAbilityBaseValue(AbilityType::Charisma, 14);
    sheet.setMaxHitPoints(10);
    sheet.setCurrentHitPoints(8);
    sheet.addTemporaryHitPoints("timed", 2, GameDuration::fromRounds(2));
    sheet.advanceTime(GameDuration::fromRounds(1));
    assert(sheet.toView().hitPoints.temporary.total == 2);
    sheet.advanceTime(GameDuration::fromRounds(1));
    assert(sheet.toView().hitPoints.temporary.total == 0);
    sheet.addTemporaryHitPoints("removable", 1, std::nullopt);
    sheet.removeTemporaryHitPoints("removable");
    sheet.addTemporaryHitPoints("manual", 3, std::nullopt);
    sheet.setNonLethalDamage(2);
    sheet.damage(4, DamageType::Lethal);
    sheet.heal(2);
    assert(sheet.toView().hitPoints.current == 9);
    sheet.setAbilityBaseValue(AbilityType::Constitution, 14);
    assert(sheet.toView().hitPoints.current == 11);
    sheet.setInitiativeAbilityType(AbilityType::Charisma);
    sheet.setSavingThrowBaseValue(SavingThrowType::Fortitude, 2);
    sheet.setSavingThrowBaseValue(SavingThrowType::Reflex, 1);
    sheet.setSavingThrowAbilityType(SavingThrowType::Reflex, AbilityType::Charisma);
    sheet.setSkillRanks(SkillType::Acrobatics, 2);
    sheet.setSkillClassSkill(SkillType::Acrobatics, true);
    sheet.setSkillAbilityType(SkillType::Craft, AbilityType::Wisdom);
    sheet.setSkillClassSkill(SkillType::Craft, true);
    sheet.setSkillSpecializationRanks(SkillType::Craft, "alchemy", 3);
    sheet.addSkillSpecialization(SkillType::Craft, "clockwork", "Meccanismi");
    sheet.setSkillSpecializationRanks(SkillType::Craft, "clockwork", 1);
    sheet.createMovementGroup("user.flight", MovementGroup({MovementGrant(MovementGrantDefinition{
        .id = "spellFly",
        .source = "Volare",
        .type = MovementType::Fly,
        .baseSpeedExpression = "12",
        .maneuverability = Maneuverability::Good,
        .affectedByArmor = true,
        .affectedByLoad = true
    })}, {MovementAdjustment(MovementAdjustmentDefinition{
        .id = "slow",
        .source = "Lentezza",
        .description = "Velocità dimezzata",
        .type = MovementAdjustmentType::SpeedMultiplier,
        .selector = MovementSelector{.type = MovementType::Fly, .grantId = std::nullopt},
        .expression = "50",
        .condition = std::nullopt
    })}));
    sheet.setMovementGroupEnabled("user.flight", true);

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

    ModifierGroup group(std::vector<TargetedModifier>{
        TargetedModifier{
            "str",
            Modifier(ModifierType::Bonus, "Potenziamento manuale", "Bonus alla Forza", BonusType::Enhancement, "2")
        },
        TargetedModifier{
            "savingThrow.all",
            Modifier(ModifierType::Bonus, "Mantello", "Bonus ai tiri salvezza", BonusType::Resistance, "1")
        }
    });
    sheet.createModifierGroup("manual.strength", std::move(group));
    sheet.setModifierGroupEnabled("manual.strength", true);

    ContributionGroup contributionGroup(std::vector<TargetedContribution>{
        TargetedContribution{
            .resourceName = "hp.max",
            .contribution = Contribution("user.toughness", "3")
        }
    });
    sheet.createContributionGroup("user.hitPoints", std::move(contributionGroup));
    sheet.setContributionGroupEnabled("user.hitPoints", true);

    ModifierGroup skillGroup(std::vector<TargetedModifier>{
        TargetedModifier{
            "skill.craft.clockwork",
            Modifier(ModifierType::Bonus, "Attrezzi", "Bonus ai meccanismi", BonusType::Circumstance, "2")
        }
    });
    sheet.createModifierGroup("manual.clockwork", std::move(skillGroup));
    sheet.setModifierGroupEnabled("manual.clockwork", true);

    CharacterSheetView view = sheet.toView();
    assert(view.abilities.size() == 6);
    assert(view.abilities[0].type == AbilityType::Strength);
    assert(view.abilities[0].baseValue == 16);
    assert(view.abilities[0].totalValue == 18);
    assert(view.hitPoints.baseMax == 10);
    assert(view.hitPoints.max == 15);
    assert(view.hitPoints.current == 14);
    assert(view.hitPoints.temporary.total == 0);
    assert(view.hitPoints.nonLethal == 0);
    assert(view.initiative.abilityType == AbilityType::Charisma);
    assert(view.initiative.totalValue == 2);
    assert(view.savingThrows.savingThrows.size() == 3);
    assert(view.savingThrows.savingThrows[0].totalValue == 5);
    assert(view.savingThrows.savingThrows[1].abilityType == AbilityType::Charisma);
    assert(view.savingThrows.savingThrows[1].totalValue == 4);
    assert(view.skills.skills.size() == 99);
    assert(view.movement.grants.size() == 2);
    assert(view.movement.grants[1].id == "spellFly");
    assert(view.movement.grants[1].effectiveUnits == 6);
    assert(view.movementGroups.groups.size() == 1);
    assert(view.movementGroups.groups[0].id == "user.flight");
    assert(view.movementGroups.groups[0].enabled);
    assert(view.resources.targets.size() == 13);
    assert(view.resources.targets[8].name == "level");
    assert(view.resources.targets[8].value == 1);
    assert(view.resources.targets[9].name == "str");
    assert(view.resources.targets[9].value == 18);
    assert(!view.resources.enhanceableResources.empty());
    const auto flyCategory = std::ranges::find(view.resources.enhanceableResources, "speed.fly", &ResourceManagerView::EnhanceableResourceView::name);
    assert(flyCategory != view.resources.enhanceableResources.end());
    assert(flyCategory->parentResources == std::vector<std::string>{"speed.all"});
    assert(view.resources.collections.size() == 3);
    assert(view.resources.collections[0] == "hp.temporary");
    assert(view.resources.collections[1] == "movement.adjustments");
    assert(view.resources.collections[2] == "movement.grants");
    assert(view.modifierGroups.groups.size() == 2);
    assert(view.modifierGroups.groups[0].id == "manual.clockwork");
    assert(view.modifierGroups.groups[0].enabled);
    assert(view.modifierGroups.groups[1].id == "manual.strength");
    assert(view.modifierGroups.groups[1].enabled);
    assert(view.contributionGroups.groups.size() == 1);
    assert(view.contributionGroups.groups[0].id == "user.hitPoints");
    assert(view.contributionGroups.groups[0].enabled);

    assert(throwsInvalidArgument([&]
    {
        sheet.removeSkillSpecialization(SkillType::Craft, "alchemy");
    }));

    sheet.removeSkillSpecialization(SkillType::Craft, "clockwork");
    assert(throwsInvalidArgument([&]
    {
        sheet.setSkillSpecializationRanks(SkillType::Craft, "clockwork", 1);
    }));

    view = sheet.toView();
    assert(view.modifierGroups.groups.size() == 1);
    assert(view.modifierGroups.groups[0].id == "manual.strength");

    sheet.addSkillSpecialization(SkillType::Craft, "clockwork", "Meccanismi");
    sheet.setSkillSpecializationRanks(SkillType::Craft, "clockwork", 4);
    ModifierGroup restoredSkillGroup(std::vector<TargetedModifier>{
        TargetedModifier{
            "skill.craft.clockwork",
            Modifier(ModifierType::Bonus, "Attrezzi", "Bonus ai meccanismi", BonusType::Circumstance, "2")
        }
    });
    sheet.createModifierGroup("manual.clockwork", std::move(restoredSkillGroup));
    sheet.setModifierGroupEnabled("manual.clockwork", true);

    CharacterSheetSaveData saveData = sheet.toSaveData();
    assert(saveData.formatVersion == 10);
    assert(saveData.abilities.size() == 6);
    assert(saveData.abilities[0].type == AbilityType::Strength);
    assert(saveData.abilities[0].baseValue == 16);
    assert(saveData.abilities[1].type == AbilityType::Dexterity);
    assert(saveData.abilities[1].baseValue == 9);
    assert(saveData.hitPoints.baseMax == 10);
    assert(saveData.hitPoints.damageTaken == 1);
    assert(saveData.hitPoints.temporary.pools.empty());
    assert(saveData.hitPoints.nonLethal == 0);
    assert(saveData.initiative.abilityType == AbilityType::Charisma);
    assert(saveData.savingThrows.savingThrows.size() == 3);
    assert(saveData.savingThrows.savingThrows[0].baseValue == 2);
    assert(saveData.savingThrows.savingThrows[1].abilityType == AbilityType::Charisma);
    assert(saveData.skills.skills.size() == 99);
    assert(saveData.movementGroups.groups.size() == 1);
    assert(saveData.movementGroups.groups[0].group.grants.size() == 1);
    assert(saveData.movementGroups.groups[0].group.adjustments.size() == 1);
    assert(saveData.modifierGroups.groups.size() == 2);
    assert(saveData.modifierGroups.groups[0].id == "manual.clockwork");
    assert(saveData.modifierGroups.groups[0].enabled);
    assert(saveData.contributionGroups.groups.size() == 1);
    assert(saveData.contributionGroups.groups[0].group.contributions[0].contribution.id == "user.toughness");

    const std::filesystem::path savePath = "character_sheet_test_save.json";
    sheet.save(savePath);
    CharacterSheet loadedSheet = CharacterSheet::load(savePath);
    CharacterSheetView loadedView = loadedSheet.toView();
    assert(loadedView.abilities[0].baseValue == 16);
    assert(loadedView.abilities[0].totalValue == 18);
    assert(loadedView.hitPoints.max == 15);
    assert(loadedView.hitPoints.current == 14);
    assert(loadedView.initiative.abilityType == AbilityType::Charisma);
    assert(loadedView.initiative.totalValue == 2);
    assert(loadedView.contributionGroups.groups.size() == 1);
    assert(loadedView.contributionGroups.groups[0].enabled);
    assert(loadedView.savingThrows.savingThrows[0].totalValue == 5);
    assert(loadedView.savingThrows.savingThrows[1].abilityType == AbilityType::Charisma);
    assert(loadedView.savingThrows.savingThrows[1].totalValue == 4);
    assert(loadedView.skills.skills.size() == 99);
    assert(loadedView.movement.grants.size() == 2);
    assert(loadedView.movement.grants[1].id == "spellFly");
    assert(loadedView.movement.grants[1].effectiveUnits == 6);
    const auto loadedClockwork = std::ranges::find_if(loadedView.skills.skills, [](const SkillView &skillView)
    {
        return skillView.resourceName == "skill.craft.clockwork";
    });
    assert(loadedClockwork != loadedView.skills.skills.end());
    assert(loadedClockwork->custom);
    assert(loadedClockwork->ranks == 4);
    assert(loadedClockwork->classSkill);
    assert(loadedClockwork->abilityType == AbilityType::Wisdom);
    assert(loadedClockwork->totalValue == 9);
    assert(loadedView.modifierGroups.groups[0].enabled);
    CharacterSheetSaveData loadedData = loadedSheet.toSaveData();
    assert(loadedData.modifierGroups.groups[0].group.modifiers[0].modifier.id == saveData.modifierGroups.groups[0].group.modifiers[0].modifier.id);
    std::filesystem::remove(savePath);

    sheet.removeSkillSpecialization(SkillType::Craft, "clockwork");
    sheet.setModifierGroupEnabled("manual.strength", false);
    sheet.destroyModifierGroup("manual.strength");
    sheet.destroyContributionGroup("user.hitPoints");

    assert(throwsInvalidArgument([&]
    {
        sheet.setModifierGroupEnabled("manual.strength", true);
    }));

    return 0;
}
