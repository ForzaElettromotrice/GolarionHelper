#pragma once

#include "golarion/play/participants.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace golarion::play::detail
{
    inline nlohmann::json participantsToJson(const ParticipantsSnapshot &snapshot)
    {
        auto participants = nlohmann::json::array();
        for (const auto &participant : snapshot.participants)
            participants.push_back({{"userId", participant.userId}, {"isHost", participant.isHost}});
        return {{"revision", snapshot.revision}, {"participants", std::move(participants)}};
    }

    inline ParticipantsSnapshot parseParticipants(const nlohmann::json &payload)
    {
        if (!payload.is_object() || payload.size() != 2 || !payload.contains("revision") || !payload.contains("participants")) throw std::invalid_argument("invalid participant snapshot fields");
        const auto &revision = payload.at("revision");
        if (!revision.is_number_integer() || (!revision.is_number_unsigned() && revision.get<std::int64_t>() < 0)) throw std::invalid_argument("participant revision must be a nonnegative integer");
        const auto &entries = payload.at("participants");
        if (!entries.is_array() || entries.empty()) throw std::invalid_argument("participant list must include the local host");
        ParticipantsSnapshot result{revision.get<std::uint64_t>(), {}};
        result.participants.reserve(entries.size());
        std::unordered_set<UserId> ids;
        std::size_t hosts = 0;
        for (const auto &entry : entries)
        {
            if (!entry.is_object() || entry.size() != 2 || !entry.contains("userId") || !entry.contains("isHost") || !entry.at("userId").is_string() || !entry.at("isHost").is_boolean()) throw std::invalid_argument("invalid participant fields");
            auto id = entry.at("userId").get<UserId>();
            if (id.empty() || std::any_of(id.begin(), id.end(), [](unsigned char value) { return value <= 32 || value == 127; }) || !ids.insert(id).second) throw std::invalid_argument("invalid or duplicate participant ID");
            const bool isHost = entry.at("isHost").get<bool>();
            hosts += isHost ? 1 : 0;
            result.participants.push_back({std::move(id), isHost});
        }
        if (hosts != 1) throw std::invalid_argument("participant list must have exactly one host");
        return result;
    }
}
