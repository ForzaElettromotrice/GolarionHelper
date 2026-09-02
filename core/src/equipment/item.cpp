#include "golarion/equipment/item.hpp"

#include "golarion/equipment/item_definition_manager.hpp"
#include "golarion/effect/effect_compiler.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <algorithm>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <utility>

namespace
{
    std::string itemEffectInstanceId(std::string_view itemInstanceId, std::string_view effectId)
    {
        const auto appendPart = [](std::string &result, std::string_view part)
        {
            result += std::to_string(part.size()) + ":" + std::string(part);
        };

        std::string result = "item.effect.";
        appendPart(result, itemInstanceId);
        appendPart(result, effectId);
        return result;
    }

    void normalizeSelectorValues(std::vector<std::string> &values, std::string_view field)
    {
        for (std::string &value : values)
        {
            value = golarion::normalize(value);
        }
        std::ranges::sort(values);
        if (std::ranges::adjacent_find(values) != values.end())
        {
            throw std::invalid_argument("item selector " + std::string(field) + " must not contain duplicates");
        }
    }

    std::int64_t totalWeight(const golarion::ItemDefinition &definition, int quantity)
    {
        if (definition.weightGrams != 0 && quantity > std::numeric_limits<std::int64_t>::max() / definition.weightGrams)
        {
            throw std::invalid_argument("item weight is out of range");
        }
        return definition.weightGrams * quantity;
    }

    std::int64_t totalVolume(const golarion::ItemDefinition &definition, int quantity)
    {
        if (definition.volumeMilliliters != 0 && quantity > std::numeric_limits<std::int64_t>::max() / definition.volumeMilliliters)
        {
            throw std::invalid_argument("item volume is out of range");
        }
        return definition.volumeMilliliters * quantity;
    }

    std::vector<golarion::ItemChoiceSelection> validatedChoices(const golarion::ItemDefinition &definition, std::vector<golarion::ItemChoiceSelection> selections)
    {
        for (golarion::ItemChoiceSelection &selection : selections)
        {
            selection.choiceId = golarion::normalize(selection.choiceId);
            for (std::string &optionId : selection.optionIds)
            {
                optionId = golarion::normalize(optionId);
            }
            std::ranges::sort(selection.optionIds);
            if (std::ranges::adjacent_find(selection.optionIds) != selection.optionIds.end())
            {
                throw std::invalid_argument("item choice selection contains duplicate options: " + selection.choiceId);
            }
        }
        std::ranges::sort(selections, {}, &golarion::ItemChoiceSelection::choiceId);
        if (std::ranges::adjacent_find(selections, {}, &golarion::ItemChoiceSelection::choiceId) != selections.end())
        {
            throw std::invalid_argument("item instance contains duplicate choice selections");
        }

        std::vector<golarion::ItemChoiceSelection> validated;
        validated.reserve(definition.choices.size());
        for (const golarion::ItemChoiceDefinition &choice : definition.choices)
        {
            const auto selection = std::ranges::find(selections, choice.id, &golarion::ItemChoiceSelection::choiceId);
            if (selection == selections.end())
            {
                throw std::invalid_argument("item instance is missing choice selection: " + choice.id);
            }
            if (selection->optionIds.size() != choice.selectionCount)
            {
                throw std::invalid_argument("item choice selection has an invalid option count: " + choice.id);
            }

            std::vector<std::string> optionIds;
            optionIds.reserve(choice.selectionCount);
            for (const golarion::ItemChoiceOptionDefinition &option : choice.options)
            {
                if (std::ranges::binary_search(selection->optionIds, option.id))
                {
                    optionIds.push_back(option.id);
                }
            }
            if (optionIds.size() != choice.selectionCount)
            {
                throw std::invalid_argument("item choice selection contains an unknown option: " + choice.id);
            }
            validated.push_back(golarion::ItemChoiceSelection{
                .choiceId = choice.id,
                .optionIds = std::move(optionIds)
            });
        }
        if (validated.size() != selections.size())
        {
            throw std::invalid_argument("item instance contains an unknown choice selection");
        }
        return validated;
    }

}

namespace golarion
{
    std::string_view displayName(ItemEffectActivation activation)
    {
        switch (activation)
        {
            case ItemEffectActivation::Possessed:
                return "Posseduto";
            case ItemEffectActivation::Equipped:
                return "Equipaggiato";
        }

        throw std::invalid_argument("unknown item effect activation");
    }

