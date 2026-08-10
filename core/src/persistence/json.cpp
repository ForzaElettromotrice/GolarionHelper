#include "golarion/persistence/json.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    using Json = nlohmann::json;

    template<typename Enum, std::size_t Size>
    std::string enumName(Enum value, const std::array<std::pair<Enum, std::string_view>, Size> &names)
    {
        for (const auto &[candidate, name] : names)
        {
            if (candidate == value)
            {
                return std::string(name);
            }
        }

        throw std::invalid_argument("unknown enum value");
    }

    template<typename Enum, std::size_t Size>
    Enum enumValue(const Json &json, const std::array<std::pair<Enum, std::string_view>, Size> &names)
    {
        const std::string value = json.get<std::string>();
        for (const auto &[candidate, name] : names)
        {
            if (name == value)
            {
                return candidate;
            }
        }

        throw std::invalid_argument("unknown enum name: " + value);
    }

    constexpr std::array AbilityTypeNames{
        std::pair{golarion::AbilityType::Strength, std::string_view("strength")},
        std::pair{golarion::AbilityType::Dexterity, std::string_view("dexterity")},
        std::pair{golarion::AbilityType::Constitution, std::string_view("constitution")},
        std::pair{golarion::AbilityType::Intelligence, std::string_view("intelligence")},
        std::pair{golarion::AbilityType::Wisdom, std::string_view("wisdom")},
        std::pair{golarion::AbilityType::Charisma, std::string_view("charisma")}
    };

    constexpr std::array ModifierTypeNames{
        std::pair{golarion::ModifierType::Bonus, std::string_view("bonus")},
        std::pair{golarion::ModifierType::Penalty, std::string_view("penalty")}
    };

    constexpr std::array SkillTypeNames{
        std::pair{golarion::SkillType::Acrobatics, std::string_view("acrobatics")},
        std::pair{golarion::SkillType::HandleAnimal, std::string_view("handleAnimal")},
        std::pair{golarion::SkillType::Craft, std::string_view("craft")},
        std::pair{golarion::SkillType::EscapeArtist, std::string_view("escapeArtist")},
        std::pair{golarion::SkillType::Disguise, std::string_view("disguise")},
        std::pair{golarion::SkillType::Ride, std::string_view("ride")},
        std::pair{golarion::SkillType::KnowledgeArcana, std::string_view("knowledgeArcana")},
        std::pair{golarion::SkillType::KnowledgeDungeoneering, std::string_view("knowledgeDungeoneering")},
        std::pair{golarion::SkillType::KnowledgeEngineering, std::string_view("knowledgeEngineering")},
        std::pair{golarion::SkillType::KnowledgeGeography, std::string_view("knowledgeGeography")},
        std::pair{golarion::SkillType::KnowledgeHistory, std::string_view("knowledgeHistory")},
        std::pair{golarion::SkillType::KnowledgeLocal, std::string_view("knowledgeLocal")},
        std::pair{golarion::SkillType::KnowledgeNature, std::string_view("knowledgeNature")},
        std::pair{golarion::SkillType::KnowledgeNobility, std::string_view("knowledgeNobility")},
        std::pair{golarion::SkillType::KnowledgePlanes, std::string_view("knowledgePlanes")},
        std::pair{golarion::SkillType::KnowledgeReligion, std::string_view("knowledgeReligion")},
        std::pair{golarion::SkillType::Diplomacy, std::string_view("diplomacy")},
        std::pair{golarion::SkillType::DisableDevice, std::string_view("disableDevice")},
        std::pair{golarion::SkillType::Stealth, std::string_view("stealth")},
        std::pair{golarion::SkillType::Heal, std::string_view("heal")},
        std::pair{golarion::SkillType::Intimidate, std::string_view("intimidate")},
        std::pair{golarion::SkillType::Perform, std::string_view("perform")},
        std::pair{golarion::SkillType::SenseMotive, std::string_view("senseMotive")},
        std::pair{golarion::SkillType::Linguistics, std::string_view("linguistics")},
        std::pair{golarion::SkillType::Swim, std::string_view("swim")},
        std::pair{golarion::SkillType::Perception, std::string_view("perception")},
        std::pair{golarion::SkillType::Profession, std::string_view("profession")},
        std::pair{golarion::SkillType::Bluff, std::string_view("bluff")},
        std::pair{golarion::SkillType::SleightOfHand, std::string_view("sleightOfHand")},
        std::pair{golarion::SkillType::Spellcraft, std::string_view("spellcraft")},
        std::pair{golarion::SkillType::Climb, std::string_view("climb")},
        std::pair{golarion::SkillType::Survival, std::string_view("survival")},
        std::pair{golarion::SkillType::UseMagicDevice, std::string_view("useMagicDevice")},
        std::pair{golarion::SkillType::Appraise, std::string_view("appraise")},
        std::pair{golarion::SkillType::Fly, std::string_view("fly")}
    };

    constexpr std::array SavingThrowTypeNames{
        std::pair{golarion::SavingThrowType::Fortitude, std::string_view("fortitude")},
        std::pair{golarion::SavingThrowType::Reflex, std::string_view("reflex")},
        std::pair{golarion::SavingThrowType::Will, std::string_view("will")}
    };

    constexpr std::array MovementTypeNames{
        std::pair{golarion::MovementType::Land, std::string_view("land")},
        std::pair{golarion::MovementType::Climb, std::string_view("climb")},
        std::pair{golarion::MovementType::Swim, std::string_view("swim")},
        std::pair{golarion::MovementType::Burrow, std::string_view("burrow")},
        std::pair{golarion::MovementType::Fly, std::string_view("fly")}
    };

    constexpr std::array ManeuverabilityNames{
        std::pair{golarion::Maneuverability::Clumsy, std::string_view("clumsy")},
        std::pair{golarion::Maneuverability::Poor, std::string_view("poor")},
        std::pair{golarion::Maneuverability::Average, std::string_view("average")},
        std::pair{golarion::Maneuverability::Good, std::string_view("good")},
        std::pair{golarion::Maneuverability::Perfect, std::string_view("perfect")}
    };

    constexpr std::array MovementAdjustmentTypeNames{
        std::pair{golarion::MovementAdjustmentType::SpeedMultiplier, std::string_view("speedMultiplier")},
        std::pair{golarion::MovementAdjustmentType::SpeedLimit, std::string_view("speedLimit")},
        std::pair{golarion::MovementAdjustmentType::ManeuverabilityChange, std::string_view("maneuverabilityChange")},
        std::pair{golarion::MovementAdjustmentType::Block, std::string_view("block")}
    };

    constexpr std::array BonusTypeNames{
        std::pair{golarion::BonusType::Alchemical, std::string_view("alchemical")},
        std::pair{golarion::BonusType::Armor, std::string_view("armor")},
        std::pair{golarion::BonusType::NaturalArmor, std::string_view("naturalArmor")},
        std::pair{golarion::BonusType::Circumstance, std::string_view("circumstance")},
        std::pair{golarion::BonusType::Insight, std::string_view("insight")},
        std::pair{golarion::BonusType::Competence, std::string_view("competence")},
        std::pair{golarion::BonusType::Deflection, std::string_view("deflection")},
        std::pair{golarion::BonusType::Luck, std::string_view("luck")},
        std::pair{golarion::BonusType::Inherent, std::string_view("inherent")},
        std::pair{golarion::BonusType::Morale, std::string_view("morale")},
        std::pair{golarion::BonusType::Enhancement, std::string_view("enhancement")},
        std::pair{golarion::BonusType::Profane, std::string_view("profane")},
        std::pair{golarion::BonusType::Racial, std::string_view("racial")},
        std::pair{golarion::BonusType::Resistance, std::string_view("resistance")},
        std::pair{golarion::BonusType::Sacred, std::string_view("sacred")},
        std::pair{golarion::BonusType::Dodge, std::string_view("dodge")},
        std::pair{golarion::BonusType::Shield, std::string_view("shield")},
        std::pair{golarion::BonusType::Size, std::string_view("size")}
    };

    Json abilityToJson(const golarion::AbilitySaveData &data)
    {
        return Json{
            {"type", enumName(data.type, AbilityTypeNames)},
            {"baseValue", data.baseValue}
        };
    }

    golarion::AbilitySaveData abilityFromJson(const Json &json)
    {
        return golarion::AbilitySaveData{
            .type = enumValue(json.at("type"), AbilityTypeNames),
            .baseValue = json.at("baseValue").get<int>()
        };
    }

    Json temporaryHitPointsToJson(const golarion::TemporaryHitPointsSaveData &data)
    {
        Json pools = Json::array();
        for (const golarion::TemporaryHitPointPoolSaveData &pool : data.pools)
        {
            Json remainingRounds = nullptr;
            if (pool.remainingDuration)
            {
                remainingRounds = pool.remainingDuration->roundCount();
            }
            pools.push_back(Json{
                {"id", pool.id},
                {"remaining", pool.remaining},
                {"remainingRounds", std::move(remainingRounds)}
            });
        }
        return Json{{"pools", std::move(pools)}};
    }

    golarion::TemporaryHitPointsSaveData temporaryHitPointsFromJson(const Json &json)
    {
        std::vector<golarion::TemporaryHitPointPoolSaveData> pools;
        pools.reserve(json.at("pools").size());
        for (const Json &pool : json.at("pools"))
        {
            std::optional<golarion::GameDuration> remainingDuration;
            if (!pool.at("remainingRounds").is_null())
            {
                remainingDuration = golarion::GameDuration::fromRounds(pool.at("remainingRounds").get<std::int64_t>());
            }
            pools.push_back(golarion::TemporaryHitPointPoolSaveData{
                .id = pool.at("id").get<std::string>(),
                .remaining = pool.at("remaining").get<int>(),
                .remainingDuration = remainingDuration
            });
        }
        return golarion::TemporaryHitPointsSaveData{.pools = std::move(pools)};
    }

    Json hitPointsToJson(const golarion::HitPointsSaveData &data)
    {
        return Json{
            {"baseMax", data.baseMax},
            {"damageTaken", data.damageTaken},
            {"temporary", temporaryHitPointsToJson(data.temporary)},
            {"nonLethal", data.nonLethal}
        };
    }

    golarion::HitPointsSaveData hitPointsFromJson(const Json &json)
    {
        return golarion::HitPointsSaveData{
            .baseMax = json.at("baseMax").get<int>(),
            .damageTaken = json.at("damageTaken").get<int>(),
            .temporary = temporaryHitPointsFromJson(json.at("temporary")),
            .nonLethal = json.at("nonLethal").get<int>()
        };
    }

    Json initiativeToJson(const golarion::InitiativeSaveData &data)
    {
        return Json{{"abilityType", enumName(data.abilityType, AbilityTypeNames)}};
    }

    golarion::InitiativeSaveData initiativeFromJson(const Json &json)
    {
        return golarion::InitiativeSaveData{
            .abilityType = enumValue(json.at("abilityType"), AbilityTypeNames)
        };
    }

    Json savingThrowToJson(const golarion::SavingThrowSaveData &data)
    {
        return Json{
            {"type", enumName(data.type, SavingThrowTypeNames)},
            {"baseValue", data.baseValue},
            {"abilityType", enumName(data.abilityType, AbilityTypeNames)}
        };
    }

    golarion::SavingThrowSaveData savingThrowFromJson(const Json &json)
    {
        return golarion::SavingThrowSaveData{
            .type = enumValue(json.at("type"), SavingThrowTypeNames),
            .baseValue = json.at("baseValue").get<int>(),
            .abilityType = enumValue(json.at("abilityType"), AbilityTypeNames)
        };
    }

    Json savingThrowsToJson(const golarion::SavingThrowsSaveData &data)
    {
        Json savingThrows = Json::array();
        for (const golarion::SavingThrowSaveData &savingThrow : data.savingThrows)
        {
            savingThrows.push_back(savingThrowToJson(savingThrow));
        }
        return Json{{"savingThrows", std::move(savingThrows)}};
    }

    golarion::SavingThrowsSaveData savingThrowsFromJson(const Json &json)
    {
        std::vector<golarion::SavingThrowSaveData> savingThrows;
        savingThrows.reserve(json.at("savingThrows").size());
        for (const Json &savingThrow : json.at("savingThrows"))
        {
            savingThrows.push_back(savingThrowFromJson(savingThrow));
        }
        return golarion::SavingThrowsSaveData{.savingThrows = std::move(savingThrows)};
    }

    Json skillToJson(const golarion::SkillSaveData &data)
    {
        Json specializationId = nullptr;
        if (data.specializationId)
        {
            specializationId = *data.specializationId;
        }

        Json specialization = nullptr;
        if (data.specialization)
        {
            specialization = *data.specialization;
        }

        return Json{
            {"type", enumName(data.type, SkillTypeNames)},
            {"specializationId", std::move(specializationId)},
            {"specialization", std::move(specialization)},
            {"abilityType", enumName(data.abilityType, AbilityTypeNames)},
            {"ranks", data.ranks},
            {"classSkill", data.classSkill},
            {"custom", data.custom}
        };
    }

    golarion::SkillSaveData skillFromJson(const Json &json)
    {
        std::optional<std::string> specializationId;
        if (!json.at("specializationId").is_null())
        {
            specializationId = json.at("specializationId").get<std::string>();
        }

        std::optional<std::string> specialization;
        if (!json.at("specialization").is_null())
        {
            specialization = json.at("specialization").get<std::string>();
        }

        return golarion::SkillSaveData{
            .type = enumValue(json.at("type"), SkillTypeNames),
            .specializationId = std::move(specializationId),
            .specialization = std::move(specialization),
            .abilityType = enumValue(json.at("abilityType"), AbilityTypeNames),
            .ranks = json.at("ranks").get<int>(),
            .classSkill = json.at("classSkill").get<bool>(),
            .custom = json.at("custom").get<bool>()
        };
    }

    Json skillsToJson(const golarion::SkillsSaveData &data)
    {
        Json skills = Json::array();
        for (const golarion::SkillSaveData &skill : data.skills)
        {
            skills.push_back(skillToJson(skill));
        }
        return Json{{"skills", std::move(skills)}};
    }

    golarion::SkillsSaveData skillsFromJson(const Json &json)
    {
        std::vector<golarion::SkillSaveData> skills;
        skills.reserve(json.at("skills").size());
        for (const Json &skill : json.at("skills"))
        {
            skills.push_back(skillFromJson(skill));
        }
        return golarion::SkillsSaveData{.skills = std::move(skills)};
    }

    Json movementSelectorToJson(const golarion::MovementSelector &selector)
    {
        Json type = nullptr;
        if (selector.type.has_value())
        {
            type = enumName(*selector.type, MovementTypeNames);
        }
        Json grantId = nullptr;
        if (selector.grantId.has_value())
        {
            grantId = *selector.grantId;
        }
        return Json{{"type", std::move(type)}, {"grantId", std::move(grantId)}};
    }

    golarion::MovementSelector movementSelectorFromJson(const Json &json)
    {
        std::optional<golarion::MovementType> type;
        if (!json.at("type").is_null())
        {
            type = enumValue(json.at("type"), MovementTypeNames);
        }
        std::optional<std::string> grantId;
        if (!json.at("grantId").is_null())
        {
            grantId = json.at("grantId").get<std::string>();
        }
        return golarion::MovementSelector{.type = type, .grantId = std::move(grantId)};
    }

    Json movementGrantToJson(const golarion::MovementGrantSaveData &grant)
    {
        Json maneuverability = nullptr;
        if (grant.maneuverability.has_value())
        {
            maneuverability = enumName(*grant.maneuverability, ManeuverabilityNames);
        }
        return Json{
            {"id", grant.id},
            {"source", grant.source},
            {"type", enumName(grant.type, MovementTypeNames)},
            {"baseSpeedExpression", grant.baseSpeedExpression},
            {"maneuverability", std::move(maneuverability)},
            {"affectedByArmor", grant.affectedByArmor},
            {"affectedByLoad", grant.affectedByLoad}
        };
    }

    golarion::MovementGrantSaveData movementGrantFromJson(const Json &grant)
    {
        std::optional<golarion::Maneuverability> maneuverability;
        if (!grant.at("maneuverability").is_null())
        {
            maneuverability = enumValue(grant.at("maneuverability"), ManeuverabilityNames);
        }
        return golarion::MovementGrantSaveData{
            .id = grant.at("id").get<std::string>(),
            .source = grant.at("source").get<std::string>(),
            .type = enumValue(grant.at("type"), MovementTypeNames),
            .baseSpeedExpression = grant.at("baseSpeedExpression").get<std::string>(),
            .maneuverability = maneuverability,
            .affectedByArmor = grant.at("affectedByArmor").get<bool>(),
            .affectedByLoad = grant.at("affectedByLoad").get<bool>()
        };
    }

    Json movementAdjustmentToJson(const golarion::MovementAdjustmentSaveData &adjustment)
    {
        Json expression = nullptr;
        if (adjustment.expression.has_value())
        {
            expression = *adjustment.expression;
        }
        Json condition = nullptr;
        if (adjustment.condition.has_value())
        {
            condition = *adjustment.condition;
        }
        return Json{
            {"id", adjustment.id},
            {"source", adjustment.source},
            {"description", adjustment.description},
            {"type", enumName(adjustment.type, MovementAdjustmentTypeNames)},
            {"selector", movementSelectorToJson(adjustment.selector)},
            {"expression", std::move(expression)},
            {"condition", std::move(condition)}
        };
    }

    golarion::MovementAdjustmentSaveData movementAdjustmentFromJson(const Json &adjustment)
    {
        std::optional<std::string> expression;
        if (!adjustment.at("expression").is_null())
        {
            expression = adjustment.at("expression").get<std::string>();
        }
        std::optional<std::string> condition;
        if (!adjustment.at("condition").is_null())
        {
            condition = adjustment.at("condition").get<std::string>();
        }
        return golarion::MovementAdjustmentSaveData{
            .id = adjustment.at("id").get<std::string>(),
            .source = adjustment.at("source").get<std::string>(),
            .description = adjustment.at("description").get<std::string>(),
            .type = enumValue(adjustment.at("type"), MovementAdjustmentTypeNames),
            .selector = movementSelectorFromJson(adjustment.at("selector")),
            .expression = std::move(expression),
            .condition = std::move(condition)
        };
    }

    Json movementGroupToJson(const golarion::MovementGroupSaveData &data)
    {
        Json grants = Json::array();
        for (const golarion::MovementGrantSaveData &grant : data.grants)
        {
            grants.push_back(movementGrantToJson(grant));
        }
        Json adjustments = Json::array();
        for (const golarion::MovementAdjustmentSaveData &adjustment : data.adjustments)
        {
            adjustments.push_back(movementAdjustmentToJson(adjustment));
        }
        return Json{{"grants", std::move(grants)}, {"adjustments", std::move(adjustments)}};
    }

    golarion::MovementGroupSaveData movementGroupFromJson(const Json &json)
    {
        std::vector<golarion::MovementGrantSaveData> grants;
        grants.reserve(json.at("grants").size());
        for (const Json &grant : json.at("grants"))
        {
            grants.push_back(movementGrantFromJson(grant));
        }
        std::vector<golarion::MovementAdjustmentSaveData> adjustments;
        adjustments.reserve(json.at("adjustments").size());
        for (const Json &adjustment : json.at("adjustments"))
        {
            adjustments.push_back(movementAdjustmentFromJson(adjustment));
        }
        return golarion::MovementGroupSaveData{.grants = std::move(grants), .adjustments = std::move(adjustments)};
    }

    Json movementGroupManagerToJson(const golarion::MovementGroupManagerSaveData &data)
    {
        Json groups = Json::array();
        for (const auto &[id, enabled, group] : data.groups)
        {
            groups.push_back(Json{{"id", id}, {"enabled", enabled}, {"group", movementGroupToJson(group)}});
        }
        return Json{{"groups", std::move(groups)}};
    }

    golarion::MovementGroupManagerSaveData movementGroupManagerFromJson(const Json &json)
    {
        std::vector<golarion::MovementGroupManagerSaveData::GroupSaveData> groups;
        groups.reserve(json.at("groups").size());
        for (const Json &group : json.at("groups"))
        {
            groups.push_back(golarion::MovementGroupManagerSaveData::GroupSaveData{
                .id = group.at("id").get<std::string>(),
                .enabled = group.at("enabled").get<bool>(),
                .group = movementGroupFromJson(group.at("group"))
            });
        }
        return golarion::MovementGroupManagerSaveData{.groups = std::move(groups)};
    }

    Json modifierToJson(const golarion::ModifierSaveData &data)
    {
        Json bonusType = nullptr;
        if (data.bonusType.has_value())
        {
            bonusType = enumName(*data.bonusType, BonusTypeNames);
        }

        Json condition = nullptr;
        if (data.condition.has_value())
        {
            condition = *data.condition;
        }

        return Json{
            {"id", data.id},
            {"type", enumName(data.type, ModifierTypeNames)},
            {"source", data.source},
            {"description", data.description},
            {"bonusType", std::move(bonusType)},
            {"expression", data.expression},
            {"condition", std::move(condition)}
        };
    }

    golarion::ModifierSaveData modifierFromJson(const Json &json)
    {
        std::optional<golarion::BonusType> bonusType;
        if (!json.at("bonusType").is_null())
        {
            bonusType = enumValue(json.at("bonusType"), BonusTypeNames);
        }

        std::optional<std::string> condition;
        if (!json.at("condition").is_null())
        {
            condition = json.at("condition").get<std::string>();
        }

        return golarion::ModifierSaveData{
            .id = json.at("id").get<std::string>(),
            .type = enumValue(json.at("type"), ModifierTypeNames),
            .source = json.at("source").get<std::string>(),
            .description = json.at("description").get<std::string>(),
            .bonusType = bonusType,
            .expression = json.at("expression").get<std::string>(),
            .condition = std::move(condition)
        };
    }

    Json modifierGroupToJson(const golarion::ModifierGroupSaveData &data)
    {
        Json modifiers = Json::array();
        for (const auto &[resourceName, modifier] : data.modifiers)
        {
            modifiers.push_back(Json{
                {"resourceName", resourceName},
                {"modifier", modifierToJson(modifier)}
            });
        }

        return Json{{"modifiers", std::move(modifiers)}};
    }

    golarion::ModifierGroupSaveData modifierGroupFromJson(const Json &json)
    {
        std::vector<golarion::ModifierGroupSaveData::TargetedModifierSaveData> modifiers;
        modifiers.reserve(json.at("modifiers").size());

        for (const Json &targetedModifier : json.at("modifiers"))
        {
            modifiers.push_back(golarion::ModifierGroupSaveData::TargetedModifierSaveData{
                .resourceName = targetedModifier.at("resourceName").get<std::string>(),
                .modifier = modifierFromJson(targetedModifier.at("modifier"))
            });
        }

        return golarion::ModifierGroupSaveData{.modifiers = std::move(modifiers)};
    }

    Json modifierGroupManagerToJson(const golarion::ModifierGroupManagerSaveData &data)
    {
        Json groups = Json::array();
        for (const auto &[id, enabled, group] : data.groups)
        {
            groups.push_back(Json{
                {"id", id},
                {"enabled", enabled},
                {"group", modifierGroupToJson(group)}
            });
        }

        return Json{{"groups", std::move(groups)}};
    }

    golarion::ModifierGroupManagerSaveData modifierGroupManagerFromJson(const Json &json)
    {
        std::vector<golarion::ModifierGroupManagerSaveData::GroupSaveData> groups;
        groups.reserve(json.at("groups").size());

        for (const Json &group : json.at("groups"))
        {
            groups.push_back(golarion::ModifierGroupManagerSaveData::GroupSaveData{
                .id = group.at("id").get<std::string>(),
                .enabled = group.at("enabled").get<bool>(),
                .group = modifierGroupFromJson(group.at("group"))
            });
        }

        return golarion::ModifierGroupManagerSaveData{.groups = std::move(groups)};
    }

    Json contributionToJson(const golarion::ContributionSaveData &data)
    {
        return Json{
            {"id", data.id},
            {"expression", data.expression}
        };
    }

    golarion::ContributionSaveData contributionFromJson(const Json &json)
    {
        return golarion::ContributionSaveData{
            .id = json.at("id").get<std::string>(),
            .expression = json.at("expression").get<std::string>()
        };
    }

    Json contributionGroupToJson(const golarion::ContributionGroupSaveData &data)
    {
        Json contributions = Json::array();
        for (const auto &[resourceName, contribution] : data.contributions)
        {
            contributions.push_back(Json{
                {"resourceName", resourceName},
                {"contribution", contributionToJson(contribution)}
            });
        }
        return Json{{"contributions", std::move(contributions)}};
    }

    golarion::ContributionGroupSaveData contributionGroupFromJson(const Json &json)
    {
        std::vector<golarion::ContributionGroupSaveData::TargetedContributionSaveData> contributions;
        contributions.reserve(json.at("contributions").size());
        for (const Json &targetedContribution : json.at("contributions"))
        {
            contributions.push_back(golarion::ContributionGroupSaveData::TargetedContributionSaveData{
                .resourceName = targetedContribution.at("resourceName").get<std::string>(),
                .contribution = contributionFromJson(targetedContribution.at("contribution"))
            });
        }
        return golarion::ContributionGroupSaveData{.contributions = std::move(contributions)};
    }

    Json contributionGroupManagerToJson(const golarion::ContributionGroupManagerSaveData &data)
    {
        Json groups = Json::array();
        for (const auto &[id, enabled, group] : data.groups)
        {
            groups.push_back(Json{
                {"id", id},
                {"enabled", enabled},
                {"group", contributionGroupToJson(group)}
            });
        }
        return Json{{"groups", std::move(groups)}};
    }

    golarion::ContributionGroupManagerSaveData contributionGroupManagerFromJson(const Json &json)
    {
        std::vector<golarion::ContributionGroupManagerSaveData::GroupSaveData> groups;
        groups.reserve(json.at("groups").size());
        for (const Json &group : json.at("groups"))
        {
            groups.push_back(golarion::ContributionGroupManagerSaveData::GroupSaveData{
                .id = group.at("id").get<std::string>(),
                .enabled = group.at("enabled").get<bool>(),
                .group = contributionGroupFromJson(group.at("group"))
            });
        }
        return golarion::ContributionGroupManagerSaveData{.groups = std::move(groups)};
    }
}

