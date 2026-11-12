#include "golarion/play/session.hpp"

#include <utility>

namespace golarion::play
{
    Session::Session(UserId hostId) : protocolManager_(resourceManager_), roleManager_(resourceManager_, std::move(hostId))
    {
    }

    Session::~Session()
    {
        try { stopHosting(); }
        catch (...) { } // Stop networking before any subsystem is destroyed.
    }

    std::uint16_t Session::startHosting(std::string_view bindAddress, std::uint16_t port)
    {
        return protocolManager_.startHosting(bindAddress, port);
    }

    void Session::stopHosting()
    {
        protocolManager_.stopHosting();
    }

    std::string Session::getSessionCode() const
    {
        return protocolManager_.getSessionCode();
    }

    void Session::registerUser(UserId userId)
    {
        roleManager_.registerUser(std::move(userId));
    }

    std::string Session::issueUserToken(const UserId &userId)
    {
        return protocolManager_.issueUserToken(userId);
    }

    void Session::saveIdentityState(const std::filesystem::path &path)
    {
        protocolManager_.saveIdentityState(path);
    }

    void Session::loadIdentityState(const std::filesystem::path &path)
    {
        protocolManager_.loadIdentityState(path);
    }

    ConnectionId Session::registerConnection()
    {
        return protocolManager_.registerConnection();
    }

    std::optional<ConnectionId> Session::bindUser(ConnectionId connectionId, const UserId &userId)
    {
        return protocolManager_.bindUser(connectionId, userId);
    }

    CommandContext Session::makeContext(ConnectionId connectionId) const
    {
        return protocolManager_.makeContext(connectionId);
    }

    bool Session::closeConnection(ConnectionId connectionId)
    {
        return protocolManager_.closeConnection(connectionId);
    }
}
