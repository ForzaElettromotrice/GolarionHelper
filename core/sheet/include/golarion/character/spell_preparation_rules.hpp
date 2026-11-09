#pragma once

#include "golarion/character/prepared_spell_slot.hpp"
#include "golarion/character/spell_definition.hpp"
#include "golarion/character/spell_slot_grant.hpp"

#include <array>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace golarion
{
    class ResourceManager;

    std::string spellLevelOverridesResource(std::string_view castingId);
    std::string preparationChecksResource(std::string_view castingId);
    std::string preparationCheckExemptionsResource(std::string_view castingId);
    std::string consumptionExemptionsResource(std::string_view castingId);
    std::string spontaneousConversionsResource(std::string_view castingId);

    // An override is local to one group, not a change to the catalog or repertoire.
    struct SpellLevelOverride
    {
        std::string id;
        std::string source;
        std::string groupId;
        std::string spellId;
        int spellLevel;
    };

    struct SpellPreparationContext
    {
        CharacterClassType spellListClass;
        const SpellDefinition &spell;
        const SpellSlotGrant &group;
        const SpellSlotId &slot;
        int spellLevel;
    };

    // Callbacks are read-only queries. Captured providers must outlive their entries.
    // They must not mutate casting state or register/remove contributions during a query.
    struct SpellPreparationCheck
    {
        std::string id;
        std::string source;
        std::function<bool(const SpellPreparationContext &)> isSatisfied;
    };

    struct SpellPreparationCheckExemption
    {
        std::string id;
        std::string source;
        std::string checkId;
        std::function<bool(const SpellPreparationContext &)> applies;
    };

    struct SpellConsumptionContext
    {
        CharacterClassType spellListClass;
        const SpellDefinition &spell;
        std::span<const SpellSlotId> slots;
    };

    struct SpellConsumptionExemption
    {
        std::string id;
        std::string source;
        std::function<bool(const SpellConsumptionContext &)> applies;
    };

    struct SpellConversionContext
    {
        CharacterClassType spellListClass;
        const SpellDefinition &preparedSpell;
        const SpellDefinition &replacementSpell;
        std::span<const SpellSlotId> slots;
        const std::map<std::string, SpellSlotGrant> &slotPools;
    };

    struct SpontaneousConversion
    {
        std::string id;
        std::string source;
        // nullopt denies this conversion. Otherwise return the replacement's required
        // slot level. One rule must authorize the entire linked preparation, including
        // all source-group restrictions. The callback is a read-only query.
        std::function<std::optional<int>(const SpellConversionContext &)> replacementLevel;
    };

    class SpellPreparationRules final
    {
    public:
        explicit SpellPreparationRules(ResourceManager &resources, std::string_view castingId);
        ~SpellPreparationRules();
        SpellPreparationRules(const SpellPreparationRules &) = delete;
        SpellPreparationRules &operator=(const SpellPreparationRules &) = delete;
        SpellPreparationRules(SpellPreparationRules &&) = delete;
        SpellPreparationRules &operator=(SpellPreparationRules &&) = delete;

        std::optional<int> levelOverride(std::string_view groupId, std::string_view spellId) const;
        void validatePreparation(const SpellPreparationContext &context) const;
        bool preservesPreparation(const SpellConsumptionContext &context) const;
        bool allowsConversion(const SpellConversionContext &context, const std::function<bool(int)> &acceptsLevel = {}) const;

    private:
        ResourceManager &resources_;
        std::array<std::string, 5> endpoints_;
        std::map<std::string, SpellLevelOverride> levels_;
        std::map<std::string, SpellPreparationCheck> checks_;
        std::map<std::string, SpellPreparationCheckExemption> checkExemptions_;
        std::map<std::string, SpellConsumptionExemption> consumptionExemptions_;
        std::map<std::string, SpontaneousConversion> conversions_;
    };
}
