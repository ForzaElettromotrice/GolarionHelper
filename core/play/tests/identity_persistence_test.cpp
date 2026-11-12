#include "golarion/play/session.hpp"
#include "golarion/play/user_token.hpp"

#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
    using namespace golarion::play;
    using Json = nlohmann::json;
    namespace fs = std::filesystem;

    void check(bool condition, const char *message)
    {
        if (!condition) throw std::runtime_error(message);
    }

    template<typename Function> bool fails(Function function)
    {
        try { function(); }
        catch (const std::exception &) { return true; }
        return false;
    }

    struct TestDirectory
    {
        fs::path path;
        TestDirectory()
        {
            path = fs::absolute(fs::current_path()) / ("identity-test-" + UserToken::generate().value());
            check(fs::create_directory(path), "cannot create owned test directory");
        }
        ~TestDirectory()
        {
            std::error_code ignored;
            // Remove only entries created in this one-level fixture; never recurse.
            for (fs::directory_iterator iterator(path, ignored), end; iterator != end && !ignored; iterator.increment(ignored))
            {
                std::error_code removeError;
                fs::remove(iterator->path(), removeError);
            }
            fs::remove(path, ignored);
        }
    };

    struct Fixture
    {
        ResourceManager resources;
        ProtocolManager protocol{resources};
        RoleManager roles{resources, "host"};
    };

    std::string readText(const fs::path &path)
    {
        std::ifstream file(path, std::ios::binary);
        check(static_cast<bool>(file), "cannot read fixture file");
        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }

    void writeText(const fs::path &path, const std::string &text)
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        file << text;
        file.close();
        check(static_cast<bool>(file), "cannot write fixture file");
    }

    void roundTripAndIssuancePolicy()
    {
        TestDirectory directory;
        const auto archive = directory.path / "identities.json";
        std::string aliceToken;
        std::string bobToken;
        {
            Fixture fixture;
            fixture.roles.registerUser("alice");
            fixture.roles.registerUser("bob");
            fixture.roles.setRole("host", "bob", Role::Spectator);
            check(fails([&] { fixture.protocol.issueUserToken("alice"); }), "token returned before configuring persistence");
            fixture.protocol.saveIdentityState(archive);
            const auto empty = readText(archive);
            check(fails([&] { fixture.protocol.issueUserToken("unknown"); }), "token issued for unknown user");
            check(readText(archive) == empty, "failed issuance changed archive");
            aliceToken = fixture.protocol.issueUserToken("alice");
            bobToken = fixture.protocol.issueUserToken("bob");
            const auto issued = readText(archive);
            check(fails([&] { fixture.protocol.issueUserToken("alice"); }), "existing credential was reissued");
            check(readText(archive) == issued && fixture.protocol.resolveUserToken(aliceToken) == "alice", "duplicate issuance changed existing credential");
            fixture.roles.setRole("host", "alice", Role::Master);
            fixture.protocol.saveIdentityState(archive);
            const auto document = Json::parse(readText(archive));
            check(document.size() == 3 && document.at("version") == 1, "unexpected identity archive schema");
            check(!document.contains("sessionId") && !document.contains("sessionCode"), "archive contains an extra Session identifier/code");
            check(document.at("tokens").size() == 2, "credentials missing from archive");
            for (const auto &entry : document.at("tokens"))
            {
                check(entry.size() == 2 && entry.contains("userId") && entry.contains("hash"), "raw token or extra fields persisted");
                check(entry.at("hash").get<std::string>().size() == 64, "saved verifier is not SHA-256");
            }
            const auto saved = readText(archive);
            Fixture newSession;
            check(fails([&] { newSession.protocol.saveIdentityState(archive); }), "new Session silently replaced an existing archive");
            check(readText(archive) == saved, "archive overwrite rejection damaged credentials");
        }
        Fixture restored;
        check(!restored.protocol.resolveUserToken(aliceToken), "another Session recognized a foreign token without loading it");
        restored.roles.registerUser("temporary");
        const auto oldConnection = restored.protocol.registerConnection();
        restored.protocol.bindUser(oldConnection, "temporary");
        const auto code = restored.protocol.getSessionCode();
        restored.protocol.loadIdentityState(archive);
        check(restored.protocol.getSessionCode() == code, "identity restore modified session code");
        check(restored.protocol.resolveUserToken(aliceToken) == "alice" && restored.protocol.resolveUserToken(bobToken) == "bob", "token-to-user associations were not restored");
        check(restored.roles.getRole("alice") == Role::Master && restored.roles.getRole("bob") == Role::Spectator && restored.roles.getRole("host") == Role::Host, "roles were reset on restoration");
        check(fails([&] { restored.roles.getRole("temporary"); }), "old runtime membership survived restoration");
        check(!restored.protocol.getUserId(oldConnection), "live binding was persisted or retained on load");
        const auto connection = restored.protocol.registerConnection();
        check(connection != oldConnection, "restoration recycled a logical ID");
        restored.protocol.bindUser(connection, *restored.protocol.resolveUserToken(aliceToken));
        check(restored.protocol.makeContext(connection).userId == "alice", "restored credential cannot identify a connection");
        check(fails([&] { restored.protocol.issueUserToken("alice"); }), "restoration allowed token rotation");
        restored.roles.registerUser("new-user");
        const auto nextToken = restored.protocol.issueUserToken("new-user");
        Fixture reopened;
        reopened.protocol.loadIdentityState(archive);
        check(reopened.protocol.resolveUserToken(nextToken) == "new-user", "loaded archive was not used for subsequent issuance");
    }

    void corruptedArchivesPreserveRuntimeState()
    {
        TestDirectory directory;
        const auto archive = directory.path / "good.json";
        const auto malformed = directory.path / "bad.json";
        Fixture fixture;
        fixture.roles.registerUser("alice");
        fixture.roles.setRole("host", "alice", Role::Master);
        fixture.protocol.saveIdentityState(archive);
        const auto token = fixture.protocol.issueUserToken("alice");
        const auto connection = fixture.protocol.registerConnection();
        fixture.protocol.bindUser(connection, "alice");
        const auto valid = Json::parse(readText(archive));
        std::vector<Json> invalid;
        const auto add = [&](auto mutate) { auto data = valid; mutate(data); invalid.push_back(std::move(data)); };
        add([](Json &data) { data["version"] = 2; });
        add([](Json &data) { data["version"] = true; });
        add([](Json &data) { data["version"] = 1.0; });
        add([](Json &data) { data["version"] = "1"; });
        add([](Json &data) { data["sessionId"] = "unwanted"; });
        add([](Json &data) { data.erase("tokens"); });
        add([](Json &data) { data["users"] = Json::object(); });
        add([](Json &data) { data["tokens"] = nullptr; });
        add([](Json &data) { data["users"][0]["role"] = "admin"; });
        add([](Json &data) { data["users"][0]["role"] = "any"; });
        add([](Json &data) { data["users"].push_back(data["users"][0]); });
        add([](Json &data) { data["users"][0]["userId"] = "bad id"; });
        add([](Json &data) { data["users"] = Json::array({Json{{"userId", "alice"}, {"role", "master"}}}); });
        add([](Json &data) { for (auto &entry : data["users"]) if (entry["role"] == "host") entry["userId"] = "other-host"; });
        add([](Json &data) { data["users"].push_back(Json{{"userId", "other-host"}, {"role", "host"}}); });
        add([](Json &data) { data["tokens"][0]["userId"] = "missing"; });
        add([](Json &data) { data["tokens"][0]["hash"] = std::string(64, 'A'); });
        add([](Json &data) { data["tokens"][0]["hash"] = "0123456789abcdef"; });
        add([](Json &data) { data["tokens"][0]["token"] = "forbidden"; });
        add([](Json &data) { data["tokens"].push_back(data["tokens"][0]); });
        add([](Json &data) { auto entry = data["tokens"][0]; entry["userId"] = "host"; data["tokens"].push_back(entry); });
        std::vector<std::string> documents;
        for (const auto &data : invalid) documents.push_back(data.dump());
        documents.push_back("{\"version\":1,\"version\":1,\"users\":[],\"tokens\":[]}");
        documents.push_back("{\"version\":1,\"users\":[{\"userId\":\"host\",\"role\":\"host\",\"role\":\"player\"}],\"tokens\":[]}");
        documents.push_back(valid.dump() + " garbage");
        documents.push_back(std::string("\xef\xbb\xbf") + valid.dump());
        documents.push_back(std::string(32, '[') + "0" + std::string(32, ']'));
        documents.push_back(std::string(4 * 1024 * 1024 + 1, ' '));
        for (const auto &document : documents)
        {
            writeText(malformed, document);
            check(fails([&] { fixture.protocol.loadIdentityState(malformed); }), "invalid archive accepted");
            check(fixture.protocol.resolveUserToken(token) == "alice" && fixture.roles.getRole("alice") == Role::Master, "failed load changed identity or role");
            check(fixture.protocol.getUserId(connection) == "alice", "failed load changed live binding");
        }
        check(fails([&] { fixture.protocol.loadIdentityState(directory.path / "missing.json"); }), "missing archive silently created");
        fixture.roles.registerUser("new-user");
        const auto next = fixture.protocol.issueUserToken("new-user");
        Fixture restored;
        restored.protocol.loadIdentityState(archive);
        check(restored.protocol.resolveUserToken(next) == "new-user", "failed load replaced configured archive path");
        ResourceManager resources;
        ProtocolManager otherProtocol(resources);
        RoleManager otherRoles(resources, "other-host");
        check(fails([&] { otherProtocol.loadIdentityState(archive); }), "restoration reassigned the host identity");
        check(otherRoles.getRole("other-host") == Role::Host, "failed host restoration changed roles");
    }

    void writeFailuresDoNotCommitCredentials()
    {
        TestDirectory directory;
        const auto archive = directory.path / "identities.json";
        const auto backup = directory.path / "before.json";
        Fixture fixture;
        fixture.roles.registerUser("alice");
        fixture.roles.registerUser("bob");
        fixture.protocol.saveIdentityState(archive);
        const auto alice = fixture.protocol.issueUserToken("alice");
        const auto saved = readText(archive);
        const auto savedRoles = fixture.roles.toSaveData();
        fixture.roles.restoreSaveData({{"host", Role::Host}});
        check(fails([&] { fixture.protocol.saveIdentityState(archive); }), "unloadable role/token associations were saved");
        check(readText(archive) == saved, "invalid snapshot damaged the archive");
        fixture.roles.restoreSaveData(savedRoles);
        check(fails([&] { fixture.protocol.saveIdentityState(directory.path / "absent" / "bad.json"); }), "save with missing parent succeeded");
        fs::rename(archive, backup);
        check(fs::create_directory(archive), "cannot create failing destination");
        check(fails([&] { fixture.protocol.issueUserToken("bob"); }), "token returned despite archive replacement failure");
        check(fixture.protocol.resolveUserToken(alice) == "alice" && readText(backup) == saved, "failed write damaged existing credential");
        for (const auto &entry : fs::directory_iterator(directory.path))
        {
            check(entry.path().filename().string().find(".tmp.") == std::string::npos, "failed write leaked a temporary file");
        }
        fs::remove(archive);
        fs::rename(backup, archive);
        const auto bob = fixture.protocol.issueUserToken("bob");
        Fixture restored;
        restored.protocol.loadIdentityState(archive);
        check(restored.protocol.resolveUserToken(alice) == "alice" && restored.protocol.resolveUserToken(bob) == "bob", "failed issuance committed an unusable credential or lost the configured path");
    }

    void lifecycleConcurrencyAndSessionFacade()
    {
        TestDirectory directory;
        const auto archive = directory.path / "identities.json";
        Fixture fixture;
        for (int index = 0; index < 8; ++index) fixture.roles.registerUser("user-" + std::to_string(index));
        fixture.protocol.saveIdentityState(archive);
        std::vector<std::future<std::string>> tasks;
        for (int index = 0; index < 8; ++index)
        {
            tasks.push_back(std::async(std::launch::async, [&, index]
            {
                const auto user = "user-" + std::to_string(index);
                auto token = fixture.protocol.issueUserToken(user);
                check(fixture.protocol.resolveUserToken(token) == user, "concurrent issuance/lookup lost identity");
                return token;
            }));
        }
        std::vector<std::string> tokens;
        for (auto &task : tasks) tokens.push_back(task.get());
        fixture.protocol.startHosting("127.0.0.1", 0);
        check(fails([&] { fixture.protocol.loadIdentityState(archive); }), "identity archive loaded during hosting");
        fixture.protocol.stopHosting();
        fixture.protocol.startHosting("127.0.0.1", 0);
        fixture.protocol.stopHosting();
        for (int index = 0; index < 8; ++index) check(fixture.protocol.resolveUserToken(tokens[index]) == "user-" + std::to_string(index), "hosting restart lost credential");
        Fixture restored;
        restored.protocol.loadIdentityState(archive);
        for (int index = 0; index < 8; ++index) check(restored.protocol.resolveUserToken(tokens[index]) == "user-" + std::to_string(index), "concurrent saves lost credentials");

        const auto sessionArchive = directory.path / "session.json";
        std::string token;
        {
            Session session("host");
            session.registerUser("alice");
            session.saveIdentityState(sessionArchive);
            token = session.issueUserToken("alice");
        }
        Session session("host");
        session.loadIdentityState(sessionArchive);
        check(fails([&] { session.registerUser("alice"); }) && fails([&] { session.issueUserToken("alice"); }), "Session did not delegate identity restoration");
        Fixture reopened;
        reopened.protocol.loadIdentityState(sessionArchive);
        check(reopened.protocol.resolveUserToken(token) == "alice", "Session issued a nonpersistent token");
    }

    void writeColdRestartFixture(const fs::path &archive, const fs::path &credential)
    {
        Fixture fixture;
        fixture.roles.registerUser("alice");
        fixture.roles.setRole("host", "alice", Role::Master);
        fixture.protocol.saveIdentityState(archive);
        writeText(credential, fixture.protocol.issueUserToken("alice"));
    }

    void readColdRestartFixture(const fs::path &archive, const fs::path &credential)
    {
        Fixture fixture;
        fixture.protocol.loadIdentityState(archive);
        const auto token = readText(credential);
        check(token.size() == 16, "client credential contains more than the original token");
        check(fixture.protocol.resolveUserToken(token) == "alice" && fixture.roles.getRole("alice") == Role::Master, "cold restart did not preserve identity and role");
    }
}

int main(int argc, char *argv[])
{
    try
    {
        if (argc == 4 && std::string_view(argv[1]) == "--write") writeColdRestartFixture(argv[2], argv[3]);
        else if (argc == 4 && std::string_view(argv[1]) == "--read") readColdRestartFixture(argv[2], argv[3]);
        else
        {
            check(argc == 1, "invalid persistence test arguments");
            roundTripAndIssuancePolicy();
            corruptedArchivesPreserveRuntimeState();
            writeFailuresDoNotCommitCredentials();
            lifecycleConcurrencyAndSessionFacade();
        }
        std::cout << "identity persistence: PASS\n";
    }
    catch (const std::exception &error)
    {
        std::cerr << "identity persistence: " << error.what() << '\n';
        return 1;
    }
}
