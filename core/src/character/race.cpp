#include "golarion/character/race.hpp"

#include "golarion/character/race_definition_manager.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <algorithm>
#include <exception>
#include <ranges>
#include <set>
#include <stdexcept>
#include <utility>

namespace
{
    std::string semanticId(std::string_view prefix, const std::vector<std::string_view> &parts)
    {
        std::string result(prefix);
        for (const std::string_view part : parts)
        {
            result += std::to_string(part.size()) + ":" + std::string(part);
        }
        return result;
    }

    std::string directEffectId(std::string_view raceId, std::string_view elementId, std::string_view effectId)
    {
        return semanticId("race.effect.direct.", {raceId, elementId, effectId});
    }

    std::string choiceEffectId(std::string_view raceId, std::string_view elementId, std::string_view choiceId, std::string_view optionId, std::string_view effectId)
    {
        return semanticId("race.effect.choice.", {raceId, elementId, choiceId, optionId, effectId});
    }

    const golarion::RacialElementDefinition &alternateFeature(const golarion::RaceDefinition &definition, std::string_view featureId)
    {
        const auto feature = std::ranges::find(definition.alternateFeatures, featureId, &golarion::RacialElementDefinition::id);
        if (feature == definition.alternateFeatures.end())
        {
            throw std::invalid_argument("alternate racial feature is not registered for the selected race: " + std::string(featureId));
        }
        return *feature;
    }

    const golarion::RacialChoiceDefinition &racialChoice(const golarion::RacialElementDefinition &element, std::string_view choiceId)
    {
        const auto choice = std::ranges::find(element.choices, choiceId, &golarion::RacialChoiceDefinition::id);
        if (choice == element.choices.end())
        {
            throw std::invalid_argument("racial choice is not registered for the active element: " + std::string(choiceId));
        }
        return *choice;
    }

    const golarion::RacialElementDefinition &racialElement(const golarion::RaceDefinition &definition, std::string_view elementId)
    {
        const auto findElement = [elementId](const std::vector<golarion::RacialElementDefinition> &elements) -> const golarion::RacialElementDefinition *
        {
            const auto element = std::ranges::find(elements, elementId, &golarion::RacialElementDefinition::id);
            return element == elements.end() ? nullptr : &*element;
        };

        if (const golarion::RacialElementDefinition *element = findElement(definition.qualities)) return *element;
        if (const golarion::RacialElementDefinition *element = findElement(definition.standardFeatures)) return *element;
        if (const golarion::RacialElementDefinition *element = findElement(definition.alternateFeatures)) return *element;
        throw std::invalid_argument("racial element is not registered for the selected race: " + std::string(elementId));
    }

    std::vector<std::string> canonicalChoiceOptions(const golarion::RacialChoiceDefinition &choice, std::vector<std::string> optionIds)
    {
        std::set<std::string> selectedOptionIds;
        for (std::string &optionId : optionIds)
        {
            optionId = golarion::normalize(optionId);
            if (!selectedOptionIds.insert(optionId).second)
            {
                throw std::invalid_argument("racial choice contains duplicate options: " + choice.id);
            }
        }
        if (optionIds.size() != choice.selectionCount)
        {
            throw std::invalid_argument("racial choice has an invalid option count: " + choice.id);
        }

        std::vector<std::string> canonicalOptionIds;
        canonicalOptionIds.reserve(choice.selectionCount);
        for (const golarion::RacialChoiceOptionDefinition &option : choice.options)
        {
            if (selectedOptionIds.contains(option.id))
            {
                canonicalOptionIds.push_back(option.id);
            }
        }
        if (canonicalOptionIds.size() != choice.selectionCount)
        {
            throw std::invalid_argument("racial choice contains an unknown option: " + choice.id);
        }
        return canonicalOptionIds;
    }

