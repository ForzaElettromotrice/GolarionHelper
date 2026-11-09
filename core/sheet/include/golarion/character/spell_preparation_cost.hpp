#pragma once

#include "golarion/character/spell_types.hpp"

#include <map>
#include <string>
#include <string_view>

namespace golarion
{
    class ResourceManager;

    std::string preparationCostRulesResource(std::string_view castingId);
    std::string preparationCostExemptionsResource(std::string_view castingId);

    struct SpellPreparationCostRule
    {
        std::string id;
        std::string source;
        SpellSchool school;
        int requiredSlots = 2;
    };

    struct SpellPreparationCostExemption
    {
        std::string id;
        std::string source;
        std::string ruleId;
    };

    struct SpellPreparationCost
    {
        int minimumSlotLevel;
        int requiredSlots = 1;
    };

    // Applicable requirements combine by maximum, not addition. Exemptions target
    // a source-owned rule ID and may be registered before the rule itself.
    class SpellPreparationCostRegistry final
    {
    public:
        explicit SpellPreparationCostRegistry(ResourceManager &resources, std::string_view castingId);
        ~SpellPreparationCostRegistry();
        SpellPreparationCostRegistry(const SpellPreparationCostRegistry &) = delete;
        SpellPreparationCostRegistry &operator=(const SpellPreparationCostRegistry &) = delete;
        SpellPreparationCostRegistry(SpellPreparationCostRegistry &&) = delete;
        SpellPreparationCostRegistry &operator=(SpellPreparationCostRegistry &&) = delete;

        SpellPreparationCost calculate(SpellSchool school, int spellLevel) const;

    private:
        void addRule(SpellPreparationCostRule rule);
        void removeRule(std::string_view id);
        void addExemption(SpellPreparationCostExemption exemption);
        void removeExemption(std::string_view id);

        ResourceManager &resources_;
        std::string rulesResource_;
        std::string exemptionsResource_;
        std::map<std::string, SpellPreparationCostRule> rules_;
        std::map<std::string, SpellPreparationCostExemption> exemptions_;
    };
}
