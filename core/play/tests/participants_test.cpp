#include "socket_support.hpp"
#include "protocol.hpp"
#include "participants_json.hpp"
#include "golarion/play/session.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <thread>

namespace
{
    using namespace golarion::play;
    using namespace std::chrono_literals;
    using Json = nlohmann::json;
    using Clock = std::chrono::steady_clock;

    void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
    template<typename Exception, typename Function> bool rejects(Function function)
    {
        try { function(); }
        catch (const Exception &) { return true; }
        return false;
    }
    template<typename T> T completed(std::future<T> result)
    {
        check(result.wait_for(3s) == std::future_status::ready, "operation did not complete");
        return result.get();
    }
    template<typename Predicate> void eventually(Predicate predicate)
    {
        const auto deadline = Clock::now() + 3s;
        while (!predicate())
        {
            check(Clock::now() < deadline, "participant snapshot did not settle");
            std::this_thread::sleep_for(5ms);
        }
    }
    struct Directory
    {
        std::filesystem::path path = std::filesystem::current_path() / ("participants-test-" + UserToken::generate().value());
        Directory() { check(std::filesystem::create_directory(path), "cannot create test directory"); }
        ~Directory()
        {
            std::error_code error;
            for (std::filesystem::directory_iterator it(path, error), end; it != end && !error; it.increment(error)) std::filesystem::remove(it->path(), error);
            std::filesystem::remove(path, error);
        }
    };
    bool ready(const detail::Socket &socket, bool writing, Clock::time_point deadline)
    {
        while (Clock::now() < deadline)
        {
            fd_set descriptors;
            FD_ZERO(&descriptors);
            FD_SET(socket.get(), &descriptors);
            timeval timeout{0, 10000};
#if defined(_WIN32)
            const int selected = ::select(0, writing ? nullptr : &descriptors, writing ? &descriptors : nullptr, nullptr, &timeout);
#else
            const int selected = ::select(socket.get() + 1, writing ? nullptr : &descriptors, writing ? &descriptors : nullptr, nullptr, &timeout);
#endif
            if (selected > 0) return true;
            if (selected < 0 && !detail::retryable(detail::socketError())) detail::fail("select");
        }
        return false;
    }
    void writeBytes(const detail::Socket &socket, const std::vector<std::uint8_t> &bytes)
    {
        const auto deadline = Clock::now() + 3s;
        std::size_t sent = 0;
        while (sent < bytes.size())
        {
            check(ready(socket, true, deadline), "socket did not become writable");
            int flags = 0;
#if defined(MSG_NOSIGNAL)
            flags = MSG_NOSIGNAL;
#endif
            const int count = static_cast<int>(::send(socket.get(), reinterpret_cast<const char *>(bytes.data() + sent), static_cast<int>(bytes.size() - sent), flags));
            if (count < 0 && detail::retryable(detail::socketError())) continue;
            check(count > 0, "socket write failed");
            sent += static_cast<std::size_t>(count);
        }
    }
    detail::ProtocolMessage readMessage(const detail::Socket &socket, std::vector<std::uint8_t> &input)
    {
        const auto deadline = Clock::now() + 3s;
        while (true)
        {
            if (auto message = detail::decodeMessage(input)) return *message;
            check(ready(socket, false, deadline), "socket did not receive a frame");
            std::array<std::uint8_t, 4096> bytes{};
            const int count = static_cast<int>(::recv(socket.get(), reinterpret_cast<char *>(bytes.data()), static_cast<int>(bytes.size()), 0));
            if (count < 0 && detail::retryable(detail::socketError())) continue;
            check(count > 0, "socket closed before a complete frame");
            input.insert(input.end(), bytes.begin(), bytes.begin() + count);
        }
    }
    detail::Socket connectPeer(std::uint16_t port)
    {
        detail::Socket socket(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
        check(socket.get() != detail::InvalidSocket, "cannot create socket");
        const auto address = detail::ipv4Address("127.0.0.1", port);
        check(::connect(socket.get(), reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == 0, "cannot connect peer");
        detail::configureSocket(socket.get());
        return socket;
    }
    ParticipantsSnapshot query(Session &session)
    {
        const auto result = completed(session.sendCommand("session.getParticipants", Json::object()));
        check(result.size() == 2 && result.contains("revision") && result.contains("participants"), "unexpected participant result schema");
        return detail::parseParticipants(result);
    }
    void rejected(std::future<Json> result, ProtocolErrorCode expected)
    {
        bool correct = false;
        try { completed(std::move(result)); }
        catch (const ProtocolError &error) { correct = error.code() == expected; }
        check(correct, "wrong participant command rejection");
    }
    Json snapshotJson(std::uint64_t revision, bool withBob = false)
    {
        auto entries = Json::array({{{"userId", "host"}, {"isHost", true}, {"displayName", "Marta"}},
            {{"userId", "alice"}, {"isHost", false}, {"displayName", "Alice"}}});
        if (withBob) entries.push_back({{"userId", "bob"}, {"isHost", false}, {"displayName", "Bob"}});
        return {{"revision", revision}, {"participants", std::move(entries)}};
    }

    void strictSnapshotCodec()
    {
        const auto valid = snapshotJson(0);
        const auto parsed = detail::parseParticipants(valid);
        check(parsed.revision == 0 && parsed.participants == std::vector<ParticipantInfo>{{"host", true, "Marta"}, {"alice", false, "Alice"}}, "participant codec changed its data");
        check(detail::participantsToJson(parsed) == valid, "participant serialization lost fields");
        auto maximum = valid;
        maximum["revision"] = std::numeric_limits<std::uint64_t>::max();
        check(detail::parseParticipants(maximum).revision == std::numeric_limits<std::uint64_t>::max(), "uint64 revision was truncated");
        std::vector<Json> invalid;
        for (const auto &value : std::array<Json, 7>{-1, 1.0, true, "1", nullptr, Json::array(), Json::object()})
        {
            auto payload = valid;
            payload["revision"] = value;
            invalid.push_back(std::move(payload));
        }
        for (const auto &value : std::array<Json, 4>{nullptr, Json::object(), Json::array(), false})
        {
            auto payload = valid;
            payload["participants"] = value;
            invalid.push_back(std::move(payload));
        }
        for (int variant = 0; variant < 12; ++variant)
        {
            auto payload = valid;
            auto &entry = payload["participants"][1];
            if (variant == 0) entry["userId"] = "host";
            if (variant == 1) entry["userId"] = "bad id";
            if (variant == 2) entry["userId"] = "";
            if (variant == 3) entry["isHost"] = true;
            if (variant == 4) entry["isHost"] = 0;
            if (variant == 5) entry["permissions"] = 7;
            if (variant == 6) payload["participants"][0]["isHost"] = false;
            if (variant == 7) payload["extra"] = true;
            if (variant == 8) payload.erase("revision");
            if (variant == 9) entry.erase("displayName");
            if (variant == 10) entry["displayName"] = " ";
            if (variant == 11) entry["displayName"] = std::string(49, 'a');
            invalid.push_back(std::move(payload));
        }
        for (const auto &payload : invalid) check(rejects<std::invalid_argument>([&] { detail::parseParticipants(payload); }), "invalid participant snapshot accepted");
    }

    void connectedListAndPresence()
    {
        Directory directory;
        const auto archive = directory.path / "identities.json";
        auto host = Session::createHost("host");
        host.setHostDisplayName("Marta");
        host.registerUser("remembered-offline");
        const auto initial = query(host);
        check(initial == ParticipantsSnapshot{1, {{"host", true, "Marta"}}}, "offline membership was advertised as connected");
        host.saveIdentityState(archive);
        const auto port = host.startHosting("127.0.0.1", 0);
        const SessionCode code(host.getSessionCode());
        {
            auto pending = connectPeer(port);
            std::vector<std::uint8_t> input;
            writeBytes(pending, detail::encodeMessage({"session.getParticipants", "pending", Json::object()}));
            const auto reply = readMessage(pending, input);
            check(reply.type == "request.rejected" && reply.payload.at("code") == "FORBIDDEN", "participant listing became public before admission");
        }
        check(query(host) == initial, "pending connection changed participant presence");
        auto first = Session::createClient();
        auto observer = Session::createClient();
        const auto a = completed(first.joinSession("127.0.0.1", port, code, "  Giulia  "));
        eventually([&] { return first.getClientState().participants && first.getClientState().participants->participants.size() == 2; });
        check(first.getClientState().participants->revision == 2, "first admission did not advance presence");
        const auto b = completed(observer.joinSession("127.0.0.1", port, code, "Giulia"));
        const auto both = query(host);
        check(both.revision == 3 && both.participants.size() == 3 && both.participants.front() == ParticipantInfo{"host", true, "Marta"}
            && both.participants[1].displayName == "Giulia" && both.participants[2].displayName == "Giulia", "host list lost display names");
        eventually([&] { return first.getClientState().participants == both && observer.getClientState().participants == both; });
        const auto hostView = host.toView();
        check(hostView.host && hostView.host->listeningPort == port && hostView.host->code.matches(code.value()), "host view omitted the running listener");
        check(query(observer) == both && hostView.host->participants == both && observer.toView().client->participants == both, "query, view and event snapshots disagree");
        auto detached = observer.getClientState();
        detached.participants->participants.clear();
        check(observer.getClientState().participants == both, "client participant snapshot aliases mutable state");
        rejected(observer.sendCommand("session.getParticipants", {{"userId", "host"}}), ProtocolErrorCode::InvalidRequest);

        const auto backup = directory.path / "backup.json";
        std::filesystem::rename(archive, backup);
        check(std::filesystem::create_directory(archive), "cannot block admission persistence");
        auto failed = Session::createClient();
        bool failedAdmission = false;
        try { completed(failed.joinSession("127.0.0.1", port, code)); }
        catch (const ProtocolError &error) { failedAdmission = error.code() == ProtocolErrorCode::AdmissionFailed; }
        check(failedAdmission && query(host) == both, "failed admission announced a participant");
        failedAdmission = false;
        try { completed(failed.resumeSession("127.0.0.1", port, code, *a.userToken, "Rename failed")); }
        catch (const ProtocolError &error) { failedAdmission = error.code() == ProtocolErrorCode::AdmissionFailed; }
        check(failedAdmission && query(host) == both && first.getClientState().state == ClientConnectionState::Connected,
            "failed rename changed the live name or replaced the existing connection");
        std::filesystem::remove(archive);
        std::filesystem::rename(backup, archive);

        for (const auto flag : {Permission::Permission1, Permission::Permission2, Permission::Permission3})
            completed(host.sendCommand("permissions.setPermission", {{"targetUserId", b.userId}, {"permission", static_cast<std::uint64_t>(flag)}, {"enabled", false}}));
        eventually([&] { return observer.getClientState().permissions == PermissionSet{}; });
        check(query(observer) == both, "zero-permission user cannot read participant presence");

        auto replacement = Session::createClient();
        const auto resumed = completed(replacement.resumeSession("127.0.0.1", port, code, *a.userToken, "Giulia R."));
        auto renamed = both;
        ++renamed.revision;
        for (auto &entry : renamed.participants)
            if (entry.userId == a.userId) entry.displayName = "Giulia R.";
        eventually([&] { return replacement.getClientState().participants == renamed && observer.getClientState().participants == renamed
            && first.getClientState().state == ClientConnectionState::Disconnected; });
        check(!first.getClientState().participants && query(host) == renamed && resumed.userId == a.userId
            && resumed.permissions == a.permissions && !resumed.userToken, "rename on resume changed identity or duplicated presence");
        replacement.disconnectFromHost();
        eventually([&] { return observer.getClientState().participants && observer.getClientState().participants->participants.size() == 2; });
        const auto departed = query(host);
        check(departed.revision == 5 && departed.participants == std::vector<ParticipantInfo>{{"host", true, "Marta"}, {b.userId, false, "Giulia"}}, "disconnect removed the wrong participant");
        completed(first.resumeSession("127.0.0.1", port, code, *a.userToken));
        eventually([&] { return observer.getClientState().participants && observer.getClientState().participants->revision == 6; });
        check(query(host).participants == renamed.participants, "resume without a name did not retain the stored name");
        host.stopHosting();
        eventually([&] { return first.getClientState().state == ClientConnectionState::Disconnected && observer.getClientState().state == ClientConnectionState::Disconnected; });
        check(!first.getClientState().participants && !observer.getClientState().participants && query(host).participants == initial.participants, "stop retained remote participant presence");
        const auto stopped = host.toView();
        check(stopped.host && !stopped.host->listeningPort && stopped.host->participants.participants == initial.participants, "stopped view retained listening or presence state");
        check(hostView.host->listeningPort == port && hostView.host->participants == both, "previous view changed after stop");
        const auto saved = host.toData();
        check(saved.code.matches(code.value()) && saved.identities.users.size() == 4, "disconnect changed the code or deleted persistent users");
    }

    void rawInitialSnapshotAndHostReconstruction()
    {
        Directory directory;
        const auto archive = directory.path / "identities.json";
        std::optional<Session> host(std::in_place, "host");
        host->saveIdentityState(archive);
        auto port = host->startHosting("127.0.0.1", 0);
        SessionCode code(host->getSessionCode());
        auto peer = connectPeer(port);
        std::vector<std::uint8_t> input;
        writeBytes(peer, detail::encodeMessage({"session.join", "raw-join", {{"sessionCode", code.value()}, {"displayName", "Giulia"}}}));
        const auto reply = readMessage(peer, input);
        check(reply.type == "request.accepted" && reply.requestId == "raw-join", "initial snapshot overtook the admission result");
        const auto user = reply.payload.at("result").at("userId").get<UserId>();
        const UserToken token(reply.payload.at("result").at("userToken").get<std::string>());
        const auto event = readMessage(peer, input);
        check(event.type == "session.participantsChanged" && !event.requestId && detail::parseParticipants(event.payload) == query(*host), "initial participant event is missing or invalid");
        peer = detail::Socket{};
        eventually([&] { return query(*host).participants.size() == 1; });
        auto client = Session::createClient();
        completed(client.resumeSession("127.0.0.1", port, code, token));
        eventually([&] { return client.getClientState().participants.has_value(); });
        const auto previousRevision = client.getClientState().participants->revision;
        host.reset();
        eventually([&] { return client.getClientState().state == ClientConnectionState::Disconnected; });
        {
            std::ifstream inputArchive(archive);
            auto v2 = Json::parse(inputArchive);
            inputArchive.close();
            check(v2.at("version") == 3, "unexpected archive version");
            v2["version"] = 2;
            for (auto &entry : v2["users"]) entry.erase("displayName");
            std::ofstream(archive) << v2;
        }
        host.emplace("host");
        host->loadIdentityState(archive);
        check(query(*host).participants == std::vector<ParticipantInfo>{{"host", true, "Host"}}, "restoring v2 identities restored stale connections");
        port = host->startHosting("127.0.0.1", 0);
        code = SessionCode(host->getSessionCode());
        const auto resumed = completed(client.resumeSession("127.0.0.1", port, code, token, "No\xc3\xab" "mie"));
        eventually([&] { return client.getClientState().participants.has_value(); });
        check(resumed.userId == user && client.getClientState().participants->revision < previousRevision && client.getClientState().participants == query(*host), "client did not reset revisions across host reconstruction");
        const auto migrated = Json::parse(std::ifstream(archive));
        check(migrated.at("version") == 3, "successful resume did not upgrade v2");
        check(client.getClientState().participants->participants.back().displayName == "No\xc3\xab" "mie", "resume lost the UTF-8 name");
        host->stopHosting();
        client.disconnectFromHost();
        host.reset();
        host.emplace("host");
        host->loadIdentityState(archive);
        port = host->startHosting("127.0.0.1", 0);
        completed(client.resumeSession("127.0.0.1", port, SessionCode(host->getSessionCode()), token));
        eventually([&] { return client.getClientState().participants.has_value(); });
        check(client.getClientState().participants->participants.back().displayName == "No\xc3\xab" "mie", "v3 reload lost the saved name");
    }

    class ScriptedHost final
    {
    public:
        using Script = std::function<void(const detail::Socket &, const std::atomic<bool> &)>;
        explicit ScriptedHost(Script script) : listener_(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
        {
            check(listener_.get() != detail::InvalidSocket, "cannot create scripted listener");
            detail::configureSocket(listener_.get());
            auto address = detail::ipv4Address("127.0.0.1", 0);
            check(::bind(listener_.get(), reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == 0 && ::listen(listener_.get(), 1) == 0, "cannot listen");
            detail::AddressLength length = sizeof(address);
            check(getsockname(listener_.get(), reinterpret_cast<sockaddr *>(&address), &length) == 0, "missing scripted port");
            port_ = ntohs(address.sin_port);
            task_ = std::async(std::launch::async, [this, script = std::move(script)]
            {
                check(ready(listener_, false, Clock::now() + 3s), "no scripted client");
                detail::Socket peer(::accept(listener_.get(), nullptr, nullptr));
                check(peer.get() != detail::InvalidSocket, "scripted accept failed");
                detail::configureSocket(peer.get());
                std::vector<std::uint8_t> input;
                const auto request = readMessage(peer, input);
                writeBytes(peer, detail::encodeMessage({"request.accepted", request.requestId, {{"result", {{"userId", "alice"}, {"permissions", 0}, {"userToken", "0123456789abcdef"}}}}}));
                script(peer, stop_);
            });
        }
        ~ScriptedHost() { stop_ = true; if (task_.valid()) task_.wait(); }
        std::uint16_t port() const { return port_; }
        void finish() { stop_ = true; task_.get(); }
    private:
        detail::Socket listener_;
        std::uint16_t port_ = 0;
        std::atomic<bool> stop_{false};
        std::future<void> task_;
    };

    void fragmentedSnapshotsAndRevisionChecks()
    {
        for (int variant = 0; variant < 5; ++variant)
        {
            std::promise<void> prefixSent, releasePrefix, releaseInvalid;
            auto prefixReleased = releasePrefix.get_future().share();
            auto invalidReleased = releaseInvalid.get_future().share();
            ScriptedHost server([&](const auto &peer, const auto &stop)
            {
                const auto event = detail::encodeMessage({"session.participantsChanged", std::nullopt, snapshotJson(8)});
                writeBytes(peer, {event.begin(), event.begin() + 2});
                prefixSent.set_value();
                check(prefixReleased.wait_for(3s) == std::future_status::ready, "partial snapshot was not released");
                auto remaining = std::vector<std::uint8_t>(event.begin() + 2, event.end());
                for (const auto &payload : {snapshotJson(7, true), snapshotJson(8), snapshotJson(9, true)})
                {
                    const auto bytes = detail::encodeMessage({"session.participantsChanged", std::nullopt, payload});
                    remaining.insert(remaining.end(), bytes.begin(), bytes.end());
                }
                writeBytes(peer, remaining);
                check(invalidReleased.wait_for(3s) == std::future_status::ready, "invalid snapshot was not released");
                auto invalid = snapshotJson(10, true);
                if (variant == 0) invalid = snapshotJson(9); // Same revision, conflicting state.
                if (variant == 1) invalid["participants"][0]["userId"] = "different-host";
                if (variant == 2) invalid["participants"].erase(invalid["participants"].begin() + 1);
                if (variant == 3) invalid["participants"][1]["permissions"] = 63;
                if (variant == 4) invalid["revision"] = true;
                writeBytes(peer, detail::encodeMessage({"session.participantsChanged", std::nullopt, invalid}));
                while (!stop) std::this_thread::sleep_for(5ms);
            });
            auto client = Session::createClient();
            const auto admission = completed(client.joinSession("127.0.0.1", server.port(), SessionCode("A7K2Q")));
            completed(prefixSent.get_future());
            check(!client.getClientState().participants, "partial snapshot was applied");
            releasePrefix.set_value();
            eventually([&] { return client.getClientState().participants && client.getClientState().participants->revision == 9; });
            check(client.getClientState().participants == detail::parseParticipants(snapshotJson(9, true)), "stale or coalesced snapshots overwrote current state");
            releaseInvalid.set_value();
            eventually([&] { return client.getClientState().state == ClientConnectionState::Disconnected; });
            check(!client.getClientState().participants && admission.userToken, "invalid snapshot retained presence or invalidated the admission result");
            server.finish();
        }
    }

    void unrepresentableSnapshotDisconnects()
    {
        Directory directory;
        auto host = Session::createHost(std::string(65500, 'h'));
        host.saveIdentityState(directory.path / "identities.json");
        const auto port = host.startHosting("127.0.0.1", 0);
        auto client = Session::createClient();
        auto admission = client.joinSession("127.0.0.1", port, SessionCode(host.getSessionCode()));
        check(admission.wait_for(3s) == std::future_status::ready, "unrepresentable snapshot left admission pending");
        try { admission.get(); }
        catch (const std::system_error &) { } // Admission may be committed before its reply can be sent.
        eventually([&] { return client.getClientState().state == ClientConnectionState::Disconnected; });
        check(!client.getClientState().participants, "wire-limit failure left an admitted client with stale presence");
        eventually([&] { return host.toView().host->participants.participants.size() == 1; });
    }

    void simultaneousAdmissions()
    {
        Directory directory;
        auto host = Session::createHost("host");
        host.saveIdentityState(directory.path / "identities.json");
        const auto port = host.startHosting("127.0.0.1", 0);
        const SessionCode code(host.getSessionCode());
        std::array<Session, 6> clients{Session::createClient(), Session::createClient(), Session::createClient(), Session::createClient(), Session::createClient(), Session::createClient()};
        std::vector<std::future<ClientAdmission>> admissions;
        for (auto &client : clients)
        {
            admissions.push_back(client.joinSession("127.0.0.1", port, code));
        }
        for (auto &admission : admissions) completed(std::move(admission));
        const auto all = query(host);
        check(all.participants.size() == 7, "simultaneous admissions lost participants");
        eventually([&] { return std::all_of(clients.begin(), clients.end(), [&](const auto &client) { return client.getClientState().participants == all; }); });
        std::vector<std::future<void>> disconnects;
        for (std::size_t index = 0; index < 3; ++index) disconnects.push_back(std::async(std::launch::async, [&, index] { clients[index].disconnectFromHost(); }));
        for (auto &disconnected : disconnects) completed(std::move(disconnected));
        eventually([&] { return query(host).participants.size() == 4; });
        const auto remaining = query(host);
        eventually([&] { return clients[3].getClientState().participants == remaining && clients[4].getClientState().participants == remaining && clients[5].getClientState().participants == remaining; });
    }
}

int main()
{
    try
    {
        detail::SocketRuntime runtime;
        strictSnapshotCodec();
        connectedListAndPresence();
        rawInitialSnapshotAndHostReconstruction();
        fragmentedSnapshotsAndRevisionChecks();
        unrepresentableSnapshotDisconnects();
        simultaneousAdmissions();
        std::cout << "Participant presence tests passed\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Participant test failed: " << error.what() << '\n';
        return 1;
    }
}
