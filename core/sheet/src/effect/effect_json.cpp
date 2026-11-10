#include "golarion/effect/effect_json.hpp"

#include "golarion/util/string_utils.hpp"

#include <nlohmann/json.hpp>

#include <optional>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace
{
    using Json = nlohmann::json;

    std::optional<std::string> optionalString(const Json &json, std::string_view field)
    {
        if (!json.contains(field))
        {
            return std::nullopt;
        }
        return golarion::normalize(json.at(field).get<std::string>());
    }

    golarion::ModifierType modifierTypeFromJson(const Json &json)
    {
        const std::string type = golarion::normalize(json.at("modifierType").get<std::string>());
        if (type == "bonus") return golarion::ModifierType::Bonus;
        if (type == "penalty") return golarion::ModifierType::Penalty;
        throw std::invalid_argument("unknown effect modifier type: " + type);
    }

    golarion::BonusType bonusTypeFromJson(const Json &json)
    {
        const std::string type = golarion::normalize(json.at("bonusType").get<std::string>());
        if (type == "generic") return golarion::BonusType::Generic;
        if (type == "alchemical") return golarion::BonusType::Alchemical;
        if (type == "armor") return golarion::BonusType::Armor;
        if (type == "naturalArmor") return golarion::BonusType::NaturalArmor;
        if (type == "circumstance") return golarion::BonusType::Circumstance;
        if (type == "insight") return golarion::BonusType::Insight;
        if (type == "competence") return golarion::BonusType::Competence;
        if (type == "deflection") return golarion::BonusType::Deflection;
        if (type == "luck") return golarion::BonusType::Luck;
        if (type == "inherent") return golarion::BonusType::Inherent;
        if (type == "morale") return golarion::BonusType::Morale;
        if (type == "enhancement") return golarion::BonusType::Enhancement;
        if (type == "profane") return golarion::BonusType::Profane;
        if (type == "racial") return golarion::BonusType::Racial;
        if (type == "resistance") return golarion::BonusType::Resistance;
        if (type == "sacred") return golarion::BonusType::Sacred;
        if (type == "dodge") return golarion::BonusType::Dodge;
        if (type == "shield") return golarion::BonusType::Shield;
        if (type == "size") return golarion::BonusType::Size;
        throw std::invalid_argument("unknown effect bonus type: " + type);
    }

    golarion::SizeCategory sizeCategoryFromJson(const Json &json)
    {
        const std::string category = golarion::normalize(json.at("category").get<std::string>());
        if (category == "fine") return golarion::SizeCategory::Fine;
        if (category == "diminutive") return golarion::SizeCategory::Diminutive;
        if (category == "tiny") return golarion::SizeCategory::Tiny;
        if (category == "small") return golarion::SizeCategory::Small;
        if (category == "medium") return golarion::SizeCategory::Medium;
        if (category == "large") return golarion::SizeCategory::Large;
        if (category == "huge") return golarion::SizeCategory::Huge;
        if (category == "gargantuan") return golarion::SizeCategory::Gargantuan;
        if (category == "colossal") return golarion::SizeCategory::Colossal;
        throw std::invalid_argument("unknown effect size category: " + category);
    }

    golarion::MovementType movementTypeFromJson(const Json &json)
    {
        const std::string type = golarion::normalize(json.at("movementType").get<std::string>());
        if (type == "land") return golarion::MovementType::Land;
        if (type == "climb") return golarion::MovementType::Climb;
        if (type == "swim") return golarion::MovementType::Swim;
        if (type == "burrow") return golarion::MovementType::Burrow;
        if (type == "fly") return golarion::MovementType::Fly;
        throw std::invalid_argument("unknown effect movement type: " + type);
    }

    golarion::Maneuverability maneuverabilityFromJson(const Json &json)
    {
        const std::string maneuverability = golarion::normalize(json.at("maneuverability").get<std::string>());
        if (maneuverability == "clumsy") return golarion::Maneuverability::Clumsy;
        if (maneuverability == "poor") return golarion::Maneuverability::Poor;
        if (maneuverability == "average") return golarion::Maneuverability::Average;
        if (maneuverability == "good") return golarion::Maneuverability::Good;
        if (maneuverability == "perfect") return golarion::Maneuverability::Perfect;
        throw std::invalid_argument("unknown effect movement maneuverability: " + maneuverability);
    }

    golarion::CarryingBodyType carryingBodyTypeFromJson(const Json &json)
    {
        const std::string type = golarion::normalize(json.at("bodyType").get<std::string>());
        if (type == "biped") return golarion::CarryingBodyType::Biped;
        if (type == "quadruped") return golarion::CarryingBodyType::Quadruped;
        throw std::invalid_argument("unknown effect carrying body type: " + type);
    }
}

