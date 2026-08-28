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
    for (AbilityType abilityType : {AbilityType::Strength, AbilityType::Dexterity, AbilityType::Constitution, AbilityType::Intelligence, AbilityType::Wisdom, AbilityType::Charisma})
    {
        manager.registerEnhanceableResource(skillCheckResourceName(abilityType));
    }
    assert(skillCheckResourceName(AbilityType::Strength) == "skillCheck.str");
    assert(skillCheckResourceName(AbilityType::Dexterity) == "skillCheck.dex");

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
    manager.addModifier(skillCheckResourceName(AbilityType::Dexterity), Modifier(ModifierType::Bonus, "Agilità", "Bonus alle prove basate su Destrezza", BonusType::Competence, "4"));
    manager.addModifier(skillCheckResourceName(AbilityType::Dexterity), Modifier(ModifierType::Penalty, "Accecato", "Penalità alle prove basate su Destrezza", std::nullopt, "4", "Quando la cecità ostacola la prova"));
    manager.addModifier(skillCheckResourceName(AbilityType::Strength), Modifier(ModifierType::Penalty, "Accecato", "Penalità alle prove basate su Forza", std::nullopt, "4", "Quando la cecità ostacola la prova"));
    assert(acrobatics.totalValue(manager) == 8);

    std::map<std::string, SkillAbilityReplacement> abilityReplacements;
    abilityReplacements.emplace("muscleMemory", SkillAbilityReplacement(SkillAbilityReplacementDefinition{
        .id = "muscleMemory",
        .source = "Memoria muscolare",
        .targetResourceName = "skill.acrobatics",
        .abilityType = AbilityType::Strength
    }));
    abilityReplacements.emplace("analyticalMovement", SkillAbilityReplacement(SkillAbilityReplacementDefinition{
        .id = "analyticalMovement",
        .source = "Movimento analitico",
        .targetResourceName = "skill.acrobatics",
        .abilityType = AbilityType::Intelligence
    }));
    std::map<std::string, SkillClassSkillGrant> classSkillGrants;
    classSkillGrants.emplace("rogueClassSkill", SkillClassSkillGrant(SkillClassSkillGrantDefinition{
        .id = "rogueClassSkill",
        .source = "Ladro",
        .targetResourceName = "skill.acrobatics"
    }));
    assert(acrobatics.totalValue(manager, 3) == 5);
    const SkillView acrobaticsView = acrobatics.toView(manager, abilityReplacements, classSkillGrants, 3);
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
    assert(acrobaticsView.abilityOptions.size() == 3);
    assert(!acrobaticsView.abilityOptions[0].replacementId.has_value());
    assert(acrobaticsView.abilityOptions[0].abilityType == AbilityType::Dexterity);
    assert(acrobaticsView.abilityOptions[0].abilityModifier == 2);
    assert(acrobaticsView.abilityOptions[0].totalValue == 8);
    assert(acrobaticsView.abilityOptions[0].modifiers.permanentTotal == 4);
    assert(acrobaticsView.abilityOptions[0].modifiers.conditionalTotals.size() == 1);
    assert(acrobaticsView.abilityOptions[0].modifiers.conditionalTotals[0].value == -4);
    assert(acrobaticsView.abilityOptions[1].replacementId == "analyticalMovement");
    assert(acrobaticsView.abilityOptions[1].abilityType == AbilityType::Intelligence);
    assert(acrobaticsView.abilityOptions[1].abilityModifier == 0);
    assert(acrobaticsView.abilityOptions[1].totalValue == 4);
    assert(acrobaticsView.abilityOptions[1].modifiers.conditionalTotals.empty());
    assert(acrobaticsView.abilityOptions[2].replacementId == "muscleMemory");
    assert(acrobaticsView.abilityOptions[2].abilityType == AbilityType::Strength);
    assert(acrobaticsView.abilityOptions[2].abilityModifier == 0);
    assert(acrobaticsView.abilityOptions[2].totalValue == 4);
    assert(acrobaticsView.abilityOptions[2].modifiers.conditionalTotals.size() == 1);
    assert(acrobaticsView.abilityOptions[2].modifiers.conditionalTotals[0].value == -4);
    assert(!acrobaticsView.trainedOnly);
    assert(acrobaticsView.usable);
    assert(!acrobaticsView.custom);
    assert(acrobaticsView.appliedArmorCheckPenalty == 3);
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
    assert(craft.totalValue(manager, 5) == 1);
    const SkillView craftView = craft.toView(manager);
    assert(craftView.specializationId == "alchemy");
    assert(craftView.name == "Alchimia");
    assert(craftView.custom);
    assert(craftView.appliedArmorCheckPenalty == 0);
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
    assert(throwsInvalidArgument([&]
    {
        acrobatics.totalValue(manager, -1);
    }));

    return 0;
}
