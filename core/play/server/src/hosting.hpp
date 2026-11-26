#pragma once

#include "golarion/play/server_protocol.hpp"

#include <exception>
#include <memory>

namespace golarion::play
{
    class ServerProtocol::Hosting final
    {
    public:
        Hosting(ServerProtocol &owner, std::string_view bindAddress, std::uint16_t port);
        ~Hosting();
        Hosting(const Hosting &) = delete;
        Hosting &operator=(const Hosting &) = delete;

        std::uint16_t port() const;
        void stop();
        std::exception_ptr failure() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
