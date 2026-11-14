#include "golarion/character/character_sheet.hpp"
#include "golarion/character/class_definition_catalog.hpp"
#include "golarion/character/class_definition_json.hpp"
#include "golarion/character/class_option_catalog.hpp"
#include "golarion/character/class_option_catalog_json.hpp"
#include "golarion/character/archetype_definition_catalog.hpp"
#include "golarion/character/archetype_definition_json.hpp"
#include "golarion/character/spellcasting.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <optional>
#include <set>
#include <string_view>
#include <tuple>

namespace
{
    using namespace golarion;
    using Json = nlohmann::json;

    // Real endpoint consumers, independent of CharacterSheet's private registry.
    struct Runtime
    {
        int level = 0;
        ResourceManager resources;
        ActionManager actions{resources};
        ReminderManager reminders{resources};
        AbilityChecks checks{resources};
        BaseAttackBonus bab{resources};
        std::array<AbilityScore, 6> abilities{
            AbilityScore{AbilityType::Strength}, AbilityScore{AbilityType::Dexterity},
            AbilityScore{AbilityType::Constitution}, AbilityScore{AbilityType::Intelligence},
            AbilityScore{AbilityType::Wisdom}, AbilityScore{AbilityType::Charisma}};
        Strikes strikes{resources};
        AttackRoutines routines{resources, actions, strikes};
        Attacks attacks{routines, strikes};
        CombatManeuvers maneuvers{resources};
        HitPoints hitPoints{resources};
        Initiative initiative{resources};
        ArmorClass armor{resources};
        Skills skills{resources};
        SavingThrows saves{resources};
        SpecialDefenses defenses{resources};
        Movement movement{resources};
        CarryingCapacity capacity{resources};
        Encumbrance encumbrance{resources, capacity};
        SizeManager size{resources};
        Languages languages{resources, [] { return 0; }};
        Spellcastings castings{resources};
        GrantedSpells grantedSpells{resources};
        FavoredClasses favored{resources};
        Feats feats{resources};
        ConditionManager conditions{resources};
        ItemCatalog itemCatalog;
        std::optional<Inventory> inventory;

        Runtime()
        {
            for (const auto &ability : abilities) ability.registerResources(resources);
            resources.registerTarget("level", [this] { return level; });
            inventory.emplace(resources, itemCatalog);
            for (const auto type : {SkillType::Craft, SkillType::Perform, SkillType::Profession})
                skills.addSpecialization(type, {"catalog_test", "Specializzazione di prova"});
        }

        void checkBase(const ClassLevelDefinition &row)
        {
            assert(resources.contributionTotal("baseAttackBonus") == row.baseAttackBonus);
            for (const auto &[type, value] : row.baseSavingThrows)
                assert(resources.contributionTotal(baseResourceName(type)) == value);
            static_cast<void>(skills.toView());
            static_cast<void>(movement.toView());
            static_cast<void>(defenses.toView());
            static_cast<void>(castings.toView());
            static_cast<void>(feats.toView());
        }

        void checkClean()
        {
            assert(resources.contributionTotal("baseAttackBonus") == 0);
            assert(resources.contributionTotal("hp.max") == 0);
            assert(resources.contributionTotal("skill.ranks.max") == 0);
            for (auto type : {SavingThrowType::Fortitude, SavingThrowType::Reflex, SavingThrowType::Will})
                assert(resources.contributionTotal(baseResourceName(type)) == 0);
            for (const auto &skill : skills.toView().skills) assert(skill.classSkillGrants.empty());
            assert(feats.toView().slots.empty());
            assert(castings.toView().castings.empty());
            assert(reminders.toView().messages.empty());
        }
    };

