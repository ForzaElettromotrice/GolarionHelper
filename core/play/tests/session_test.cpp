#include "golarion/play/session.hpp"

#include <cassert>
#include <stdexcept>
#include <type_traits>

namespace
{
    using namespace golarion::play;

    template<typename Function>
    bool rejects(Function function)
    {
        try { function(); }
        catch (const std::invalid_argument &) { return true; }
        return false;
    }

    void connectionLifecycleDelegation()
    {
        Session session("host");
        const ConnectionId first = session.registerConnection();
        assert(first != 0);
        assert(!session.makeContext(first).userId && session.makeContext(first).connectionId == first);
        assert(!session.bindUser(first, "alice"));
        assert(session.makeContext(first).userId == "alice");
        assert(session.makeContext(first).connectionId == first);

        const ConnectionId replacement = session.registerConnection();
        assert(replacement != first);
        assert(session.bindUser(replacement, "alice") == first);
        assert(rejects([&] { session.makeContext(first); }));
        assert(!session.closeConnection(first));
        assert(session.makeContext(replacement).userId == "alice");
        assert(session.makeContext(replacement).connectionId == replacement);
        assert(session.closeConnection(replacement));
        assert(!session.closeConnection(replacement));
        assert(rejects([&] { session.makeContext(replacement); }));
    }

    void independentSessions()
    {
        Session first("host");
        Session second("host");
        const ConnectionId firstId = first.registerConnection();
        const ConnectionId secondId = second.registerConnection();
        first.bindUser(firstId, "alice");
        second.bindUser(secondId, "bob");
        const ConnectionId replacement = first.registerConnection();
        assert(first.bindUser(replacement, "alice") == firstId);
        assert(second.makeContext(secondId).userId == "bob");
        assert(!first.closeConnection(firstId));
        assert(second.closeConnection(secondId));
        assert(first.makeContext(replacement).userId == "alice");
    }

    void explicitHostAndClientSessions()
    {
        static_assert(!std::is_copy_constructible_v<Session> && !std::is_move_constructible_v<Session>);
        auto host = Session::createHost("host");
        auto client = Session::createClient();
        assert(host.getSessionCode().size() == 5);
        const auto state = client.getClientState();
        assert(state.state == ClientConnectionState::Disconnected && !state.userId && !state.role);
        client.disconnectFromHost();
        client.disconnectFromHost();
        const auto rejectsMode = [](auto function)
        {
            try { function(); }
            catch (const std::logic_error &) { return true; }
            return false;
        };
        assert(rejectsMode([&] { client.startHosting("127.0.0.1", 0); }));
        assert(rejectsMode([&] { client.stopHosting(); }));
        assert(rejectsMode([&] { client.getSessionCode(); }));
        assert(rejectsMode([&] { client.registerUser("alice"); }));
        assert(rejectsMode([&] { client.issueUserToken("alice"); }));
        assert(rejectsMode([&] { client.saveIdentityState("unused-client-identities.json"); }));
        assert(rejectsMode([&] { client.loadIdentityState("unused-client-identities.json"); }));
        assert(rejectsMode([&] { client.registerConnection(); }));
        assert(rejectsMode([&] { client.bindUser(1, "alice"); }));
        assert(rejectsMode([&] { client.makeContext(1); }));
        assert(rejectsMode([&] { client.closeConnection(1); }));
        assert(rejectsMode([&] { host.getClientState(); }));
        assert(rejectsMode([&] { host.disconnectFromHost(); }));
        assert(rejectsMode([&] { host.joinSession("127.0.0.1", 1, SessionCode("A7K2Q")); }));
        assert(rejectsMode([&] { host.resumeSession("127.0.0.1", 1, SessionCode("A7K2Q"), UserToken("0123456789abcdef")); }));
    }
}

int main()
{
    connectionLifecycleDelegation();
    independentSessions();
    explicitHostAndClientSessions();
}
