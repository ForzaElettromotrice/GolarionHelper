#pragma once

#include "golarion/play/resource_manager.hpp"
#include "golarion/play/role_manager.hpp"

namespace golarion::play
{
    class Session final
    {
    public:
        explicit Session(UserId hostId);

    private:
        ResourceManager resourceManager_;
        RoleManager roleManager_;
    };
}
