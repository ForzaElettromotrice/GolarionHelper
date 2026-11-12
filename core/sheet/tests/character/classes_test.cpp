#include "golarion/character/classes.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <cassert>
#include <stdexcept>
#include <type_traits>

namespace
{
    template<class Action> void rejects(Action action)
    {
        bool rejected = false;
        try { action(); } catch (const std::invalid_argument &) { rejected = true; }
        assert(rejected);
    }

    golarion::ClassDefinition definition(golarion::CharacterClassType type)
    {
        using namespace golarion;
        return {
            .type = type,
            .name = "Classe di prova",
            .description = "Definizione sintetica.",
            .hitDie = ClassHitDie::D8,
            .skillRanksPerLevel = 2,
            .classSkills = {},
            .progression = {
                {1, 0, {{SavingThrowType::Fortitude, 0}, {SavingThrowType::Reflex, 0}, {SavingThrowType::Will, 2}}},
                {2, 1, {{SavingThrowType::Fortitude, 0}, {SavingThrowType::Reflex, 0}, {SavingThrowType::Will, 3}}}
            },
            .features = {{
                .id = "path", .name = "Percorso", .description = "Una scelta.",
                .progression = {{
                    .id = "path.selection", .level = 1,
                    .choices = {{
                        .id = "path", .prompt = "Scegli", .selectionCount = 1,
                        .options = {{
                            .id = "first", .name = "Primo", .description = "Bonus crescente.",
                            .progression = {{
                                .id = "path.first", .level = 1,
                                .effects = {ContributionEffectDefinition{"bonus", "bonus", "@classLevel"}}
                            }}
                        }}
                    }}
                }}
            }}
        };
    }
}

int main()
{
    using namespace golarion;
    static_assert(!std::is_copy_constructible_v<Classes>);
    static_assert(!std::is_move_constructible_v<Classes>);
    const auto wizard = definition(CharacterClassType::Wizard);
    const auto bard = definition(CharacterClassType::Bard);
    const ClassDefinitionManager catalog({wizard, bard});
    assert(&catalog.get(CharacterClassType::Wizard) == &catalog.get(CharacterClassType::Wizard));
    assert(catalog.get(CharacterClassType::Wizard).type == CharacterClassType::Wizard);
    rejects([&] { ClassDefinitionManager duplicates({wizard, wizard}); });
    rejects([&] { catalog.get(CharacterClassType::Fighter); });
    auto invalid = wizard;
    invalid.type = static_cast<CharacterClassType>(-1);
    rejects([&] { ClassDefinitionManager invalidCatalog({invalid}); });
    assert(characterClassId(CharacterClassType::Wizard) == "wizard");

    ResourceManager resources;
    resources.registerAccumulatedResource("baseAttackBonus");
    resources.registerAccumulatedResource("hp.max");
    resources.registerAccumulatedResource("skill.ranks.max");
    resources.registerTarget("intMod", [] { return 0; });
    resources.registerTarget("intPermanentMod", [] { return 0; });
    resources.registerTarget("intSkillRanksMod", [] { return 0; });
    resources.registerAccumulatedResource("bonus");
    FavoredClasses favored(resources);
    for (auto type : {SavingThrowType::Fortitude, SavingThrowType::Reflex, SavingThrowType::Will})
        resources.registerAccumulatedResource(baseResourceName(type));
    {
        Classes classes(resources, favored, catalog);
        assert(classes.level() == 0);
        rejects([&] { classes.addLevel(CharacterClassType::Fighter); });
        rejects([&] { classes.selectOption(CharacterClassType::Wizard, "path.selection", "path", "first"); });
        assert(classes.level() == 0);
        classes.addLevel(CharacterClassType::Wizard);
        assert(classes.level() == 1);
        classes.selectOption(CharacterClassType::Wizard, "path.selection", "path", "first");
        classes.addLevel(CharacterClassType::Bard);
        assert(classes.level() == 2);
        assert(resources.contributionTotal("savingThrow.will.base") == 4);
        classes.addLevel(CharacterClassType::Wizard);
        assert(classes.level() == 3);
        assert(resources.contributionTotal("baseAttackBonus") == 1);
        assert(resources.contributionTotal("savingThrow.will.base") == 5);
        assert(resources.contributionTotal("bonus") == 2);
        assert(resources.targetValue("class.wizard.level") == 2);
        assert(resources.targetValue("class.bard.level") == 1);
        rejects([&] { classes.addLevel(CharacterClassType::Wizard); });
        assert(classes.level() == 3 && resources.contributionTotal("bonus") == 2);
        classes.unselectOption(CharacterClassType::Wizard, "path.selection", "path", "first");
        assert(resources.contributionTotal("bonus") == 0);
    }
    assert(resources.contributionTotal("baseAttackBonus") == 0);
    assert(resources.contributionTotal("savingThrow.will.base") == 0);
    rejects([&] { resources.targetValue("class.wizard.level"); });
    {
        Classes classes(resources, favored);
        rejects([&] { classes.addLevel(CharacterClassType::Wizard); });
        assert(classes.level() == 0);
    }
    auto failing = wizard;
    failing.features[0].progression[0].effects = {ContributionEffectDefinition{"failure", "missing", "1"}};
    const ClassDefinitionManager failingCatalog({failing});
    {
        Classes classes(resources, favored, failingCatalog);
        rejects([&] { classes.addLevel(CharacterClassType::Wizard); });
        rejects([&] { classes.addLevel(CharacterClassType::Wizard); });
        assert(classes.level() == 0);
        assert(resources.contributionTotal("savingThrow.will.base") == 0);
        rejects([&] { resources.targetValue("class.wizard.level"); });
    }
    return 0;
}