    void checkLifecycle(const ClassDefinition &definition)
    {
        Runtime runtime;
        {
            CharacterClass character(runtime.resources, std::string(characterClassId(definition.type)), definition, 0);
            runtime.checkClean();
            for (int level = 1; level <= 20; ++level)
            {
                runtime.level = level;
                character.setLevel(level, static_cast<int>(definition.hitDie));
                assert(character.level() == level);
                runtime.checkBase(definition.progression.at(static_cast<std::size_t>(level - 1)));
                const auto data = character.toData();
                assert(data.level == level && data.type == definition.type);
                assert(data.hitDieResults.size() == static_cast<std::size_t>(level));
            }
            const auto saved = character.toData();
            const auto savedCastings = runtime.castings.toData();
            Runtime restored;
            restored.level = 20;
            {
                CharacterClass loaded(restored.resources, std::string(characterClassId(definition.type)), definition, 0);
                loaded.load(saved);
                restored.castings.load(savedCastings);
                const auto actual = loaded.toData();
                assert(actual.type == saved.type && actual.level == saved.level);
                assert(actual.hitDieResults == saved.hitDieResults);
                assert(actual.archetypeIds == saved.archetypeIds);
                assert(actual.choices.empty() && saved.choices.empty());
                restored.checkBase(definition.progression.back());
                assert(restored.castings.toData().castings.size() == savedCastings.castings.size());
            }
            restored.level = 0;
            restored.checkClean();
            for (int level = 19; level >= 0; --level)
            {
                runtime.level = level;
                character.setLevel(level);
                if (level > 0) runtime.checkBase(definition.progression.at(static_cast<std::size_t>(level - 1)));
                else runtime.checkClean();
            }
        }
        runtime.checkClean();
    }

    void checkOptionLifecycle(const ClassDefinition &definition)
    {
        Runtime runtime;
        runtime.level = 20;
        {
            const std::string classId(characterClassId(definition.type));
            CharacterClass character(runtime.resources, classId, definition, 20);
            for (int level = 1; level <= 20; ++level) character.setHitDieResult(level, static_cast<int>(definition.hitDie));
            std::size_t selections = 0;
            for (;;)
            {
                std::optional<ClassOptionDefinition> next;
                const auto view = character.toView();
                for (const auto &feature : view.features)
                    for (const auto &stage : feature.acquisitions)
                    {
                        if (!stage.active) continue;
                        for (const auto &choice : stage.choices)
                        {
                            if (choice.remainingSelections == 0 || next) continue;
                            const auto option = std::find_if(choice.options.begin(), choice.options.end(), [](const auto &value) { return !value.selected; });
                            assert(option != choice.options.end());
                            next = ClassOptionDefinition{stage.id, choice.id, option->id};
                        }
                    }
                if (!next) break;
                character.selectOption(*next);
                assert(++selections < 1000);
            }
            const auto saved = character.toData();
            const auto encodedChoices = [](const CharacterClassData &data)
            {
                Json result = Json::array();
                for (const auto &choice : data.choices)
                    result.push_back({choice.acquisitionId, choice.choiceId, choice.optionIds});
                return result;
            };
            assert(character.pendingChoices().empty());
            Runtime restored;
            restored.level = 20;
            {
                CharacterClass loaded(restored.resources, classId, definition, 0);
                loaded.load(saved);
                assert(encodedChoices(loaded.toData()) == encodedChoices(saved));
                assert(loaded.pendingChoices().empty());
            }
            restored.checkClean();
            for (int level = 19; level >= 0; --level)
            {
                runtime.level = level;
                character.setLevel(level);
            }
            runtime.checkClean();
            runtime.level = 20;
            character.setLevel(20);
            for (int level = 1; level <= 20; ++level) character.setHitDieResult(level, static_cast<int>(definition.hitDie));
            assert(encodedChoices(character.toData()) == encodedChoices(saved));
            assert(character.pendingChoices().empty());
        }
        runtime.checkClean();
    }

    std::map<int, int> capacities(const Spellcasting &casting)
    {
        std::map<int, int> result;
        for (const auto &[id, pool] : casting.slotPools())
        {
            static_cast<void>(id);
            for (const auto &[level, count] : pool.dailyCapacityByLevel) result[level] += count;
        }
        return result;
    }

