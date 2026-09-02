#include "golarion/effect/effect_json.hpp"

#include "golarion/util/string_utils.hpp"

#include <nlohmann/json.hpp>

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

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
        throw std::invalid_argument("unknown effect definition type: " + type);
    }
}
