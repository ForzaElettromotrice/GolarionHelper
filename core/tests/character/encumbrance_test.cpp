#include "golarion/character/ability.hpp"
#include "golarion/character/armor_class.hpp"
#include "golarion/character/carrying_capacity.hpp"
#include "golarion/character/encumbrance.hpp"
#include "golarion/character/movement.hpp"
#include "golarion/character/skills.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/armor_class_view.hpp"
#include "golarion/view/encumbrance_view.hpp"
#include "golarion/view/movement_view.hpp"

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
        AbilityScore(AbilityType::Wisdom, 10),
        AbilityScore(AbilityType::Charisma, 10)
    };
    for (const AbilityScore &ability : abilities)
    {
        ability.registerResources(resourceManager);
    }

    ArmorClass armorClass(resourceManager);
    Skills skills(resourceManager);
    Movement movement(resourceManager);
    CarryingCapacity carryingCapacity(resourceManager);
    Encumbrance encumbrance(resourceManager, carryingCapacity);

    resourceManager.addToCollection(MovementGrantsResource, MovementGrant(MovementGrantDefinition{
        .id = "burdenedLand",
        .source = "Razza",
        .type = MovementType::Land,
        .baseSpeedExpression = "6",
        .maneuverability = std::nullopt,
        .affectedByArmor = true,
        .affectedByLoad = true,
        .supportsRunning = true
    }));
    resourceManager.addToCollection(MovementGrantsResource, MovementGrant(MovementGrantDefinition{
        .id = "steadyLand",
        .source = "Passo fermo",
        .type = MovementType::Land,
        .baseSpeedExpression = "6",
        .maneuverability = std::nullopt,
        .affectedByArmor = true,
        .affectedByLoad = false,
        .supportsRunning = true
    }));

    const auto skillView = [&skills](SkillType type) -> SkillView
    {
        const SkillsView view = skills.toView();
        const auto skill = std::ranges::find(view.skills, type, &SkillView::type);
        assert(skill != view.skills.end());
        return *skill;
    };
    const auto movementGrantView = [&movement](std::string_view id) -> MovementGrantView
    {
        const MovementView view = movement.toView();
        const auto grant = std::ranges::find(view.grants, id, &MovementGrantView::id);
        assert(grant != view.grants.end());
        return *grant;
    };

    EncumbranceView view = encumbrance.toView();
    assert(view.category == LoadCategory::Light);
    assert(view.totalWeightGrams == 0);
    assert(view.effects.maximumDexterityBonus == std::nullopt);
    assert(view.effects.armorCheckPenalty == 0);
    assert(view.effects.runMultiplierPenalty == 0);
    assert(!view.effects.preventsRunning);
    assert(!armorClass.maximumDexterityBonus().has_value());
    assert(skillView(SkillType::Acrobatics).abilityOptions[0].totalValue == 2);
    assert(movementGrantView("burdenedLand").effectiveUnits == 6);
    assert(movementGrantView("burdenedLand").run.effectiveMultiplier == 4);

    resourceManager.addToCollection(CarriedWeightsResource, CarriedWeight(CarriedWeightDefinition{
        .id = "equipment",
        .source = "Equipaggiamento",
        .grams = 17000
    }));
    view = encumbrance.toView();
    assert(view.category == LoadCategory::Medium);
    assert(view.weights.size() == 1);
    assert(view.weights[0].id == "equipment");
    assert(view.lightLoadMaxGrams == 16500);
    assert(view.mediumLoadMaxGrams == 33000);
    assert(view.heavyLoadMaxGrams == 50000);
    assert(view.effects.maximumDexterityBonus == 3);
    assert(view.effects.armorCheckPenalty == 3);
    assert(view.effects.reducesMovement);
    assert(view.effects.runMultiplierPenalty == 0);
    assert(!view.effects.preventsRunning);
    assert(armorClass.maximumDexterityBonus() == 3);
    assert(skillView(SkillType::Acrobatics).abilityOptions[0].totalValue == -1);
    assert(movementGrantView("burdenedLand").effectiveUnits == 4);
    assert(movementGrantView("steadyLand").effectiveUnits == 6);
    assert(movementGrantView("burdenedLand").run.effectiveMultiplier == 4);

    resourceManager.addToCollection(ArmorCheckPenaltiesResource, ArmorCheckPenalty(ArmorCheckPenaltyDefinition{
        .id = "wornArmor",
        .source = "Armatura indossata",
        .expression = "5"
    }));
    assert(skills.toView().armorCheckPenalty.total == 5);
    assert(skillView(SkillType::Acrobatics).abilityOptions[0].totalValue == -3);
    resourceManager.removeFromCollection(ArmorCheckPenaltiesResource, "wornArmor");
    assert(skills.toView().armorCheckPenalty.total == 3);

    resourceManager.addToCollection(CarriedWeightsResource, CarriedWeight(CarriedWeightDefinition{
        .id = "treasure",
        .source = "Tesoro",
        .grams = 20000
    }));
    view = encumbrance.toView();
    assert(view.category == LoadCategory::Heavy);
    assert(view.totalWeightGrams == 37000);
    assert(view.effects.maximumDexterityBonus == 1);
    assert(view.effects.armorCheckPenalty == 6);
    assert(view.effects.runMultiplierPenalty == 1);
    assert(!view.effects.preventsRunning);
    assert(armorClass.maximumDexterityBonus() == 1);
    assert(skillView(SkillType::Acrobatics).abilityOptions[0].totalValue == -4);
    assert(movementGrantView("burdenedLand").effectiveUnits == 4);
    assert(movementGrantView("burdenedLand").run.effectiveMultiplier == 3);
    assert(movementGrantView("steadyLand").run.effectiveMultiplier == 3);

    resourceManager.addModifier(CarryingCapacityStrengthResource, Modifier(ModifierType::Bonus, "Forza da soma", "Forza effettiva per il trasporto", BonusType::Enhancement, "8"));
    view = encumbrance.toView();
    assert(view.category == LoadCategory::Light);
    assert(view.lightLoadMaxGrams == 50000);
    assert(!armorClass.maximumDexterityBonus().has_value());
    assert(skillView(SkillType::Acrobatics).abilityOptions[0].totalValue == 2);
    assert(movementGrantView("burdenedLand").effectiveUnits == 6);
    assert(movementGrantView("burdenedLand").run.effectiveMultiplier == 4);

    resourceManager.addToCollection(CarriedWeightsResource, CarriedWeight(CarriedWeightDefinition{
        .id = "statue",
        .source = "Statua",
        .grams = 300000
    }));
    view = encumbrance.toView();
    assert(view.category == LoadCategory::Overloaded);
    assert(view.effects.losesDexterityBonusToArmorClass);
    assert(view.effects.movementSpeedLimitUnits == 1);
    assert(view.effects.runMultiplierPenalty == 0);
    assert(view.effects.preventsRunning);
    assert(!view.withinLiftFromGroundLimit);
    assert(view.withinPushOrDragLimit);
    assert(armorClass.toView().abilityBonusSuppressed);
    assert(skillView(SkillType::Acrobatics).abilityOptions[0].totalValue == -4);
    assert(movementGrantView("burdenedLand").effectiveUnits == 1);
    assert(movementGrantView("steadyLand").effectiveUnits == 6);
    assert(!movementGrantView("burdenedLand").run.usable);
    assert(!movementGrantView("steadyLand").run.usable);

    assert(throwsInvalidArgument([&]
    {
        resourceManager.addToCollection(CarriedWeightsResource, CarriedWeight(CarriedWeightDefinition{
            .id = "equipment",
            .source = "Duplicato",
            .grams = 1
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(CarriedWeight(CarriedWeightDefinition{
            .id = "invalid",
            .source = "Errore",
            .grams = -1
        }));
    }));
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(CarriedWeightsResource, "missing");
    }));

    resourceManager.removeFromCollection(CarriedWeightsResource, "statue");
    resourceManager.removeFromCollection(CarriedWeightsResource, "treasure");
    resourceManager.removeFromCollection(CarriedWeightsResource, "equipment");
    view = encumbrance.toView();
    assert(view.category == LoadCategory::Light);
    assert(!armorClass.toView().abilityBonusSuppressed);
    assert(skillView(SkillType::Acrobatics).abilityOptions[0].totalValue == 2);
    assert(movementGrantView("burdenedLand").effectiveUnits == 6);
    assert(movementGrantView("burdenedLand").run.effectiveMultiplier == 4);

    assert(displayName(LoadCategory::Light) == "Leggero");
    assert(displayName(LoadCategory::Overloaded) == "Sovraccarico");

    return 0;
}
