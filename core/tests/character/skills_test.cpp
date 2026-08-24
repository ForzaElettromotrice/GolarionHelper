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
    resourceManager.addToCollection(SkillClassSkillGrantsResource, SkillClassSkillGrant(SkillClassSkillGrantDefinition{
        .id = "rogueAcrobatics",
        .source = "Ladro",
        .targetResourceName = "skill.acrobatics"
    }));
    assert(resourceManager.modifierTotal("skill.acrobatics") == 0);

    resourceManager.addModifier("skill.all", Modifier(ModifierType::Bonus, "Tratto", "Bonus a tutte le abilità", BonusType::Racial, "1"));
    resourceManager.addToCollection(ArmorCheckPenaltiesResource, ArmorCheckPenalty(ArmorCheckPenaltyDefinition{
        .id = "armor",
        .source = "Armatura",
        .expression = "2"
    }));
    assert(resourceManager.modifierTotal("skill.acrobatics") == -1);
    assert(resourceManager.modifierTotal("skill.perception") == 1);

    resourceManager.addToCollection(SkillClassSkillGrantsResource, SkillClassSkillGrant(SkillClassSkillGrantDefinition{
        .id = "artisanCraft",
        .source = "Artigiano",
        .targetResourceName = "skill.craft"
    }));
    skills.setSpecializationRanks(SkillType::Craft, "alchemy", 3);
    resourceManager.addModifier("skill.craft", Modifier(ModifierType::Bonus, "Laboratorio", "Bonus ad Artigianato", BonusType::Competence, "2"));
    assert(resourceManager.modifierTotal("skill.craft.alchemy") == 3);

    resourceManager.addToCollection(SkillAbilityReplacementsResource, SkillAbilityReplacement(SkillAbilityReplacementDefinition{
        .id = "inspiredCraft",
        .source = "Artigiano ispirato",
        .targetResourceName = "skill.craft",
        .abilityType = AbilityType::Wisdom
    }));
    skills.setSpecializationRanks(SkillType::Craft, "weapons", 1);
    assert(resourceManager.modifierTotal("skill.craft.weapons") == 3);

    assert(resourceManager.modifierTotal("skill.perform.dance") == 1);
    assert(resourceManager.modifierTotal("skill.profession.sailor") == 1);

    skills.addSpecialization(SkillType::Craft, "customClockwork", "Meccanismi Personalizzati");
    assert(resourceManager.modifierTotal("skill.craft.customClockwork") == 3);

    const ResourceManagerView resourceView = resourceManager.toView();
    const auto enhanceableResource = [&resourceView](std::string_view name) -> const ResourceManagerView::EnhanceableResourceView &
    {
        const auto entry = std::ranges::find(resourceView.enhanceableResources, name, &ResourceManagerView::EnhanceableResourceView::name);
        if (entry == resourceView.enhanceableResources.end())
        {
            throw std::invalid_argument("enhanceable resource is missing");
        }
        return *entry;
    };
    assert(enhanceableResource("skill.all").parentResources.empty());
    assert(enhanceableResource("skill.craft").parentResources == std::vector<std::string>{"skill.all"});
    assert(enhanceableResource("skill.craft.alchemy").parentResources == std::vector<std::string>{"skill.craft"});
    assert(enhanceableResource("skill.craft.customClockwork").parentResources == std::vector<std::string>{"skill.craft"});
    assert(enhanceableResource("skill.knowledge").parentResources == std::vector<std::string>{"skill.all"});
    assert(enhanceableResource("skill.knowledge.arcana").parentResources == std::vector<std::string>{"skill.knowledge"});
    assert(enhanceableResource("skill.armorCheckPenalty").parentResources == std::vector<std::string>{"skill.all"});
    assert(enhanceableResource("skill.acrobatics").parentResources == std::vector<std::string>{"skill.armorCheckPenalty"});
    assert(enhanceableResource("skill.perception").parentResources == std::vector<std::string>{"skill.all"});
    assert(enhanceableResource("skillCheck.str").parentResources.empty());
    assert(enhanceableResource("skillCheck.dex").parentResources.empty());
    assert(enhanceableResource("skillCheck.con").parentResources.empty());
    assert(enhanceableResource("skillCheck.int").parentResources.empty());
    assert(enhanceableResource("skillCheck.wis").parentResources.empty());
    assert(enhanceableResource("skillCheck.cha").parentResources.empty());

    const SkillsView view = skills.toView();
    assert(view.armorCheckPenalty.total == 2);
    assert(view.armorCheckPenalty.sources.size() == 1);
    assert(view.armorCheckPenalty.sources[0].id == "armor");
    assert(view.armorCheckPenalty.sources[0].constraining);
    assert(view.skills.size() == 99);
    const auto alchemyView = std::ranges::find_if(view.skills, [](const SkillView &skillView)
    {
        return skillView.resourceName == "skill.craft.alchemy";
    });
    assert(alchemyView != view.skills.end());
    assert(!alchemyView->custom);
    assert(alchemyView->ranks == 3);
    assert(alchemyView->classSkill);
    assert(alchemyView->classSkillGrants.size() == 1);
    assert(alchemyView->classSkillGrants[0].id == "artisanCraft");
    assert(alchemyView->classSkillGrants[0].targetResourceName == "skill.craft");
    assert(alchemyView->abilityOptions.size() == 2);
    assert(alchemyView->abilityOptions[0].abilityType == AbilityType::Intelligence);
    assert(alchemyView->abilityOptions[0].totalValue == 9);
    assert(alchemyView->abilityOptions[1].replacementId == "inspiredCraft");
    assert(alchemyView->abilityOptions[1].source == "Artigiano ispirato");
    assert(alchemyView->abilityOptions[1].abilityType == AbilityType::Wisdom);
    assert(alchemyView->abilityOptions[1].totalValue == 10);

    const auto customView = std::ranges::find_if(view.skills, [](const SkillView &skillView)
    {
        return skillView.resourceName == "skill.craft.customClockwork";
    });
    assert(customView != view.skills.end());
    assert(customView->custom);
    assert(customView->name == "Meccanismi Personalizzati");
    assert(customView->classSkill);
    assert(customView->classSkillGrants.size() == 1);
    assert(customView->classSkillGrants[0].id == "artisanCraft");
    assert(customView->abilityOptions.size() == 2);
    assert(customView->abilityOptions[1].replacementId == "inspiredCraft");

    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.addToCollection(SkillAbilityReplacementsResource, SkillAbilityReplacement(SkillAbilityReplacementDefinition{
            .id = "inspiredCraft",
            .source = "Duplicato",
            .targetResourceName = "skill.craft",
            .abilityType = AbilityType::Charisma
        }));
    }));
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.addToCollection(SkillClassSkillGrantsResource, SkillClassSkillGrant(SkillClassSkillGrantDefinition{
            .id = "artisanCraft",
            .source = "Duplicato",
            .targetResourceName = "skill.craft"
        }));
    }));
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.addToCollection(SkillClassSkillGrantsResource, SkillClassSkillGrant(SkillClassSkillGrantDefinition{
            .id = "invalidTarget",
            .source = "Target non valido",
            .targetResourceName = "str"
        }));
    }));
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.addToCollection(SkillAbilityReplacementsResource, SkillAbilityReplacement(SkillAbilityReplacementDefinition{
            .id = "invalidTarget",
            .source = "Target non valido",
            .targetResourceName = "str",
            .abilityType = AbilityType::Charisma
        }));
    }));

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

    resourceManager.removeFromCollection(SkillAbilityReplacementsResource, "inspiredCraft");
    const SkillsView viewWithoutReplacement = skills.toView();
    const auto alchemyWithoutReplacement = std::ranges::find_if(viewWithoutReplacement.skills, [](const SkillView &skillView)
    {
        return skillView.resourceName == "skill.craft.alchemy";
    });
    assert(alchemyWithoutReplacement != viewWithoutReplacement.skills.end());
    assert(alchemyWithoutReplacement->abilityOptions.size() == 1);
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.removeFromCollection(SkillAbilityReplacementsResource, "inspiredCraft");
    }));

    resourceManager.removeFromCollection(SkillClassSkillGrantsResource, "artisanCraft");
    const SkillsView viewWithoutClassSkillGrant = skills.toView();
    const auto alchemyWithoutClassSkillGrant = std::ranges::find_if(viewWithoutClassSkillGrant.skills, [](const SkillView &skillView)
    {
        return skillView.resourceName == "skill.craft.alchemy";
    });
    assert(alchemyWithoutClassSkillGrant != viewWithoutClassSkillGrant.skills.end());
    assert(!alchemyWithoutClassSkillGrant->classSkill);
    assert(alchemyWithoutClassSkillGrant->classSkillGrants.empty());
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.removeFromCollection(SkillClassSkillGrantsResource, "artisanCraft");
    }));

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
