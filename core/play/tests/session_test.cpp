#include "golarion/play/session.hpp"

#include <cassert>
#include <stdexcept>

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
}

int main()
{
    connectionLifecycleDelegation();
    independentSessions();
}
