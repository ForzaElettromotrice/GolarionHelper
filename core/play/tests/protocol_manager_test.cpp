#include "golarion/play/server_protocol.hpp"
#include "golarion/play/session_server.hpp"

#include <array>
#include <cassert>
#include <chrono>
#include <future>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
    using namespace golarion::play;
    using Json = nlohmann::json;

    template<typename Function>
    bool rejects(Function function)
    {
        try { function(); }
        catch (const std::invalid_argument &) { return true; }
        return false;
    }

    void sessionCodeVerification()
    {
        ResourceManager resources;
        ServerProtocol protocol(resources);
        const auto code = protocol.getSessionCode();
        assert(code.size() == 5);
        assert(protocol.verifySessionCode(code));
        auto lowercase = code;
        for (char &character : lowercase)
        {
            if (character >= 'A' && character <= 'Z') character = static_cast<char>(character - 'A' + 'a');
        }
        assert(protocol.verifySessionCode(lowercase));
        auto different = code;
        different[0] = code[0] == 'A' ? 'B' : 'A';
        assert(!protocol.verifySessionCode(different));
        assert(!protocol.verifySessionCode(""));
        assert(!protocol.verifySessionCode(" " + code));
        assert(protocol.getSessionCode() == code);
    }

    void registrationAndDispatch()
    {
        ResourceManager resources;
        ServerProtocol protocol(resources);
        PermissionManager permissions(resources, "host");
        permissions.registerUser("player");
        const auto connection = protocol.registerConnection();
        protocol.bindUser(connection, "player");
        const auto remote = protocol.makeContext(connection);
        const Json payload{{"value", 7}, {"details", {{"enabled", true}}}};
        resources.invoke<ServerProtocol::AddCommandSignature>("protocol.add_command", " example.echo ", Permission::Permission2, [](const CommandContext &context, const Json &input)
        {
            return Json{{"sender", *context.userId}, {"connection", *context.connectionId}, {"payload", input}};
        });
        const Json result = protocol.dispatch("\texample.echo\n", remote, payload);
        assert(result.at("sender") == "player");
        assert(result.at("connection") == connection);
        assert(result.at("payload") == payload);
        assert(payload.at("value") == 7);

        protocol.add_command("counter", Permission::ChangePermissions, [value = 0](const CommandContext &context, const Json &) mutable
        {
            assert(!context.connectionId);
            return Json{{"value", ++value}};
        });
        assert(protocol.dispatch("counter", CommandContext{"host"}, Json::object()).at("value") == 1);
        assert(protocol.dispatch("counter", CommandContext{"host"}, Json::object()).at("value") == 2);
        protocol.add_command("Counter", Permission::ChangePermissions, [](const CommandContext &, const Json &) { return Json(nullptr); });
        assert(protocol.dispatch("Counter", CommandContext{"host"}, Json::object()).is_null());

        resources.registerFeature<void()>("protocol.dispatch", [] {});
        resources.unregisterFeature("protocol.dispatch");
    }

    void invalidRegistrationAndPayload()
    {
        ResourceManager resources;
        ServerProtocol protocol(resources);
        PermissionManager permissions(resources, "host");
        int calls = 0;
        const auto callback = [&calls](const CommandContext &, const Json &) { return Json(++calls); };
        for (const std::string_view name : {"", " \t\r\n"})
        {
            assert(rejects([&] { protocol.add_command(name, Permission::Permission2, callback); }));
            assert(rejects([&] { protocol.dispatch(name, CommandContext{"host"}, Json::object()); }));
        }
        assert(rejects([&] { protocol.add_command("empty", Permission::Permission2, {}); }));
        for (const Permission invalid : {static_cast<Permission>(3), static_cast<Permission>(64), static_cast<Permission>(99)})
        {
            assert(rejects([&] { protocol.add_command("invalid", invalid, callback); }));
        }
        protocol.add_command("value", Permission::Permission2, callback);
        assert(rejects([&] { protocol.add_command(" value ", Permission::Permission1, [](const CommandContext &, const Json &) { return Json(99); }); }));
        assert(rejects([&] { protocol.dispatch("missing", CommandContext{"host"}, Json::object()); }));
        assert(rejects([&] { protocol.dispatch("value", CommandContext{}, Json::object()); }));
        assert(rejects([&] { protocol.dispatch("value", CommandContext{"unknown", ConnectionId{1}}, Json::object()); }));
        for (const Json &invalidPayload : std::array<Json, 6>{Json(nullptr), Json::array(), Json("text"), Json(5), Json(1.5), Json(true)})
        {
            assert(rejects([&] { protocol.dispatch("value", CommandContext{"host"}, invalidPayload); }));
        }
        assert(calls == 0);
        assert(protocol.dispatch("value", CommandContext{"host"}, Json::object()) == 1);
    }

    void permissionMatrixAndPayloadCannotOverrideSender()
    {
        ResourceManager resources;
        ServerProtocol protocol(resources);
        PermissionManager permissions(resources, "host");
        permissions.registerUser("user");
        const auto connection = protocol.registerConnection();
        protocol.bindUser(connection, "user");
        int calls = 0;
        for (unsigned required = 0; required < 64; ++required)
        {
            protocol.add_command("mask." + std::to_string(required), PermissionSet::fromMask(required), [&calls](const CommandContext &context, const Json &)
            {
                ++calls;
                return Json(*context.userId);
            });
        }
        for (unsigned owned = 0; owned < 64; ++owned)
        {
            permissions.restoreSaveData({{"host", PermissionSet::all()}, {"user", PermissionSet::fromMask(owned)}});
            for (unsigned required = 0; required < 64; ++required)
            {
                const auto context = protocol.makeContext(connection);
                const auto name = "mask." + std::to_string(required);
                const int before = calls;
                const Json spoofed{{"userId", "host"}, {"permissions", 63}};
                if ((owned & required) == required)
                {
                    assert(protocol.dispatch(name, context, spoofed) == "user");
                    assert(calls == before + 1);
                }
                else
                {
                    assert(rejects([&] { protocol.dispatch(name, context, spoofed); }));
                    assert(calls == before);
                }
            }
        }
    }

    void currentPermissionsAndDomainRestrictions()
    {
        ResourceManager resources;
        ServerProtocol protocol(resources);
        PermissionManager permissions(resources, "host");
        for (const UserId id : {"delegate", "other-delegate", "player"}) permissions.registerUser(id);
        permissions.updatePermissions("host", "delegate", Permission::ChangePermissions, PermissionSet{});
        permissions.updatePermissions("host", "other-delegate", Permission::ChangePermissions, PermissionSet{});
        int calls = 0;
        resources.invoke<ServerProtocol::AddCommandSignature>("protocol.add_command", "test.updatePermission", Permission::ChangePermissions, [&resources, &calls](const CommandContext &context, const Json &payload)
        {
            ++calls;
            const PermissionSet selected(static_cast<Permission>(payload.at("flag").get<std::uint64_t>()));
            const bool enabled = payload.at("enabled").get<bool>();
            resources.invoke<PermissionManager::UpdatePermissionsSignature>("permissions.updatePermissions", *context.userId, payload.at("target").get<UserId>(), enabled ? selected : PermissionSet{}, enabled ? PermissionSet{} : selected);
            return Json(true);
        });
        const auto connection = protocol.registerConnection();
        protocol.bindUser(connection, "delegate");
        const auto context = protocol.makeContext(connection);
        const auto change = [&](const UserId &target, Permission flag, bool enabled)
        {
            return protocol.dispatch("test.updatePermission", context, {{"target", target}, {"flag", static_cast<std::uint64_t>(flag)}, {"enabled", enabled}});
        };
        assert(change("player", Permission::Permission2, false) == true);
        assert(!permissions.hasPermission("player", Permission::Permission2));
        assert(change("other-delegate", Permission::Permission5, true) == true);
        assert(rejects([&] { change("delegate", Permission::Permission5, true); }));
        assert(rejects([&] { change("host", Permission::Permission5, false); }));
        assert(rejects([&] { change("player", Permission::ChangePermissions, true); }));
        assert(rejects([&] { change("other-delegate", Permission::ChangePermissions, false); }));
        permissions.updatePermissions("host", "delegate", PermissionSet{}, Permission::ChangePermissions);
        const int before = calls;
        assert(rejects([&] { change("player", Permission::Permission2, true); }));
        assert(calls == before);
        permissions.updatePermissions("host", "delegate", Permission::ChangePermissions, PermissionSet{});
        assert(change("player", Permission::Permission2, true) == true);
    }

    void dependencyLifecycleAndRegistrationCollision()
    {
        ResourceManager resources;
        resources.registerFeature<int()>("protocol.add_command", [] { return 42; });
        assert(rejects([&] { ServerProtocol failed(resources); }));
        assert(resources.invoke<int()>("protocol.add_command") == 42);
        resources.unregisterFeature("protocol.add_command");
        auto retained = std::make_shared<int>(7);
        const std::weak_ptr<int> observer = retained;
        {
            ServerProtocol protocol(resources);
            assert(rejects([&] { ServerProtocol duplicate(resources); }));
            int calls = 0;
            resources.invoke<ServerProtocol::AddCommandSignature>("protocol.add_command", "value", Permission::Permission1, [&calls, retained](const CommandContext &, const Json &)
            {
                ++calls;
                return Json(*retained);
            });
            retained.reset();
            assert(rejects([&] { protocol.dispatch("value", CommandContext{"host"}, Json::object()); }));
            assert(calls == 0);
            {
                PermissionManager permissions(resources, "host");
                assert(protocol.dispatch("value", CommandContext{"host"}, Json::object()) == 7);
            }
            assert(rejects([&] { protocol.dispatch("value", CommandContext{"host"}, Json::object()); }));
            assert(calls == 1);
            assert(!observer.expired());
        }
        assert(observer.expired());
        resources.registerFeature<int()>("protocol.add_command", [] { return 9; });
        assert(resources.invoke<int()>("protocol.add_command") == 9);
        resources.unregisterFeature("protocol.add_command");
    }

    void nestedDispatchAndCallbackExceptions()
    {
        ResourceManager resources;
        ServerProtocol protocol(resources);
        PermissionManager permissions(resources, "host");
        protocol.add_command("outer", Permission::ChangePermissions, [&protocol, registered = false](const CommandContext &context, const Json &payload) mutable
        {
            if (!registered)
            {
                for (int index = 0; index < 32; ++index)
                {
                    protocol.add_command("nested." + std::to_string(index), Permission::Permission2, [index](const CommandContext &, const Json &) { return Json(index); });
                }
                registered = true;
            }
            return protocol.dispatch("nested.31", context, payload);
        });
        assert(protocol.dispatch("outer", CommandContext{"host"}, Json::object()) == 31);
        assert(protocol.dispatch("outer", CommandContext{"host"}, Json::object()) == 31);

        protocol.add_command("failure", Permission::ChangePermissions, [](const CommandContext &, const Json &) -> Json { throw std::runtime_error("callback failed"); });
        bool propagated = false;
        try { protocol.dispatch("failure", CommandContext{"host"}, Json::object()); }
        catch (const std::runtime_error &error) { propagated = std::string(error.what()) == "callback failed"; }
        assert(propagated);
        assert(protocol.dispatch("outer", CommandContext{"host"}, Json::object()) == 31);
    }

    void independentInstancesAndSessionConstruction()
    {
        ResourceManager firstResources;
        ResourceManager secondResources;
        ServerProtocol first(firstResources);
        ServerProtocol second(secondResources);
        PermissionManager firstPermissions(firstResources, "host");
        PermissionManager secondPermissions(secondResources, "host");
        first.add_command("value", Permission::ChangePermissions, [](const CommandContext &, const Json &) { return Json(3); });
        second.add_command("value", Permission::ChangePermissions, [](const CommandContext &, const Json &) { return Json(9); });
        assert(first.dispatch("value", CommandContext{"host"}, Json::object()) == 3);
        assert(second.dispatch("value", CommandContext{"host"}, Json::object()) == 9);
        assert(rejects([] { SessionServer invalid(""); }));
        SessionServer session("host");
        SessionServer other("host");
    }

    void pendingConnectionsAndIds()
    {
        ResourceManager resources;
        ServerProtocol protocol(resources);
        const ConnectionId first = protocol.registerConnection();
        const ConnectionId second = protocol.registerConnection();
        assert(first != 0 && second != 0 && first != second);
        assert(!protocol.getUserId(first));
        assert(!protocol.getUserId(second));
        assert(!protocol.makeContext(first).userId && protocol.makeContext(first).connectionId == first);
        assert(protocol.closeConnection(first));
        assert(!protocol.closeConnection(first));
        const ConnectionId third = protocol.registerConnection();
        assert(third != 0 && third != first && third != second);
        assert(!protocol.getUserId(first));
        assert(rejects([&] { protocol.makeContext(first); }));
        for (const ConnectionId unknown : {ConnectionId{0}, std::numeric_limits<ConnectionId>::max()})
        {
            assert(!protocol.getUserId(unknown));
            assert(!protocol.closeConnection(unknown));
            assert(rejects([&] { protocol.makeContext(unknown); }));
            assert(rejects([&] { protocol.bindUser(unknown, "alice"); }));
        }
    }

    void bindingAndDetachedContext()
    {
        ResourceManager resources;
        ServerProtocol protocol(resources);
        const ConnectionId alice = protocol.registerConnection();
        const ConnectionId bob = protocol.registerConnection();
        assert(!protocol.bindUser(alice, "alice"));
        assert(!protocol.bindUser(bob, "bob"));
        assert(protocol.getUserId(alice) == UserId("alice"));
        assert(protocol.getUserId(bob) == UserId("bob"));
        const auto context = protocol.makeContext(alice);
        assert(context.userId == "alice");
        assert(context.connectionId == alice);
        auto detachedUser = protocol.getUserId(alice);
        *detachedUser = "changed";
        auto detachedContext = context;
        detachedContext.userId = "host";
        detachedContext.connectionId = bob;
        assert(protocol.makeContext(alice).userId == "alice");
        assert(protocol.makeContext(alice).connectionId == alice);
        assert(protocol.getUserId(bob) == UserId("bob"));
        assert(!protocol.bindUser(alice, "alice"));
        assert(protocol.getUserId(alice) == UserId("alice"));

        const ConnectionId differentCase = protocol.registerConnection();
        assert(!protocol.bindUser(differentCase, "Alice"));
        assert(protocol.getUserId(differentCase) == UserId("Alice"));
        assert(protocol.getUserId(alice) == UserId("alice"));
    }

    void replacementAndLateDisconnect()
    {
        ResourceManager resources;
        ServerProtocol protocol(resources);
        const ConnectionId oldAlice = protocol.registerConnection();
        const ConnectionId bob = protocol.registerConnection();
        protocol.bindUser(oldAlice, "alice");
        protocol.bindUser(bob, "bob");
        const CommandContext oldSnapshot = protocol.makeContext(oldAlice);
        const ConnectionId newAlice = protocol.registerConnection();
        assert(protocol.bindUser(newAlice, "alice") == oldAlice);
        assert(!protocol.getUserId(oldAlice));
        assert(rejects([&] { protocol.makeContext(oldAlice); }));
        assert(rejects([&] { protocol.bindUser(oldAlice, "alice"); }));
        assert(!protocol.closeConnection(oldAlice));
        assert(protocol.getUserId(newAlice) == UserId("alice"));
        assert(protocol.getUserId(bob) == UserId("bob"));
        assert(protocol.makeContext(newAlice).connectionId == newAlice);
        assert(oldSnapshot.userId == "alice" && oldSnapshot.connectionId == oldAlice);

        const ConnectionId newestAlice = protocol.registerConnection();
        assert(protocol.bindUser(newestAlice, "alice") == newAlice);
        assert(!protocol.closeConnection(newAlice));
        assert(!protocol.closeConnection(oldAlice));
        assert(protocol.getUserId(newestAlice) == UserId("alice"));
        assert(protocol.getUserId(bob) == UserId("bob"));
        assert(protocol.closeConnection(newestAlice));
        assert(!protocol.getUserId(newestAlice));
        assert(rejects([&] { protocol.makeContext(newestAlice); }));
        const ConnectionId resumed = protocol.registerConnection();
        assert(!protocol.bindUser(resumed, "alice"));
        assert(protocol.makeContext(resumed).userId == "alice");
        assert(protocol.closeConnection(bob));
        assert(protocol.getUserId(resumed) == UserId("alice"));
    }

    void invalidBindingPreservesExistingConnections()
    {
        ResourceManager resources;
        ServerProtocol protocol(resources);
        const ConnectionId alice = protocol.registerConnection();
        const ConnectionId bob = protocol.registerConnection();
        const ConnectionId pending = protocol.registerConnection();
        protocol.bindUser(alice, "alice");
        protocol.bindUser(bob, "bob");
        assert(rejects([&] { protocol.bindUser(alice, "bob"); }));
        assert(rejects([&] { protocol.bindUser(bob, "alice"); }));
        assert(protocol.getUserId(alice) == UserId("alice"));
        assert(protocol.getUserId(bob) == UserId("bob"));
        for (const UserId invalid : {"", " ", " alice", "alice ", "bad id", "bad\tid", "bad\nid", "bad\x7f"})
        {
            assert(rejects([&] { protocol.bindUser(pending, invalid); }));
            assert(!protocol.getUserId(pending));
            assert(protocol.getUserId(alice) == UserId("alice"));
            assert(protocol.getUserId(bob) == UserId("bob"));
        }
        assert(protocol.closeConnection(pending));
        assert(rejects([&] { protocol.bindUser(pending, "alice"); }));
        assert(protocol.getUserId(alice) == UserId("alice"));
    }

    void contextsAreBuiltForEachExecution()
    {
        ResourceManager resources;
        ServerProtocol protocol(resources);
        PermissionManager permissions(resources, "host");
        permissions.registerUser("alice");
        int calls = 0;
        protocol.add_command("echo", Permission::Permission2, [&calls](const CommandContext &context, const nlohmann::json &)
        {
            ++calls;
            return nlohmann::json{{"userId", *context.userId}, {"connectionId", *context.connectionId}};
        });
        const ConnectionId first = protocol.registerConnection();
        protocol.bindUser(first, "alice");
        const auto stale = protocol.makeContext(first);
        assert(protocol.dispatch("echo", protocol.makeContext(first), nlohmann::json::object()).at("connectionId") == first);
        const ConnectionId replacement = protocol.registerConnection();
        assert(protocol.bindUser(replacement, "alice") == first);
        assert(rejects([&] { protocol.dispatch("echo", stale, nlohmann::json::object()); }));
        assert(rejects([&] { protocol.dispatch("echo", CommandContext{"host", replacement}, nlohmann::json::object()); }));
        assert(rejects([&] { protocol.dispatch("echo", protocol.makeContext(first), nlohmann::json::object()); }));
        assert(calls == 1);
        assert(protocol.dispatch("echo", protocol.makeContext(replacement), nlohmann::json::object()).at("userId") == "alice");
        permissions.applyPreset("host", "alice", PermissionPreset::Spectator);
        assert(rejects([&] { protocol.dispatch("echo", protocol.makeContext(replacement), nlohmann::json::object()); }));
        assert(calls == 2);
    }

    template<typename Function> bool rejectsWithCode(ProtocolErrorCode code, Function function)
    {
        try { function(); }
        catch (const ProtocolError &error) { return error.code() == code; }
        return false;
    }

    void anonymousCommandsAndBuiltinRegistration()
    {
        ResourceManager resources;
        ServerProtocol protocol(resources);
        PermissionManager permissions(resources, "host");
        permissions.registerUser("alice");
        int calls = 0;
        protocol.add_command("public", Permission::NoAuth, [&](const CommandContext &context, const Json &)
        {
            ++calls;
            return Json(context.userId.has_value());
        });
        protocol.add_command("authenticated", Permission::Permission1, [](const auto &, const auto &) { return Json(true); });
        const auto pending = protocol.registerConnection();
        const auto anonymous = protocol.makeContext(pending);
        assert(!anonymous.userId && anonymous.connectionId == pending);
        assert(protocol.dispatch(" public ", anonymous, Json::object()) == false);
        assert(protocol.dispatch("public", CommandContext{"host"}, Json::object()) == true);
        assert(rejectsWithCode(ProtocolErrorCode::Forbidden, [&] { protocol.dispatch("authenticated", anonymous, Json::object()); }));
        assert(rejectsWithCode(ProtocolErrorCode::NotAdmitted, [&] { protocol.dispatch("public", CommandContext{}, Json::object()); }));
        assert(rejectsWithCode(ProtocolErrorCode::NotAdmitted, [&] { protocol.dispatch("public", CommandContext{"host", pending}, Json::object()); }));
        assert(rejectsWithCode(ProtocolErrorCode::CommandFailed, [&] { protocol.dispatch("public", CommandContext{"unknown"}, Json::object()); }));
        assert(rejectsWithCode(ProtocolErrorCode::NotAdmitted, [&] { protocol.dispatch("public", CommandContext{UserId{}}, Json::object()); }));
        assert(calls == 2);
        for (const std::string_view name : {"session.join", " session.join ", "session.resume", "\tsession.resume\n"})
        {
            assert(rejects([&] { protocol.add_command(name, Permission::Any, [](const auto &, const auto &) { return Json("override"); }); }));
        }
        assert(rejectsWithCode(ProtocolErrorCode::InvalidRequest, [&] { protocol.dispatch(" session.join ", anonymous, Json::object()); }));
        assert(rejectsWithCode(ProtocolErrorCode::InvalidRequest, [&] { protocol.dispatch("session.join", CommandContext{"host"}, Json::object()); }));
        protocol.bindUser(pending, "alice");
        assert(rejectsWithCode(ProtocolErrorCode::NotAdmitted, [&] { protocol.dispatch("public", anonymous, Json::object()); }));
        assert(protocol.dispatch("public", protocol.makeContext(pending), Json::object()) == true);
        assert(rejectsWithCode(ProtocolErrorCode::AlreadyAdmitted, [&] { protocol.dispatch(" session.join ", protocol.makeContext(pending), Json::object()); }));
        const auto replacement = protocol.registerConnection();
        protocol.bindUser(replacement, "alice");
        assert(rejectsWithCode(ProtocolErrorCode::NotAdmitted, [&] { protocol.dispatch("public", anonymous, Json::object()); }));
        const auto closed = protocol.registerConnection();
        const auto staleAnonymous = protocol.makeContext(closed);
        protocol.closeConnection(closed);
        assert(rejectsWithCode(ProtocolErrorCode::NotAdmitted, [&] { protocol.dispatch("public", staleAnonymous, Json::object()); }));
        assert(calls == 3);

        constexpr CommandRequirements publicAccess(Permission::NoAuth);
        constexpr CommandRequirements authenticatedAccess(Permission::Any);
        static_assert(publicAccess.access == CommandAccess::Public && publicAccess.permissions == PermissionSet{});
        static_assert(authenticatedAccess.access == CommandAccess::Authenticated && authenticatedAccess.permissions == PermissionSet{});
        protocol.add_command("no-flags", Permission::Any, [](const auto &, const auto &) { return Json(true); });
        const auto pendingWithoutPermissions = protocol.registerConnection();
        assert(rejectsWithCode(ProtocolErrorCode::Forbidden, [&] { protocol.dispatch("no-flags", protocol.makeContext(pendingWithoutPermissions), Json::object()); }));
        for (const auto flag : {Permission::Permission1, Permission::Permission2, Permission::Permission3}) permissions.updatePermissions("host", "alice", PermissionSet{}, flag);
        assert(protocol.dispatch("no-flags", protocol.makeContext(replacement), Json::object()) == true);
        assert(rejects([&] { protocol.add_command("bad-access", CommandRequirements{static_cast<CommandAccess>(99)}, [](const auto &, const auto &) { return Json(true); }); }));
        assert(rejects([&] { protocol.add_command("public-with-flags", CommandRequirements{CommandAccess::Public, Permission::Permission1}, [](const auto &, const auto &) { return Json(true); }); }));

        ResourceManager missingPermissions;
        ServerProtocol incomplete(missingPermissions);
        incomplete.add_command("public", Permission::NoAuth, [](const auto &, const auto &) { return Json(true); });
        const auto unknown = incomplete.registerConnection();
        assert(incomplete.dispatch("public", incomplete.makeContext(unknown), Json::object()) == true);
    }

    void typedFailuresAndLifecycleReentry()
    {
        ResourceManager resources;
        ServerProtocol protocol(resources);
        PermissionManager permissions(resources, "host");
        permissions.registerUser("alice");
        protocol.add_command("master-only", Permission::Permission4, [](const auto &, const auto &) { return Json(nullptr); });
        assert(rejectsWithCode(ProtocolErrorCode::UnknownCommand, [&] { protocol.dispatch("missing", CommandContext{"host"}, Json::object()); }));
        assert(rejectsWithCode(ProtocolErrorCode::Forbidden, [&] { protocol.dispatch("master-only", CommandContext{"alice"}, Json::object()); }));
        assert(rejectsWithCode(ProtocolErrorCode::InvalidRequest, [&] { protocol.dispatch("master-only", CommandContext{"host"}, Json::array()); }));
        assert(rejectsWithCode(ProtocolErrorCode::NotAdmitted, [&] { protocol.dispatch("master-only", CommandContext{"host", ConnectionId{1}}, Json::object()); }));
        assert(rejects([] { throw ProtocolError(static_cast<ProtocolErrorCode>(99), "invalid code"); }));
        protocol.add_command("lifecycle", Permission::ChangePermissions, [&](const auto &, const auto &)
        {
            int rejected = 0;
            try { protocol.startHosting("127.0.0.1", 0); }
            catch (const std::logic_error &) { ++rejected; }
            try { protocol.stopHosting(); }
            catch (const std::logic_error &) { ++rejected; }
            try { protocol.loadIdentityState("unused.json"); }
            catch (const std::logic_error &) { ++rejected; }
            return Json(rejected);
        });
        assert(protocol.dispatch("lifecycle", CommandContext{"host"}, Json::object()) == 3);
        protocol.stopHosting();
    }

    void serializedCallbacksAndBindingChanges()
    {
        using namespace std::chrono_literals;
        for (const bool replace : {true, false})
        {
            ResourceManager resources;
            ServerProtocol protocol(resources);
            PermissionManager permissions(resources, "host");
            permissions.registerUser("alice");
            const auto first = protocol.registerConnection();
            protocol.bindUser(first, "alice");
            const auto context = protocol.makeContext(first);
            const auto next = protocol.registerConnection();
            std::promise<void> entered;
            auto enteredFuture = entered.get_future();
            std::promise<void> release;
            auto released = release.get_future().share();
            protocol.add_command("slow", Permission::Permission2, [&](const auto &, const auto &)
            {
                entered.set_value();
                if (released.wait_for(3s) != std::future_status::ready) throw std::runtime_error("test callback was not released");
                return Json(7);
            });
            auto dispatch = std::async(std::launch::async, [&] { return protocol.dispatch("slow", context, Json::object()); });
            assert(enteredFuture.wait_for(1s) == std::future_status::ready);
            std::promise<void> attempting;
            auto attempted = attempting.get_future();
            auto change = std::async(std::launch::async, [&]
            {
                attempting.set_value();
                if (replace) assert(protocol.bindUser(next, "alice") == first);
                else assert(protocol.closeConnection(first));
            });
            assert(attempted.wait_for(1s) == std::future_status::ready);
            const bool waited = change.wait_for(30ms) == std::future_status::timeout;
            release.set_value();
            assert(dispatch.get() == 7);
            change.get();
            assert(waited);
            assert(rejectsWithCode(ProtocolErrorCode::NotAdmitted, [&] { protocol.dispatch("slow", context, Json::object()); }));
        }

        ResourceManager resources;
        ServerProtocol protocol(resources);
        PermissionManager permissions(resources, "host");
        protocol.add_command("counter", Permission::ChangePermissions, [count = 0](const auto &, const auto &) mutable { return Json(++count); });
        std::vector<std::future<void>> tasks;
        for (int index = 0; index < 4; ++index)
        {
            tasks.push_back(std::async(std::launch::async, [&, index]
            {
                for (int iteration = 0; iteration < 100; ++iteration)
                {
                    const auto name = "dynamic." + std::to_string(index) + "." + std::to_string(iteration);
                    resources.invoke<ServerProtocol::AddCommandSignature>("protocol.add_command", name, Permission::ChangePermissions, [](const auto &, const auto &) { return Json(true); });
                    assert(protocol.dispatch(name, CommandContext{"host"}, Json::object()) == true);
                    static_cast<void>(protocol.dispatch("counter", CommandContext{"host"}, Json::object()));
                }
            }));
        }
        for (auto &task : tasks) task.get();
        assert(protocol.dispatch("counter", CommandContext{"host"}, Json::object()) == 401);
    }

    void independentConnections()
    {
        ResourceManager firstResources;
        ResourceManager secondResources;
        ServerProtocol first(firstResources);
        ServerProtocol second(secondResources);
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
}

int main()
{
    sessionCodeVerification();
    registrationAndDispatch();
    invalidRegistrationAndPayload();
    permissionMatrixAndPayloadCannotOverrideSender();
    currentPermissionsAndDomainRestrictions();
    dependencyLifecycleAndRegistrationCollision();
    nestedDispatchAndCallbackExceptions();
    independentInstancesAndSessionConstruction();
    pendingConnectionsAndIds();
    bindingAndDetachedContext();
    replacementAndLateDisconnect();
    invalidBindingPreservesExistingConnections();
    contextsAreBuiltForEachExecution();
    typedFailuresAndLifecycleReentry();
    anonymousCommandsAndBuiltinRegistration();
    serializedCallbacksAndBindingChanges();
    independentConnections();
}