    void checkExamples()
    {
        auto &manager = ClassDefinitionManager::instance();
        const auto &fighter = manager.get(CharacterClassType::Fighter);
        assert(fighter.progression.back().baseAttackBonus == 20);
        assert(fighter.progression.back().baseSavingThrows.at(SavingThrowType::Fortitude) == 12);
        assert(fighter.progression.back().baseSavingThrows.at(SavingThrowType::Reflex) == 6);
        assert(fighter.progression.back().baseSavingThrows.at(SavingThrowType::Will) == 6);
        const auto &monk = manager.get(CharacterClassType::Monk);
        const auto &unchained = manager.get(CharacterClassType::UnchainedMonk);
        assert(monk.hitDie == ClassHitDie::D8 && unchained.hitDie == ClassHitDie::D10);
        assert(monk.progression.back().baseAttackBonus == 15 && unchained.progression.back().baseAttackBonus == 20);
        assert(monk.progression.back().baseSavingThrows.at(SavingThrowType::Will) == 12);
        assert(unchained.progression.back().baseSavingThrows.at(SavingThrowType::Will) == 6);

        Runtime runtime;
        runtime.level = 1;
        CharacterClass wizard(runtime.resources, "wizard", manager.get(CharacterClassType::Wizard), 1);
        auto view = runtime.castings.toView();
        assert(view.castings.size() == 1);
        const std::string id = view.castings.front().id;
        const auto initial = capacities(runtime.castings.get(id));
        assert(initial.at(0) == 3 && initial.at(1) == 1);
        std::set<std::string> initialQuotas;
        for (const auto &quota : view.castings.front().repertoireDetails.quotas) initialQuotas.insert(quota.grant.id);
        assert(!initialQuotas.empty());
        runtime.level = 20;
        wizard.setLevel(20);
        view = runtime.castings.toView();
        assert(view.castings.size() == 1 && view.castings.front().id == id);
        const auto final = capacities(runtime.castings.get(id));
        assert(final.at(0) == 4 && final.at(9) == 4);
        std::set<std::string> finalQuotas;
        for (const auto &quota : view.castings.front().repertoireDetails.quotas) finalQuotas.insert(quota.grant.id);
        for (const auto &quota : initialQuotas) assert(finalQuotas.contains(quota));
        runtime.level = 1;
        wizard.setLevel(1);
        assert(capacities(runtime.castings.get(id)) == initial);
    }

    void checkMulticlassCasting()
    {
        Runtime runtime;
        {
            Classes classes(runtime.resources, runtime.favored);
            classes.addLevel(CharacterClassType::Wizard, 6);
            classes.addLevel(CharacterClassType::Bard, 8);
            runtime.level = classes.level();
            assert(classes.level() == 2);
            const auto view = runtime.castings.toView();
            assert(view.castings.size() == 2);
            assert(view.castings[0].id != view.castings[1].id);
            const auto first = capacities(runtime.castings.get(view.castings[0].id));
            const auto second = capacities(runtime.castings.get(view.castings[1].id));
            assert(first.at(1) == 1 && second.at(1) == 1);
            classes.removeLevel(CharacterClassType::Wizard);
            runtime.level = classes.level();
            assert(runtime.castings.toView().castings.size() == 1);
            classes.removeLevel(CharacterClassType::Bard);
            runtime.level = 0;
        }
        runtime.checkClean();
    }

