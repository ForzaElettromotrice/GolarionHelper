#include "user_tokens.hpp"

#include <iostream>
#include <stdexcept>

namespace
{
    using namespace golarion::play;
    using detail::UserTokens;

    void check(bool condition, const char *message)
    {
        if (!condition) throw std::runtime_error(message);
    }

    template<typename Exception, typename Function> bool rejects(Function function)
    {
        try { function(); }
        catch (const Exception &) { return true; }
        return false;
    }

    void issuanceResolutionAndIsolation()
    {
        UserTokens registry;
        const auto alice = registry.issue("alice", [] { return UserToken("0123456789abcdef"); });
        check(registry.resolve(alice) == "alice", "issued token was not recognized");
        check(!registry.resolve("0123456789ABCDEF"), "token was normalized");
        check(!registry.resolve("invalid") && !registry.resolve("0000000000000000"), "invalid or unknown token accepted");
        check(!UserTokens{}.resolve(alice), "token recognized by another registry");
        int duplicateCalls = 0;
        check(rejects<std::invalid_argument>([&]
        {
            registry.issue("alice", [&] { ++duplicateCalls; return UserToken("ffffffffffffffff"); });
        }), "second token issued for same user");
        check(duplicateCalls == 0 && registry.resolve(alice) == "alice", "duplicate issuance changed credentials");
        int attempts = 0;
        const auto bob = registry.issue("bob", [&] { return UserToken(++attempts == 1 ? "0123456789abcdef" : "ffffffffffffffff"); });
        check(attempts == 2 && registry.resolve(bob) == "bob", "token collision was not retried");
        check(registry.resolve(alice) == "alice", "collision overwrote an existing identity");
    }

    void failedGenerationAndPersistenceData()
    {
        UserTokens registry;
        registry.issue("alice", [] { return UserToken("0123456789abcdef"); });
        int attempts = 0;
        check(rejects<std::runtime_error>([&]
        {
            registry.issue("bob", [&] { ++attempts; return UserToken("0123456789abcdef"); });
        }), "exhausted generator was accepted");
        check(attempts == 32, "collision retries are not bounded");
        check(rejects<std::runtime_error>([&]
        {
            registry.issue("bob", []() -> UserToken { throw std::runtime_error("test generator failed"); });
        }), "generator failure was swallowed");
        registry.issue("bob", [] { return UserToken("ffffffffffffffff"); });
        const auto data = registry.toSaveData();
        check(data.size() == 2 && data[0].userId == "alice" && data[1].userId == "bob", "credential data is not deterministic");
        auto restored = UserTokens::fromSaveData(data);
        check(restored.resolve("0123456789abcdef") == "alice" && restored.resolve("ffffffffffffffff") == "bob", "hash-only data did not restore credentials");
        auto duplicate = data;
        duplicate.push_back(data.front());
        check(rejects<std::invalid_argument>([&] { UserTokens::fromSaveData(duplicate); }), "duplicate saved user accepted");
        duplicate = data;
        duplicate.back().tokenHash = duplicate.front().tokenHash;
        check(rejects<std::invalid_argument>([&] { UserTokens::fromSaveData(duplicate); }), "shared saved token accepted");
    }
}

int main()
{
    try
    {
        issuanceResolutionAndIsolation();
        failedGenerationAndPersistenceData();
        std::cout << "user tokens: PASS\n";
    }
    catch (const std::exception &error)
    {
        std::cerr << "user tokens: " << error.what() << '\n';
        return 1;
    }
}
