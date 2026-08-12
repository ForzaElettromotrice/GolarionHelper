#include "golarion/character/ability.hpp"
#include "golarion/character/skill.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/data/skill_save_data.hpp"
#include "golarion/view/skill_view.hpp"

#include <cassert>
#include <map>
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

    assert(displayName(SkillType::Acrobatics) == "Acrobazia");
    assert(resourceName(SkillType::KnowledgeArcana) == "skill.knowledge.arcana");
    assert(defaultAbility(SkillType::Acrobatics) == AbilityType::Dexterity);
    assert(trainedOnly(SkillType::HandleAnimal));
    assert(requiresSpecialization(SkillType::Craft));
    assert(appliesArmorCheckPenalty(SkillType::DisableDevice));
    assert(!appliesArmorCheckPenalty(SkillType::Perception));

    ResourceManager manager;
    AbilityScore strength(AbilityType::Strength, 10);
    AbilityScore dexterity(AbilityType::Dexterity, 14);
    AbilityScore intelligence(AbilityType::Intelligence, 10);
    strength.registerResources(manager);
    dexterity.registerResources(manager);
    intelligence.registerResources(manager);

    Skill acrobatics(SkillType::Acrobatics);
    acrobatics.registerResources(manager);
    assert(acrobatics.usable());
    assert(acrobatics.totalValue(manager) == 2);
    assert(throwsInvalidArgument([&]
    {
        manager.targetValue("skill.acrobatics");
    }));

    acrobatics.setRanks(2);
    manager.addModifier("skill.acrobatics", Modifier(ModifierType::Bonus, "Talento", "Bonus ad Acrobazia", BonusType::Competence, "2"));
    assert(acrobatics.totalValue(manager) == 6);

    std::map<std::string, SkillAbilityReplacement> abilityReplacements;
    abilityReplacements.emplace("muscleMemory", SkillAbilityReplacement(SkillAbilityReplacementDefinition{
        .id = "muscleMemory",
        .source = "Memoria muscolare",
        .targetResourceName = "skill.acrobatics",
        .abilityType = AbilityType::Strength
    }));
    std::map<std::string, SkillClassSkillGrant> classSkillGrants;
    classSkillGrants.emplace("rogueClassSkill", SkillClassSkillGrant(SkillClassSkillGrantDefinition{
        .id = "rogueClassSkill",
        .source = "Ladro",
        .targetResourceName = "skill.acrobatics"
    }));
    assert(acrobatics.totalValue(manager) == 6);
    const SkillView acrobaticsView = acrobatics.toView(manager, abilityReplacements, classSkillGrants);
    assert(acrobaticsView.type == SkillType::Acrobatics);
    assert(!acrobaticsView.specializationId);
    assert(acrobaticsView.name == "Acrobazia");
    assert(acrobaticsView.resourceName == "skill.acrobatics");
    assert(acrobaticsView.ranks == 2);
    assert(acrobaticsView.classSkill);
    assert(acrobaticsView.classSkillBonus == 3);
    assert(acrobaticsView.classSkillGrants.size() == 1);
    assert(acrobaticsView.classSkillGrants[0].source == "Ladro");
    assert(acrobaticsView.classSkillGrants[0].targetResourceName == "skill.acrobatics");
    assert(acrobaticsView.abilityOptions.size() == 2);
    assert(!acrobaticsView.abilityOptions[0].replacementId.has_value());
    assert(acrobaticsView.abilityOptions[0].abilityType == AbilityType::Dexterity);
    assert(acrobaticsView.abilityOptions[0].abilityModifier == 2);
    assert(acrobaticsView.abilityOptions[0].totalValue == 9);
    assert(acrobaticsView.abilityOptions[1].replacementId == "muscleMemory");
    assert(acrobaticsView.abilityOptions[1].abilityType == AbilityType::Strength);
    assert(acrobaticsView.abilityOptions[1].abilityModifier == 0);
    assert(acrobaticsView.abilityOptions[1].totalValue == 7);
    assert(!acrobaticsView.trainedOnly);
    assert(acrobaticsView.usable);
    assert(!acrobaticsView.custom);
    assert(acrobaticsView.modifiers.total == 2);
    const SkillSaveData acrobaticsData = acrobatics.toSaveData();
    assert(acrobaticsData.type == SkillType::Acrobatics);
    assert(!acrobaticsData.specializationId);
    assert(acrobaticsData.ranks == 2);
    assert(!acrobaticsData.custom);

    Skill handleAnimal(SkillType::HandleAnimal);
    assert(!handleAnimal.usable());
    handleAnimal.setRanks(1);
    assert(handleAnimal.usable());

    Skill craft(SkillType::Craft, "alchemy", "Alchimia");
    craft.registerResources(manager);
    craft.setRanks(1);
    assert(craft.totalValue(manager) == 1);
    const SkillView craftView = craft.toView(manager);
    assert(craftView.specializationId == "alchemy");
    assert(craftView.name == "Alchimia");
    assert(craftView.custom);
    const SkillSaveData craftData = craft.toSaveData();
    assert(craftData.specializationId == "alchemy");
    assert(craftData.specialization == "Alchimia");
    assert(craftData.custom);

    assert(throwsInvalidArgument([]
    {
        Skill genericCraft(SkillType::Craft);
    }));
    assert(throwsInvalidArgument([]
    {
        Skill specializedAcrobatics(SkillType::Acrobatics, "tumbling", "Capriole");
    }));
    assert(throwsInvalidArgument([]
    {
        Skill invalidId(SkillType::Craft, "bad-id", "Alchimia");
    }));
    assert(throwsInvalidArgument([&]
    {
        acrobatics.setRanks(-1);
    }));

    return 0;
}