    void checkArchetypes()
    {
        const auto catalog = Json::parse(embedded::ArchetypeDefinitionCatalogJson);
        assert(!catalog.empty());
        std::set<std::string> ids;
        std::map<CharacterClassType, std::set<std::string>> baselineActive;
        for (const auto &entry : catalog)
        {
            const auto archetype = archetypeDefinitionFromJson(entry);
            std::cerr << "Archetype catalog: " << characterClassId(archetype.classType) << '/' << archetype.name << '\n';
            assert(ids.insert(archetype.id).second);
            const auto canonical = archetypeDefinitionToJson(archetype);
            assert(archetypeDefinitionToJson(archetypeDefinitionFromJson(canonical)) == canonical);
            const auto &manager = ClassDefinitionManager::instance();
            assert(archetypeDefinitionToJson(manager.getArchetype(archetype.id)) == canonical);
            if (!baselineActive.contains(archetype.classType))
            {
                Runtime baseline;
                baseline.level = 20;
                CharacterClass character(baseline.resources, std::string(characterClassId(archetype.classType)), manager.get(archetype.classType), 20);
                auto &active = baselineActive[archetype.classType];
                for (const auto &feature : character.toView().features)
                    for (const auto &acquisition : feature.acquisitions)
                        if (acquisition.active) active.insert(acquisition.id);
            }
            Runtime runtime;
            {
                CharacterClass character(runtime.resources, std::string(characterClassId(archetype.classType)), manager.get(archetype.classType), 0);
                character.addArchetype(archetype);
                for (int level = 1; level <= 20; ++level)
                {
                    runtime.level = level;
                    character.setLevel(level);
                    const auto view = character.toView();
                    assert(view.archetypes.size() == 1);
                    for (const auto &feature : view.features)
                        if (std::find(archetype.removedFeatureIds.begin(), archetype.removedFeatureIds.end(), feature.id) != archetype.removedFeatureIds.end())
                            for (const auto &acquisition : feature.acquisitions) assert(!acquisition.active);
                }
                for (int score : {8, 18})
                {
                    for (auto &ability : runtime.abilities) ability.setBaseValue(score);
                    static_cast<void>(runtime.skills.toView());
                    static_cast<void>(runtime.saves.toView());
                    static_cast<void>(runtime.armor.toView());
                    static_cast<void>(runtime.movement.toView());
                    static_cast<void>(runtime.defenses.toView());
                    for (const auto &ability : runtime.abilities) static_cast<void>(ability.toView(runtime.resources));
                }
                for (auto &ability : runtime.abilities) ability.setBaseValue(10);
                const auto saved = character.toData();
                {
                    Runtime restored;
                    restored.level = saved.level;
                    CharacterClass copy(restored.resources, std::string(characterClassId(archetype.classType)), manager.get(archetype.classType), 0);
                    copy.addArchetype(archetype);
                    copy.load(saved);
                    assert(copy.toData().archetypeIds == saved.archetypeIds);
                }
                character.removeArchetype(archetype.id);
                assert(character.toView().archetypes.empty());
                for (const auto &feature : character.toView().features)
                    if (std::find(archetype.removedFeatureIds.begin(), archetype.removedFeatureIds.end(), feature.id) != archetype.removedFeatureIds.end())
                            for (const auto &acquisition : feature.acquisitions) assert(acquisition.active == baselineActive.at(archetype.classType).contains(acquisition.id));
                character.addArchetype(archetype);
                for (int level = 19; level >= 0; --level)
                {
                    runtime.level = level;
                    character.setLevel(level);
                }
                character.removeArchetype(archetype.id);
            }
            runtime.checkClean();
        }

        const auto &manager = ClassDefinitionManager::instance();
        const auto find = [&](std::string_view name) -> ArchetypeDefinition
        {
            for (const auto &entry : catalog)
            {
                auto definition = archetypeDefinitionFromJson(entry);
                if (definition.classType == CharacterClassType::Barbarian && definition.name == name) return definition;
            }
            throw std::invalid_argument("missing catalog archetype example");
        };
        Runtime runtime;
        CharacterClass barbarian(runtime.resources, "barbarian", manager.get(CharacterClassType::Barbarian), 20);
        const auto drunk = find("Bruto Ubriaco");
        const auto savage = find("Barbaro Selvaggio");
        const auto invulnerable = find("Iracondo Invulnerabile");
        barbarian.addArchetype(drunk);
        barbarian.addArchetype(savage);
        assert(barbarian.toView().archetypes.size() == 2);
        bool rejected = false;
        try { barbarian.addArchetype(invulnerable); }
        catch (const std::invalid_argument &) { rejected = true; }
        assert(rejected && barbarian.toView().archetypes.size() == 2);
        barbarian.removeArchetype(savage.id);
        barbarian.addArchetype(invulnerable);
        assert(barbarian.toView().archetypes.size() == 2);
        auto reductions = runtime.defenses.toView().damageReductions;
        assert(reductions.size() == 1 && reductions.front().effectiveValue == 10);
        barbarian.setLevel(2);
        reductions = runtime.defenses.toView().damageReductions;
        assert(reductions.size() == 1 && reductions.front().effectiveValue == 1);
        barbarian.setLevel(1);
        assert(runtime.defenses.toView().damageReductions.empty());
        barbarian.removeArchetype(invulnerable.id);
        barbarian.setLevel(20);
        reductions = runtime.defenses.toView().damageReductions;
        assert(reductions.size() == 1 && reductions.front().effectiveValue == 5);
    }

    ArchetypeDefinition catalogArchetype(CharacterClassType type, std::string_view name)
    {
        static const auto catalog = Json::parse(embedded::ArchetypeDefinitionCatalogJson);
        for (const auto &entry : catalog)
            if (entry.at("classType") == characterClassId(type) && entry.at("name") == name)
                return ClassDefinitionManager::instance().getArchetype(entry.at("id").get<std::string>());
        throw std::invalid_argument("missing catalog archetype example: " + std::string(name));
    }