namespace golarion::persistence
{
    std::string toJson(const CharacterSheetSaveData &data)
    {
        Json abilities = Json::array();
        for (const AbilitySaveData &ability : data.abilities)
        {
            abilities.push_back(abilityToJson(ability));
        }

        return Json{
            {"formatVersion", data.formatVersion},
            {"abilities", std::move(abilities)},
            {"hitPoints", hitPointsToJson(data.hitPoints)},
            {"initiative", initiativeToJson(data.initiative)},
            {"savingThrows", savingThrowsToJson(data.savingThrows)},
            {"skills", skillsToJson(data.skills)},
            {"movementGroups", movementGroupManagerToJson(data.movementGroups)},
            {"modifierGroups", modifierGroupManagerToJson(data.modifierGroups)},
            {"contributionGroups", contributionGroupManagerToJson(data.contributionGroups)}
        }.dump(4);
    }

    CharacterSheetSaveData fromJson(std::string_view value)
    {
        const Json json = Json::parse(value.begin(), value.end());

        std::vector<AbilitySaveData> abilities;
        abilities.reserve(json.at("abilities").size());
        for (const Json &ability : json.at("abilities"))
        {
            abilities.push_back(abilityFromJson(ability));
        }

        MovementGroupManagerSaveData movementGroups;
        if (json.contains("movementGroups"))
        {
            movementGroups = movementGroupManagerFromJson(json.at("movementGroups"));
        }

        return CharacterSheetSaveData{
            .formatVersion = json.at("formatVersion").get<int>(),
            .abilities = std::move(abilities),
            .hitPoints = hitPointsFromJson(json.at("hitPoints")),
            .initiative = initiativeFromJson(json.at("initiative")),
            .savingThrows = savingThrowsFromJson(json.at("savingThrows")),
            .skills = skillsFromJson(json.at("skills")),
            .movementGroups = std::move(movementGroups),
            .modifierGroups = modifierGroupManagerFromJson(json.at("modifierGroups")),
            .contributionGroups = contributionGroupManagerFromJson(json.at("contributionGroups"))
        };
    }

    void saveToFile(const CharacterSheetSaveData &data, const std::filesystem::path &path)
    {
        std::ofstream file(path);
        if (!file)
        {
            throw std::runtime_error("could not open save file: " + path.string());
        }

        file << toJson(data);
        if (!file)
        {
            throw std::runtime_error("could not write save file: " + path.string());
        }
    }

    CharacterSheetSaveData loadFromFile(const std::filesystem::path &path)
    {
        std::ifstream file(path);
        if (!file)
        {
            throw std::runtime_error("could not open save file: " + path.string());
        }

        const std::string json(std::istreambuf_iterator<char>(file), {});
        if (file.bad())
        {
            throw std::runtime_error("could not read save file: " + path.string());
        }

        return fromJson(json);
    }
}