    std::vector<std::string> canonicalAlternateFeatures(const golarion::RaceDefinition &definition, const std::vector<std::string> &featureIds)
    {
        std::set<std::string> requestedIds;
        for (const std::string &featureId : featureIds)
        {
            const std::string id = golarion::normalize(featureId);
            alternateFeature(definition, id);
            if (!requestedIds.insert(id).second)
            {
                throw std::invalid_argument("alternate racial feature is duplicated: " + id);
            }
        }

        std::set<std::string> replacedElementIds;
        std::vector<std::string> canonicalIds;
        canonicalIds.reserve(requestedIds.size());
        for (const golarion::RacialElementDefinition &feature : definition.alternateFeatures)
        {
            if (!requestedIds.contains(feature.id))
            {
                continue;
            }
            for (const std::string &replacement : feature.replaces)
            {
                if (!replacedElementIds.insert(replacement).second)
                {
                    throw std::invalid_argument("alternate racial features cannot replace the same baseline element: " + replacement);
                }
            }
            canonicalIds.push_back(feature.id);
        }
        return canonicalIds;
    }
}

namespace golarion
{
    Race::Race(ResourceManager &resourceManager) : resourceManager_(resourceManager)
    {
    }

    Race::~Race()
    {
        for (auto effectId = appliedEffectIds_.rbegin(); effectId != appliedEffectIds_.rend(); ++effectId)
        {
            appliedCleanups_.at(*effectId)();
        }
    }

    void Race::setRace(std::string_view raceDefinitionId)
    {
        const RaceDefinition &definition = RaceDefinitionManager::instance().get(raceDefinitionId);
        if (definition_.has_value() && definition_->get().id == definition.id)
        {
            return;
        }

        CompiledEffects compiledEffects = compileEffects(definition);
        const std::vector<std::string> alternateFeatureIds;
        const ChoiceSelections choices;
        const std::vector<std::string> desiredIds = desiredEffectIds(definition, alternateFeatureIds, choices);
        transitionEffects(desiredIds, compiledEffects);

        definition_ = std::cref(definition);
        alternateFeatureIds_.clear();
        choices_.clear();
        compiledEffects_ = std::move(compiledEffects);
    }

    void Race::clearRace()
    {
        if (!definition_.has_value())
        {
            return;
        }

        const CompiledEffects noEffects;
        transitionEffects({}, noEffects);
        definition_.reset();
        alternateFeatureIds_.clear();
        choices_.clear();
        compiledEffects_.clear();
    }

    void Race::selectAlternateFeature(std::string_view featureId)
    {
        if (!definition_.has_value())
        {
            throw std::logic_error("a race must be selected before choosing an alternate racial feature");
        }

        const std::string id = normalize(featureId);
        const RaceDefinition &definition = definition_->get();
        const RacialElementDefinition &selectedFeature = alternateFeature(definition, id);
        if (std::ranges::find(alternateFeatureIds_, id) != alternateFeatureIds_.end())
        {
            throw std::invalid_argument("alternate racial feature is already selected: " + id);
        }

        for (const std::string &selectedId : alternateFeatureIds_)
        {
            const RacialElementDefinition &alreadySelected = alternateFeature(definition, selectedId);
            for (const std::string &replacement : selectedFeature.replaces)
            {
                if (std::ranges::find(alreadySelected.replaces, replacement) != alreadySelected.replaces.end())
                {
                    throw std::invalid_argument("alternate racial features cannot replace the same baseline element: " + replacement);
                }
            }
        }

        std::vector<std::string> alternateFeatureIds;
        alternateFeatureIds.reserve(alternateFeatureIds_.size() + 1);
        for (const RacialElementDefinition &feature : definition.alternateFeatures)
        {
            if (feature.id == id || std::ranges::find(alternateFeatureIds_, feature.id) != alternateFeatureIds_.end())
            {
                alternateFeatureIds.push_back(feature.id);
            }
        }

        const std::vector<std::string> desiredIds = desiredEffectIds(definition, alternateFeatureIds, choices_);
        transitionEffects(desiredIds, compiledEffects_);
        alternateFeatureIds_ = std::move(alternateFeatureIds);
    }