    void checkArchetypeProgressionCorrections()
    {
        const auto &manager = ClassDefinitionManager::instance();
        for (const auto &[type, name, originalRanks] : {
            std::tuple{CharacterClassType::Cleric, "Cardinale", 2},
            std::tuple{CharacterClassType::Druid, "Parlafolletto", 4}})
        {
            Runtime runtime;
            CharacterClass character(runtime.resources, std::string(characterClassId(type)), manager.get(type), 0);
            const auto archetype = catalogArchetype(type, name);
            character.addArchetype(archetype);
            for (int level = 1; level <= 20; ++level)
            {
                runtime.level = level;
                character.setLevel(level);
                assert(runtime.resources.contributionTotal(BaseAttackBonusResource) == level / 2);
                for (int intelligence : {2, 8, 10, 18})
                {
                    runtime.abilities[3].setBaseValue(intelligence);
                    assert(runtime.resources.contributionTotal(MaxSkillRanksResource) == level * std::max(1, 6 + (intelligence - 10) / 2));
                }
            }
            character.removeArchetype(archetype.id);
            assert(runtime.resources.contributionTotal(BaseAttackBonusResource) == 15);
            assert(runtime.resources.contributionTotal(MaxSkillRanksResource) == 20 * (originalRanks + 4));
        }
        for (const auto &[type, name, good] : {
            std::tuple{CharacterClassType::Kineticist, "Elementalista Oscuro", SavingThrowType::Will},
            std::tuple{CharacterClassType::Vigilante, "Bruto", SavingThrowType::Fortitude}})
        {
            Runtime runtime;
            CharacterClass character(runtime.resources, std::string(characterClassId(type)), manager.get(type), 0);
            const auto archetype = catalogArchetype(type, name);
            character.addArchetype(archetype);
            for (int level = 1; level <= 20; ++level)
            {
                runtime.level = level;
                character.setLevel(level);
                for (const auto save : {SavingThrowType::Fortitude, SavingThrowType::Reflex, SavingThrowType::Will})
                    assert(runtime.resources.contributionTotal(baseResourceName(save)) == (save == good ? 2 + level / 2 : level / 3));
            }
            character.removeArchetype(archetype.id);
            runtime.checkBase(manager.get(type).progression.back());
        }
    }

    void checkArchetypeCastingContinuity()
    {
        const auto &manager = ClassDefinitionManager::instance();
        for (const auto &[type, name] : {
            std::pair{CharacterClassType::Wizard, "Bombarolo Arcano"},
            std::pair{CharacterClassType::Magus, "Kensai"},
            std::pair{CharacterClassType::Magus, "Crea Fatture"}})
        {
            Runtime runtime;
            runtime.level = 20;
            CharacterClass character(runtime.resources, std::string(characterClassId(type)), manager.get(type), 20);
            const auto before = runtime.castings.toView().castings;
            assert(before.size() == 1);
            const auto id = before.front().id;
            auto *casting = &runtime.castings.get(id);
            runtime.resources.addToCollection(spellSlotsResource(id), SpellSlotGrant{"external", "Concessione esterna di prova", {{1, 1}}, std::nullopt});
            const auto originalCapacity = capacities(*casting);
            std::optional<RepertoireSelectionDefinition> selection;
            if (type == CharacterClassType::Wizard)
            {
                assert(!before.front().repertoireDetails.quotas.empty());
                selection = RepertoireSelectionDefinition{before.front().repertoireDetails.quotas.front().grant.id, 1, "magicMissile"};
                casting->selectRepertoireSpell(*selection);
            }
            else casting->addToRepertoire("magicMissile");
            const auto archetype = catalogArchetype(type, name);
            character.addArchetype(archetype);
            assert(runtime.castings.toView().castings.size() == 1 && &runtime.castings.get(id) == casting);
            auto current = runtime.castings.toView().castings.front();
            assert(std::find(current.repertoire.begin(), current.repertoire.end(), "magicMissile") != current.repertoire.end());
            if (selection)
                assert(std::any_of(current.repertoireDetails.selections.begin(), current.repertoireDetails.selections.end(), [&](const auto &entry) { return entry.active && entry.selection == *selection; }));
            const auto changedCapacity = capacities(*casting);
            if (type == CharacterClassType::Wizard) assert(!changedCapacity.contains(0));
            else if (std::string_view(name) == "Kensai")
            {
                for (const auto &[level, capacity] : originalCapacity)
                    if (level > 0) assert(changedCapacity.at(level) == capacity - 1);
            }
            else assert(changedCapacity == originalCapacity);
            character.removeArchetype(archetype.id);
            assert(&runtime.castings.get(id) == casting && capacities(*casting) == originalCapacity);
            current = runtime.castings.toView().castings.front();
            assert(std::find(current.repertoire.begin(), current.repertoire.end(), "magicMissile") != current.repertoire.end());
            runtime.resources.removeFromCollection(spellSlotsResource(id), "external");
        }
    }

