#include "golarion/character/spell_preparation_rules.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <stdexcept>
#include <algorithm>
#include <type_traits>
#include <utility>

namespace
{
    void validate(golarion::SpellLevelOverride &entry)
    {
        entry.groupId = golarion::normalize(entry.groupId);
        entry.spellId = golarion::normalize(entry.spellId);
        if (entry.spellLevel < 0 || entry.spellLevel > 9) throw std::invalid_argument("spell level must be between 0 and 9");
    }

    void validate(golarion::SpellPreparationCheck &entry)
    {
        if (!entry.isSatisfied) throw std::invalid_argument("preparation check callback must not be empty");
    }

    void validate(golarion::SpellPreparationCheckExemption &entry)
    {
        entry.checkId = golarion::normalize(entry.checkId);
        if (!entry.applies) throw std::invalid_argument("preparation exemption callback must not be empty");
    }

    void validate(golarion::SpellConsumptionExemption &entry)
    {
        if (!entry.applies) throw std::invalid_argument("consumption exemption callback must not be empty");
    }

    void validate(golarion::SpontaneousConversion &entry)
    {
        if (!entry.replacementLevel) throw std::invalid_argument("conversion callback must not be empty");
    }
}

namespace golarion
{
    std::string spellLevelOverridesResource(std::string_view castingId)
    {
        return "spellcasting." + normalize(castingId) + ".spellLevelOverrides";
    }

    std::string preparationChecksResource(std::string_view castingId)
    {
        return "spellcasting." + normalize(castingId) + ".preparationChecks";
    }

    std::string preparationCheckExemptionsResource(std::string_view castingId)
    {
        return "spellcasting." + normalize(castingId) + ".preparationCheckExemptions";
    }

    std::string consumptionExemptionsResource(std::string_view castingId)
    {
        return "spellcasting." + normalize(castingId) + ".consumptionExemptions";
    }

    std::string spontaneousConversionsResource(std::string_view castingId)
    {
        return "spellcasting." + normalize(castingId) + ".spontaneousConversions";
    }

    SpellPreparationRules::SpellPreparationRules(ResourceManager &resources, std::string_view castingId)
        : resources_(resources), endpoints_{spellLevelOverridesResource(castingId), preparationChecksResource(castingId), preparationCheckExemptionsResource(castingId), consumptionExemptionsResource(castingId), spontaneousConversionsResource(castingId)}
    {
        std::size_t registered = 0;
        const auto registerEntries = [&](auto &entries)
        {
            using Entry = typename std::remove_reference_t<decltype(entries)>::mapped_type;
            resources_.registerCollectionResource<Entry>(endpoints_[registered],
                [&entries](Entry entry)
                {
                    entry.id = normalize(entry.id);
                    entry.source = normalize(entry.source);
                    validate(entry);
                    const auto id = entry.id;
                    if (!entries.emplace(id, std::move(entry)).second) throw std::invalid_argument("spell rule is already registered: " + id);
                },
                [&entries](std::string_view id)
                {
                    if (!entries.erase(normalize(id))) throw std::invalid_argument("spell rule is not registered");
                });
            ++registered;
        };
        try
        {
            registerEntries(levels_);
            registerEntries(checks_);
            registerEntries(checkExemptions_);
            registerEntries(consumptionExemptions_);
            registerEntries(conversions_);
        }
        catch (...)
        {
            while (registered > 0) resources_.unregisterCollectionResource(endpoints_[--registered]);
            throw;
        }
    }

    SpellPreparationRules::~SpellPreparationRules()
    {
        for (const auto &endpoint : endpoints_) resources_.unregisterCollectionResource(endpoint);
    }

    std::optional<int> SpellPreparationRules::levelOverride(std::string_view groupId, std::string_view spellId) const
    {
        const auto group = normalize(groupId);
        const auto spell = normalize(spellId);
        std::optional<int> result;
        for (const auto &[id, entry] : levels_)
        {
            if (entry.groupId != group || entry.spellId != spell) continue;
            if (result && *result != entry.spellLevel) throw std::invalid_argument("conflicting spell level overrides");
            result = entry.spellLevel;
        }
        return result;
    }

    void SpellPreparationRules::validatePreparation(const SpellPreparationContext &context) const
    {
        for (const auto &[id, check] : checks_)
        {
            bool exempt = false;
            for (const auto &[exemptionId, exemption] : checkExemptions_)
                if (exemption.checkId == id && exemption.applies(context))
                {
                    exempt = true;
                    break;
                }
            if (!exempt && !check.isSatisfied(context)) throw std::invalid_argument("spell preparation check failed: " + id);
        }
    }

    bool SpellPreparationRules::preservesPreparation(const SpellConsumptionContext &context) const
    {
        for (const auto &[id, exemption] : consumptionExemptions_)
            if (exemption.applies(context)) return true;
        return false;
    }

    bool SpellPreparationRules::allowsConversion(const SpellConversionContext &context, const std::function<bool(int)> &acceptsLevel) const
    {
        if (context.slots.empty()) throw std::invalid_argument("conversion requires a preparation");
        for (const auto &[id, conversion] : conversions_)
        {
            const auto level = conversion.replacementLevel(context);
            if (!level) continue;
            if (*level < 0 || *level > 9) throw std::invalid_argument("conversion level must be between 0 and 9");
            if (std::ranges::all_of(context.slots, [&](const auto &slot) { return slot.spellLevel >= *level; }) && (!acceptsLevel || acceptsLevel(*level))) return true;
        }
        return false;
    }
}
