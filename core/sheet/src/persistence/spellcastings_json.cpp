#include "spellcastings_json.hpp"

#include <nlohmann/json.hpp>
#include <array>
#include <limits>
#include <stdexcept>

namespace
{
    using namespace golarion;
    using Json = nlohmann::json;

    constexpr std::array MetamagicNames{
        std::pair{MetamagicType::Empower, "empower"}, std::pair{MetamagicType::Enlarge, "enlarge"},
        std::pair{MetamagicType::Extend, "extend"}, std::pair{MetamagicType::Heighten, "heighten"},
        std::pair{MetamagicType::Maximize, "maximize"}, std::pair{MetamagicType::Quicken, "quicken"},
        std::pair{MetamagicType::Silent, "silent"}, std::pair{MetamagicType::Still, "still"},
        std::pair{MetamagicType::Widen, "widen"}, std::pair{MetamagicType::Elemental, "elemental"},
        std::pair{MetamagicType::Reach, "reach"}, std::pair{MetamagicType::Intensified, "intensified"},
        std::pair{MetamagicType::Selective, "selective"}
    };
    constexpr std::array EnergyNames{
        std::pair{DamageType::Acid, "acid"}, std::pair{DamageType::Cold, "cold"},
        std::pair{DamageType::Electricity, "electricity"}, std::pair{DamageType::Fire, "fire"}
    };
    constexpr std::array DamageModeNames{
        std::pair{ElementalDamageMode::Replace, "replace"}, std::pair{ElementalDamageMode::Split, "split"}
    };

    template<typename Enum, std::size_t N> std::string name(Enum value, const std::array<std::pair<Enum, const char *>, N> &names)
    {
        for (const auto &[candidate, text] : names) if (candidate == value) return text;
        throw std::invalid_argument("unknown spellcasting enum value");
    }

    template<typename Enum, std::size_t N> Enum value(const Json &json, const std::array<std::pair<Enum, const char *>, N> &names)
    {
        const auto text = json.get<std::string>();
        for (const auto &[candidate, label] : names) if (text == label) return candidate;
        throw std::invalid_argument("unknown spellcasting enum name: " + text);
    }

    const Json &array(const Json &json)
    {
        if (!json.is_array()) throw std::invalid_argument("spellcasting field must be an array");
        return json;
    }

    template<typename Integer> Integer integer(const Json &json, Integer minimum, Integer maximum)
    {
        if (!json.is_number_integer() || (!json.is_number_unsigned() && json.get<std::int64_t>() < 0))
            throw std::invalid_argument("spellcasting field must be a nonnegative integer");
        const auto number = json.get<std::uint64_t>();
        if (number < static_cast<std::uint64_t>(minimum) || number > static_cast<std::uint64_t>(maximum))
            throw std::invalid_argument("spellcasting integer is out of range");
        return static_cast<Integer>(number);
    }

    Json slotToJson(const SpellSlotId &slot)
    {
        if (slot.spellLevel < 0 || slot.spellLevel > 9) throw std::invalid_argument("invalid spell slot level");
        return {{"groupId", slot.groupId}, {"spellLevel", slot.spellLevel}, {"index", slot.index}};
    }

    SpellSlotId slotFromJson(const Json &json)
    {
        return {json.at("groupId").get<std::string>(), integer<int>(json.at("spellLevel"), 0, 9),
            integer<std::size_t>(json.at("index"), 0, std::numeric_limits<std::size_t>::max())};
    }

    Json slotsToJson(const std::vector<SpellSlotId> &slots)
    {
        auto json = Json::array();
        for (const auto &slot : slots) json.push_back(slotToJson(slot));
        return json;
    }

    std::vector<SpellSlotId> slotsFromJson(const Json &json)
    {
        std::vector<SpellSlotId> slots;
        for (const auto &slot : array(json)) slots.push_back(slotFromJson(slot));
        return slots;
    }

