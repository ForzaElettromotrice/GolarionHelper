#include "golarion/equipment/item_definition_manager.hpp"

#include <cassert>
#include <stdexcept>
#include <variant>

namespace
{
    template<typename Exception, typename Function>
    bool throws(Function function)
    {
        try
        {
            function();
            return false;
        }
        catch (const Exception &)
        {
            return true;
        }
    }
}

int main()
{
    using namespace golarion;

    ItemDefinitionManager &manager = ItemDefinitionManager::instance();
    assert(&manager == &ItemDefinitionManager::instance());
    const ItemDefinition &rope = manager.get(" hempRope15m ");
    const ItemDefinition &sameRope = manager.get("hempRope15m");
    assert(&rope == &sameRope);
    assert(rope.id == "hempRope15m");
    assert(rope.name == "Corda di canapa (15 m)");
    assert(rope.weightGrams == 5000);
    assert(rope.volumeMilliliters == 5000);
    assert((rope.tags == std::vector<std::string>{"adventuringGear", "rope"}));
    assert(!rope.slot.has_value());
    assert(!rope.container.has_value());
    assert(rope.effects.empty());
    assert(rope.choices.empty());
    assert(displayName(ItemEffectActivation::Possessed) == "Posseduto");
    assert(displayName(ItemEffectActivation::Equipped) == "Equipaggiato");

    const ItemDefinition &backpack = manager.get("commonBackpack");
    assert(backpack.name == "Zaino comune");
    assert(backpack.weightGrams == 1000);
    assert(backpack.volumeMilliliters == 0);
    assert((backpack.tags == std::vector<std::string>{"adventuringGear", "backpack", "container"}));
    assert(backpack.container.has_value());
    assert(!backpack.container->maximumContentsWeightGrams.has_value());
    assert(backpack.container->maximumContentsVolumeMilliliters == 57000);
    assert(!backpack.container->acceptedItems.has_value());
    assert(backpack.container->quantityLimits.empty());
    assert(!backpack.container->ignoresContentsWeight);
    assert(!backpack.container->ignoresContentsVolume);
    assert(backpack.container->allowsPossessionEffects);

    const ItemDefinition &bagOfHolding = manager.get("bagOfHoldingTypeI");
    assert(bagOfHolding.name == "Borsa conservante (tipo I)");
    assert(bagOfHolding.weightGrams == 7500);
    assert(bagOfHolding.volumeMilliliters == 0);
    assert(bagOfHolding.container.has_value());
    assert(bagOfHolding.container->maximumContentsWeightGrams == 125000);
    assert(bagOfHolding.container->maximumContentsVolumeMilliliters == 810000);
    assert(bagOfHolding.container->ignoresContentsWeight);
    assert(bagOfHolding.container->ignoresContentsVolume);
    assert(bagOfHolding.container->allowsPossessionEffects);

    const ItemDefinition &cloak = manager.get("cloakOfResistance1");
    assert(cloak.name == "Mantello della Resistenza +1");
    assert(cloak.weightGrams == 500);
    assert(cloak.slot == EquipmentSlot::Shoulders);
    assert(cloak.effects.size() == 1);
    assert(cloak.effects[0].activation == ItemEffectActivation::Equipped);
    const ModifierEffectDefinition &cloakEffect = std::get<ModifierEffectDefinition>(cloak.effects[0].effect);
    assert(cloakEffect.id == "savingThrows");
    assert(cloakEffect.resource == "savingThrow.all");
    assert(cloakEffect.type == ModifierType::Bonus);
    assert(cloakEffect.bonusType == BonusType::Resistance);
    assert(cloakEffect.expression == "1");
    const ItemDefinition &ring = manager.get("ringOfProtection1");
    assert(ring.name == "Anello di Protezione +1");
    assert(ring.weightGrams == 0);
    assert(ring.slot == EquipmentSlot::Ring);
    assert(ring.effects.size() == 1);
    assert(ring.effects[0].activation == ItemEffectActivation::Equipped);
    const ModifierEffectDefinition &ringEffect = std::get<ModifierEffectDefinition>(ring.effects[0].effect);
    assert(ringEffect.resource == "armorClass.all");
    assert(ringEffect.bonusType == BonusType::Deflection);

    const ItemDefinition &belt = manager.get("beltOfPhysicalMight2");
    assert(belt.slot == EquipmentSlot::Belt);
    assert(belt.effects.empty());
    assert(belt.choices.size() == 1);
    assert(belt.choices[0].id == "abilities");
    assert(belt.choices[0].selectionCount == 2);
    assert(belt.choices[0].options.size() == 3);
    assert(belt.choices[0].options[0].id == "strength");
    const ModifierEffectDefinition &strengthEffect = std::get<ModifierEffectDefinition>(belt.choices[0].options[0].effects[0].effect);
    assert(strengthEffect.resource == "str");
    assert(strengthEffect.bonusType == BonusType::Enhancement);
    assert(strengthEffect.expression == "2");

    assert(throws<std::invalid_argument>([&manager]
    {
        static_cast<void>(manager.get("missing"));
    }));

    return 0;
}
