#include "golarion/play/protocol_manager.hpp"
#include "participants_json.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace golarion::play
{
    void ProtocolManager::registerParticipantCommands()
    {
        add_command("session.getParticipants", CommandRequirements{}, [this](const CommandContext &, const nlohmann::json &payload)
        {
            if (!payload.empty()) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "session.getParticipants requires an empty payload");
            return detail::participantsToJson(getParticipants());
        });
    }

    ParticipantsSnapshot ProtocolManager::getParticipants() const
    {
        requireHostMode();
        const std::lock_guard executionLock(executionMutex_);
        const auto hostId = resources_.invoke<PermissionManager::GetHostIdSignature>("permissions.getHostId");
        ParticipantsSnapshot result{participantsRevision_, {{hostId, true}}};
        const std::lock_guard connectionsLock(connectionsMutex_);
        for (const auto &[userId, connectionId] : activeConnections_)
        {
            const auto connection = connections_.find(connectionId);
            if (userId != hostId && connection != connections_.end() && connection->second == userId) result.participants.push_back({userId, false});
        }
        std::sort(result.participants.begin() + 1, result.participants.end(), [](const auto &left, const auto &right) { return left.userId < right.userId; });
        return result;
    }

    void ProtocolManager::sendParticipantsSnapshot(const UserId &userId) noexcept
    {
        try
        {
            const std::lock_guard executionLock(executionMutex_);
            {
                const std::lock_guard eventsLock(eventsMutex_);
                if (!acceptingEvents_) return;
            }
            sendEvent(userId, "session.participantsChanged", detail::participantsToJson(getParticipants()));
        }
        catch (...) { failParticipantsDelivery(userId); }
    }

    void ProtocolManager::publishParticipantsChanged() noexcept
    {
        try
        {
            const std::lock_guard executionLock(executionMutex_);
            if (participantsRevision_ == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("participant revision space is exhausted");
            ++participantsRevision_;
            {
                const std::lock_guard eventsLock(eventsMutex_);
                if (!acceptingEvents_) return;
            }
            const auto snapshot = getParticipants();
            const auto payload = detail::participantsToJson(snapshot);
            for (const auto &participant : snapshot.participants)
                sendEvent(participant.userId, "session.participantsChanged", payload);
        }
        catch (...) { failParticipantsDelivery(); }
    }

    void ProtocolManager::failParticipantsDelivery(std::optional<std::string_view> userId) noexcept
    {
        try
        {
            const std::lock_guard connectionsLock(connectionsMutex_);
            const std::lock_guard eventsLock(eventsMutex_);
            if (!acceptingEvents_) return;
            for (const auto &[id, connectionId] : activeConnections_)
            {
                if (userId && id != *userId) continue;
                auto &queue = events_[connectionId];
                queue.overflowed = true;
                queue.frames.clear();
                queue.bytes = 0;
            }
        }
        catch (...) { } // Preserve committed bindings even if failure cleanup cannot allocate.
    }
}
