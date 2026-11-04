#include "golarion/equipment/item.hpp"

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

    const ItemInstance rope(ItemInstanceDefinition{
        .id = "rope.1",
        .itemDefinitionId = "hempRope15m",
        .quantity = 2
    });
    assert(rope.weightGrams() == 10000);
    assert(rope.volumeMilliliters() == 10000);
    assert(rope.matches(ItemSelector(ItemSelectorDefinition{.definitionIds = {"hempRope15m"}})));
    assert(rope.matches(ItemSelector(ItemSelectorDefinition{.tags = {"rope"}})));
    assert(!rope.matches(ItemSelector(ItemSelectorDefinition{.definitionIds = {"arrow"}})));
    assert(!rope.matches(ItemSelector(ItemSelectorDefinition{.tags = {"ammunition"}})));
    assert(rope.matchingQuantity(ItemSelector(ItemSelectorDefinition{})) == 2);
    assert(rope.matchingQuantity(ItemSelector(ItemSelectorDefinition{.tags = {"rope"}})) == 2);
    assert(rope.matchingQuantity(ItemSelector(ItemSelectorDefinition{.tags = {"ammunition"}})) == 0);
    const ItemInstance updatedRope(ItemInstanceDefinition{
        .id = "rope.1",
        .itemDefinitionId = "hempRope15m",
        .quantity = 3
    });
    assert(updatedRope.weightGrams() == 15000);
    assert(updatedRope.volumeMilliliters() == 15000);

    const ItemInstance belt(ItemInstanceDefinition{
        .id = "belt.1",
        .itemDefinitionId = "beltOfPhysicalMight2",
        .quantity = 1,
        .choices = {
            ItemChoiceSelection{
                .choiceId = "abilities",
                .optionIds = {"strength", "dexterity"}
            }
        }
    });
    assert(belt.weightGrams() == 500);

    assert(throwsInvalidArgument([]
    {
        static_cast<void>(ItemInstance(ItemInstanceDefinition{
            .id = "invalid.definition",
            .itemDefinitionId = "missing",
            .quantity = 1
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(ItemSelector(ItemSelectorDefinition{.tags = {"rope", "rope"}}));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(ItemSelector(ItemSelectorDefinition{.definitionIds = {" "}}));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(ItemInstance(ItemInstanceDefinition{
            .id = "invalid.quantity",
            .itemDefinitionId = "hempRope15m",
            .quantity = 0
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(ItemInstance(ItemInstanceDefinition{
            .id = "belt.missingChoice",
            .itemDefinitionId = "beltOfPhysicalMight2",
            .quantity = 1
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(ItemInstance(ItemInstanceDefinition{
            .id = "belt.tooFewChoices",
            .itemDefinitionId = "beltOfPhysicalMight2",
            .quantity = 1,
            .choices = {ItemChoiceSelection{.choiceId = "abilities", .optionIds = {"strength"}}}
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(ItemInstance(ItemInstanceDefinition{
            .id = "belt.duplicateChoice",
            .itemDefinitionId = "beltOfPhysicalMight2",
            .quantity = 1,
            .choices = {ItemChoiceSelection{.choiceId = "abilities", .optionIds = {"strength", "strength"}}}
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(ItemInstance(ItemInstanceDefinition{
            .id = "belt.unknownChoice",
            .itemDefinitionId = "beltOfPhysicalMight2",
            .quantity = 1,
            .choices = {ItemChoiceSelection{.choiceId = "abilities", .optionIds = {"strength", "wisdom"}}}
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(ItemInstance(ItemInstanceDefinition{
            .id = "belt.stack",
            .itemDefinitionId = "beltOfPhysicalMight2",
            .quantity = 2,
            .choices = {ItemChoiceSelection{.choiceId = "abilities", .optionIds = {"strength", "dexterity"}}}
        }));
    }));

    return 0;
}
