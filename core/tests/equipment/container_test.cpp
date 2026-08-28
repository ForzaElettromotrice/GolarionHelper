#include "golarion/equipment/container.hpp"
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

    static_cast<void>(Container(ContainerDefinition{
        .id = "worn",
        .name = "Indossati",
        .ownerItemId = std::nullopt,
        .maximumContentsWeightGrams = std::nullopt,
        .maximumContentsVolumeMilliliters = std::nullopt,
        .ignoresContentsWeight = false,
        .ignoresContentsVolume = false
    }));
    static_cast<void>(Container(ContainerDefinition{
        .id = "item.backpack.1",
        .name = "Zaino",
        .ownerItemId = "backpack.1",
        .maximumContentsWeightGrams = 20000,
        .maximumContentsVolumeMilliliters = 30000,
        .ignoresContentsWeight = false,
        .ignoresContentsVolume = false
    }));
    static_cast<void>(Container(ContainerDefinition{
        .id = "item.bagOfHolding.1",
        .name = "Borsa conservante",
        .ownerItemId = "bagOfHolding.1",
        .maximumContentsWeightGrams = 125000,
        .maximumContentsVolumeMilliliters = 850000,
        .ignoresContentsWeight = true,
        .ignoresContentsVolume = true
    }));

    const ItemDefinition containerItemDefinition{
        .id = "sampleContainer",
        .name = "Contenitore di esempio",
        .weightGrams = 1000,
        .volumeMilliliters = 2000,
        .tags = {"container"},
        .slot = std::nullopt,
        .container = ItemContainerDefinition{
            .maximumContentsWeightGrams = 10000,
            .maximumContentsVolumeMilliliters = 20000,
            .acceptedItems = ItemSelector(ItemSelectorDefinition{.tags = {"rope"}}),
            .quantityLimits = {{
                .maximumQuantity = 3,
                .selector = ItemSelector(ItemSelectorDefinition{.tags = {"rope"}})
            }},
            .ignoresContentsWeight = true,
            .ignoresContentsVolume = true
        }
    };
    assert(containerItemDefinition.container.has_value());
    assert(containerItemDefinition.container->maximumContentsWeightGrams == 10000);
    assert(containerItemDefinition.container->maximumContentsVolumeMilliliters == 20000);
    assert(containerItemDefinition.container->acceptedItems.has_value());
    assert(containerItemDefinition.container->quantityLimits.size() == 1);
    assert(containerItemDefinition.container->quantityLimits[0].maximumQuantity == 3);
    assert(containerItemDefinition.container->ignoresContentsWeight);
    assert(containerItemDefinition.container->ignoresContentsVolume);
    assert(containerItemDefinition.container->allowsPossessionEffects);

    const Container inactiveStorage(ContainerDefinition{
        .id = "home",
        .name = "Casa",
        .allowsPossessionEffects = false
    });
    static_cast<void>(inactiveStorage);

    const ItemInstance twoRopes(ItemInstanceDefinition{
        .id = "rope.2",
        .itemDefinitionId = "hempRope15m",
        .quantity = 2
    });
    const auto unusedMeasureResolver = [](std::string_view)
    {
        return 0;
    };
    const auto unusedQuantityResolver = [](std::string_view, const ItemSelector &)
    {
        return 0;
    };
    const ContainerResolvers unusedResolvers{
        .itemWeight = unusedMeasureResolver,
        .itemVolume = unusedMeasureResolver,
        .itemQuantity = unusedQuantityResolver
    };

    const Container unlimited(ContainerDefinition{
        .id = "worn",
        .name = "Indossati"
    });
    unlimited.validateItem(twoRopes, 1000000, 1000000, {});

    const Container limited(ContainerDefinition{
        .id = "backpack",
        .name = "Zaino",
        .maximumContentsWeightGrams = 10000
    });
    limited.validateAdditionalContents(10000, 0, unusedResolvers);
    assert(throwsInvalidArgument([&limited, &unusedResolvers]
    {
        limited.validateAdditionalContents(10001, 0, unusedResolvers);
    }));
    assert(throwsInvalidArgument([&limited, &unusedResolvers]
    {
        limited.validateAdditionalContents(-1, 0, unusedResolvers);
    }));
    assert(throwsInvalidArgument([&limited]
    {
        limited.validateAdditionalContents(1, 0, {});
    }));

    const Container volumeLimited(ContainerDefinition{
        .id = "volumeLimited",
        .name = "Volume limitato",
        .maximumContentsVolumeMilliliters = 10000
    });
    volumeLimited.validateAdditionalContents(0, 10000, unusedResolvers);
    assert(throwsInvalidArgument([&volumeLimited, &unusedResolvers]
    {
        volumeLimited.validateAdditionalContents(0, 10001, unusedResolvers);
    }));
    assert(throwsInvalidArgument([&volumeLimited, &unusedResolvers]
    {
        volumeLimited.validateAdditionalContents(0, -1, unusedResolvers);
    }));
    assert(throwsInvalidArgument([&volumeLimited]
    {
        volumeLimited.validateAdditionalContents(0, 1, {});
    }));

    const Container quantityLimited(ContainerDefinition{
        .id = "quantityLimited",
        .name = "Quantità limitata",
        .quantityLimits = {{.maximumQuantity = 2, .selector = std::nullopt}}
    });
    quantityLimited.validateItem(twoRopes, twoRopes.weightGrams(), twoRopes.volumeMilliliters(), unusedResolvers);
    assert(throwsInvalidArgument([&quantityLimited, &twoRopes]
    {
        quantityLimited.validateItem(twoRopes, twoRopes.weightGrams(), twoRopes.volumeMilliliters(), {});
    }));
    const Container tooSmall(ContainerDefinition{
        .id = "tooSmall",
        .name = "Troppo piccolo",
        .quantityLimits = {{.maximumQuantity = 1, .selector = std::nullopt}}
    });
    assert(throwsInvalidArgument([&tooSmall, &twoRopes, &unusedResolvers]
    {
        tooSmall.validateItem(twoRopes, twoRopes.weightGrams(), twoRopes.volumeMilliliters(), unusedResolvers);
    }));
    const Container ropesOnly(ContainerDefinition{
        .id = "ropesOnly",
        .name = "Solo corde",
        .acceptedItems = ItemSelector(ItemSelectorDefinition{.tags = {"rope"}}),
        .quantityLimits = {{
            .maximumQuantity = 3,
            .selector = ItemSelector(ItemSelectorDefinition{.tags = {"rope"}})
        }}
    });
    ropesOnly.validateItem(twoRopes, twoRopes.weightGrams(), twoRopes.volumeMilliliters(), unusedResolvers);
    const Container ammunitionOnly(ContainerDefinition{
        .id = "ammunitionOnly",
        .name = "Solo munizioni",
        .acceptedItems = ItemSelector(ItemSelectorDefinition{.tags = {"ammunition"}})
    });
    assert(throwsInvalidArgument([&ammunitionOnly, &twoRopes]
    {
        ammunitionOnly.validateItem(twoRopes, twoRopes.weightGrams(), twoRopes.volumeMilliliters(), {});
    }));
    const Container ammunitionLimit(ContainerDefinition{
        .id = "ammunitionLimit",
        .name = "Munizioni limitate",
        .quantityLimits = {{
            .maximumQuantity = 1,
            .selector = ItemSelector(ItemSelectorDefinition{.tags = {"ammunition"}})
        }}
    });
    ammunitionLimit.validateItem(twoRopes, twoRopes.weightGrams(), twoRopes.volumeMilliliters(), {});

    assert(throwsInvalidArgument([]
    {
        static_cast<void>(Container(ContainerDefinition{
            .id = " ",
            .name = "Inventario"
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(Container(ContainerDefinition{
            .id = "invalid.quantity",
            .name = "Contenitore non valido",
            .quantityLimits = {{.maximumQuantity = -1, .selector = std::nullopt}}
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(Container(ContainerDefinition{
            .id = "inventory",
            .name = " "
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(Container(ContainerDefinition{
            .id = "item.backpack.1",
            .name = "Zaino",
            .ownerItemId = " "
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(Container(ContainerDefinition{
            .id = "invalid.weight",
            .name = "Contenitore non valido",
            .maximumContentsWeightGrams = -1
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(Container(ContainerDefinition{
            .id = "invalid.volume",
            .name = "Contenitore non valido",
            .maximumContentsVolumeMilliliters = -1
        }));
    }));

    return 0;
}
