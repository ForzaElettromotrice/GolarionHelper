#include "golarion/character/spell_preparation_cost.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace
{
    void validateSchool(golarion::SpellSchool school)
    {
        using golarion::SpellSchool;
        switch (school)
        {
        case SpellSchool::Abjuration:
        case SpellSchool::Conjuration:
        case SpellSchool::Divination:
        case SpellSchool::Enchantment:
        case SpellSchool::Evocation:
        case SpellSchool::Illusion:
        case SpellSchool::Necromancy:
        case SpellSchool::Transmutation:
        case SpellSchool::Universal:
            return;
        }
        throw std::invalid_argument("invalid preparation cost school");
    }
}

namespace golarion
{
    std::string preparationCostRulesResource(std::string_view castingId)
    {
        return "spellcasting." + normalize(castingId) + ".preparationCostRules";
    }

    std::string preparationCostExemptionsResource(std::string_view castingId)
    {
        return "spellcasting." + normalize(castingId) + ".preparationCostExemptions";
    }

    SpellPreparationCostRegistry::SpellPreparationCostRegistry(ResourceManager &resources, std::string_view castingId)
        : resources_(resources), rulesResource_(preparationCostRulesResource(castingId)), exemptionsResource_(preparationCostExemptionsResource(castingId))
    {
        resources_.registerCollectionResource<SpellPreparationCostRule>(rulesResource_,
            [this](SpellPreparationCostRule rule) { addRule(std::move(rule)); },
            [this](std::string_view id) { removeRule(id); });
        try
        {
            resources_.registerCollectionResource<SpellPreparationCostExemption>(exemptionsResource_,
                [this](SpellPreparationCostExemption exemption) { addExemption(std::move(exemption)); },
                [this](std::string_view id) { removeExemption(id); });
        }
        catch (...)
        {
            resources_.unregisterCollectionResource(rulesResource_);
            throw;
        }
    }

    SpellPreparationCostRegistry::~SpellPreparationCostRegistry()
    {
        resources_.unregisterCollectionResource(exemptionsResource_);
        resources_.unregisterCollectionResource(rulesResource_);
    }

    void SpellPreparationCostRegistry::addRule(SpellPreparationCostRule rule)
    {
        rule.id = normalize(rule.id);
        rule.source = normalize(rule.source);
        validateSchool(rule.school);
        if (rule.requiredSlots < 1) throw std::invalid_argument("preparation slot requirement must be positive");
        const auto id = rule.id;
        if (!rules_.emplace(id, std::move(rule)).second)
            throw std::invalid_argument("preparation cost rule is already registered: " + id);
    }

    void SpellPreparationCostRegistry::removeRule(std::string_view id)
    {
        if (!rules_.erase(normalize(id))) throw std::invalid_argument("preparation cost rule is not registered");
    }

    void SpellPreparationCostRegistry::addExemption(SpellPreparationCostExemption exemption)
    {
        exemption.id = normalize(exemption.id);
        exemption.source = normalize(exemption.source);
        exemption.ruleId = normalize(exemption.ruleId);
        const auto id = exemption.id;
        if (!exemptions_.emplace(id, std::move(exemption)).second)
            throw std::invalid_argument("preparation cost exemption is already registered: " + id);
    }

    void SpellPreparationCostRegistry::removeExemption(std::string_view id)
    {
        if (!exemptions_.erase(normalize(id))) throw std::invalid_argument("preparation cost exemption is not registered");
    }

    SpellPreparationCost SpellPreparationCostRegistry::calculate(SpellSchool school, int spellLevel) const
    {
        validateSchool(school);
        if (spellLevel < 0 || spellLevel > 9) throw std::invalid_argument("spell level must be between 0 and 9");
        SpellPreparationCost result{spellLevel, 1};
        for (const auto &[id, rule] : rules_)
        {
            if (rule.school != school) continue;
            const bool exempt = std::any_of(exemptions_.begin(), exemptions_.end(), [&](const auto &entry) { return entry.second.ruleId == id; });
            if (!exempt) result.requiredSlots = std::max(result.requiredSlots, rule.requiredSlots);
        }
        return result;
    }
}
