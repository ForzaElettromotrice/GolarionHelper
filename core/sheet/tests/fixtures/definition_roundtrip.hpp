#pragma once
#include "golarion/effect/effect_json.hpp"
#include <nlohmann/json.hpp>
#include <cassert>

inline void assertJsonContains(const nlohmann::json &actual, const nlohmann::json &expected)
{
    if (expected.is_object())
    {
        assert(actual.is_object());
        for (const auto &[key, value] : expected.items()) assertJsonContains(actual.at(key), value);
    }
    else if (expected.is_array())
    {
        assert(actual.is_array() && actual.size() == expected.size());
        for (std::size_t i = 0; i < expected.size(); ++i) assertJsonContains(actual[i], expected[i]);
    }
    else assert(actual == expected);
}

inline golarion::EffectDefinition roundTripEffect(const nlohmann::json &json)
{
    auto original = golarion::effectDefinitionFromJson(json);
    const auto encoded = golarion::effectDefinitionToJson(original);
    auto restored = golarion::effectDefinitionFromJson(encoded);
    assert(restored.index() == original.index());
    assert(golarion::effectDefinitionToJson(restored) == encoded);
    return restored;
}
