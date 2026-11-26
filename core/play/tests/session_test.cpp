#include "golarion/play/session_client.hpp"
#include "golarion/play/session_server.hpp"
#include "golarion/play/campaign.hpp"

#include <cassert>
#include <stdexcept>
#include <type_traits>

namespace
{
    using namespace golarion::play;

    template<typename T> concept HasClientState = requires(T &session) { session.getClientState(); };
    template<typename T> concept CanHost = requires(T &session) { session.startHosting("127.0.0.1", 0); };

    template<typename T> concept CanSendEvents = requires(T &p, const nlohmann::json &payload) { p.sendEvent("user", "changed", payload); };
    static_assert(CanHost<ServerProtocol> && !CanHost<ClientProtocol>);
    static_assert(HasClientState<ClientProtocol> && !HasClientState<ServerProtocol>);
    static_assert(CanSendEvents<ServerProtocol> && !CanSendEvents<ClientProtocol>);

    template<typename Function>
    bool rejects(Function function)
    {
        try { function(); }
        catch (const std::invalid_argument &) { return true; }
        return false;
    }

    void connectionLifecycleDelegation()
    {
        SessionServer session("host");
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
        SessionServer first("host");
        SessionServer second("host");
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
        static_assert(!std::is_copy_constructible_v<SessionClient> && !std::is_move_constructible_v<SessionClient>);
        SessionServer host("host");
        SessionClient client;
        assert(host.getSessionCode().size() == 5);
        const auto hostView = host.toView();
        static_assert(std::is_same_v<decltype(host.toView()), HostSessionView>);
        assert(hostView.code.matches(host.getSessionCode()) && !hostView.listeningPort);
        const auto hostData = host.toData();
        assert(hostData.code.matches(host.getSessionCode()) && hostData.identities.hostId == "host");
        host.registerUser("alice");
        assert(host.toData().identities.users.size() == hostData.identities.users.size() + 1);
        const auto clientView = client.toView();
        static_assert(std::is_same_v<decltype(client.toView()), ClientSnapshot>);
        assert(clientView.state == ClientConnectionState::Disconnected);
        const auto state = client.getClientState();
        assert(state.state == ClientConnectionState::Disconnected && !state.userId && !state.permissions);
        client.disconnectFromHost();
        client.disconnectFromHost();
        static_assert(!HasClientState<SessionServer> && !HasClientState<ServerProtocol> && !CanHost<SessionClient>);
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
