#pragma once

#include "golarion/character/damage.hpp"
#include "golarion/character/damage_reduction.hpp"
#include "golarion/character/fast_healing.hpp"
#include "golarion/character/immunity.hpp"
#include "golarion/character/regeneration.hpp"
#include "golarion/character/spell_resistance.hpp"

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    inline constexpr std::string_view EnergyResistanceGrantsResource = "specialDefense.energyResistance.grants";
    inline constexpr std::string_view EnergyResistanceAdjustmentsResource = "specialDefense.energyResistance.adjustments";
    inline constexpr std::string_view DamageReductionGrantsResource = "specialDefense.damageReduction.grants";
    inline constexpr std::string_view DamageReductionAdjustmentsResource = "specialDefense.damageReduction.adjustments";
    inline constexpr std::string_view ImmunityGrantsResource = "specialDefense.immunity.grants";
    inline constexpr std::string_view SpellResistanceGrantsResource = "specialDefense.spellResistance.grants";
    inline constexpr std::string_view SpellResistanceAdjustmentsResource = "specialDefense.spellResistance.adjustments";
    inline constexpr std::string_view FastHealingGrantsResource = "specialDefense.fastHealing.grants";
    inline constexpr std::string_view RegenerationGrantsResource = "specialDefense.regeneration.grants";

    class ResourceManager;
    struct SpecialDefensesView;

    struct EnergyResistanceDefinition
    {
        std::string id;
        std::string source;
        DamageType energy;
        std::string expression;
        std::vector<std::string> tags{};
    };

    class EnergyResistance final
    {
    public:
        explicit EnergyResistance(EnergyResistanceDefinition definition);

    private:
        friend class EnergyResistanceSelector;
        friend class SpecialDefenses;

        std::string id_;
        std::string source_;
        DamageType energy_;
        std::string expression_;
        std::vector<std::string> tags_;
    };

    struct EnergyResistanceSelectorDefinition
    {
        std::optional<DamageType> energy = std::nullopt;
        std::optional<std::string> grantId = std::nullopt;
        std::vector<std::string> anyTags{};
        std::vector<std::string> excludedGrantIds{};
    };

    class EnergyResistanceSelector final
    {
    public:
        explicit EnergyResistanceSelector(EnergyResistanceSelectorDefinition definition = {});

    private:
        friend class SpecialDefenses;

        bool matches(const EnergyResistance &resistance) const;

        std::optional<DamageType> energy_;
        std::optional<std::string> grantId_;
        std::vector<std::string> anyTags_;
        std::vector<std::string> excludedGrantIds_;
    };

    struct EnergyResistanceAdjustmentDefinition
    {
        std::string id;
        std::string source;
        EnergyResistanceSelector selector;
        std::string expression;
        std::string stackingGroup;
    };

    class EnergyResistanceAdjustment final
    {
    public:
        explicit EnergyResistanceAdjustment(EnergyResistanceAdjustmentDefinition definition);

    private:
        friend class SpecialDefenses;

        std::string id_;
        std::string source_;
        EnergyResistanceSelector selector_;
        std::string expression_;
        std::string stackingGroup_;
    };

    struct DamageReductionDefinition
    {
        std::string id;
        std::string source;
        std::string expression;
        DamageReductionBypass bypass;
        std::optional<std::string> applicability = std::nullopt;
        std::vector<std::string> tags{};
        std::vector<std::string> stacksWithTags{};
    };

    class DamageReduction final
    {
    public:
        explicit DamageReduction(DamageReductionDefinition definition);

    private:
        friend class DamageReductionSelector;
        friend class SpecialDefenses;

        std::string id_;
        std::string source_;
        std::string expression_;
        DamageReductionBypass bypass_;
        std::optional<std::string> applicability_;
        std::vector<std::string> tags_;
        std::vector<std::string> stacksWithTags_;
    };

    struct DamageReductionSelectorDefinition
    {
        std::optional<std::string> grantId = std::nullopt;
        std::vector<std::string> anyTags{};
        std::vector<std::string> excludedGrantIds{};
    };

    class DamageReductionSelector final
    {
    public:
        explicit DamageReductionSelector(DamageReductionSelectorDefinition definition = {});

    private:
        friend class SpecialDefenses;

        bool matches(const DamageReduction &damageReduction) const;

        std::optional<std::string> grantId_;
        std::vector<std::string> anyTags_;
        std::vector<std::string> excludedGrantIds_;
    };

    struct DamageReductionAdjustmentDefinition
    {
        std::string id;
        std::string source;
        DamageReductionSelector selector;
        std::string expression;
        std::string stackingGroup;
    };

    class DamageReductionAdjustment final
    {
    public:
        explicit DamageReductionAdjustment(DamageReductionAdjustmentDefinition definition);

    private:
        friend class SpecialDefenses;

        std::string id_;
        std::string source_;
        DamageReductionSelector selector_;
        std::string expression_;
        std::string stackingGroup_;
    };

    class SpecialDefenses final
    {
    public:
        explicit SpecialDefenses(ResourceManager &resourceManager);

        SpecialDefenses(const SpecialDefenses &) = delete;
        SpecialDefenses &operator=(const SpecialDefenses &) = delete;
        SpecialDefenses(SpecialDefenses &&) = delete;
        SpecialDefenses &operator=(SpecialDefenses &&) = delete;

        SpecialDefensesView toView();

    private:
        void addEnergyResistance(EnergyResistance resistance);
        void removeEnergyResistance(std::string_view resistanceId);
        void addEnergyResistanceAdjustment(EnergyResistanceAdjustment adjustment);
        void removeEnergyResistanceAdjustment(std::string_view adjustmentId);
        void addDamageReduction(DamageReduction damageReduction);
        void removeDamageReduction(std::string_view damageReductionId);
        void addDamageReductionAdjustment(DamageReductionAdjustment adjustment);
        void removeDamageReductionAdjustment(std::string_view adjustmentId);
        void addImmunity(Immunity immunity);
        void removeImmunity(std::string_view immunityId);
        void addSpellResistance(SpellResistance spellResistance);
        void removeSpellResistance(std::string_view spellResistanceId);
        void addSpellResistanceAdjustment(SpellResistanceAdjustment adjustment);
        void removeSpellResistanceAdjustment(std::string_view adjustmentId);
        void addFastHealing(FastHealing fastHealing);
        void removeFastHealing(std::string_view fastHealingId);
        void addRegeneration(Regeneration regeneration);
        void removeRegeneration(std::string_view regenerationId);

        ResourceManager &resourceManager_;
        std::map<std::string, EnergyResistance> energyResistances_;
        std::map<std::string, EnergyResistanceAdjustment> energyResistanceAdjustments_;
        std::map<std::string, DamageReduction> damageReductions_;
        std::map<std::string, DamageReductionAdjustment> damageReductionAdjustments_;
        std::map<std::string, Immunity> immunities_;
        std::map<std::string, SpellResistance> spellResistances_;
        std::map<std::string, SpellResistanceAdjustment> spellResistanceAdjustments_;
        std::map<std::string, FastHealing> fastHealingGrants_;
        std::map<std::string, Regeneration> regenerationGrants_;
    };
}
