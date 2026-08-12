#include "golarion/character/ability.hpp"
#include "golarion/character/character_sheet.hpp"

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
    sheet.advanceTime(GameDuration::fromRounds(1));
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
    assert(view.abilities.size() == 6);
    assert(view.abilities[0].type == AbilityType::Strength);
    assert(view.abilities[0].baseValue == 16);
    assert(view.abilities[0].totalValue == 16);
    assert(view.baseAttackBonus.total == 0);
    assert(view.baseAttackBonus.contributions.contributions.empty());
    assert(view.attacks.attacks.empty());
    assert(view.combatManeuvers.baseAttackBonus == 0);
    assert(view.combatManeuvers.specialSizeModifier == 0);
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
    assert(view.movement.grants.size() == 1);
    assert(view.movement.grants[0].id == "racial");
    assert(view.movement.grants[0].effectiveUnits == 6);
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

    CharacterSheetSaveData saveData = sheet.toSaveData();
    assert(saveData.formatVersion == 14);
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
    const std::filesystem::path savePath = "character_sheet_test_save.json";
    sheet.save(savePath);
    CharacterSheet loadedSheet = CharacterSheet::load(savePath);
    CharacterSheetView loadedView = loadedSheet.toView();
    assert(loadedView.abilities[0].baseValue == 16);
    assert(loadedView.abilities[0].totalValue == 16);
    assert(loadedView.attacks.attacks.empty());
    assert(loadedView.combatManeuvers.maneuvers.size() == 10);
    assert(loadedView.combatManeuvers.maneuvers[0].bonus.abilityOptions[0].totalValue == 3);
    assert(loadedView.combatManeuvers.maneuvers[0].defense.totalValue == 12);
    assert(loadedView.hitPoints.max == 2);
    assert(loadedView.hitPoints.current == 2);
    assert(loadedView.initiative.abilityOptions.size() == 1);
    assert(loadedView.initiative.abilityOptions[0].abilityType == AbilityType::Dexterity);
    assert(loadedView.initiative.abilityOptions[0].totalValue == -1);
    assert(!loadedView.armorClass.maximumDexterityBonus.has_value());
    assert(loadedView.armorClass.abilityOptions.size() == 1);
    assert(loadedView.armorClass.abilityOptions[0].abilityType == AbilityType::Dexterity);
    assert(loadedView.armorClass.abilityOptions[0].values[0].totalValue == 9);
    assert(loadedView.savingThrows.savingThrows[0].abilityOptions[0].totalValue == 2);
    assert(loadedView.savingThrows.savingThrows[1].abilityOptions[0].abilityType == AbilityType::Dexterity);
    assert(loadedView.savingThrows.savingThrows[1].abilityOptions[0].totalValue == -1);
    assert(loadedView.skills.skills.size() == 99);
    assert(loadedView.movement.grants.size() == 1);
    assert(loadedView.movement.grants[0].id == "racial");
    assert(loadedView.movement.grants[0].effectiveUnits == 6);
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
    assert(loadedClockwork->abilityOptions[0].totalValue == 4);
    std::filesystem::remove(savePath);

    sheet.removeSkillSpecialization(SkillType::Craft, "clockwork");

    return 0;
}
