#include "golarion/equipment/item_definition_manager.hpp"

#include "golarion/equipment/item_definition_catalog.hpp"
#include "golarion/util/string_utils.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <ranges>
#include <stdexcept>
#include <utility>

namespace
{
    using Json = nlohmann::json;

    std::vector<std::string> normalizedTags(const Json &json, std::string_view definitionId)
    {
        std::vector<std::string> tags = json.at("tags").get<std::vector<std::string>>();
        for (std::string &tag : tags)
        {
            tag = golarion::normalize(tag);
        }
        std::ranges::sort(tags);
        if (std::ranges::adjacent_find(tags) != tags.end())
        {
            throw std::invalid_argument("item definition tags must not contain duplicates: " + std::string(definitionId));
        }
        return tags;
    }

    golarion::ItemSelector itemSelectorFromJson(const Json &json)
    {
        return golarion::ItemSelector(golarion::ItemSelectorDefinition{
            .definitionIds = json.value("definitionIds", std::vector<std::string>{}),
            .tags = json.value("tags", std::vector<std::string>{})
        });
    }

    std::optional<std::int64_t> optionalNonnegativeInteger(const Json &json, std::string_view field, std::string_view description)
    {
        if (!json.contains(field))
        {
            return std::nullopt;
        }

        const std::int64_t value = json.at(field).get<std::int64_t>();
        if (value < 0)
        {
            throw std::invalid_argument(std::string(description) + " must not be negative");
        }
        return value;
    }

    std::optional<golarion::ItemSelector> optionalItemSelector(const Json &json, std::string_view field)
    {
        if (!json.contains(field))
        {
            return std::nullopt;
        }
        return itemSelectorFromJson(json.at(field));
    }

    std::vector<golarion::ItemQuantityLimit> quantityLimitsFromJson(const Json &json)
    {
        std::vector<golarion::ItemQuantityLimit> quantityLimits;
        for (const Json &quantityLimitJson : json.value("quantityLimits", Json::array()))
        {
            const std::int64_t maximumQuantity = quantityLimitJson.at("maximumQuantity").get<std::int64_t>();
            if (maximumQuantity < 0)
            {
                throw std::invalid_argument("item container maximum quantity must not be negative");
            }
            quantityLimits.push_back(golarion::ItemQuantityLimit{
                .maximumQuantity = maximumQuantity,
                .selector = optionalItemSelector(quantityLimitJson, "selector")
            });
        }
        return quantityLimits;
    }

    std::optional<golarion::ItemContainerDefinition> itemContainerDefinitionFromJson(const Json &json)
    {
        if (!json.contains("container"))
        {
            return std::nullopt;
        }

        const Json &containerJson = json.at("container");
        return golarion::ItemContainerDefinition{
            .maximumContentsWeightGrams = optionalNonnegativeInteger(containerJson, "maximumContentsWeightGrams", "item container maximum contents weight"),
            .maximumContentsVolumeMilliliters = optionalNonnegativeInteger(containerJson, "maximumContentsVolumeMilliliters", "item container maximum contents volume"),
            .acceptedItems = optionalItemSelector(containerJson, "acceptedItems"),
            .quantityLimits = quantityLimitsFromJson(containerJson),
            .ignoresContentsWeight = containerJson.value("ignoresContentsWeight", false),
            .ignoresContentsVolume = containerJson.value("ignoresContentsVolume", false),
            .allowsPossessionEffects = containerJson.value("allowsPossessionEffects", true)
        };
    }

    std::optional<golarion::EquipmentSlot> equipmentSlotFromJson(const Json &json)
    {
        if (!json.contains("slot"))
        {
            return std::nullopt;
        }

        const std::string slot = golarion::normalize(json.at("slot").get<std::string>());
        if (slot == "ring") return golarion::EquipmentSlot::Ring;
        if (slot == "armor") return golarion::EquipmentSlot::Armor;
        if (slot == "belt") return golarion::EquipmentSlot::Belt;
        if (slot == "neck") return golarion::EquipmentSlot::Neck;
        if (slot == "body") return golarion::EquipmentSlot::Body;
        if (slot == "headband") return golarion::EquipmentSlot::Headband;
        if (slot == "hands") return golarion::EquipmentSlot::Hands;
        if (slot == "eyes") return golarion::EquipmentSlot::Eyes;
        if (slot == "feet") return golarion::EquipmentSlot::Feet;
        if (slot == "wrists") return golarion::EquipmentSlot::Wrists;
        if (slot == "shield") return golarion::EquipmentSlot::Shield;
        if (slot == "shoulders") return golarion::EquipmentSlot::Shoulders;
        if (slot == "head") return golarion::EquipmentSlot::Head;
        if (slot == "chest") return golarion::EquipmentSlot::Chest;
        throw std::invalid_argument("unknown item equipment slot: " + slot);
    }