    void Race::removeAlternateFeature(std::string_view featureId)
    {
        if (!definition_.has_value())
        {
            throw std::logic_error("a race must be selected before removing an alternate racial feature");
        }

        const std::string id = normalize(featureId);
        const auto selectedFeature = std::ranges::find(alternateFeatureIds_, id);
        if (selectedFeature == alternateFeatureIds_.end())
        {
            throw std::invalid_argument("alternate racial feature is not selected: " + id);
        }

        std::vector<std::string> alternateFeatureIds = alternateFeatureIds_;
        alternateFeatureIds.erase(alternateFeatureIds.begin() + std::distance(alternateFeatureIds_.begin(), selectedFeature));
        const std::vector<std::string> desiredIds = desiredEffectIds(definition_->get(), alternateFeatureIds, choices_);
        transitionEffects(desiredIds, compiledEffects_);
        alternateFeatureIds_ = std::move(alternateFeatureIds);
    }

    void Race::setChoice(std::string_view elementId, std::string_view choiceId, std::vector<std::string> optionIds)
    {
        if (!definition_.has_value())
        {
            throw std::logic_error("a race must be selected before setting a racial choice");
        }

        const std::string normalizedElementId = normalize(elementId);
        const std::string normalizedChoiceId = normalize(choiceId);
        const RaceDefinition &definition = definition_->get();
        const std::vector<const RacialElementDefinition *> elements = activeElements(definition, alternateFeatureIds_);
        const auto element = std::ranges::find_if(elements, [&normalizedElementId](const RacialElementDefinition *candidate)
        {
            return candidate->id == normalizedElementId;
        });
        if (element == elements.end())
        {
            throw std::invalid_argument("racial element is not active: " + normalizedElementId);
        }
        const RacialChoiceDefinition &choice = racialChoice(**element, normalizedChoiceId);

        ChoiceSelections choices = choices_;
        choices.insert_or_assign(ChoiceKey{normalizedElementId, normalizedChoiceId}, canonicalChoiceOptions(choice, std::move(optionIds)));
        const std::vector<std::string> desiredIds = desiredEffectIds(definition, alternateFeatureIds_, choices);
        transitionEffects(desiredIds, compiledEffects_);
        choices_ = std::move(choices);
    }

    RaceView Race::toView() const
    {
        if (!definition_.has_value())
        {
            return RaceView{
                .race = std::nullopt,
                .activeElements = {}
            };
        }

        const RaceDefinition &definition = definition_->get();
        std::vector<RacialElementView> elementViews;
        for (const RacialElementDefinition *element : activeElements(definition, alternateFeatureIds_))
        {
            std::vector<RacialChoiceSelectionView> choiceViews;
            std::vector<MissingRacialChoiceView> missingChoiceViews;
            for (const RacialChoiceDefinition &choice : element->choices)
            {
                const auto selection = choices_.find(ChoiceKey{element->id, choice.id});
                if (selection == choices_.end())
                {
                    std::vector<RacialChoiceOptionView> options;
                    options.reserve(choice.options.size());
                    for (const RacialChoiceOptionDefinition &option : choice.options)
                    {
                        options.push_back(RacialChoiceOptionView{
                            .id = option.id,
                            .name = option.name
                        });
                    }
                    missingChoiceViews.push_back(MissingRacialChoiceView{
                        .id = choice.id,
                        .prompt = choice.prompt,
                        .selectionCount = choice.selectionCount,
                        .options = std::move(options)
                    });
                    continue;
                }

                std::vector<RacialChoiceOptionView> selectedOptions;
                selectedOptions.reserve(selection->second.size());
                for (const RacialChoiceOptionDefinition &option : choice.options)
                {
                    if (std::ranges::find(selection->second, option.id) != selection->second.end())
                    {
                        selectedOptions.push_back(RacialChoiceOptionView{
                            .id = option.id,
                            .name = option.name
                        });
                    }
                }
                choiceViews.push_back(RacialChoiceSelectionView{
                    .id = choice.id,
                    .prompt = choice.prompt,
                    .selectedOptions = std::move(selectedOptions)
                });
            }

            elementViews.push_back(RacialElementView{
                .id = element->id,
                .name = element->name,
                .description = element->description,
                .choices = std::move(choiceViews),
                .missingChoices = std::move(missingChoiceViews)
            });
        }

        return RaceView{
            .race = RaceSummaryView{
                .id = definition.id,
                .name = definition.name
            },
            .activeElements = std::move(elementViews)
        };
    }

