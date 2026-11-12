#include "collection_effect.hpp"
#include "golarion/util/string_utils.hpp"
#include <nlohmann/json.hpp>
#include <limits>

namespace
{
    using namespace golarion;
    using Json = nlohmann::json;
    std::string text(const Json &j, const char *key) { return normalize(j.at(key).get<std::string>()); }
    std::optional<std::string> optionalText(const Json &j, const char *key)
    {
        return j.contains(key) ? std::optional(text(j, key)) : std::nullopt;
    }
    int integer(const Json &j, const char *key, int fallback)
    {
        if (!j.contains(key)) return fallback;
        const auto &value = j.at(key);
        if (!value.is_number_integer() || value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
            throw std::invalid_argument("invalid integer effect field: " + std::string(key));
        return value.get<int>();
    }
    std::vector<std::string> strings(const Json &j, const char *key)
    {
        if (!j.contains(key)) return {};
        const auto &values = j.at(key);
        if (!values.is_array()) throw std::invalid_argument("effect field must be an array: " + std::string(key));
        std::vector<std::string> result;
        std::set<std::string> seen;
        for (const auto &value : values)
        {
            auto name = normalize(value.get<std::string>());
            if (!seen.insert(name).second) throw std::invalid_argument("duplicate effect array value");
            result.push_back(std::move(name));
        }
        return result;
    }
    template<class E> E enumeration(const Json &j, const char *key, std::initializer_list<std::pair<std::string_view, E>> values)
    {
        const auto name = text(j, key);
        for (const auto &[label, value] : values) if (label == name) return value;
        throw std::invalid_argument("unknown effect enum value for " + std::string(key) + ": " + name);
    }
    AbilityType ability(const Json &j)
    {
        return enumeration<AbilityType>(j, "ability", {{"str", AbilityType::Strength}, {"dex", AbilityType::Dexterity},
            {"con", AbilityType::Constitution}, {"int", AbilityType::Intelligence}, {"wis", AbilityType::Wisdom}, {"cha", AbilityType::Charisma}});
    }
    DamageType energy(const Json &j)
    {
        return enumeration<DamageType>(j, "energy", {{"acid", DamageType::Acid}, {"cold", DamageType::Cold},
            {"electricity", DamageType::Electricity}, {"fire", DamageType::Fire}, {"sonic", DamageType::Sonic},
            {"force", DamageType::Force}, {"positiveEnergy", DamageType::PositiveEnergy}, {"negativeEnergy", DamageType::NegativeEnergy}});
    }
    ArmorClassSuppressionCause cause(const Json &j, const char *key)
    {
        return enumeration<ArmorClassSuppressionCause>(j, key, {{"other", ArmorClassSuppressionCause::Other},
            {"flatFooted", ArmorClassSuppressionCause::FlatFooted}, {"invisibleAttacker", ArmorClassSuppressionCause::InvisibleAttacker},
            {"feint", ArmorClassSuppressionCause::Feint}, {"pinned", ArmorClassSuppressionCause::Pinned}});
    }
    MovementSelector movementSelector(const Json &j)
    {
        if (!j.contains("selector")) return {};
        const auto &s = j.at("selector");
        if (!s.is_object()) throw std::invalid_argument("movement selector must be an object");
        MovementSelector selector;
        if (s.contains("movementType")) selector.type = enumeration<MovementType>(s, "movementType", {
            {"land", MovementType::Land}, {"climb", MovementType::Climb}, {"swim", MovementType::Swim}, {"burrow", MovementType::Burrow}, {"fly", MovementType::Fly}});
        selector.grantId = optionalText(s, "grantId");
        selector.affectedByArmorOnly = s.value("affectedByArmorOnly", false);
        selector.affectedByLoadOnly = s.value("affectedByLoadOnly", false);
        return selector;
    }
    template<class D> D defenseSelector(const Json &j)
    {
        const auto s = j.value("selector", Json::object());
        if (!s.is_object()) throw std::invalid_argument("defense selector must be an object");
        D result;
        result.grantId = optionalText(s, "grantId");
        result.anyTags = strings(s, "anyTags");
        result.excludedGrantIds = strings(s, "excludedGrantIds");
        if constexpr (requires { result.energy; }) if (s.contains("energy")) result.energy = energy(s);
        return result;
    }
    std::vector<Requirement> requirements(const Json &j)
    {
        if (!j.contains("requirements")) return {};
        if (!j.at("requirements").is_array()) throw std::invalid_argument("requirements must be an array");
        std::vector<Requirement> result;
        for (const auto &r : j.at("requirements")) result.emplace_back(text(r, "expression"), text(r, "failureReason"));
        return result;
    }
    ActionCost actionCost(const Json &j)
    {
        using E = ActionCost;
        return enumeration<E>(j, "cost", {{"standard", E::Standard}, {"move", E::Move}, {"fullRound", E::FullRound}, {"swift", E::Swift},
            {"immediate", E::Immediate}, {"free", E::Free}, {"replacesAttack", E::ReplacesAttack}, {"notAnAction", E::NotAnAction}});
    }
    ArmorClassType defense(const Json &j)
    {
        using E = ArmorClassType;
        return enumeration<E>(j, "defense", {{"normal", E::Normal}, {"touch", E::Touch}, {"flatFooted", E::FlatFooted}});
    }
    WeaponWeight weight(const Json &j)
    {
        using E = WeaponWeight;
        return enumeration<E>(j, "weaponWeight", {{"light", E::Light}, {"oneHanded", E::OneHanded}, {"twoHanded", E::TwoHanded}});
    }
    DamageAbilityRule damageAbilityRule(const Json &j)
    {
        using E = DamageAbilityRule;
        return enumeration<E>(j, "damageAbilityRule", {{"none", E::None}, {"full", E::Full}, {"halfPositiveFullPenalty", E::HalfPositiveFullPenalty},
            {"oneAndHalfPositiveFullPenalty", E::OneAndHalfPositiveFullPenalty}, {"penaltyOnly", E::PenaltyOnly}});
    }
    DamageDice dice(const Json &j) { return DamageDice({integer(j, "diceCount", 0), integer(j, "dieSize", 0)}); }
    DamageComponent damageComponent(const Json &j)
    {
        using E = DamageType;
        std::vector<DamageType> types;
        for (const auto &name : strings(j, "types")) types.push_back(enumeration<E>(Json{{"type", name}}, "type", {
            {"bludgeoning", E::Bludgeoning}, {"piercing", E::Piercing}, {"slashing", E::Slashing}, {"acid", E::Acid}, {"cold", E::Cold},
            {"electricity", E::Electricity}, {"fire", E::Fire}, {"sonic", E::Sonic}, {"force", E::Force}, {"positiveEnergy", E::PositiveEnergy},
            {"negativeEnergy", E::NegativeEnergy}, {"untyped", E::Untyped}}));
        std::vector<DamageTrait> traits;
        for (const auto &name : strings(j, "traits")) traits.push_back(enumeration<DamageTrait>(Json{{"trait", name}}, "trait", {
            {"precision", DamageTrait::Precision}, {"bleed", DamageTrait::Bleed}, {"nonLethal", DamageTrait::NonLethal}}));
        return DamageComponent({text(j, "id"), "effect",
            enumeration<DamageComponentRole>(j, "role", {{"base", DamageComponentRole::Base}, {"additional", DamageComponentRole::Additional}}),
            dice(j.at("dice")), std::move(types),
            enumeration<DamageTypeMode>(j, "typeMode", {{"all", DamageTypeMode::All}, {"choice", DamageTypeMode::Choice}}),
            enumeration<DamageCriticalRule>(j, "criticalRule", {{"multiplied", DamageCriticalRule::Multiplied}, {"notMultiplied", DamageCriticalRule::NotMultiplied}, {"criticalOnly", DamageCriticalRule::CriticalOnly}}), std::move(traits)});
    }
    AttackTag attackTag(const std::string &name)
    {
        return enumeration<AttackTag>(Json{{"tag", name}}, "tag", {{"weapon", AttackTag::Weapon}, {"thrown", AttackTag::Thrown},
            {"projectile", AttackTag::Projectile}, {"natural", AttackTag::Natural}, {"unarmed", AttackTag::Unarmed}});
    }
    RoutineAttackProgressionDefinition progression(const Json &j)
    {
        return {enumeration<RoutineAttackProgressionType>(j, "kind", {{"fixed", RoutineAttackProgressionType::Fixed}, {"baseAttackBonusIteratives", RoutineAttackProgressionType::BaseAttackBonusIteratives}}),
            optionalText(j, "countExpression"), normalize(j.value("attackBonusAdjustmentExpression", "0"))};
    }
}

namespace golarion
{
    std::optional<CollectionEffectDefinition> collectionEffectFromJson(const Json &j)
    {
        const auto type = text(j, "type"), id = text(j, "id");
        const std::string source = "effect";
        const auto finish = [&](CollectionEffectEntry entry) -> std::optional<CollectionEffectDefinition>
        {
            CollectionEffectDefinition result{id, std::move(entry)};
            compileCollectionEffect(result, {id, source});
            return result;
        };
        if (type == "routineProgressionGrant") return finish(RoutineProgressionEffectEntry{id, source, text(j, "routineId"), text(j, "slotId"), progression(j.at("progression"))});
        if (type == "routineAttackBonusAdjustment")
        {
            RoutineAttackBonusAdjustmentDefinition entry{id, source, text(j, "routineId"), optionalText(j, "slotId"), text(j, "expression"), {}};
            if (j.contains("requirements"))
            {
                if (!j.at("requirements").is_array()) throw std::invalid_argument("assignment requirements must be an array");
                for (const auto &r : j.at("requirements"))
                {
                    RoutineAssignmentRequirementDefinition requirement{.slotId = text(r, "slotId"), .strikeGrantId = optionalText(r, "strikeGrantId")};
                    if (r.contains("requiredTag")) requirement.requiredTag = attackTag(text(r, "requiredTag"));
                    if (r.contains("weaponWeight")) requirement.weaponWeight = weight(r);
                    if (r.contains("purpose")) requirement.weaponWeightPurpose = enumeration<WeaponWeightPurpose>(r, "purpose", {{"general", WeaponWeightPurpose::General}, {"twoWeaponFighting", WeaponWeightPurpose::TwoWeaponFighting}});
                    entry.requirements.emplace_back(requirement);
                }
            }
            return finish(entry);
        }
        if (type == "attackRoutine")
        {
            AttackRoutineEffectEntry entry{id, source, text(j, "name"), text(j, "actionId"), {}};
            if (!j.at("slots").is_array()) throw std::invalid_argument("routine slots must be an array");
            for (const auto &s : j.at("slots"))
            {
                StrikeSelectorDefinition selector;
                const auto selection = s.value("selector", Json::object());
                if (!selection.is_object()) throw std::invalid_argument("strike selector must be an object");
                for (const auto &mode : strings(selection, "allowedModes")) selector.allowedModes.push_back(enumeration<AttackMode>(Json{{"mode", mode}}, "mode", {{"melee", AttackMode::Melee}, {"ranged", AttackMode::Ranged}}));
                for (const auto &tag : strings(selection, "requiredTags")) selector.requiredTags.push_back(attackTag(tag));
                for (const auto &tag : strings(selection, "forbiddenTags")) selector.forbiddenTags.push_back(attackTag(tag));
                selector.allowedGrantIds = strings(selection, "allowedGrantIds");
                RoutineSlotEffectEntry slot{.id = text(s, "id"), .name = text(s, "name"), .selector = StrikeSelector(selector),
                    .selectionMode = enumeration<RoutineSlotSelectionMode>(s, "selectionMode", {{"chooseOne", RoutineSlotSelectionMode::ChooseOne}, {"allMatching", RoutineSlotSelectionMode::AllMatching}}),
                    .strikeUsage = enumeration<StrikeUsage>(s, "strikeUsage", {{"default", StrikeUsage::Default}, {"naturalSecondary", StrikeUsage::NaturalSecondary}, {"singleNatural", StrikeUsage::SingleNatural}, {"naturalSecondaryWhenCombined", StrikeUsage::NaturalSecondaryWhenCombined}}),
                    .baseAttackBonusAdjustmentExpression = normalize(s.value("baseAttackBonusAdjustmentExpression", "0")), .assignmentRequired = s.value("assignmentRequired", true)};
                if (!s.at("progressions").is_array()) throw std::invalid_argument("routine progressions must be an array");
                for (const auto &p : s.at("progressions")) slot.progressions.push_back(progression(p));
                if (s.contains("damageAbilityRule")) slot.damageAbilityRuleOverride = damageAbilityRule(s);
                if (s.contains("handUsage")) slot.handUsage = enumeration<HandUsage>(s, "handUsage", {{"oneHanded", HandUsage::OneHanded}, {"twoHanded", HandUsage::TwoHanded}});
                if (s.contains("handRole")) slot.handRole = enumeration<AttackHandRole>(s, "handRole", {{"primary", AttackHandRole::Primary}, {"offHand", AttackHandRole::OffHand}});
                entry.slots.push_back(std::move(slot));
            }
            return finish(entry);
        }
        if (type == "abilityReplacement")
        {
            auto stage = enumeration<AbilityReplacementStage>(j, "stage", {{"base", AbilityReplacementStage::Base}, {"final", AbilityReplacementStage::Final}});
            return finish(AbilityScoreReplacementEffectEntry{{id, source, text(j, "expression"), stage, requirements(j)}, ability(j)});
        }
        if (type == "combatManeuverAbilityReplacement") return finish(CombatManeuverAbilityReplacementDefinition{id, source, text(j, "resource"), ability(j)});
        if (type == "combatManeuverDexteritySuppression") return finish(CombatManeuverDefenseDexteritySuppressionDefinition{id, source});
        if (type == "actionCategory") return finish(ActionCategoryDefinition{id, source, text(j, "name")});
        if (type == "actionGrant") return finish(ActionDefinition{id, source, text(j, "categoryId"), text(j, "name"), text(j, "description"), actionCost(j), strings(j, "tags")});
        if (type == "actionNote") return finish(ActionNoteDefinition{id, source, text(j, "actionId"), text(j, "text")});
        if (type == "actionCostReplacement") return finish(ActionCostReplacementDefinition{id, source, text(j, "actionId"), actionCost(j)});
        if (type == "actionInhibition")
        {
            const auto s = j.value("selector", Json::object());
            if (!s.is_object()) throw std::invalid_argument("action selector must be an object");
            ActionSelectorDefinition selector{optionalText(s, "actionId"), optionalText(s, "categoryId"), {}, strings(s, "requiredTags")};
            if (s.contains("cost")) selector.cost = actionCost(s);
            return finish(ActionInhibitionDefinition{id, source, ActionSelector(selector), text(j, "reason")});
        }
        if (type == "attackAbilityReplacement") return finish(AttackAbilityReplacementDefinition{id, source, text(j, "resource"), ability(j)});
        if (type == "damageAbilityReplacement") return finish(DamageAbilityReplacementDefinition{id, source, text(j, "resource"), ability(j)});
        if (type == "attackRequirement") return finish(AttackRequirementDefinition{id, source, text(j, "resource"), Requirement(text(j, "expression"), text(j, "failureReason")), optionalText(j, "condition")});
        if (type == "attackDefenseReplacement") return finish(AttackDefenseReplacementDefinition{id, source, text(j, "resource"), defense(j), optionalText(j, "condition")});
        if (type == "damageComponentGrant") return finish(DamageComponentGrantDefinition{id, source, text(j, "resource"), damageComponent(j.at("component"))});
        if (type == "criticalAdjustment")
        {
            using E = CriticalAdjustmentType;
            auto adjustment = enumeration<E>(j, "adjustment", {{"threatRangeMultiplier", E::ThreatRangeMultiplier}, {"threatMinimum", E::ThreatMinimum}, {"multiplierIncrease", E::MultiplierIncrease}, {"multiplierSet", E::MultiplierSet}});
            std::optional<int> maximum;
            if (j.contains("maximumMultiplier")) maximum = integer(j, "maximumMultiplier", 0);
            return finish(CriticalAdjustmentDefinition{id, source, text(j, "resource"), adjustment, text(j, "expression"), maximum, optionalText(j, "condition")});
        }
        if (type == "attackDistanceAdjustment")
        {
            using E = AttackDistanceAdjustmentType;
            return finish(AttackDistanceAdjustmentDefinition{id, source, text(j, "resource"), enumeration<E>(j, "adjustment", {{"multiplier", E::Multiplier}, {"minimum", E::Minimum}, {"maximum", E::Maximum}}), text(j, "expression"), optionalText(j, "condition")});
        }
        if (type == "strikeWeaponWeightAdjustment")
            return finish(StrikeWeaponWeightAdjustmentDefinition{id, source, text(j, "resource"), enumeration<WeaponWeightPurpose>(j, "purpose", {{"general", WeaponWeightPurpose::General}, {"twoWeaponFighting", WeaponWeightPurpose::TwoWeaponFighting}}), weight(j)});
        if (type == "strikeUsageAdjustment")
        {
            using E = StrikeUsage;
            auto usage = enumeration<E>(j, "usage", {{"default", E::Default}, {"naturalSecondary", E::NaturalSecondary}, {"singleNatural", E::SingleNatural}, {"naturalSecondaryWhenCombined", E::NaturalSecondaryWhenCombined}});
            std::optional<DamageAbilityRule> rule;
            if (j.contains("damageAbilityRule")) rule = damageAbilityRule(j);
            return finish(StrikeUsageAdjustmentDefinition{id, source, text(j, "resource"), usage, optionalText(j, "attackPenaltyExpression"), rule});
        }
        if (type == "damageDiceAdjustment")
        {
            using E = DamageDiceAdjustmentType;
            auto adjustment = enumeration<E>(j, "adjustment", {{"progressionSteps", E::ProgressionSteps}, {"diceCountMultiplier", E::DiceCountMultiplier}, {"set", E::Set}});
            std::optional<DamageDice> replacement;
            if (j.contains("dice")) replacement = dice(j.at("dice"));
            return finish(DamageDiceAdjustmentDefinition{id, source, text(j, "resource"),
                enumeration<DamageComponentRole>(j, "role", {{"base", DamageComponentRole::Base}, {"additional", DamageComponentRole::Additional}}),
                enumeration<DamageComponentOriginFilter>(j, "origin", {{"any", DamageComponentOriginFilter::Any}, {"intrinsic", DamageComponentOriginFilter::Intrinsic}, {"externalGrant", DamageComponentOriginFilter::ExternalGrant}}),
                optionalText(j, "componentGrantId"), optionalText(j, "componentId"), adjustment, optionalText(j, "expression"), replacement, optionalText(j, "condition")});
        }
        if (type == "strikeGrant")
        {
            StrikeGrantDefinition strike{.id = id, .source = source, .name = text(j, "name"),
                .mode = enumeration<AttackMode>(j, "mode", {{"melee", AttackMode::Melee}, {"ranged", AttackMode::Ranged}}),
                .usageChannels = strings(j, "usageChannels"), .damageAbility = ability(j), .damageAbilityRule = damageAbilityRule(j),
                .criticalThreatMinimum = integer(j, "criticalThreatMinimum", 20), .criticalMultiplier = integer(j, "criticalMultiplier", 2),
                .requirements = requirements(j)};
            for (const auto &tag : strings(j, "tags")) strike.tags.push_back(enumeration<AttackTag>(Json{{"tag", tag}}, "tag", {
                {"weapon", AttackTag::Weapon}, {"thrown", AttackTag::Thrown}, {"projectile", AttackTag::Projectile}, {"natural", AttackTag::Natural}, {"unarmed", AttackTag::Unarmed}}));
            if (j.contains("naturalAttackClassification")) strike.naturalAttackClassification = enumeration<NaturalAttackClassification>(j, "naturalAttackClassification", {{"primary", NaturalAttackClassification::Primary}, {"secondary", NaturalAttackClassification::Secondary}});
            if (j.contains("weaponWeight")) strike.weaponWeight = weight(j);
            if (j.contains("defense")) strike.defenseType = defense(j);
            if (j.contains("reach"))
            {
                const auto &r = j.at("reach");
                strike.reach = AttackReachDefinition{integer(r, "minimumUnits", 0), integer(r, "maximumUnits", 1)};
            }
            if (j.contains("range"))
            {
                const auto &r = j.at("range");
                strike.range = AttackRangeDefinition{integer(r, "incrementUnits", 1), integer(r, "maximumIncrements", 10), integer(r, "penaltyPerAdditionalIncrement", 2)};
            }
            const auto &components = j.at("damageComponents");
            if (!components.is_array()) throw std::invalid_argument("damage components must be an array");
            for (const auto &component : components) strike.damageComponents.push_back(damageComponent(component));
            return finish(std::move(strike));
        }
        if (type == "energyResistance") return finish(EnergyResistanceDefinition{id, source, energy(j), text(j, "expression"), strings(j, "tags")});
        if (type == "energyResistanceAdjustment") return finish(EnergyResistanceAdjustmentDefinition{id, source,
            EnergyResistanceSelector(defenseSelector<EnergyResistanceSelectorDefinition>(j)), text(j, "expression"), text(j, "stackingGroup")});
        if (type == "energyVulnerability") return finish(EnergyVulnerabilityDefinition{id, source, energy(j), optionalText(j, "applicability")});
        if (type == "immunity") return finish(ImmunityDefinition{id, source, text(j, "targetId"), text(j, "name"), optionalText(j, "applicability")});
        if (type == "spellResistance") return finish(SpellResistanceDefinition{id, source, text(j, "expression"), optionalText(j, "applicability"), j.value("canBeLowered", true), strings(j, "tags")});
        if (type == "spellResistanceAdjustment") return finish(SpellResistanceAdjustmentDefinition{id, source,
            SpellResistanceSelector(defenseSelector<SpellResistanceSelectorDefinition>(j)), text(j, "expression"), text(j, "stackingGroup"), optionalText(j, "applicability")});
        if (type == "fastHealing") return finish(FastHealingDefinition{id, source, text(j, "expression"), optionalText(j, "applicability"), normalize(j.value("stackingGroup", "fastHealing")), strings(j, "tags")});
        if (type == "regeneration") return finish(RegenerationDefinition{id, source, text(j, "expression"), optionalText(j, "interruption"), optionalText(j, "applicability"), strings(j, "tags")});
        if (type == "damageReductionAdjustment") return finish(DamageReductionAdjustmentDefinition{id, source,
            DamageReductionSelector(defenseSelector<DamageReductionSelectorDefinition>(j)), text(j, "expression"), text(j, "stackingGroup")});
        if (type == "damageReduction")
        {
            DamageReductionBypass bypass;
            const auto &alternatives = j.at("bypass");
            if (!alternatives.is_array()) throw std::invalid_argument("damage reduction bypass must be an array of alternatives");
            for (const auto &alternative : alternatives)
            {
                if (!alternative.is_array() || alternative.empty()) throw std::invalid_argument("bypass alternative must be a nonempty array");
                DamageReductionBypassAlternative value;
                for (const auto &trait : alternative)
                {
                    using E = DamageReductionBypassTrait;
                    value.allOf.push_back(enumeration<E>(Json{{"trait", trait}}, "trait", {{"bludgeoning", E::Bludgeoning}, {"piercing", E::Piercing},
                        {"slashing", E::Slashing}, {"magic", E::Magic}, {"epic", E::Epic}, {"adamantine", E::Adamantine}, {"silver", E::Silver},
                        {"coldIron", E::ColdIron}, {"good", E::Good}, {"evil", E::Evil}, {"lawful", E::Lawful}, {"chaotic", E::Chaotic}}));
                }
                bypass.anyOf.push_back(std::move(value));
            }
            return finish(DamageReductionDefinition{id, source, text(j, "expression"), std::move(bypass), optionalText(j, "applicability"), strings(j, "tags"), strings(j, "stacksWithTags")});
        }
        if (type == "sizeReplacement")
        {
            using E = SizeCategory;
            auto category = enumeration<E>(j, "category", {{"fine", E::Fine}, {"diminutive", E::Diminutive}, {"tiny", E::Tiny}, {"small", E::Small},
                {"medium", E::Medium}, {"large", E::Large}, {"huge", E::Huge}, {"gargantuan", E::Gargantuan}, {"colossal", E::Colossal}});
            return finish(SizeReplacementDefinition{id, source, category, j.value("acceptsAdjustments", true)});
        }
        if (type == "sizeAdjustment") return finish(SizeAdjustmentDefinition{id, source, integer(j, "steps", 0)});
        if (type == "carryingCapacityMultiplier") return finish(CarryingCapacityMultiplierDefinition{id, source, text(j, "stackingGroup"), integer(j, "numerator", 1), integer(j, "denominator", 1)});
        if (type == "movementAdjustment")
        {
            using E = MovementAdjustmentType;
            auto adjustment = enumeration<E>(j, "adjustment", {{"speedMultiplier", E::SpeedMultiplier}, {"reducedByArmorOrLoad", E::ReducedByArmorOrLoad},
                {"speedLimit", E::SpeedLimit}, {"maneuverabilityChange", E::ManeuverabilityChange}, {"block", E::Block}});
            return finish(MovementAdjustmentDefinition{id, source, text(j, "description"), adjustment, movementSelector(j), optionalText(j, "expression"), optionalText(j, "condition")});
        }
        if (type == "runAdjustment")
        {
            using E = RunAdjustmentType;
            auto adjustment = enumeration<E>(j, "adjustment", {{"increase", E::Increase}, {"penalty", E::Penalty}, {"replacement", E::Replacement}, {"block", E::Block}});
            return finish(RunAdjustmentDefinition{id, source, text(j, "description"), adjustment, text(j, "stackingGroup"), movementSelector(j), optionalText(j, "expression"), optionalText(j, "condition")});
        }
        if (type == "movementBoundedBonus")
        {
            using E = BonusType;
            auto bonus = enumeration<E>(j, "bonusType", {{"generic", E::Generic}, {"alchemical", E::Alchemical}, {"armor", E::Armor}, {"naturalArmor", E::NaturalArmor},
                {"circumstance", E::Circumstance}, {"insight", E::Insight}, {"competence", E::Competence}, {"deflection", E::Deflection}, {"luck", E::Luck},
                {"inherent", E::Inherent}, {"morale", E::Morale}, {"enhancement", E::Enhancement}, {"profane", E::Profane}, {"racial", E::Racial},
                {"resistance", E::Resistance}, {"sacred", E::Sacred}, {"dodge", E::Dodge}, {"shield", E::Shield}, {"size", E::Size}});
            const int percent = integer(j, "maximumPercent", 200);
            if (percent < 100) throw std::invalid_argument("bounded bonus percentage must be at least 100");
            return finish(MovementBoundedBonus{id, source, text(j, "description"), movementSelector(j), bonus, text(j, "expression"), percent});
        }
        if (type == "armorClassAbilityReplacement") return finish(ArmorClassAbilityReplacementDefinition{id, source, ability(j)});
        if (type == "armorClassAbilitySuppression") return finish(ArmorClassAbilitySuppressionDefinition{id, source, cause(j, "cause")});
        if (type == "armorClassDexterityRetention")
        {
            ArmorClassDexterityRetention retention{id, source, {}};
            for (const auto &name : strings(j, "causes"))
            {
                auto value = cause(Json{{"cause", name}}, "cause");
                if (value != ArmorClassSuppressionCause::FlatFooted && value != ArmorClassSuppressionCause::InvisibleAttacker)
                    throw std::invalid_argument("this suppression cause cannot be overridden");
                retention.causes.insert(value);
            }
            if (retention.causes.empty()) throw std::invalid_argument("retention requires causes");
            return finish(retention);
        }
        if (type == "maximumDexterityLimit")
        {
            using E = MaximumDexterityLimitType;
            return finish(MaximumDexterityLimitDefinition{id, source, enumeration<E>(j, "limitType", {{"armor", E::Armor}, {"shield", E::Shield}, {"load", E::Load}, {"other", E::Other}}), text(j, "expression")});
        }
        if (type == "armorCheckPenalty") return finish(ArmorCheckPenaltyDefinition{id, source, text(j, "expression")});
        if (type == "skillAbilityReplacement") return finish(SkillAbilityReplacementDefinition{id, source, text(j, "resource"), ability(j)});
        if (type == "classSkillGrant") return finish(SkillClassSkillGrantDefinition{id, source, text(j, "resource")});
        if (type == "initiativeAbilityReplacement") return finish(InitiativeAbilityReplacementDefinition{id, source, ability(j)});
        if (type == "savingThrowAbilityReplacement")
        {
            using E = SavingThrowType;
            return finish(SavingThrowAbilityReplacementDefinition{id, source, enumeration<E>(j, "savingThrow", {{"fortitude", E::Fortitude}, {"reflex", E::Reflex}, {"will", E::Will}}), ability(j)});
        }
        if (type == "temporaryHitPoints")
        {
            std::optional<GameDuration> duration;
            if (j.contains("durationRounds")) duration = GameDuration::fromRounds(integer(j, "durationRounds", 0));
            return finish(TemporaryHitPointGrant{id, text(j, "expression"), duration});
        }
        if (type == "conditionEntry") return finish(ConditionEntryDefinition{id, text(j, "conditionId"), source, integer(j, "severity", 1),
            j.value("contributesToEscalation", true), optionalText(j, "stackingGroup"), optionalText(j, "parameter")});
        return std::nullopt;
    }
}
