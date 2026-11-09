#pragma once

#include "golarion/character/spellcasting.hpp"
#include "golarion/character/spell_preparation_rules.hpp"
#include "golarion/data/flexible_spellcasting_data.hpp"
#include "golarion/view/flexible_spellcasting_view.hpp"

namespace golarion
{
    std::string preparationSlotsResource(std::string_view castingId);
    std::string metamagicMixingPermissionsResource(std::string_view castingId);

    struct MetamagicMixingContext
    {
        CharacterClassType spellListClass;
        const SpellDefinition &spell;
        const SpellSlotId &preparationId;
        const SpellSlotId &slotId;
        const MetamagicEvaluation &prepared;
        std::span<const MetamagicApplication> additions;
    };

    struct MetamagicMixingPermission
    {
        std::string id;
        std::string source;
        // Pure query. Does not grant the individual metamagic applications.
        std::function<bool(const MetamagicMixingContext &)> applies;
        std::optional<MetamagicUsageCost> usageCost;
    };

    class FlexibleSpellcasting final : public Spellcasting
    {
    public:
        // Pure query used for both group kinds; must enforce list membership when unfiltered.
        using SpellLevelResolver = std::function<std::optional<int>(CharacterClassType, const SpellSlotGrant &, std::string_view)>;
        explicit FlexibleSpellcasting(ResourceManager &resources, std::string_view castingId, CharacterClassType spellListClass, SpellLevelResolver resolveLevel = {});
        ~FlexibleSpellcasting() override;

        void prepare(const SpellSlotId &preparationId, std::string_view spellId, const std::vector<MetamagicApplication> &metamagic = {});
        void clearPreparation(const SpellSlotId &preparationId);
        // A paid mixing permission must be selected explicitly. Otherwise only free permissions are considered.
        MetamagicEvaluation consume(const SpellSlotId &slotId, const SpellSlotId &preparationId, const std::vector<MetamagicApplication> &metamagic = {}, std::optional<std::string_view> mixingPermissionId = std::nullopt);
        void clearSlot(const SpellSlotId &slotId);
        FlexibleSpellcastingData toData() const;
        void load(const FlexibleSpellcastingData &data);
        FlexibleSpellcastingView toView() const;

    private:
        friend class Spellcastings;
        std::function<void()> stageLoad(const FlexibleSpellcastingData &data);
        SpellSlotId checkedSlot(const SpellSlotId &id, const std::map<std::string, SpellSlotGrant> &groups) const;
        int spellLevel(const SpellSlotGrant &group, const std::string &spellId) const;
        MetamagicEvaluation checkedPreparation(const SpellSlotId &id, const std::string &spellId, const std::vector<MetamagicApplication> &metamagic) const;
        void onSlotGroupRemoved(std::string_view id) noexcept override;
        ResourceManager &resources_;
        std::array<std::string, 3> endpoints_;
        SpellLevelResolver resolveLevel_;
        std::map<std::string, SpellSlotGrant> preparationGroups_;
        std::map<std::string, MetamagicMixingPermission> mixingPermissions_;
        std::map<std::string, SpellConsumptionExemption> exemptions_;
        std::map<SpellSlotId, MetamagicEvaluation> preparations_;
        std::set<SpellSlotId> consumedSlots_;
    };
}
