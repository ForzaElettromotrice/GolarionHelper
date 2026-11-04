#include "golarion/character/ability.hpp"
#include "golarion/character/carrying_capacity.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/carrying_capacity_view.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <string_view>

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
    AbilityScore strength(AbilityType::Strength, 10);
    strength.registerResources(resourceManager);
    CarryingCapacity carryingCapacity(resourceManager);

    CarryingCapacityView view = carryingCapacity.toView();
    assert(view.strength == 10);
    assert(view.effectiveStrength == 10);
    assert(view.bodyType == CarryingBodyType::Biped);
    assert(!view.bodyTypeBase.has_value());
    assert(view.size == SizeCategory::Medium);
    assert(view.baseHeavyLoadMaxGrams == 50000);
    assert(view.combinedMultiplierNumerator == 1);
    assert(view.combinedMultiplierDenominator == 1);
    assert(view.multipliers.empty());
    assert(view.lightLoadMaxGrams == 16500);
    assert(view.mediumLoadMaxGrams == 33000);
    assert(view.heavyLoadMaxGrams == 50000);
    assert(view.liftFromGroundMaxGrams == 100000);
    assert(view.pushOrDragMaxGrams == 250000);

    resourceManager.addToCollection(CarryingCapacityMultipliersResource, CarryingCapacityMultiplier(CarryingCapacityMultiplierDefinition{
        .id = "spell.antHaul.first",
        .source = "Carico della Formica",
        .stackingGroup = "antHaul",
        .numerator = 3,
        .denominator = 1
    }));
    resourceManager.addToCollection(CarryingCapacityMultipliersResource, CarryingCapacityMultiplier(CarryingCapacityMultiplierDefinition{
        .id = "spell.antHaul.second",
        .source = "Carico della Formica",
        .stackingGroup = "antHaul",
        .numerator = 3,
        .denominator = 1
    }));
    resourceManager.addToCollection(CarryingCapacityMultipliersResource, CarryingCapacityMultiplier(CarryingCapacityMultiplierDefinition{
        .id = "effect.lesserHaul",
        .source = "Carico minore",
        .stackingGroup = "antHaul",
        .numerator = 2,
        .denominator = 1
    }));
    resourceManager.addToCollection(CarryingCapacityMultipliersResource, CarryingCapacityMultiplier(CarryingCapacityMultiplierDefinition{
        .id = "item.horseshoes",
        .source = "Ferri del Grosso Carico",
        .stackingGroup = "horseshoesGreatBurden",
        .numerator = 2,
        .denominator = 1
    }));
    resourceManager.addToCollection(CarryingCapacityMultipliersResource, CarryingCapacityMultiplier(CarryingCapacityMultiplierDefinition{
        .id = "armor.burdenless",
        .source = "Armatura Senza Peso",
        .stackingGroup = "burdenless",
        .numerator = 6,
        .denominator = 4
    }));
    view = carryingCapacity.toView();
    assert(view.baseHeavyLoadMaxGrams == 50000);
    assert(view.combinedMultiplierNumerator == 9);
    assert(view.combinedMultiplierDenominator == 1);
    assert(view.heavyLoadMaxGrams == 450000);
    assert(view.liftFromGroundMaxGrams == 900000);
    assert(view.pushOrDragMaxGrams == 2250000);
    assert(view.multipliers.size() == 5);
    assert(std::ranges::count(view.multipliers, true, &CarryingCapacityMultiplierView::applied) == 3);
    const auto secondAntHaul = std::ranges::find(view.multipliers, "spell.antHaul.second", &CarryingCapacityMultiplierView::id);
    const auto lesserHaul = std::ranges::find(view.multipliers, "effect.lesserHaul", &CarryingCapacityMultiplierView::id);
    const auto burdenless = std::ranges::find(view.multipliers, "armor.burdenless", &CarryingCapacityMultiplierView::id);
    assert(secondAntHaul != view.multipliers.end());
    assert(!secondAntHaul->applied);
    assert(secondAntHaul->notAppliedReason == "Un'altra istanza dello stesso effetto è già applicata");
    assert(lesserHaul != view.multipliers.end());
    assert(!lesserHaul->applied);
    assert(lesserHaul->notAppliedReason == "Superato da un moltiplicatore più alto dello stesso effetto");
    assert(burdenless != view.multipliers.end());
    assert(burdenless->numerator == 3);
    assert(burdenless->denominator == 2);

    assert(throwsInvalidArgument([&]
    {
        resourceManager.addToCollection(CarryingCapacityMultipliersResource, CarryingCapacityMultiplier(CarryingCapacityMultiplierDefinition{
            .id = "spell.antHaul.first",
            .source = "Duplicato",
            .stackingGroup = "duplicate",
            .numerator = 2,
            .denominator = 1
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(CarryingCapacityMultiplier(CarryingCapacityMultiplierDefinition{
            .id = "invalid",
            .source = "Errore",
            .stackingGroup = "invalid",
            .numerator = 1,
            .denominator = 0
        }));
    }));

    resourceManager.removeFromCollection(CarryingCapacityMultipliersResource, "spell.antHaul.first");
    resourceManager.removeFromCollection(CarryingCapacityMultipliersResource, "spell.antHaul.second");
    resourceManager.removeFromCollection(CarryingCapacityMultipliersResource, "effect.lesserHaul");
    resourceManager.removeFromCollection(CarryingCapacityMultipliersResource, "item.horseshoes");
    resourceManager.removeFromCollection(CarryingCapacityMultipliersResource, "armor.burdenless");
    assert(carryingCapacity.toView().multipliers.empty());
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(CarryingCapacityMultipliersResource, "missing");
    }));

    strength.setBaseValue(1);
    view = carryingCapacity.toView();
    assert(view.lightLoadMaxGrams == 1500);
    assert(view.mediumLoadMaxGrams == 3000);
    assert(view.heavyLoadMaxGrams == 5000);
    resourceManager.addToCollection(CarryingCapacitySizeResource, CarryingCapacitySize(CarryingCapacitySizeDefinition{
        .id = "size.fine",
        .source = "Taglia",
        .category = SizeCategory::Fine
    }));
    assert(carryingCapacity.toView().lightLoadMaxGrams == 187);
    resourceManager.removeFromCollection(CarryingCapacitySizeResource, "size.fine");

    strength.setBaseValue(29);
    assert(carryingCapacity.toView().heavyLoadMaxGrams == 700000);
    strength.setBaseValue(30);
    assert(carryingCapacity.toView().heavyLoadMaxGrams == 800000);
    strength.setBaseValue(39);
    assert(carryingCapacity.toView().heavyLoadMaxGrams == 2800000);
    strength.setBaseValue(40);
    assert(carryingCapacity.toView().heavyLoadMaxGrams == 3200000);

    strength.setBaseValue(10);
    resourceManager.addModifier(CarryingCapacityStrengthResource, Modifier(ModifierType::Bonus, "Cintura da soma", "Forza effettiva per il trasporto", BonusType::Enhancement, "8"));
    resourceManager.addModifier(CarryingCapacityStrengthResource, Modifier(ModifierType::Bonus, "Potenziamento minore", "Forza effettiva per il trasporto", BonusType::Enhancement, "4"));
    view = carryingCapacity.toView();
    assert(view.strength == 10);
    assert(view.strengthModifiers.total == 8);
    assert(view.effectiveStrength == 18);
    assert(view.heavyLoadMaxGrams == 150000);

    resourceManager.addToCollection(CarryingCapacitySizeResource, CarryingCapacitySize(CarryingCapacitySizeDefinition{
        .id = "size.current",
        .source = "Taglia",
        .category = SizeCategory::Small
    }));
    view = carryingCapacity.toView();
    assert(view.size == SizeCategory::Small);
    assert(view.lightLoadMaxGrams == 37500);
    assert(view.mediumLoadMaxGrams == 75000);
    assert(view.heavyLoadMaxGrams == 112500);

    resourceManager.addToCollection(CarryingCapacityBodyTypeResource, CarryingBodyTypeBase(CarryingBodyTypeBaseDefinition{
        .id = "ancestry.horse",
        .source = "Cavallo",
        .type = CarryingBodyType::Quadruped
    }));
    view = carryingCapacity.toView();
    assert(view.bodyType == CarryingBodyType::Quadruped);
    assert(view.bodyTypeBase.has_value());
    assert(view.bodyTypeBase->id == "ancestry.horse");
    assert(view.bodyTypeBase->source == "Cavallo");
    assert(view.heavyLoadMaxGrams == 150000);

    assert(throwsInvalidArgument([&]
    {
        resourceManager.addToCollection(CarryingCapacityBodyTypeResource, CarryingBodyTypeBase(CarryingBodyTypeBaseDefinition{
            .id = "ancestry.other",
            .source = "Altra razza",
            .type = CarryingBodyType::Quadruped
        }));
    }));
    assert(throwsInvalidArgument([&]
    {
        resourceManager.addToCollection(CarryingCapacitySizeResource, CarryingCapacitySize(CarryingCapacitySizeDefinition{
            .id = "size.other",
            .source = "Altra taglia",
            .category = SizeCategory::Large
        }));
    }));

    resourceManager.removeFromCollection(CarryingCapacityBodyTypeResource, "ancestry.horse");
    assert(carryingCapacity.toView().bodyType == CarryingBodyType::Biped);
    resourceManager.removeFromCollection(CarryingCapacitySizeResource, "size.current");
    assert(carryingCapacity.toView().size == SizeCategory::Medium);

    resourceManager.addModifier(CarryingCapacityStrengthResource, Modifier(ModifierType::Penalty, "Debolezza", "Penalità alla Forza effettiva per il trasporto", std::nullopt, "30"));
    view = carryingCapacity.toView();
    assert(view.effectiveStrength == 0);
    assert(view.lightLoadMaxGrams == 0);
    assert(view.mediumLoadMaxGrams == 0);
    assert(view.heavyLoadMaxGrams == 0);

    assert(displayName(CarryingBodyType::Biped) == std::string_view("Bipede"));
    assert(displayName(CarryingBodyType::Quadruped) == std::string_view("Quadrupede"));

    return 0;
}
