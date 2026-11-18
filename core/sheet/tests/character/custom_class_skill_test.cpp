#include "golarion/character/character_sheet.hpp"
#include "golarion/persistence/json.hpp"
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <stdexcept>

namespace
{
    template<class Action> void rejects(Action action)
    {
        bool rejected = false;
        try { action(); } catch (const std::invalid_argument &) { rejected = true; }
        assert(rejected);
    }
    golarion::SkillView view(golarion::CharacterSheet &sheet, golarion::SkillType type, std::optional<std::string> id = std::nullopt)
    {
        auto skills = sheet.toView().skills.skills;
        return *std::ranges::find_if(skills, [&](const auto &skill) { return skill.type == type && skill.specializationId == id; });
    }
}

int main()
{
    using namespace golarion;
    CharacterSheet sheet;
    sheet.addCustomClassSkill(SkillType::Acrobatics);
    assert(view(sheet, SkillType::Acrobatics).customClassSkill);
    assert(view(sheet, SkillType::Acrobatics).classSkillBonus == 0);
    sheet.setSkillRanks(SkillType::Acrobatics, 1);
    assert(view(sheet, SkillType::Acrobatics).classSkillBonus == 3);
    rejects([&] { sheet.addCustomClassSkill(SkillType::Acrobatics); });
    rejects([&] { sheet.removeCustomClassSkill(SkillType::Fly); });
    rejects([&] { sheet.addCustomClassSkill(SkillType::Craft); });
    rejects([&] { sheet.addCustomClassSkill(SkillType::Craft, "missing"); });
    rejects([&] { sheet.addCustomClassSkill(SkillType::Fly, "invalid"); });
    sheet.addCustomClassSkill(SkillType::Craft, "alchemy");
    sheet.addSkillSpecialization(SkillType::Craft, {"test_custom", "Test specialization"});
    sheet.addCustomClassSkill(SkillType::Craft, "test_custom");

    EnhancementGroupDefinition group;
    group.id = "external";
    group.name = "External source";
    group.active = true;
    group.enhancements = {CollectionEffectDefinition{"classSkill", SkillClassSkillGrantDefinition{
        "source", "External", "skill.acrobatics"}}};
    sheet.addEnhancementGroup(group);

    const auto path = std::filesystem::temp_directory_path() / "golarion-custom-class-skill-test.json";
    sheet.save(path);
    auto restored = CharacterSheet::load(path);
    std::filesystem::remove(path);
    assert(restored.toSaveData().formatVersion == 43);
    assert(view(restored, SkillType::Craft, "alchemy").customClassSkill);
    assert(view(restored, SkillType::Craft, "test_custom").customClassSkill);
    restored.removeCustomClassSkill(SkillType::Acrobatics);
    assert(!view(restored, SkillType::Acrobatics).customClassSkill);
    assert(view(restored, SkillType::Acrobatics).classSkill);
    rejects([&] { restored.addCustomClassSkill(SkillType::Acrobatics); });
    rejects([&] { restored.removeCustomClassSkill(SkillType::Acrobatics); });
    restored.removeEnhancementGroup("external");
    assert(!view(restored, SkillType::Acrobatics).classSkill);
    restored.removeCustomClassSkill(SkillType::Craft, "alchemy");
    assert(!view(restored, SkillType::Craft, "alchemy").classSkill);
    assert(view(restored, SkillType::Craft, "test_custom").classSkill);
}
