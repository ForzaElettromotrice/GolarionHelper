#include "golarion/character/character_class.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace golarion
{
    void CharacterClass::removeArchetype(const std::string &archetypeId)
    {
        const auto entry = archetypes_.find(archetypeId);
        if (entry == archetypes_.end()) throw std::invalid_argument("archetype is not selected");

        std::set<std::string> removed, restored, affectedFeatures;
        std::function<void(const std::vector<ClassFeatureProgressionDefinition> &, std::set<std::string> &)> collect;
        collect = [&](const auto &stages, auto &ids)
        {
            for (const auto &stage : stages)
            {
                ids.insert(stage.id);
                for (const auto &choice : stage.choices) for (const auto &option : choice.options) collect(option.progression, ids);
            }
        };
        for (const auto &feature : entry->second.addedFeatures) collect(feature.progression, removed);
        for (const auto &impact : entry->second.affectedFeatures) affectedFeatures.insert(impact.featureId);
        for (const auto &feature : definition_.features)
            if (affectedFeatures.contains(feature.id)) collect(feature.progression, restored);

        auto selections = selections_;
        std::erase_if(selections, [&](const auto &selection) { return removed.contains(selection.first.first) || restored.contains(selection.first.first); });
        auto previousAcquisitions = acquisitions_;
        auto previousOrder = acquisitionOrder_;
        auto previousReplacers = replacers_;
        auto previousEffects = effects_;
        auto previousDisabled = disabledAcquisitions_;
        try
        {
            for (const auto &id : removed)
            {
                for (const auto &effect : acquisitions_.at(id).effectIds) effects_.erase(effect);
                acquisitions_.erase(id);
            }
            std::erase_if(acquisitionOrder_, [&](const auto &id) { return removed.contains(id); });
            // Compatible archetypes cannot disable acquisitions of the same original feature.
            for (const auto &id : restored) disabledAcquisitions_.erase(id);
            rebuildReplacers();
            update(level_, std::move(selections));
        }
        catch (...)
        {
            acquisitions_.swap(previousAcquisitions);
            acquisitionOrder_.swap(previousOrder);
            replacers_.swap(previousReplacers);
            effects_.swap(previousEffects);
            disabledAcquisitions_.swap(previousDisabled);
            throw;
        }
        // Keep the definitions alive until all old callbacks have been cleaned up.
        archetypes_.erase(entry);
    }

    void CharacterClass::addArchetype(ArchetypeDefinition archetype)
    {
        if (archetype.id.empty() || archetype.classType != definition_.type || archetypes_.contains(archetype.id))
            throw std::invalid_argument("invalid, duplicate or wrong-class archetype");
        std::set<std::string> originalFeatures, featureIds;
        std::map<std::string, std::string> acquisitionOwners;
        std::function<void(const std::vector<ClassFeatureProgressionDefinition> &, const std::string &)> collect;
        collect = [&](const auto &stages, const auto &featureId)
        {
            for (const auto &stage : stages)
            {
                acquisitionOwners.emplace(stage.id, featureId);
                for (const auto &choice : stage.choices) for (const auto &option : choice.options) collect(option.progression, featureId);
            }
        };
        for (const auto &feature : definition_.features)
        {
            originalFeatures.insert(feature.id);
            collect(feature.progression, feature.id);
        }
        featureIds = originalFeatures;
        std::set<std::string> occupied;
        for (const auto &[id, existing] : archetypes_)
        {
            for (const auto &impact : existing.affectedFeatures) occupied.insert(impact.featureId);
            for (const auto &feature : existing.addedFeatures) featureIds.insert(feature.id);
        }
        std::map<std::string, ArchetypeFeatureChangeType> impacts;
        for (const auto &impact : archetype.affectedFeatures)
        {
            if (!originalFeatures.contains(impact.featureId) || occupied.contains(impact.featureId) ||
                !impacts.emplace(impact.featureId, impact.type).second ||
                (impact.type != ArchetypeFeatureChangeType::Alter && impact.type != ArchetypeFeatureChangeType::Replace))
                throw std::invalid_argument("unknown, duplicate or incompatible archetype feature impact");
        }
        std::set<std::string> removedFeatures, removedAcquisitions;
        for (const auto &id : archetype.removedFeatureIds)
            if (!impacts.contains(id) || !removedFeatures.insert(id).second)
                throw std::invalid_argument("invalid archetype feature removal");
        for (const auto &[id, type] : impacts)
            if (type == ArchetypeFeatureChangeType::Replace && !removedFeatures.contains(id))
                throw std::invalid_argument("replaced archetype feature must be removed");
        for (const auto &id : archetype.removedAcquisitionIds)
        {
            const auto owner = acquisitionOwners.find(id);
            if (owner == acquisitionOwners.end() || !impacts.contains(owner->second) || removedFeatures.contains(owner->second) || !removedAcquisitions.insert(id).second)
                throw std::invalid_argument("invalid archetype acquisition removal");
        }
        for (const auto &[id, owner] : acquisitionOwners) if (removedFeatures.contains(owner)) removedAcquisitions.insert(id);

        // New progression replacements may only reference this archetype's new
        // acquisitions. Original-class removals must be declared structurally.
        std::set<std::string> newIds;
        std::vector<std::string> replacementTargets;
        std::function<void(const std::vector<ClassFeatureProgressionDefinition> &)> validateAdded = [&](const auto &stages)
        {
            for (const auto &stage : stages)
            {
                if (acquisitions_.contains(stage.id) || !newIds.insert(stage.id).second)
                    throw std::invalid_argument("archetype acquisition ID collides");
                replacementTargets.insert(replacementTargets.end(), stage.replaces.begin(), stage.replaces.end());
                for (const auto &choice : stage.choices) for (const auto &option : choice.options) validateAdded(option.progression);
            }
        };
        for (const auto &feature : archetype.addedFeatures)
        {
            if (feature.id.empty() || !featureIds.insert(feature.id).second) throw std::invalid_argument("archetype feature ID collides");
            validateAdded(feature.progression);
        }
        for (const auto &id : replacementTargets) if (!newIds.contains(id)) throw std::invalid_argument("archetype progression replaces an external acquisition");

        auto previousAcquisitions = acquisitions_;
        auto previousOrder = acquisitionOrder_;
        auto previousReplacers = replacers_;
        auto previousEffects = effects_;
        auto previousDisabled = disabledAcquisitions_;
        const auto id = archetype.id;
        const auto entry = archetypes_.emplace(id, std::move(archetype)).first;
        try
        {
            for (const auto &feature : entry->second.addedFeatures) index(feature.progression, {}, {}, {});
            disabledAcquisitions_.insert(removedAcquisitions.begin(), removedAcquisitions.end());
            rebuildReplacers();
            update(level_, selections_);
        }
        catch (...)
        {
            acquisitions_.swap(previousAcquisitions);
            acquisitionOrder_.swap(previousOrder);
            replacers_.swap(previousReplacers);
            effects_.swap(previousEffects);
            disabledAcquisitions_.swap(previousDisabled);
            archetypes_.erase(entry);
            throw;
        }
    }
}
