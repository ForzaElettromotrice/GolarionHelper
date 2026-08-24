#include "golarion/character/ability.hpp"
#include "golarion/data/ability_save_data.hpp"
#include "golarion/view/ability_view.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <algorithm>
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

    ResourceManager manager;
    AbilityChecks abilityChecks(manager);
    AbilityScore strength(AbilityType::Strength);
    strength.registerResources(manager);
    manager.addModifier("str", Modifier(ModifierType::Bonus, "Cintura", "Bonus alla Forza", BonusType::Enhancement, "4"));
    manager.addModifier(AbilityCheckRootResource, Modifier(ModifierType::Penalty, "Infermo", "Penalità alle prove di caratteristica", std::nullopt, "2"));

    assert(strength.baseValue() == 10);
    assert(strength.totalValue(manager) == 14);
    assert(strength.modifier(manager) == 2);
    assert(manager.targetValue("str") == 14);
    assert(manager.targetValue("strMod") == 2);

    AbilityView view = strength.toView(manager);
    assert(view.type == AbilityType::Strength);
    assert(view.baseValue == 10);
    assert(view.totalValue == 14);
    assert(view.modifier == 2);
    assert(view.modifiers.total == 4);
    assert(view.modifiers.modifiers.size() == 1);
    assert(view.checkTotal == 0);
    assert(view.checkModifiers.total == -2);
    assert(view.effectiveBaseValue == 10);
    assert(view.modifiedValue == 14);
    assert(view.replacements.empty());
    assert(manager.enhanceableResourceIsOrInheritsFrom(abilityCheckResourceName(AbilityType::Strength), AbilityCheckRootResource));

    manager.registerTarget("eidolonStr", []
    {
        return 18;
    });
    manager.addToCollection(abilityReplacementsResourceName(AbilityType::Strength), AbilityReplacement(AbilityReplacementDefinition{
        .id = "synthesist.strength",
        .source = "Eidolon fuso",
        .expression = "@eidolonStr",
        .stage = AbilityReplacementStage::Base,
        .requirements = {}
    }));
    manager.addToCollection(abilityReplacementsResourceName(AbilityType::Strength), AbilityReplacement(AbilityReplacementDefinition{
        .id = "synthesist.strength.sameValue",
        .source = "Seconda fonte compatibile",
        .expression = "18",
        .stage = AbilityReplacementStage::Base,
        .requirements = {}
    }));
    manager.addToCollection(abilityReplacementsResourceName(AbilityType::Strength), AbilityReplacement(AbilityReplacementDefinition{
        .id = "inactive.strength",
        .source = "Fonte inattiva",
        .expression = "20",
        .stage = AbilityReplacementStage::Base,
        .requirements = {Requirement("0", "La fonte non è attiva")}
    }));
    view = strength.toView(manager);
    assert(view.baseValue == 10);
    assert(view.effectiveBaseValue == 18);
    assert(view.modifiedValue == 22);
    assert(view.totalValue == 22);
    assert(view.modifier == 6);
    assert(view.replacements.size() == 3);
    const auto inactiveReplacement = std::ranges::find(view.replacements, "inactive.strength", &AbilityReplacementView::id);
    assert(inactiveReplacement != view.replacements.end());
    assert(!inactiveReplacement->active);
    assert(!inactiveReplacement->applied);
    assert(manager.targetValue("str") == 22);
    assert(manager.targetValue("strMod") == 6);

    manager.addToCollection(abilityReplacementsResourceName(AbilityType::Strength), AbilityReplacement(AbilityReplacementDefinition{
        .id = "helpless.strength",
        .source = "Valore finale imposto",
        .expression = "0",
        .stage = AbilityReplacementStage::Final,
        .requirements = {}
    }));
    manager.addToCollection(abilityReplacementsResourceName(AbilityType::Strength), AbilityReplacement(AbilityReplacementDefinition{
        .id = "higherFinal.strength",
        .source = "Valore finale superiore",
        .expression = "5",
        .stage = AbilityReplacementStage::Final,
        .requirements = {}
    }));
    view = strength.toView(manager);
    assert(view.effectiveBaseValue == 18);
    assert(view.modifiedValue == 22);
    assert(view.totalValue == 0);
    assert(view.modifier == -5);
    const auto lowerFinalReplacement = std::ranges::find(view.replacements, "helpless.strength", &AbilityReplacementView::id);
    const auto higherFinalReplacement = std::ranges::find(view.replacements, "higherFinal.strength", &AbilityReplacementView::id);
    assert(lowerFinalReplacement != view.replacements.end());
    assert(higherFinalReplacement != view.replacements.end());
    assert(lowerFinalReplacement->active);
    assert(lowerFinalReplacement->applied);
    assert(higherFinalReplacement->active);
    assert(!higherFinalReplacement->applied);
    assert(manager.targetValue("str") == 0);
    assert(manager.targetValue("strMod") == -5);
    manager.removeFromCollection(abilityReplacementsResourceName(AbilityType::Strength), "higherFinal.strength");
    manager.removeFromCollection(abilityReplacementsResourceName(AbilityType::Strength), "helpless.strength");

    manager.addToCollection(abilityReplacementsResourceName(AbilityType::Strength), AbilityReplacement(AbilityReplacementDefinition{
        .id = "higherBase.strength",
        .source = "Valore base superiore",
        .expression = "20",
        .stage = AbilityReplacementStage::Base,
        .requirements = {}
    }));
    view = strength.toView(manager);
    assert(view.effectiveBaseValue == 20);
    assert(view.modifiedValue == 24);
    assert(view.totalValue == 24);
    const auto higherBaseReplacement = std::ranges::find(view.replacements, "higherBase.strength", &AbilityReplacementView::id);
    const auto lowerBaseReplacement = std::ranges::find(view.replacements, "synthesist.strength", &AbilityReplacementView::id);
    assert(higherBaseReplacement != view.replacements.end());
    assert(lowerBaseReplacement != view.replacements.end());
    assert(higherBaseReplacement->active);
    assert(higherBaseReplacement->applied);
    assert(lowerBaseReplacement->active);
    assert(!lowerBaseReplacement->applied);
    assert(manager.targetValue("str") == 24);
    manager.removeFromCollection(abilityReplacementsResourceName(AbilityType::Strength), "higherBase.strength");
    assert(strength.totalValue(manager) == 22);
    manager.removeFromCollection(abilityReplacementsResourceName(AbilityType::Strength), "inactive.strength");
    manager.removeFromCollection(abilityReplacementsResourceName(AbilityType::Strength), "synthesist.strength.sameValue");
    manager.removeFromCollection(abilityReplacementsResourceName(AbilityType::Strength), "synthesist.strength");
    assert(strength.totalValue(manager) == 14);

    AbilitySaveData saveData = strength.toSaveData();
    assert(saveData.type == AbilityType::Strength);
    assert(saveData.baseValue == 10);

    AbilityScore lowStrength(AbilityType::Strength, 9);
    ResourceManager lowStrengthManager;
    AbilityChecks lowStrengthAbilityChecks(lowStrengthManager);
    lowStrength.registerResources(lowStrengthManager);
    assert(lowStrength.modifier(lowStrengthManager) == -1);

    assert(throwsInvalidArgument([]
    {
        AbilityScore invalid(AbilityType::Strength, 0);
    }));

    assert(displayName(AbilityType::Strength) == "Forza");
    assert(displayName(AbilityReplacementStage::Base) == "Valore base");
    assert(displayName(AbilityReplacementStage::Final) == "Valore finale");
    assert(resourceName(AbilityType::Charisma) == "cha");
    assert(abilityReplacementsResourceName(AbilityType::Dexterity) == "ability.dex.replacements");

    return 0;
}