    ItemSelector::ItemSelector(ItemSelectorDefinition definition)
        : definitionIds_(std::move(definition.definitionIds)),
          tags_(std::move(definition.tags))
    {
        normalizeSelectorValues(definitionIds_, "definition IDs");
        normalizeSelectorValues(tags_, "tags");
    }

    ItemInstance::ItemInstance(ItemInstanceDefinition definition)
        : id_(normalize(definition.id)),
          itemDefinition_(ItemDefinitionManager::instance().get(definition.itemDefinitionId)),
          quantity_(definition.quantity),
          choices_(validatedChoices(itemDefinition_.get(), std::move(definition.choices)))
    {
        if (quantity_ < 1)
        {
            throw std::invalid_argument("item quantity must be at least 1");
        }
        if (quantity_ != 1 && (!itemDefinition_.get().effects.empty() || !itemDefinition_.get().choices.empty()))
        {
            throw std::invalid_argument("item instances with effects or choices must have quantity 1");
        }

        const auto compile = [this](const ItemEffectDefinition &effect, const std::string &effectPath)
        {
            const std::string id = effectId(effect.effect);
            EffectApply apply = compileEffect(effect.effect, EffectContext{
                .instanceId = itemEffectInstanceId(id_, effectPath),
                .source = itemDefinition_.get().name
            });
            if (!apply)
            {
                throw std::invalid_argument("compiled item effect must not be empty: " + id);
            }

            CompiledEffect compiled{
                .id = id,
                .apply = std::move(apply)
            };
            if (effect.activation == ItemEffectActivation::Possessed)
            {
                possessedEffects_.push_back(std::move(compiled));
            }
            else
            {
                equippedEffects_.push_back(std::move(compiled));
            }
        };

        for (const ItemEffectDefinition &effect : itemDefinition_.get().effects)
        {
            compile(effect, "direct." + effectId(effect.effect));
        }
        for (std::size_t choiceIndex = 0; choiceIndex < itemDefinition_.get().choices.size(); ++choiceIndex)
        {
            const ItemChoiceDefinition &choice = itemDefinition_.get().choices[choiceIndex];
            const ItemChoiceSelection &selection = choices_[choiceIndex];
            for (const ItemChoiceOptionDefinition &option : choice.options)
            {
                if (std::ranges::find(selection.optionIds, option.id) == selection.optionIds.end())
                {
                    continue;
                }
                for (const ItemEffectDefinition &effect : option.effects)
                {
                    compile(effect, "choice." + choice.id + "." + option.id + "." + effectId(effect.effect));
                }
            }
        }
    }

    std::int64_t ItemInstance::weightGrams() const
    {
        return totalWeight(itemDefinition_.get(), quantity_);
    }

    std::int64_t ItemInstance::volumeMilliliters() const
    {
        return totalVolume(itemDefinition_.get(), quantity_);
    }

    bool ItemInstance::matches(const ItemSelector &selector) const
    {
        if (selector.definitionIds_.empty() && selector.tags_.empty())
        {
            return true;
        }

        const ItemDefinition &definition = itemDefinition_.get();
        if (std::ranges::binary_search(selector.definitionIds_, definition.id))
        {
            return true;
        }
        return std::ranges::any_of(selector.tags_, [&definition](const std::string &tag)
        {
            return std::ranges::binary_search(definition.tags, tag);
        });
    }

    std::int64_t ItemInstance::matchingQuantity(const ItemSelector &selector) const
    {
        return matches(selector) ? quantity_ : 0;
    }

    std::vector<EffectCleanup> ItemInstance::applyEffects(ResourceManager &resourceManager, ItemEffectActivation activation) const
    {
        const std::vector<CompiledEffect> &effects = activation == ItemEffectActivation::Possessed ? possessedEffects_ : equippedEffects_;
        std::vector<EffectCleanup> cleanups;
        cleanups.reserve(effects.size());
        try
        {
            for (const CompiledEffect &effect : effects)
            {
                EffectCleanup cleanup = effect.apply(resourceManager);
                if (!cleanup)
                {
                    throw std::invalid_argument("item effect cleanup callback must not be empty: " + effect.id);
                }
                cleanups.push_back(std::move(cleanup));
            }
        }
        catch (...)
        {
            removeEffects(cleanups);
            throw;
        }
        return cleanups;
    }

    void ItemInstance::removeEffects(std::vector<EffectCleanup> &cleanups) const noexcept
    {
        for (auto cleanup = cleanups.rbegin(); cleanup != cleanups.rend(); ++cleanup)
        {
            (*cleanup)();
        }
        cleanups.clear();
    }

}
