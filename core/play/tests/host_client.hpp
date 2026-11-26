#pragma once

#include "golarion/play/session_client.hpp"

namespace golarion::play::test
{
    inline void connectHost(SessionClient &client, auto &server, std::uint16_t port)
    {
        client.resumeSession("127.0.0.1", port, SessionCode(server.getSessionCode()), server.issueHostConnectionToken()).get();
    }
}