    MetamagicApplication applicationFromJson(const Json &json)
    {
        MetamagicApplication application{value(json.at("type"), MetamagicNames), json.at("grantId").get<std::string>(), {}};
        const auto &options = json.at("options");
        switch (application.type)
        {
        case MetamagicType::Heighten:
            if (!options.is_object() || options.size() != 1) throw std::invalid_argument("invalid heighten options");
            application.options = HeightenMetamagicOptions{integer<int>(options.at("effectiveSpellLevel"), 1, 9)};
            break;
        case MetamagicType::Reach:
            if (!options.is_object() || options.size() != 1) throw std::invalid_argument("invalid reach options");
            application.options = ReachMetamagicOptions{integer<int>(options.at("rangeSteps"), 1, 3)};
            break;
        case MetamagicType::Elemental:
            if (!options.is_object() || options.size() != 2) throw std::invalid_argument("invalid elemental options");
            application.options = ElementalMetamagicOptions{value(options.at("energy"), EnergyNames), value(options.at("mode"), DamageModeNames)};
            break;
        default:
            if (!options.is_null()) throw std::invalid_argument("metamagic does not accept options");
        }
        return application;
    }

    Json applicationsToJson(const std::vector<MetamagicApplication> &applications)
    {
        auto json = Json::array();
        for (const auto &application : applications)
        {
            Json options = nullptr;
            std::visit([&](const auto &choice)
            {
                using T = std::decay_t<decltype(choice)>;
                if constexpr (std::is_same_v<T, HeightenMetamagicOptions>) options = {{"effectiveSpellLevel", choice.effectiveSpellLevel}};
                else if constexpr (std::is_same_v<T, ReachMetamagicOptions>) options = {{"rangeSteps", choice.rangeSteps}};
                else if constexpr (std::is_same_v<T, ElementalMetamagicOptions>) options = {{"energy", name(choice.energy, EnergyNames)}, {"mode", name(choice.mode, DamageModeNames)}};
            }, application.options);
            Json entry{{"type", name(application.type, MetamagicNames)}, {"grantId", application.grantId}, {"options", options}};
            applicationFromJson(entry); // Apply the same structural checks on both boundaries.
            json.push_back(std::move(entry));
        }
        return json;
    }

    std::vector<MetamagicApplication> applicationsFromJson(const Json &json)
    {
        std::vector<MetamagicApplication> applications;
        for (const auto &entry : array(json)) applications.push_back(applicationFromJson(entry));
        return applications;
    }

    std::optional<PreparedMetamagicData> snapshotFromJson(const Json &json)
    {
        if (json.is_null()) return std::nullopt;
        return PreparedMetamagicData{integer<int>(json.at("baseSpellLevel"), 0, 9), integer<int>(json.at("effectiveSpellLevel"), 0, 9),
            integer<int>(json.at("requiredSlotLevel"), 0, 9), integer<int>(json.at("nominalSlotLevel"), 0, std::numeric_limits<int>::max()),
            characterClassFromId(json.at("spellListClass").get<std::string>()), integer<int>(json.at("paidSlots"), 1, std::numeric_limits<int>::max())};
    }

    Json snapshotToJson(const std::optional<PreparedMetamagicData> &snapshot, bool required)
    {
        if (!snapshot)
        {
            if (required) throw std::invalid_argument("prepared evaluation is required in this save format");
            return nullptr;
        }
        Json json{{"baseSpellLevel", snapshot->baseSpellLevel}, {"effectiveSpellLevel", snapshot->effectiveSpellLevel},
            {"requiredSlotLevel", snapshot->requiredSlotLevel}, {"nominalSlotLevel", snapshot->nominalSlotLevel},
            {"spellListClass", characterClassId(snapshot->spellListClass)}, {"paidSlots", snapshot->paidSlots}};
        snapshotFromJson(json);
        return json;
    }

    std::optional<PreparedMetamagicData> readSnapshot(const Json &selection, bool required)
    {
        if (!required) return std::nullopt; // Pre-34 saves must use legacy validation.
        const auto snapshot = snapshotFromJson(selection.at("evaluation"));
        if (!snapshot) throw std::invalid_argument("missing prepared evaluation");
        return snapshot;
    }
}

