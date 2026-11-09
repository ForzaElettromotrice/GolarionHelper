#pragma once

#include "golarion/character/spellcasting.hpp"
#include "golarion/character/spell_preparation_cost.hpp"
#include "golarion/character/spell_preparation_rules.hpp"
#include "golarion/character/prepared_spell_slot.hpp"
#include "golarion/data/prepared_spellcasting_data.hpp"
#include "golarion/view/prepared_spellcasting_view.hpp"

#include <functional>

namespace golarion
{
    class PreparedSpellcasting final : public Spellcasting
    {
    public:
        // Resolve the effective level for this list/group; nullopt means unavailable.
        // With no explicit group filter, the resolver must enforce list membership.
        // If omitted, preparation reads SpellDefinition::spellLevelByClass from the catalog.
        // Registered group/spell level overrides take precedence over this fallback.
        using SpellLevelResolver = std::function<std::optional<int>(CharacterClassType, const SpellSlotGrant &, std::string_view)>;

        explicit PreparedSpellcasting(ResourceManager &resources, std::string_view castingId, CharacterClassType spellListClass, SpellLevelResolver resolveLevel = {});

        void prepare(const SpellSlotId &slot, std::string_view spellId, const std::vector<MetamagicApplication> &metamagic = {});
        void prepare(const std::vector<SpellSlotId> &slots, std::string_view spellId, const std::vector<MetamagicApplication> &metamagic = {});
        MetamagicEvaluation consume(const SpellSlotId &slot, std::optional<std::string_view> replacementSpellId = std::nullopt, const std::vector<MetamagicApplication> &metamagic = {});
        void clear(const SpellSlotId &slot);
        PreparedSpellSlot slotState(const SpellSlotId &slot) const;
        PreparedSpellcastingData toData() const;
        void load(const PreparedSpellcastingData &data);
        PreparedSpellcastingView toView() const;

    private:
        friend class Spellcastings;
        std::function<void()> stageLoad(const PreparedSpellcastingData &data);
        SpellSlotId checkedSlot(const SpellSlotId &slot) const;
        enum class PreparationValidation { Preparing, Restoring };
        struct Preparation;
        int preparationLevel(const SpellSlotId &slot, const std::string &spellId) const;
        Preparation checkedPreparation(const std::vector<SpellSlotId> &slots, const std::string &spellId, PreparationValidation mode, const std::vector<MetamagicApplication> &metamagic) const;
        void onSlotGroupRemoved(std::string_view id) noexcept override;

        SpellLevelResolver resolveLevel_;
        SpellPreparationCostRegistry preparationCosts_;
        SpellPreparationRules preparationRules_;
        struct Preparation
        {
            PreparedSpellSlot state;
            std::vector<SpellSlotId> slots;
            MetamagicEvaluation evaluation;
        };
        std::map<SpellSlotId, Preparation> preparations_;
    };
}