    void checkPopulatedClassChoices()
    {
        const auto &manager = ClassDefinitionManager::instance();
        const auto &wizard = manager.get(CharacterClassType::Wizard);
        const auto feature = [&](const ClassDefinition &definition, std::string_view name) -> const ClassFeatureDefinition &
        {
            const auto found = std::find_if(definition.features.begin(), definition.features.end(), [&](const auto &entry) { return entry.name == name; });
            assert(found != definition.features.end());
            return *found;
        };
        const auto &schoolStage = feature(wizard, "Scuola Arcana").progression.front();
        const auto &school = schoolStage.choices.front();
        assert(school.options.size() == 32);
        const auto &specialist = school.options.front();
        const auto &opposedStage = specialist.progression.front();
        const auto &opposed = opposedStage.choices.front();
        assert(opposed.selectionCount == 2 && opposed.options.size() == 7);
        const auto &bondStage = feature(wizard, "Legame Arcano").progression.front();
        const auto &bond = bondStage.choices.front();
        const auto familiar = std::find_if(bond.options.begin(), bond.options.end(), [](const auto &entry) { return entry.id == "familiar"; });
        assert(familiar != bond.options.end());
        const auto &speciesStage = familiar->progression.front();
        const auto &species = speciesStage.choices.front();
        assert(species.options.size() == 42);
        Runtime runtime;
        {
            CharacterClass character(runtime.resources, "wizard", wizard, 1);
            const auto remaining = [&]
            {
                std::size_t result = 0;
                for (const auto &choice : character.pendingChoices())
                    if (choice.type() == PendingChoiceType::ClassOption) result += choice.remaining;
                return result;
            };
            assert(remaining() == 2);
            character.selectOption({schoolStage.id, school.id, specialist.id});
            assert(remaining() == 3);
            character.selectOption({opposedStage.id, opposed.id, opposed.options[0].id});
            character.selectOption({opposedStage.id, opposed.id, opposed.options[1].id});
            character.selectOption({bondStage.id, bond.id, familiar->id});
            assert(remaining() == 1);
            character.selectOption({speciesStage.id, species.id, species.options.front().id});
            assert(remaining() == 0);
            const auto saved = character.toData();
            assert(saved.choices.size() == 4);
            {
                Runtime restored;
                CharacterClass copy(restored.resources, "wizard", wizard, 0);
                copy.load(saved);
                assert(copy.toData().choices.size() == 4);
                for (const auto &choice : copy.pendingChoices()) assert(choice.type() != PendingChoiceType::ClassOption);
            }
            const auto bomber = catalogArchetype(CharacterClassType::Wizard, "Bombarolo Arcano");
            character.addArchetype(bomber);
            character.removeArchetype(bomber.id);
            // Removing an archetype resets affected original-feature branches,
            // including choices added by the separate option catalog.
            const auto afterRemoval = character.toData();
            for (const auto &impact : bomber.affectedFeatures)
                if (impact.featureId == feature(wizard, "Legame Arcano").id)
                    assert(std::none_of(afterRemoval.choices.begin(), afterRemoval.choices.end(), [&](const auto &choice) { return choice.acquisitionId == bondStage.id; }));
        }
        runtime.checkClean();
    }
}

int main()
{
    const auto catalog = Json::parse(golarion::embedded::ClassDefinitionCatalogJson);
    assert(catalog.size() == 44);
    std::set<CharacterClassType> types;
    const auto definitions = classDefinitionsWithOptionsFromJson(catalog, Json::parse(golarion::embedded::ClassOptionCatalogJson));
    for (const auto &definition : definitions)
    {
        std::cerr << "Class catalog: " << characterClassId(definition.type) << '\n';
        assert(types.insert(definition.type).second);
        assert(definition.progression.size() == 20);
        assert(!definition.classSkills.empty());
        for (std::size_t index = 0; index < definition.progression.size(); ++index)
        {
            const auto &row = definition.progression[index];
            assert(row.level == static_cast<int>(index) + 1 && row.baseAttackBonus <= row.level);
            assert(row.baseSavingThrows.size() == 3);
        }
        const auto canonical = classDefinitionToJson(definition);
        assert(classDefinitionToJson(classDefinitionFromJson(canonical)) == canonical);
        assert(classDefinitionToJson(ClassDefinitionManager::instance().get(definition.type)) == canonical);
        checkLifecycle(definition);
        checkOptionLifecycle(definition);
    }
    assert(!types.contains(CharacterClassType::Adept));
    checkExamples();
    checkMulticlassCasting();
    checkArchetypes();
    checkArchetypeProgressionCorrections();
    checkArchetypeCastingContinuity();
    checkPopulatedClassChoices();
}
