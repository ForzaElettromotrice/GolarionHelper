#pragma once
#include "golarion/effect/effect_compiler.hpp"
#include <nlohmann/json_fwd.hpp>

namespace golarion
{
    nlohmann::json collectionEffectToJson(const CollectionEffectDefinition &definition);
    nlohmann::json castingRuleEffectToJson(const CastingRuleEffectDefinition &definition);
    EffectApply compileCollectionEffect(CollectionEffectDefinition definition, EffectContext context);
    std::optional<CollectionEffectDefinition> collectionEffectFromJson(const nlohmann::json &json);
    EffectApply compileCastingRuleEffect(CastingRuleEffectDefinition definition, EffectContext context);
    std::optional<CastingRuleEffectDefinition> castingRuleEffectFromJson(const nlohmann::json &json);
}
