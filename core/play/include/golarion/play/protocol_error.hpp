#pragma once

#include <stdexcept>
#include <string>
#include <utility>

namespace golarion::play
{
    enum class ProtocolErrorCode
    {
        InvalidRequest,
        InvalidSessionCode,
        InvalidUserToken,
        NotAdmitted,
        AlreadyAdmitted,
        AdmissionFailed,
        UnknownCommand,
        Forbidden,
        CommandFailed
    };

    class ProtocolError final : public std::invalid_argument
    {
    public:
        ProtocolError(ProtocolErrorCode code, std::string message) : std::invalid_argument(std::move(message)), code_(code)
        {
            switch (code_)
            {
                case ProtocolErrorCode::InvalidRequest:
                case ProtocolErrorCode::InvalidSessionCode:
                case ProtocolErrorCode::InvalidUserToken:
                case ProtocolErrorCode::NotAdmitted:
                case ProtocolErrorCode::AlreadyAdmitted:
                case ProtocolErrorCode::AdmissionFailed:
                case ProtocolErrorCode::UnknownCommand:
                case ProtocolErrorCode::Forbidden:
                case ProtocolErrorCode::CommandFailed:
                    return;
            }
            throw std::invalid_argument("unknown protocol error code");
        }

        ProtocolErrorCode code() const noexcept { return code_; }

    private:
        ProtocolErrorCode code_;
    };
}
