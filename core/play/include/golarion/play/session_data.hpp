#pragma once

#include "golarion/play/identity_save_data.hpp"
#include "golarion/play/session_code.hpp"

namespace golarion::play
{
    struct SessionData
    {
        SessionCode code;
        IdentitySaveData identities;
    };
}
