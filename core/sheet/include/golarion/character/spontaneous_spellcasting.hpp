#pragma once
#include "golarion/character/spell_operation_definition.hpp"

#include "golarion/character/spellcasting.hpp"
#include "golarion/character/spell_preparation_rules.hpp"
#include "golarion/data/spontaneous_spellcasting_data.hpp"
#include "golarion/view/spontaneous_spellcasting_view.hpp"

namespace golarion
{
    class SpellCatalog;
    class SpontaneousSpellcasting final : public Spellcasting
    {
    public:
        // Pure level/list query for special grants; omitted means catalog class-list lookup.
        // With no explicit slot filter, a custom resolver must enforce list membership.
        using SpellLevelResolver = std::function<std::optional<int>(CharacterClassType, const SpellSlotGrant &, std::string_view)>;
        explicit SpontaneousSpellcasting(ResourceManager &resources, std::string_view castingId, CharacterClassType spellListClass, SpellLevelResolver resolveLevel = {}, const SpellCatalog *catalog = nullptr);
        ~SpontaneousSpellcasting() override;

        SpellCastResult consume(const SpellSlotId &slot, const SpontaneousSpellConsumptionDefinition &definition);
        void clear(const SpellSlotId &slot);
        SpontaneousSpellSlotState slotState(const SpellSlotId &slot) const;
        SpontaneousSpellcastingData toData() const;
        void load(const SpontaneousSpellcastingData &data);
        SpontaneousSpellcastingView toView() const;

    private:
        friend class Spellcastings;
        std::function<void()> stageLoad(const SpontaneousSpellcastingData &data);
        SpellSlotId checkedSlot(const SpellSlotId &slot) const;
        void onSlotGroupRemoved(std::string_view id) noexcept override;
        ResourceManager &resources_;
        std::string exemptionsResource_;
        SpellLevelResolver resolveLevel_;
        std::map<std::string, SpellConsumptionExemption> exemptions_;
        std::set<SpellSlotId> consumedSlots_;
    };
}