namespace golarion
{
    EffectDefinition effectDefinitionFromJson(const nlohmann::json &json)
    {
        if (!json.is_object())
        {
            throw std::invalid_argument("effect definition must be a JSON object");
        }

        const std::string type = normalize(json.at("type").get<std::string>());
        const std::string id = normalize(json.at("id").get<std::string>());
        if (type == "modifier")
        {
            const ModifierType modifierType = modifierTypeFromJson(json);
            std::optional<BonusType> bonusType;
            if (json.contains("bonusType"))
            {
                bonusType = bonusTypeFromJson(json);
            }
            if (modifierType == ModifierType::Bonus && !bonusType.has_value())
            {
                throw std::invalid_argument("bonus effect requires bonusType: " + id);
            }
            if (modifierType == ModifierType::Penalty && bonusType.has_value())
            {
                throw std::invalid_argument("penalty effect must not specify bonusType: " + id);
            }
            return ModifierEffectDefinition{
                .id = id,
                .resource = normalize(json.at("resource").get<std::string>()),
                .description = normalize(json.at("description").get<std::string>()),
                .type = modifierType,
                .bonusType = bonusType,
                .expression = normalize(json.at("expression").get<std::string>()),
                .condition = optionalString(json, "condition")
            };
        }
        if (type == "contribution")
        {
            return ContributionEffectDefinition{
                .id = id,
                .resource = normalize(json.at("resource").get<std::string>()),
                .expression = normalize(json.at("expression").get<std::string>())
            };
        }
        if (type == "sizeBase")
        {
            return SizeBaseEffectDefinition{
                .id = id,
                .category = sizeCategoryFromJson(json)
            };
        }
        if (type == "movementGrant")
        {
            const MovementType movementType = movementTypeFromJson(json);
            std::optional<Maneuverability> maneuverability;
            if (json.contains("maneuverability"))
            {
                maneuverability = maneuverabilityFromJson(json);
            }
            if (movementType != MovementType::Fly && maneuverability.has_value())
            {
                throw std::invalid_argument("maneuverability is only valid for fly movement effects: " + id);
            }
            return MovementGrantEffectDefinition{
                .id = id,
                .type = movementType,
                .baseSpeedExpression = normalize(json.at("baseSpeedExpression").get<std::string>()),
                .maneuverability = maneuverability,
                .affectedByArmor = json.at("affectedByArmor").get<bool>(),
                .affectedByLoad = json.at("affectedByLoad").get<bool>(),
                .supportsRunning = json.value("supportsRunning", false)
            };
        }
        if (type == "carryingBodyType")
        {
            return CarryingBodyTypeEffectDefinition{
                .id = id,
                .type = carryingBodyTypeFromJson(json)
            };
        }
        if (type == "reminder")
        {
            return ReminderEffectDefinition{
                .id = id,
                .message = normalize(json.at("message").get<std::string>())
            };
        }

        if (type == "favoredClassGrant" || type == "favoredClassBonus")
        {
            std::set<CharacterClassType> classes;
            if (json.contains("classes"))
            {
                if (!json.at("classes").is_array()) throw std::invalid_argument("favored classes must be an array");
                for (const auto &entry : json.at("classes"))
                    if (!classes.insert(characterClassFromId(entry.get<std::string>())).second) throw std::invalid_argument("duplicate favored class restriction");
            }
            auto integer = [&](const char *field, int fallback, int minimum)
            {
                if (!json.contains(field)) return fallback;
                const auto &value = json.at(field);
                if (!value.is_number_integer() || value < minimum || value > std::numeric_limits<int>::max()) throw std::invalid_argument("invalid favored class integer field");
                return value.get<int>();
            };
            if (type == "favoredClassGrant")
                return FavoredClassGrantEffectDefinition{id, integer("additionalChoices", 0, 0), std::move(classes), json.value("allClasses", false)};

            FavoredClassBonusEffectDefinition option{
                .id = id, .name = normalize(json.at("name").get<std::string>()), .description = json.at("description").get<std::string>(),
                .classes = std::move(classes), .selectionsPerBenefit = integer("selectionsPerBenefit", 1, 1)
            };
            if (json.contains("replaces"))
            {
                if (!json.at("replaces").is_array()) throw std::invalid_argument("favored class replacements must be an array");
                for (const auto &entry : json.at("replaces"))
                {
                    const auto name = entry.get<std::string>();
                    FavoredClassBaseBonus base;
                    if (name == "hitPoint") base = FavoredClassBaseBonus::HitPoint;
                    else if (name == "skillRank") base = FavoredClassBaseBonus::SkillRank;
                    else throw std::invalid_argument("unknown favored class base bonus");
                    if (!option.replaces.insert(base).second) throw std::invalid_argument("duplicate favored class replacement");
                }
            }
            if (!json.at("effects").is_array()) throw std::invalid_argument("favored class benefits must be an array");
            std::set<std::string> ids;
            for (const auto &entry : json.at("effects"))
            {
                const auto nestedType = normalize(entry.at("type").get<std::string>());
                if (nestedType == "favoredClassGrant" || nestedType == "favoredClassBonus") throw std::invalid_argument("favored class benefits cannot grant favored class rules");
                auto effect = effectDefinitionFromJson(entry);
                if (!ids.insert(effectId(effect)).second) throw std::invalid_argument("duplicate favored class benefit ID");
                option.effects.push_back(std::visit([](auto value) -> FavoredClassBenefitEffect
                {
                    if constexpr (std::is_constructible_v<FavoredClassBenefitEffect, decltype(value)>) return value;
                    else throw std::invalid_argument("unsupported favored class benefit effect");
                }, std::move(effect)));
            }
            return option;
        }

        if (type == "featSlotGrant")
        {
            FeatSlotFilter filter = AnyFeat{};
            if (json.contains("filter"))
            {
                const auto &value = json.at("filter");
                if (!value.is_object()) throw std::invalid_argument("feat slot filter must be an object");
                const auto kind = value.at("kind").get<std::string>();
                if (kind == "any")
                {
                    if (value.size() != 1) throw std::invalid_argument("any feat filter cannot contain restrictions");
                }
                else if (kind == "type")
                {
                    if (value.size() != 2 || !value.contains("featType")) throw std::invalid_argument("type feat filter requires only featType");
                    filter = FeatTypeFilter{featTypeFromName(normalize(value.at("featType").get<std::string>()))};
                }
                else if (kind == "ids")
                {
                    if (value.size() != 2 || !value.contains("featIds")) throw std::invalid_argument("ID feat filter requires only featIds");
                    const auto &ids = value.at("featIds");
                    if (!ids.is_array() || ids.empty()) throw std::invalid_argument("featIds must be a nonempty array");
                    FeatIdFilter result;
                    std::set<std::string> seen;
                    for (const auto &entry : ids)
                    {
                        auto featId = normalize(entry.get<std::string>());
                        if (!seen.insert(featId).second) throw std::invalid_argument("duplicate feat ID in slot filter");
                        result.ids.push_back(std::move(featId));
                    }
                    filter = std::move(result);
                }
                else throw std::invalid_argument("unknown feat slot filter kind: " + kind);
            }
            return FeatSlotGrantEffectDefinition{.id = id, .filter = std::move(filter)};
        }

        if (type == "spellGrant")
        {
            const auto &usage = json.at("usage");
            const auto kind = usage.at("type").get<std::string>();
            GrantedSpellUsage mode;
            if (kind == "limited") mode = GrantedSpellUsage::Limited;
            else if (kind == "atWill") mode = GrantedSpellUsage::AtWill;
            else if (kind == "constant") mode = GrantedSpellUsage::Constant;
            else if (kind == "sharedPool") mode = GrantedSpellUsage::SharedPool;
            else throw std::invalid_argument("unknown granted spell usage: " + kind);
            const auto positive = [](const Json &value)
            {
                if (!value.is_number_integer() || value <= 0 || value > std::numeric_limits<int>::max()) throw std::invalid_argument("spell usage value must be a positive integer");
                return value.get<int>();
            };
            SpellGrantEffectDefinition effect{id, normalize(json.at("spellId").get<std::string>()), mode};
            if (usage.contains("capacity")) effect.capacity = positive(usage.at("capacity"));
            if (usage.contains("poolId")) effect.poolId = normalize(usage.at("poolId").get<std::string>());
            if (usage.contains("cost")) effect.cost = positive(usage.at("cost"));
            for (const auto &[key, value] : usage.items())
                if (key != "type" && !(mode == GrantedSpellUsage::Limited && key == "capacity") &&
                    !(mode == GrantedSpellUsage::SharedPool && (key == "poolId" || key == "cost")))
                    throw std::invalid_argument("unexpected spell usage field: " + key);
            validateSpellGrantEffectDefinition(effect);
            return effect;
        }
        if (type == "languageGrant")
        {
            return LanguageGrantEffectDefinition{
                .id = normalize(json.at("id").get<std::string>()),
                .languageId = normalize(json.at("languageId").get<std::string>())
            };
        }
        throw std::invalid_argument("unknown effect definition type: " + type);
    }
}
