#include "golarion/character/ability.hpp"
#include "golarion/character/skills.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/skills_view.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <stdexcept>

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

    ResourceManager resourceManager;
    std::array abilities{
        AbilityScore(AbilityType::Strength, 10),
        AbilityScore(AbilityType::Dexterity, 14),
        AbilityScore(AbilityType::Constitution, 10),
        AbilityScore(AbilityType::Intelligence, 10),
        AbilityScore(AbilityType::Wisdom, 12),
        AbilityScore(AbilityType::Charisma, 10)
    };
    for (const AbilityScore &ability : abilities)
    {
        ability.registerResources(resourceManager);
    }

    Skills skills(resourceManager);
    skills.setRanks(SkillType::Acrobatics, 2);
    skills.setClassSkill(SkillType::Acrobatics, true);
    assert(resourceManager.modifierTotal("skill.acrobatics") == 0);

    resourceManager.addModifier("skill.all", Modifier(ModifierType::Bonus, "Tratto", "Bonus a tutte le abilità", BonusType::Racial, "1"));
    resourceManager.addModifier("skill.armorCheckPenalty", Modifier(ModifierType::Penalty, "Armatura", "Penalità di armatura", std::nullopt, "2"));
    assert(resourceManager.modifierTotal("skill.acrobatics") == -1);
    assert(resourceManager.modifierTotal("skill.perception") == 1);

    skills.setClassSkill(SkillType::Craft, true);
    skills.setSpecializationRanks(SkillType::Craft, "alchemy", 3);
    resourceManager.addModifier("skill.craft", Modifier(ModifierType::Bonus, "Laboratorio", "Bonus ad Artigianato", BonusType::Competence, "2"));
    assert(resourceManager.modifierTotal("skill.craft.alchemy") == 3);

    skills.setAbilityType(SkillType::Craft, AbilityType::Wisdom);
    skills.setSpecializationRanks(SkillType::Craft, "weapons", 1);
    assert(resourceManager.modifierTotal("skill.craft.weapons") == 3);

    assert(resourceManager.modifierTotal("skill.perform.dance") == 1);
    assert(resourceManager.modifierTotal("skill.profession.sailor") == 1);

    skills.addSpecialization(SkillType::Craft, "customClockwork", "Meccanismi Personalizzati");
    assert(resourceManager.modifierTotal("skill.craft.customClockwork") == 3);

    const SkillsView view = skills.toView();
    assert(view.skills.size() == 99);
    const auto alchemyView = std::ranges::find_if(view.skills, [](const SkillView &skillView)
    {
        return skillView.resourceName == "skill.craft.alchemy";
    });
    assert(alchemyView != view.skills.end());
    assert(!alchemyView->custom);
    assert(alchemyView->ranks == 3);
    assert(alchemyView->classSkill);
    assert(alchemyView->abilityType == AbilityType::Wisdom);
    assert(alchemyView->totalValue == 10);

    const auto customView = std::ranges::find_if(view.skills, [](const SkillView &skillView)
    {
        return skillView.resourceName == "skill.craft.customClockwork";
    });
    assert(customView != view.skills.end());
    assert(customView->custom);
    assert(customView->name == "Meccanismi Personalizzati");

    const SkillsSaveData saveData = skills.toSaveData();
    assert(saveData.skills.size() == 99);
    const auto savedAlchemy = std::ranges::find_if(saveData.skills, [](const SkillSaveData &skillData)
    {
        return skillData.specializationId == "alchemy";
    });
    assert(savedAlchemy != saveData.skills.end());
    assert(!savedAlchemy->custom);
    const auto savedCustom = std::ranges::find_if(saveData.skills, [](const SkillSaveData &skillData)
    {
        return skillData.specializationId == "customClockwork";
    });
    assert(savedCustom != saveData.skills.end());
    assert(savedCustom->custom);

    assert(throwsInvalidArgument([&]
    {
        resourceManager.targetValue("skill.craft.alchemy");
    }));

    assert(throwsInvalidArgument([&]
    {
        skills.setRanks(SkillType::Craft, 1);
    }));
    assert(throwsInvalidArgument([&]
    {
        skills.addSpecialization(SkillType::Acrobatics, "tumbling", "Capriole");
    }));
    assert(throwsInvalidArgument([&]
    {
        skills.addSpecialization(SkillType::Craft, "alchemy", "Alchimia");
    }));
    assert(throwsInvalidArgument([&]
    {
        skills.setSpecializationRanks(SkillType::Craft, "missing", 1);
    }));

    return 0;
}
