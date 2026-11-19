#include "golarion/play/session.hpp"
#include "golarion/play/campaign.hpp"

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
        const auto hostView = host.toView();
        assert(hostView.mode == SessionMode::Host && hostView.host && !hostView.client);
        assert(hostView.host->code.matches(host.getSessionCode()) && !hostView.host->listeningPort);
        const auto hostData = host.toData();
        assert(hostData.code.matches(host.getSessionCode()) && hostData.identities.hostId == "host");
        host.registerUser("alice");
        assert(host.toData().identities.users.size() == hostData.identities.users.size() + 1);
        const auto clientView = client.toView();
        assert(clientView.mode == SessionMode::Client && !clientView.host && clientView.client);
        assert(clientView.client->state == ClientConnectionState::Disconnected);
        const auto state = client.getClientState();
        assert(state.state == ClientConnectionState::Disconnected && !state.userId && !state.permissions);
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
        assert(rejectsMode([&] { client.toData(); }));
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

    void campaignSnapshots()
    {
        const SessionCode initialCode("A7K2Q");
        const UserToken token("0123456789abcdef");
        const HostedCampaign hosted(HostedCampaignData{"hosted", "Campagna", initialCode});
        assert(hosted.toView().name == "Campagna" && hosted.toView().code->matches(initialCode.value()));
        assert(hosted.toData().code->matches(initialCode.value()));

        JoinedCampaign joined(JoinedCampaignData{"joined", "127.0.0.1:42500", initialCode, "Campagna", token});
        const auto original = joined.toView();
        joined.updateConnection("127.0.0.1:42501", SessionCode("B8L3R"), "");
        assert(original.address == "127.0.0.1:42500" && original.code.matches(initialCode.value()));
        const auto data = joined.toData();
        assert(data.id == "joined" && data.name == "Campagna" && data.token.value() == token.value());
        assert(joined.toView().address == data.address && data.code.matches("B8L3R"));
    }
}

int main()
{
    connectionLifecycleDelegation();
    independentSessions();
    explicitHostAndClientSessions();
    campaignSnapshots();
}