    std::optional<golarion::CoinDenomination> coinDenominationFromJson(const Json &json)
    {
        if (!json.contains("coinDenomination"))
        {
            return std::nullopt;
        }

        const std::string denomination = golarion::normalize(json.at("coinDenomination").get<std::string>());
        if (denomination == "copper") return golarion::CoinDenomination::Copper;
        if (denomination == "silver") return golarion::CoinDenomination::Silver;
        if (denomination == "gold") return golarion::CoinDenomination::Gold;
        if (denomination == "platinum") return golarion::CoinDenomination::Platinum;
        throw std::invalid_argument("unknown coin denomination: " + denomination);
    }

    golarion::ItemDefinition itemDefinitionFromJson(const Json &json)
    {
        const std::string id = golarion::normalize(json.at("id").get<std::string>());
        golarion::ItemDefinition definition{
            .id = id,
            .name = golarion::normalize(json.at("name").get<std::string>()),
            .weightGrams = json.at("weightGrams").get<std::int64_t>(),
            .volumeMilliliters = json.at("volumeMilliliters").get<std::int64_t>(),
            .tags = normalizedTags(json, id),
            .coinDenomination = coinDenominationFromJson(json),
            .slot = equipmentSlotFromJson(json),
            .container = itemContainerDefinitionFromJson(json),
            .effects = {}
        };
        if (definition.weightGrams < 0)
        {
            throw std::invalid_argument("item definition weight must not be negative: " + definition.id);
        }
        if (definition.volumeMilliliters < 0)
        {
            throw std::invalid_argument("item definition volume must not be negative: " + definition.id);
        }
        if (definition.coinDenomination.has_value())
        {
            if (definition.weightGrams != 10)
            {
                throw std::invalid_argument("coin item definition must weigh 10 grams: " + definition.id);
            }
            if (definition.slot.has_value() || definition.container.has_value())
            {
                throw std::invalid_argument("coin item definition cannot be equipped or contain items: " + definition.id);
            }
        }
        return definition;
    }
}

namespace golarion
{
    ItemDefinitionManager &ItemDefinitionManager::instance()
    {
        static ItemDefinitionManager manager;
        return manager;
    }

    const ItemDefinition &ItemDefinitionManager::get(std::string_view itemDefinitionId)
    {
        const std::string normalizedId = normalize(itemDefinitionId);
        const std::scoped_lock lock(mutex_);
        if (!loaded_)
        {
            loadCatalog();
        }

        const auto definition = definitions_.find(normalizedId);
        if (definition == definitions_.end())
        {
            throw std::invalid_argument("item definition is not registered: " + normalizedId);
        }
        return definition->second;
    }

    void ItemDefinitionManager::registerEffect(std::string_view itemDefinitionId, ItemEffectDefinition effect)
    {
        const std::string normalizedDefinitionId = normalize(itemDefinitionId);
        const std::scoped_lock lock(mutex_);
        if (!loaded_)
        {
            loadCatalog();
        }

        const auto definition = definitions_.find(normalizedDefinitionId);
        if (definition == definitions_.end())
        {
            throw std::invalid_argument("item definition is not registered: " + normalizedDefinitionId);
        }
        if (definition->second.coinDenomination.has_value())
        {
            throw std::invalid_argument("coin item definitions cannot receive effects: " + normalizedDefinitionId);
        }

        effect.id = normalize(effect.id);
        effect.description = normalize(effect.description);
        if (!effect.apply)
        {
            throw std::invalid_argument("item effect apply callback must not be empty");
        }
        if (std::ranges::any_of(definition->second.effects, [&effect](const ItemEffectDefinition &registeredEffect)
        {
            return registeredEffect.id == effect.id;
        }))
        {
            throw std::invalid_argument("item effect is already registered for definition: " + effect.id);
        }
        definition->second.effects.push_back(std::move(effect));
    }

    void ItemDefinitionManager::loadCatalog()
    {
        try
        {
            const Json catalog = Json::parse(embedded::ItemDefinitionCatalogJson);
            if (!catalog.is_array())
            {
                throw std::invalid_argument("item definition catalog must be a JSON array");
            }

            std::map<std::string, ItemDefinition> loadedDefinitions;
            for (const Json &entry : catalog)
            {
                if (!entry.is_object())
                {
                    throw std::invalid_argument("item definition catalog entries must be JSON objects");
                }

                ItemDefinition definition = itemDefinitionFromJson(entry);
                const std::string id = definition.id;
                if (!loadedDefinitions.emplace(id, std::move(definition)).second)
                {
                    throw std::invalid_argument("item definition is duplicated: " + id);
                }
            }

            definitions_ = std::move(loadedDefinitions);
            loaded_ = true;
        }
        catch (const nlohmann::json::exception &exception)
        {
            throw std::invalid_argument("invalid embedded item definition catalog: " + std::string(exception.what()));
        }
    }
}