namespace golarion::persistence
{
    Json spellcastingsToJson(const SpellcastingsData &data, bool requireSnapshots)
    {
        auto json = Json::array();
        for (const auto &casting : data.castings)
        {
            Json entry{{"id", casting.id}, {"repertoire", casting.repertoire}};
            std::visit([&](const auto &state)
            {
                using T = std::decay_t<decltype(state)>;
                if constexpr (std::is_same_v<T, PreparedSpellcastingData>)
                {
                    entry["mode"] = "prepared";
                    entry["preparations"] = Json::array();
                    for (const auto &selection : state.preparations)
                        entry["preparations"].push_back({{"slot", slotToJson(selection.slot)}, {"spellId", selection.spellId},
                            {"consumed", selection.consumed}, {"additionalSlots", slotsToJson(selection.additionalSlots)}, {"metamagic", applicationsToJson(selection.metamagic)}, {"evaluation", snapshotToJson(selection.evaluation, requireSnapshots)}});
                }
                else
                {
                    entry["consumedSlots"] = slotsToJson(state.consumedSlots);
                    if constexpr (std::is_same_v<T, SpontaneousSpellcastingData>) entry["mode"] = "spontaneous";
                    else
                    {
                        entry["mode"] = "flexible";
                        entry["preparations"] = Json::array();
                        for (const auto &selection : state.preparations)
                            entry["preparations"].push_back({{"preparationId", slotToJson(selection.preparationId)}, {"spellId", selection.spellId}, {"metamagic", applicationsToJson(selection.metamagic)}, {"evaluation", snapshotToJson(selection.evaluation, requireSnapshots)}});
                    }
                }
            }, casting.state);
            json.push_back(std::move(entry));
        }
        return json;
    }

    SpellcastingsData spellcastingsFromJson(const Json &json, bool requireSnapshots)
    {
        SpellcastingsData data;
        for (const auto &entry : array(json))
        {
            SpellcastingData casting{entry.at("id").get<std::string>(), array(entry.at("repertoire")).get<std::vector<std::string>>(), {}};
            const auto mode = entry.at("mode").get<std::string>();
            if (mode == "prepared")
            {
                PreparedSpellcastingData state;
                for (const auto &selection : array(entry.at("preparations")))
                    state.preparations.push_back({slotFromJson(selection.at("slot")), selection.at("spellId").get<std::string>(),
                        selection.at("consumed").get<bool>(), slotsFromJson(selection.at("additionalSlots")), applicationsFromJson(selection.at("metamagic")), readSnapshot(selection, requireSnapshots)});
                casting.state = std::move(state);
            }
            else if (mode == "spontaneous") casting.state = SpontaneousSpellcastingData{slotsFromJson(entry.at("consumedSlots"))};
            else if (mode == "flexible")
            {
                FlexibleSpellcastingData state;
                state.consumedSlots = slotsFromJson(entry.at("consumedSlots"));
                for (const auto &selection : array(entry.at("preparations")))
                    state.preparations.push_back({slotFromJson(selection.at("preparationId")), selection.at("spellId").get<std::string>(), applicationsFromJson(selection.at("metamagic")), readSnapshot(selection, requireSnapshots)});
                casting.state = std::move(state);
            }
            else throw std::invalid_argument("unknown spellcasting mode: " + mode);
            data.castings.push_back(std::move(casting));
        }
        return data;
    }

    std::map<std::string, int> metamagicUsesFromJson(const Json &json)
    {
        if (!json.is_object()) throw std::invalid_argument("metamagic uses must be an object");
        std::map<std::string, int> uses;
        for (const auto &[id, remaining] : json.items())
        {
            if (id.empty() || id.find_first_not_of(" \t\r\n") == std::string::npos)
                throw std::invalid_argument("metamagic pool ID must not be blank");
            uses.emplace(id, integer<int>(remaining, 0, std::numeric_limits<int>::max()));
        }
        return uses;
    }

    Json metamagicUsesToJson(const std::optional<std::map<std::string, int>> &uses)
    {
        if (!uses) throw std::invalid_argument("metamagic uses are required in this save format");
        Json json = *uses;
        metamagicUsesFromJson(json);
        return json;
    }
}
