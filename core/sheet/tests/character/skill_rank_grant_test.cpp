#include "golarion/character/skills.hpp"
#include "golarion/effect/effect_compiler.hpp"
#include "golarion/effect/effect_json.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <nlohmann/json.hpp>
#include <cassert>
#include <stdexcept>

namespace
{
    template<class Action> void rejects(Action action)
    {
        bool rejected = false;
        try { action(); } catch (const std::exception &) { rejected = true; }
        assert(rejected);
    }
}

int main()
{
    using namespace golarion;
    ResourceManager resources;
    for (const auto name : {"strMod", "dexMod", "conMod", "intMod", "wisMod", "chaMod"})
        resources.registerTarget(name, [] { return 1; });
    int level = 7;
    resources.registerTarget("level", [&] { return level; });
    Skills skills(resources);
    auto view = [&](SkillType type, std::optional<std::string> specialization = std::nullopt)
    {
        for (const auto &entry : skills.toView().skills)
            if (entry.type == type && entry.specializationId == specialization) return entry;
        throw std::logic_error("skill view not found");
    };
    assert(!view(SkillType::Spellcraft).usable);
    resources.addToCollection(SkillClassSkillGrantsResource, SkillClassSkillGrant({"class", "Classe", "skill.spellcraft"}));
    const auto definition = effectDefinitionFromJson(nlohmann::json{
        {"id", "ranks"}, {"type", "skillRankGrant"}, {"skill", "skill.spellcraft"}, {"expression", "@level"}});
    const auto apply = compileEffect(definition, {"item.ranks", "Oggetto"});
    auto cleanup = apply(resources);
    auto skillView = view(SkillType::Spellcraft);
    assert(skillView.ranks == 0 && skillView.effectiveRanks == 7 && skillView.usable);
    assert(skillView.classSkillBonus == 3 && skillView.abilityOptions[0].totalValue == 11);
    assert(skillView.rankGrants.size() == 1 && skillView.rankGrants[0].source == "Oggetto");
    assert(skills.toView().spentRanks == 0);
    rejects([&] { apply(resources); });
    skills.setRanks(SkillType::Spellcraft, 3);
    assert(view(SkillType::Spellcraft).effectiveRanks == 7 && skills.toView().spentRanks == 3);
    for (const auto &saved : skills.toSaveData().skills)
        if (saved.type == SkillType::Spellcraft) assert(saved.ranks == 3);
    const auto endpoint = skillRankGrantsResource(SkillType::Spellcraft);
    resources.addToCollection(endpoint, SkillRankGrant{"sage", "Altra fonte", "4"});
    assert(view(SkillType::Spellcraft).effectiveRanks == 7);
    level = 9;
    assert(view(SkillType::Spellcraft).effectiveRanks == 9);
    cleanup();
    assert(view(SkillType::Spellcraft).effectiveRanks == 4);
    resources.removeFromCollection(endpoint, "sage");
    assert(view(SkillType::Spellcraft).effectiveRanks == 3);
    skills.setRanks(SkillType::Spellcraft, 0);
    assert(!view(SkillType::Spellcraft).usable && view(SkillType::Spellcraft).classSkillBonus == 0);
    resources.addToCollection(endpoint, SkillRankGrant{"invalid", "Fonte", "-1"});
    rejects([&] { skills.toView(); });
    resources.removeFromCollection(endpoint, "invalid");
    skills.addSpecialization(SkillType::Craft, "test", "Prova");
    auto specializedCleanup = compileEffect(SkillRankGrantEffectDefinition{"ranks", SkillType::Craft, "4", "test"}, {"craft.ranks", "Oggetto"})(resources);
    assert(view(SkillType::Craft, "test").effectiveRanks == 4);
    assert(view(SkillType::Craft, "alchemy").effectiveRanks == 0);
    specializedCleanup();
    assert(view(SkillType::Craft, "test").effectiveRanks == 0);
    for (const auto input : {
        nlohmann::json{{"skill", "unknown"}, {"expression", "4"}},
        nlohmann::json{{"skill", "skill.craft"}, {"expression", "4"}},
        nlohmann::json{{"skill", "skill.spellcraft"}, {"expression", "4"}, {"specializationId", "test"}},
        nlohmann::json{{"skill", "skill.craft"}, {"expression", "4"}, {"specializationId", "bad.id"}},
        nlohmann::json{{"skill", "skill.spellcraft"}, {"expression", " "}}
    })
    {
        auto invalid = input;
        invalid["id"] = "ranks";
        invalid["type"] = "skillRankGrant";
        rejects([&] { effectDefinitionFromJson(invalid); });
    }
    rejects([&] { compileEffect(SkillRankGrantEffectDefinition{"ranks", static_cast<SkillType>(999), "4"}, {"bad", "Fonte"}); });
}