    RaceSaveData Race::toSaveData() const
    {
        std::vector<RacialChoiceSelectionSaveData> choices;
        choices.reserve(choices_.size());
        for (const auto &[key, optionIds] : choices_)
        {
            choices.push_back(RacialChoiceSelectionSaveData{
                .elementId = key.first,
                .choiceId = key.second,
                .optionIds = optionIds
            });
        }

        return RaceSaveData{
            .raceDefinitionId = definition_.has_value() ? std::optional<std::string>(definition_->get().id) : std::nullopt,
            .alternateFeatureIds = alternateFeatureIds_,
            .choices = std::move(choices)
        };
    }

    void Race::load(const RaceSaveData &data)
    {
        if (!data.raceDefinitionId.has_value())
        {
            if (!data.alternateFeatureIds.empty() || !data.choices.empty())
            {
                throw std::invalid_argument("race save without a race cannot contain alternate features or choices");
            }
            clearRace();
            return;
        }

        const RaceDefinition &definition = RaceDefinitionManager::instance().get(*data.raceDefinitionId);
        const std::vector<std::string> alternateFeatureIds = canonicalAlternateFeatures(definition, data.alternateFeatureIds);
        ChoiceSelections choices;
        for (const RacialChoiceSelectionSaveData &selection : data.choices)
        {
            const std::string elementId = normalize(selection.elementId);
            const std::string choiceId = normalize(selection.choiceId);
            const RacialElementDefinition &element = racialElement(definition, elementId);
            const RacialChoiceDefinition &choice = racialChoice(element, choiceId);
            if (!choices.emplace(ChoiceKey{elementId, choiceId}, canonicalChoiceOptions(choice, selection.optionIds)).second)
            {
                throw std::invalid_argument("racial choice is duplicated in save data: " + choiceId);
            }
        }

        CompiledEffects compiledEffects = compileEffects(definition);
        const std::vector<std::string> desiredIds = desiredEffectIds(definition, alternateFeatureIds, choices);
        transitionEffects(desiredIds, compiledEffects);
        definition_ = std::cref(definition);
        alternateFeatureIds_ = alternateFeatureIds;
        choices_ = std::move(choices);
        compiledEffects_ = std::move(compiledEffects);
    }

    std::vector<const RacialElementDefinition *> Race::activeElements(const RaceDefinition &definition, const std::vector<std::string> &alternateFeatureIds) const
    {
        std::set<std::string> replacedElementIds;
        for (const std::string &featureId : alternateFeatureIds)
        {
            const RacialElementDefinition &feature = alternateFeature(definition, featureId);
            replacedElementIds.insert(feature.replaces.begin(), feature.replaces.end());
        }

        std::vector<const RacialElementDefinition *> elements;
        elements.reserve(definition.qualities.size() + definition.standardFeatures.size() + alternateFeatureIds.size());
        for (const RacialElementDefinition &element : definition.qualities)
        {
            if (!replacedElementIds.contains(element.id))
            {
                elements.push_back(&element);
            }
        }
        for (const RacialElementDefinition &element : definition.standardFeatures)
        {
            if (!replacedElementIds.contains(element.id))
            {
                elements.push_back(&element);
            }
        }
        for (const RacialElementDefinition &feature : definition.alternateFeatures)
        {
            if (std::ranges::find(alternateFeatureIds, feature.id) != alternateFeatureIds.end())
            {
                elements.push_back(&feature);
            }
        }
        return elements;
    }

