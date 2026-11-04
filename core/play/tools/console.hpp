#pragma once

#include <charconv>
#include <csignal>
#include <cstdint>
#include <stdexcept>
#include <string_view>

namespace golarion::play::tools
{
    inline volatile std::sig_atomic_t stopRequested = 0;

    inline void requestStop(int)
    {
        stopRequested = 1;
    }

    inline void installStopHandlers()
    {
        std::signal(SIGINT, requestStop);
        std::signal(SIGTERM, requestStop);
    }

    inline std::uint16_t parsePort(std::string_view text, bool allowZero)
    {
        unsigned int port = 0;
        const auto result = std::from_chars(text.data(), text.data() + text.size(), port);
        if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || port > 65535 || (!allowZero && port == 0))
        {
            throw std::invalid_argument("La porta indicata non è valida.");
        }
        return static_cast<std::uint16_t>(port);
    }
}
