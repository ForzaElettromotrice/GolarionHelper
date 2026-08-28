#include "golarion/equipment/item_definition_manager.hpp"

#include <cassert>
#include <stdexcept>

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
    const ItemDefinition &ring = manager.get("ringOfProtection1");
    assert(ring.name == "Anello di Protezione +1");
    assert(ring.weightGrams == 0);
    assert(ring.slot == EquipmentSlot::Ring);

    manager.registerEffect("hempRope15m", ItemEffectDefinition{
        .id = "test.effect",
        .description = "Effetto di prova",
        .activation = ItemEffectActivation::Possessed,
        .apply = [](ResourceManager &, const ItemEffectContext &)
        {
            return [] {};
        }
    });
    assert(rope.effects.size() == 1);
    assert(rope.effects[0].id == "test.effect");
    assert(rope.effects[0].description == "Effetto di prova");
    assert(rope.effects[0].activation == ItemEffectActivation::Possessed);
    assert(throws<std::invalid_argument>([&manager]
    {
        manager.registerEffect("hempRope15m", ItemEffectDefinition{
            .id = "test.effect",
            .description = "Duplicato",
            .activation = ItemEffectActivation::Possessed,
            .apply = [](ResourceManager &, const ItemEffectContext &)
            {
                return [] {};
            }
        });
    }));
    assert(throws<std::invalid_argument>([&manager]
    {
        manager.registerEffect("hempRope15m", ItemEffectDefinition{
            .id = "empty.callback",
            .description = "Callback assente",
            .activation = ItemEffectActivation::Possessed,
            .apply = {}
        });
    }));
    assert(throws<std::invalid_argument>([&manager]
    {
        static_cast<void>(manager.get("missing"));
    }));

    return 0;
}
