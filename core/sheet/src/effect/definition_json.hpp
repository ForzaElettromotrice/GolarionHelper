#pragma once

#include "golarion/effect/effect_definition.hpp"
#include "golarion/equipment/item.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace golarion::definition_json
{
    using Json = nlohmann::json;

    inline std::string_view enumName(ModifierType value)
    {
        switch (value)
        {
            case ModifierType::Bonus: return "bonus";
            case ModifierType::Penalty: return "penalty";
        }
        throw std::invalid_argument("unknown ModifierType value");
    }

    inline std::string_view enumName(BonusType value)
    {
        switch (value)
        {
            case BonusType::Generic: return "generic";
            case BonusType::Alchemical: return "alchemical";
            case BonusType::Armor: return "armor";
            case BonusType::NaturalArmor: return "naturalArmor";
            case BonusType::Circumstance: return "circumstance";
            case BonusType::Insight: return "insight";
            case BonusType::Competence: return "competence";
            case BonusType::Deflection: return "deflection";
            case BonusType::Luck: return "luck";
            case BonusType::Inherent: return "inherent";
            case BonusType::Morale: return "morale";
            case BonusType::Enhancement: return "enhancement";
            case BonusType::Profane: return "profane";
            case BonusType::Racial: return "racial";
            case BonusType::Resistance: return "resistance";
            case BonusType::Sacred: return "sacred";
            case BonusType::Dodge: return "dodge";
            case BonusType::Shield: return "shield";
            case BonusType::Size: return "size";
        }
        throw std::invalid_argument("unknown BonusType value");
    }

    inline std::string_view enumName(SizeCategory value)
    {
        switch (value)
        {
            case SizeCategory::Fine: return "fine";
            case SizeCategory::Diminutive: return "diminutive";
            case SizeCategory::Tiny: return "tiny";
            case SizeCategory::Small: return "small";
            case SizeCategory::Medium: return "medium";
            case SizeCategory::Large: return "large";
            case SizeCategory::Huge: return "huge";
            case SizeCategory::Gargantuan: return "gargantuan";
            case SizeCategory::Colossal: return "colossal";
        }
        throw std::invalid_argument("unknown SizeCategory value");
    }

    inline std::string_view enumName(MovementType value)
    {
        switch (value)
        {
            case MovementType::Land: return "land";
            case MovementType::Climb: return "climb";
            case MovementType::Swim: return "swim";
            case MovementType::Burrow: return "burrow";
            case MovementType::Fly: return "fly";
        }
        throw std::invalid_argument("unknown MovementType value");
    }

    inline std::string_view enumName(Maneuverability value)
    {
        switch (value)
        {
            case Maneuverability::Clumsy: return "clumsy";
            case Maneuverability::Poor: return "poor";
            case Maneuverability::Average: return "average";
            case Maneuverability::Good: return "good";
            case Maneuverability::Perfect: return "perfect";
        }
        throw std::invalid_argument("unknown Maneuverability value");
    }

    inline std::string_view enumName(CarryingBodyType value)
    {
        switch (value)
        {
            case CarryingBodyType::Biped: return "biped";
            case CarryingBodyType::Quadruped: return "quadruped";
        }
        throw std::invalid_argument("unknown CarryingBodyType value");
    }

    inline std::string_view enumName(AbilityType value)
    {
        switch (value)
        {
            case AbilityType::Strength: return "str";
            case AbilityType::Dexterity: return "dex";
            case AbilityType::Constitution: return "con";
            case AbilityType::Intelligence: return "int";
            case AbilityType::Wisdom: return "wis";
            case AbilityType::Charisma: return "cha";
        }
        throw std::invalid_argument("unknown AbilityType value");
    }

    inline std::string_view enumName(DamageType value)
    {
        switch (value)
        {
            case DamageType::Acid: return "acid";
            case DamageType::Cold: return "cold";
            case DamageType::Electricity: return "electricity";
            case DamageType::Fire: return "fire";
            case DamageType::Sonic: return "sonic";
            case DamageType::Force: return "force";
            case DamageType::PositiveEnergy: return "positiveEnergy";
            case DamageType::NegativeEnergy: return "negativeEnergy";
            case DamageType::Bludgeoning: return "bludgeoning";
            case DamageType::Piercing: return "piercing";
            case DamageType::Slashing: return "slashing";
            case DamageType::Untyped: return "untyped";
        }
        throw std::invalid_argument("unknown DamageType value");
    }

    inline std::string_view enumName(ArmorClassSuppressionCause value)
    {
        switch (value)
        {
            case ArmorClassSuppressionCause::Other: return "other";
            case ArmorClassSuppressionCause::FlatFooted: return "flatFooted";
            case ArmorClassSuppressionCause::InvisibleAttacker: return "invisibleAttacker";
            case ArmorClassSuppressionCause::Feint: return "feint";
            case ArmorClassSuppressionCause::Pinned: return "pinned";
        }
        throw std::invalid_argument("unknown ArmorClassSuppressionCause value");
    }

    inline std::string_view enumName(ActionCost value)
    {
        switch (value)
        {
            case ActionCost::Standard: return "standard";
            case ActionCost::Move: return "move";
            case ActionCost::FullRound: return "fullRound";
            case ActionCost::Swift: return "swift";
            case ActionCost::Immediate: return "immediate";
            case ActionCost::Free: return "free";
            case ActionCost::ReplacesAttack: return "replacesAttack";
            case ActionCost::NotAnAction: return "notAnAction";
        }
        throw std::invalid_argument("unknown ActionCost value");
    }

    inline std::string_view enumName(ArmorClassType value)
    {
        switch (value)
        {
            case ArmorClassType::Normal: return "normal";
            case ArmorClassType::Touch: return "touch";
            case ArmorClassType::FlatFooted: return "flatFooted";
        }
        throw std::invalid_argument("unknown ArmorClassType value");
    }

    inline std::string_view enumName(WeaponWeight value)
    {
        switch (value)
        {
            case WeaponWeight::Light: return "light";
            case WeaponWeight::OneHanded: return "oneHanded";
            case WeaponWeight::TwoHanded: return "twoHanded";
        }
        throw std::invalid_argument("unknown WeaponWeight value");
    }

    inline std::string_view enumName(DamageAbilityRule value)
    {
        switch (value)
        {
            case DamageAbilityRule::None: return "none";
            case DamageAbilityRule::Full: return "full";
            case DamageAbilityRule::HalfPositiveFullPenalty: return "halfPositiveFullPenalty";
            case DamageAbilityRule::OneAndHalfPositiveFullPenalty: return "oneAndHalfPositiveFullPenalty";
            case DamageAbilityRule::PenaltyOnly: return "penaltyOnly";
        }
        throw std::invalid_argument("unknown DamageAbilityRule value");
    }

    inline std::string_view enumName(DamageTrait value)
    {
        switch (value)
        {
            case DamageTrait::Precision: return "precision";
            case DamageTrait::Bleed: return "bleed";
            case DamageTrait::NonLethal: return "nonLethal";
        }
        throw std::invalid_argument("unknown DamageTrait value");
    }

    inline std::string_view enumName(DamageComponentRole value)
    {
        switch (value)
        {
            case DamageComponentRole::Base: return "base";
            case DamageComponentRole::Additional: return "additional";
        }
        throw std::invalid_argument("unknown DamageComponentRole value");
    }

    inline std::string_view enumName(DamageTypeMode value)
    {
        switch (value)
        {
            case DamageTypeMode::All: return "all";
            case DamageTypeMode::Choice: return "choice";
        }
        throw std::invalid_argument("unknown DamageTypeMode value");
    }

    inline std::string_view enumName(DamageCriticalRule value)
    {
        switch (value)
        {
            case DamageCriticalRule::Multiplied: return "multiplied";
            case DamageCriticalRule::NotMultiplied: return "notMultiplied";
            case DamageCriticalRule::CriticalOnly: return "criticalOnly";
        }
        throw std::invalid_argument("unknown DamageCriticalRule value");
    }

    inline std::string_view enumName(AttackTag value)
    {
        switch (value)
        {
            case AttackTag::Weapon: return "weapon";
            case AttackTag::Thrown: return "thrown";
            case AttackTag::Projectile: return "projectile";
            case AttackTag::Natural: return "natural";
            case AttackTag::Unarmed: return "unarmed";
        }
        throw std::invalid_argument("unknown AttackTag value");
    }

    inline std::string_view enumName(RoutineAttackProgressionType value)
    {
        switch (value)
        {
            case RoutineAttackProgressionType::Fixed: return "fixed";
            case RoutineAttackProgressionType::BaseAttackBonusIteratives: return "baseAttackBonusIteratives";
        }
        throw std::invalid_argument("unknown RoutineAttackProgressionType value");
    }

    inline std::string_view enumName(WeaponWeightPurpose value)
    {
        switch (value)
        {
            case WeaponWeightPurpose::General: return "general";
            case WeaponWeightPurpose::TwoWeaponFighting: return "twoWeaponFighting";
        }
        throw std::invalid_argument("unknown WeaponWeightPurpose value");
    }

    inline std::string_view enumName(AttackMode value)
    {
        switch (value)
        {
            case AttackMode::Melee: return "melee";
            case AttackMode::Ranged: return "ranged";
        }
        throw std::invalid_argument("unknown AttackMode value");
    }

    inline std::string_view enumName(RoutineSlotSelectionMode value)
    {
        switch (value)
        {
            case RoutineSlotSelectionMode::ChooseOne: return "chooseOne";
            case RoutineSlotSelectionMode::AllMatching: return "allMatching";
        }
        throw std::invalid_argument("unknown RoutineSlotSelectionMode value");
    }

    inline std::string_view enumName(StrikeUsage value)
    {
        switch (value)
        {
            case StrikeUsage::Default: return "default";
            case StrikeUsage::NaturalSecondary: return "naturalSecondary";
            case StrikeUsage::SingleNatural: return "singleNatural";
            case StrikeUsage::NaturalSecondaryWhenCombined: return "naturalSecondaryWhenCombined";
        }
        throw std::invalid_argument("unknown StrikeUsage value");
    }

    inline std::string_view enumName(HandUsage value)
    {
        switch (value)
        {
            case HandUsage::OneHanded: return "oneHanded";
            case HandUsage::TwoHanded: return "twoHanded";
        }
        throw std::invalid_argument("unknown HandUsage value");
    }

    inline std::string_view enumName(AttackHandRole value)
    {
        switch (value)
        {
            case AttackHandRole::Primary: return "primary";
            case AttackHandRole::OffHand: return "offHand";
        }
        throw std::invalid_argument("unknown AttackHandRole value");
    }

    inline std::string_view enumName(AbilityReplacementStage value)
    {
        switch (value)
        {
            case AbilityReplacementStage::Base: return "base";
            case AbilityReplacementStage::Final: return "final";
        }
        throw std::invalid_argument("unknown AbilityReplacementStage value");
    }

    inline std::string_view enumName(CriticalAdjustmentType value)
    {
        switch (value)
        {
            case CriticalAdjustmentType::ThreatRangeMultiplier: return "threatRangeMultiplier";
            case CriticalAdjustmentType::ThreatMinimum: return "threatMinimum";
            case CriticalAdjustmentType::MultiplierIncrease: return "multiplierIncrease";
            case CriticalAdjustmentType::MultiplierSet: return "multiplierSet";
        }
        throw std::invalid_argument("unknown CriticalAdjustmentType value");
    }

    inline std::string_view enumName(AttackDistanceAdjustmentType value)
    {
        switch (value)
        {
            case AttackDistanceAdjustmentType::Multiplier: return "multiplier";
            case AttackDistanceAdjustmentType::Minimum: return "minimum";
            case AttackDistanceAdjustmentType::Maximum: return "maximum";
        }
        throw std::invalid_argument("unknown AttackDistanceAdjustmentType value");
    }

    inline std::string_view enumName(DamageDiceAdjustmentType value)
    {
        switch (value)
        {
            case DamageDiceAdjustmentType::ProgressionSteps: return "progressionSteps";
            case DamageDiceAdjustmentType::DiceCountMultiplier: return "diceCountMultiplier";
            case DamageDiceAdjustmentType::Set: return "set";
        }
        throw std::invalid_argument("unknown DamageDiceAdjustmentType value");
    }

    inline std::string_view enumName(DamageComponentOriginFilter value)
    {
        switch (value)
        {
            case DamageComponentOriginFilter::Any: return "any";
            case DamageComponentOriginFilter::Intrinsic: return "intrinsic";
            case DamageComponentOriginFilter::ExternalGrant: return "externalGrant";
        }
        throw std::invalid_argument("unknown DamageComponentOriginFilter value");
    }

    inline std::string_view enumName(NaturalAttackClassification value)
    {
        switch (value)
        {
            case NaturalAttackClassification::Primary: return "primary";
            case NaturalAttackClassification::Secondary: return "secondary";
        }
        throw std::invalid_argument("unknown NaturalAttackClassification value");
    }

    inline std::string_view enumName(DamageReductionBypassTrait value)
    {
        switch (value)
        {
            case DamageReductionBypassTrait::Bludgeoning: return "bludgeoning";
            case DamageReductionBypassTrait::Piercing: return "piercing";
            case DamageReductionBypassTrait::Slashing: return "slashing";
            case DamageReductionBypassTrait::Magic: return "magic";
            case DamageReductionBypassTrait::Epic: return "epic";
            case DamageReductionBypassTrait::Adamantine: return "adamantine";
            case DamageReductionBypassTrait::Silver: return "silver";
            case DamageReductionBypassTrait::ColdIron: return "coldIron";
            case DamageReductionBypassTrait::Good: return "good";
            case DamageReductionBypassTrait::Evil: return "evil";
            case DamageReductionBypassTrait::Lawful: return "lawful";
            case DamageReductionBypassTrait::Chaotic: return "chaotic";
        }
        throw std::invalid_argument("unknown DamageReductionBypassTrait value");
    }

    inline std::string_view enumName(MovementAdjustmentType value)
    {
        switch (value)
        {
            case MovementAdjustmentType::SpeedMultiplier: return "speedMultiplier";
            case MovementAdjustmentType::ReducedByArmorOrLoad: return "reducedByArmorOrLoad";
            case MovementAdjustmentType::SpeedLimit: return "speedLimit";
            case MovementAdjustmentType::ManeuverabilityChange: return "maneuverabilityChange";
            case MovementAdjustmentType::Block: return "block";
        }
        throw std::invalid_argument("unknown MovementAdjustmentType value");
    }

    inline std::string_view enumName(RunAdjustmentType value)
    {
        switch (value)
        {
            case RunAdjustmentType::Increase: return "increase";
            case RunAdjustmentType::Penalty: return "penalty";
            case RunAdjustmentType::Replacement: return "replacement";
            case RunAdjustmentType::Block: return "block";
        }
        throw std::invalid_argument("unknown RunAdjustmentType value");
    }

    inline std::string_view enumName(MaximumDexterityLimitType value)
    {
        switch (value)
        {
            case MaximumDexterityLimitType::Armor: return "armor";
            case MaximumDexterityLimitType::Shield: return "shield";
            case MaximumDexterityLimitType::Load: return "load";
            case MaximumDexterityLimitType::Other: return "other";
        }
        throw std::invalid_argument("unknown MaximumDexterityLimitType value");
    }

    inline std::string_view enumName(SavingThrowType value)
    {
        switch (value)
        {
            case SavingThrowType::Fortitude: return "fortitude";
            case SavingThrowType::Reflex: return "reflex";
            case SavingThrowType::Will: return "will";
        }
        throw std::invalid_argument("unknown SavingThrowType value");
    }

    inline std::string_view enumName(SpellSchool value)
    {
        switch (value)
        {
            case SpellSchool::Abjuration: return "abjuration";
            case SpellSchool::Conjuration: return "conjuration";
            case SpellSchool::Divination: return "divination";
            case SpellSchool::Enchantment: return "enchantment";
            case SpellSchool::Evocation: return "evocation";
            case SpellSchool::Illusion: return "illusion";
            case SpellSchool::Necromancy: return "necromancy";
            case SpellSchool::Transmutation: return "transmutation";
            case SpellSchool::Universal: return "universal";
        }
        throw std::invalid_argument("unknown SpellSchool value");
    }

    inline std::string_view enumName(MetamagicPhase value)
    {
        switch (value)
        {
            case MetamagicPhase::Preparation: return "preparation";
            case MetamagicPhase::Casting: return "casting";
        }
        throw std::invalid_argument("unknown MetamagicPhase value");
    }

    inline std::string_view enumName(MetamagicType value)
    {
        switch (value)
        {
            case MetamagicType::Empower: return "empower";
            case MetamagicType::Enlarge: return "enlarge";
            case MetamagicType::Extend: return "extend";
            case MetamagicType::Heighten: return "heighten";
            case MetamagicType::Maximize: return "maximize";
            case MetamagicType::Quicken: return "quicken";
            case MetamagicType::Silent: return "silent";
            case MetamagicType::Still: return "still";
            case MetamagicType::Widen: return "widen";
            case MetamagicType::Elemental: return "elemental";
            case MetamagicType::Reach: return "reach";
            case MetamagicType::Intensified: return "intensified";
            case MetamagicType::Selective: return "selective";
        }
        throw std::invalid_argument("unknown MetamagicType value");
    }

    inline std::string_view enumName(MetamagicSlotCostPolicy value)
    {
        switch (value)
        {
            case MetamagicSlotCostPolicy::Standard: return "standard";
            case MetamagicSlotCostPolicy::Waived: return "waived";
        }
        throw std::invalid_argument("unknown MetamagicSlotCostPolicy value");
    }

    inline std::string_view enumName(MetamagicRodGrade value)
    {
        switch (value)
        {
            case MetamagicRodGrade::Lesser: return "lesser";
            case MetamagicRodGrade::Normal: return "normal";
            case MetamagicRodGrade::Greater: return "greater";
        }
        throw std::invalid_argument("unknown MetamagicRodGrade value");
    }

    inline std::string_view enumName(CastingMetamagicScope value)
    {
        switch (value)
        {
            case CastingMetamagicScope::AnyCast: return "anyCast";
            case CastingMetamagicScope::SpontaneousConversionOnly: return "spontaneousConversionOnly";
        }
        throw std::invalid_argument("unknown CastingMetamagicScope value");
    }

    inline std::string_view enumName(EquipmentSlot value)
    {
        switch (value)
        {
            case EquipmentSlot::Ring: return "ring";
            case EquipmentSlot::Armor: return "armor";
            case EquipmentSlot::Belt: return "belt";
            case EquipmentSlot::Neck: return "neck";
            case EquipmentSlot::Body: return "body";
            case EquipmentSlot::Headband: return "headband";
            case EquipmentSlot::Hands: return "hands";
            case EquipmentSlot::Eyes: return "eyes";
            case EquipmentSlot::Feet: return "feet";
            case EquipmentSlot::Wrists: return "wrists";
            case EquipmentSlot::Shield: return "shield";
            case EquipmentSlot::Shoulders: return "shoulders";
            case EquipmentSlot::Head: return "head";
            case EquipmentSlot::Chest: return "chest";
        }
        throw std::invalid_argument("unknown EquipmentSlot value");
    }

    inline std::string_view enumName(CoinDenomination value)
    {
        switch (value)
        {
            case CoinDenomination::Copper: return "copper";
            case CoinDenomination::Silver: return "silver";
            case CoinDenomination::Gold: return "gold";
            case CoinDenomination::Platinum: return "platinum";
        }
        throw std::invalid_argument("unknown CoinDenomination value");
    }

    inline std::string_view enumName(ItemEffectActivation value)
    {
        switch (value)
        {
            case ItemEffectActivation::Possessed: return "possessed";
            case ItemEffectActivation::Equipped: return "equipped";
        }
        throw std::invalid_argument("unknown ItemEffectActivation value");
    }

    inline std::string_view enumName(SkillRankBudgetPolicy value)
    {
        switch (value)
        {
            case SkillRankBudgetPolicy::Included: return "included";
            case SkillRankBudgetPolicy::Excluded: return "excluded";
        }
        throw std::invalid_argument("unknown SkillRankBudgetPolicy value");
    }

    inline std::string_view enumName(LanguageOrigin value)
    {
        switch (value)
        {
            case LanguageOrigin::Intelligence: return "intelligence";
            case LanguageOrigin::Linguistics: return "linguistics";
        }
        throw std::invalid_argument("unknown LanguageOrigin value");
    }

    inline std::string_view enumName(SpellcastingMode value)
    {
        switch (value)
        {
            case SpellcastingMode::Prepared: return "prepared";
            case SpellcastingMode::Spontaneous: return "spontaneous";
            case SpellcastingMode::Flexible: return "flexible";
        }
        throw std::invalid_argument("unknown SpellcastingMode value");
    }

    inline std::string_view enumName(SpellSlotPoolType value)
    {
        switch (value)
        {
            case SpellSlotPoolType::Casting: return "casting";
            case SpellSlotPoolType::Preparation: return "preparation";
        }
        throw std::invalid_argument("unknown SpellSlotPoolType value");
    }

    inline std::string_view enumName(GrantedSpellUsage value)
    {
        switch (value)
        {
            case GrantedSpellUsage::Limited: return "limited";
            case GrantedSpellUsage::AtWill: return "atWill";
            case GrantedSpellUsage::Constant: return "constant";
            case GrantedSpellUsage::SharedPool: return "sharedPool";
        }
        throw std::invalid_argument("unknown GrantedSpellUsage value");
    }

    inline std::string_view enumName(FavoredClassBaseBonus value)
    {
        switch (value)
        {
            case FavoredClassBaseBonus::HitPoint: return "hitPoint";
            case FavoredClassBaseBonus::SkillRank: return "skillRank";
        }
        throw std::invalid_argument("unknown FavoredClassBaseBonus value");
    }

    inline std::string_view enumName(FeatType value)
    {
        switch (value)
        {
            case FeatType::General: return "general";
            case FeatType::Combat: return "combat";
            case FeatType::Critical: return "critical";
            case FeatType::ItemCreation: return "itemCreation";
            case FeatType::Metamagic: return "metamagic";
            case FeatType::Teamwork: return "teamwork";
            case FeatType::Style: return "style";
            case FeatType::Achievement: return "achievement";
            case FeatType::Channeling: return "channeling";
            case FeatType::Grit: return "grit";
            case FeatType::Panache: return "panache";
            case FeatType::Performance: return "performance";
            case FeatType::Racial: return "racial";
            case FeatType::Story: return "story";
            case FeatType::Mythic: return "mythic";
            case FeatType::Monster: return "monster";
            case FeatType::Stare: return "stare";
            case FeatType::HeroPoint: return "heroPoint";
            case FeatType::Alignment: return "alignment";
        }
        throw std::invalid_argument("unknown FeatType value");
    }
    inline std::string_view enumName(CharacterClassType value) { return characterClassId(value); }

    template<class T> Json value(const T &entry)
    {
        if constexpr (std::is_enum_v<T>) return enumName(entry);
        else return entry;
    }
    template<class T> Json value(const std::vector<T> &entries)
    {
        Json result = Json::array();
        for (const auto &entry : entries) result.push_back(value(entry));
        return result;
    }
    template<class T> Json value(const std::set<T> &entries)
    {
        Json result = Json::array();
        for (const auto &entry : entries) result.push_back(value(entry));
        return result;
    }
    template<class T> void put(Json &json, const char *name, const T &entry) { json[name] = value(entry); }
    template<class T> void put(Json &json, const char *name, const std::optional<T> &entry)
    {
        if (entry) json[name] = value(*entry);
    }
    inline Json requirements(const std::vector<Requirement> &entries)
    {
        Json result = Json::array();
        for (const auto &entry : entries) result.push_back(Json{{"expression", entry.expression()}, {"failureReason", entry.failureReason()}});
        return result;
    }
}
