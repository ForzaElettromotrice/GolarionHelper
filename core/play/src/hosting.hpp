#pragma once

#include "golarion/play/protocol_manager.hpp"

#include <exception>
#include <memory>

namespace golarion::play
{
    class ProtocolManager::Hosting final
    {
    public:
        Hosting(ProtocolManager &owner, std::string_view bindAddress, std::uint16_t port);
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
