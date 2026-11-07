#include "golarion/play/session.hpp"

#include <utility>

namespace golarion::play
{
    Session::Session(UserId hostId) : roleManager_(resourceManager_, std::move(hostId))
    {
    }
}
