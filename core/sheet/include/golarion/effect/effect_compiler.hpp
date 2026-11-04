#pragma once

#include "golarion/effect/effect_definition.hpp"

#include <functional>
#include <string>

namespace golarion
{
    class ResourceManager;

    struct EffectContext
    {
        std::string instanceId;
        std::string source;
    };

    using EffectCleanup = std::function<void()>;
    using EffectApply = std::function<EffectCleanup(ResourceManager &)>;

    EffectApply compileEffect(EffectDefinition definition, EffectContext context);
}