    Race::CompiledEffects Race::compileEffects(const RaceDefinition &definition) const
    {
        CompiledEffects compiledEffects;
        const auto compileElement = [&definition, &compiledEffects](const RacialElementDefinition &element)
        {
            const std::string source = definition.name + " — " + element.name;
            const auto compile = [&compiledEffects, &source](const EffectDefinition &effect, const std::string &id)
            {
                CompiledEffect compiledEffect{
                    .id = effectId(effect),
                    .apply = compileEffect(effect, EffectContext{
                        .instanceId = id,
                        .source = source
                    })
                };
                if (!compiledEffect.apply)
                {
                    throw std::logic_error("compiled racial effect must not be empty: " + compiledEffect.id);
                }
                if (!compiledEffects.emplace(id, std::move(compiledEffect)).second)
                {
                    throw std::logic_error("compiled racial effect ID is duplicated: " + id);
                }
            };

            for (const EffectDefinition &effect : element.effects)
            {
                const std::string id = directEffectId(definition.id, element.id, effectId(effect));
                compile(effect, id);
            }
            for (const RacialChoiceDefinition &choice : element.choices)
            {
                for (const RacialChoiceOptionDefinition &option : choice.options)
                {
                    for (const EffectDefinition &effect : option.effects)
                    {
                        const std::string id = choiceEffectId(definition.id, element.id, choice.id, option.id, effectId(effect));
                        compile(effect, id);
                    }
                }
            }
        };

        for (const RacialElementDefinition &element : definition.qualities) compileElement(element);
        for (const RacialElementDefinition &element : definition.standardFeatures) compileElement(element);
        for (const RacialElementDefinition &element : definition.alternateFeatures) compileElement(element);
        return compiledEffects;
    }

    std::vector<std::string> Race::desiredEffectIds(const RaceDefinition &definition, const std::vector<std::string> &alternateFeatureIds, const ChoiceSelections &choices) const
    {
        std::vector<std::string> desiredIds;
        for (const RacialElementDefinition *element : activeElements(definition, alternateFeatureIds))
        {
            for (const EffectDefinition &effect : element->effects)
            {
                desiredIds.push_back(directEffectId(definition.id, element->id, effectId(effect)));
            }
            for (const RacialChoiceDefinition &choice : element->choices)
            {
                const auto selection = choices.find(ChoiceKey{element->id, choice.id});
                if (selection == choices.end())
                {
                    continue;
                }
                for (const RacialChoiceOptionDefinition &option : choice.options)
                {
                    if (std::ranges::find(selection->second, option.id) == selection->second.end())
                    {
                        continue;
                    }
                    for (const EffectDefinition &effect : option.effects)
                    {
                        desiredIds.push_back(choiceEffectId(definition.id, element->id, choice.id, option.id, effectId(effect)));
                    }
                }
            }
        }
        return desiredIds;
    }

    void Race::transitionEffects(const std::vector<std::string> &desiredIds, const CompiledEffects &desiredCompiledEffects)
    {
        const std::set<std::string> desiredIdSet(desiredIds.begin(), desiredIds.end());
        std::set<std::string> removedIds;
        for (const std::string &effectId : appliedEffectIds_)
        {
            if (!desiredIdSet.contains(effectId))
            {
                removedIds.insert(effectId);
            }
        }

        for (auto effectId = appliedEffectIds_.rbegin(); effectId != appliedEffectIds_.rend(); ++effectId)
        {
            if (removedIds.contains(*effectId))
            {
                appliedCleanups_.at(*effectId)();
                appliedCleanups_.erase(*effectId);
            }
        }

        std::vector<std::string> addedIds;
        try
        {
            for (const std::string &effectId : desiredIds)
            {
                if (appliedCleanups_.contains(effectId))
                {
                    continue;
                }
                const CompiledEffect &effect = desiredCompiledEffects.at(effectId);
                EffectCleanup cleanup = effect.apply(resourceManager_);
                if (!cleanup)
                {
                    throw std::logic_error("racial effect cleanup must not be empty: " + effect.id);
                }
                appliedCleanups_.emplace(effectId, std::move(cleanup));
                addedIds.push_back(effectId);
            }
        }
        catch (...)
        {
            for (auto effectId = addedIds.rbegin(); effectId != addedIds.rend(); ++effectId)
            {
                appliedCleanups_.at(*effectId)();
                appliedCleanups_.erase(*effectId);
            }
            try
            {
                for (const std::string &effectId : appliedEffectIds_)
                {
                    if (!removedIds.contains(effectId))
                    {
                        continue;
                    }
                    EffectCleanup cleanup = compiledEffects_.at(effectId).apply(resourceManager_);
                    if (!cleanup)
                    {
                        throw std::logic_error("restored racial effect cleanup must not be empty");
                    }
                    appliedCleanups_.emplace(effectId, std::move(cleanup));
                }
            }
            catch (...)
            {
                std::terminate();
            }
            throw;
        }

        appliedEffectIds_ = desiredIds;
    }
}
