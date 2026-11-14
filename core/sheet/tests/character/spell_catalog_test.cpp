#include "golarion/character/character_sheet.hpp"
#include "golarion/character/prepared_spellcasting.hpp"
#include "golarion/character/spontaneous_spellcasting.hpp"
#include "golarion/character/spell_cast_result.hpp"
#include "golarion/character/spell_definition_catalog.hpp"
#include "golarion/character/spell_definition_json.hpp"
#include "golarion/character/spell_definition_manager.hpp"
#include "golarion/effect/effect_json.hpp"
#include "golarion/persistence/json.hpp"

#include <nlohmann/json.hpp>

#include <cassert>
#include <algorithm>
#include <iostream>
#include <set>
#include <string_view>

namespace
{
    using namespace golarion;
    using Json = nlohmann::json;
    constexpr std::string_view Marker = "Ehi, appena hai tempo vedi di correggermi se necessario!";

    void checkEffectMetadata(const Json &value)
    {
        if (value.is_object())
        {
            if (value.value("type", "") == "reminder")
                assert(value.at("message").get<std::string>().ends_with(Marker));
            for (const auto &[key, child] : value.items())
            {
                // Total character level must not stand in for a source's caster level.
                if (child.is_string() && (key == "expression" || key.ends_with("Expression")))
                    assert(child.get<std::string>().find("@level") == std::string::npos);
                checkEffectMetadata(child);
            }
        }
        else if (value.is_array())
            for (const auto &child : value) checkEffectMetadata(child);
    }

    void checkCastEffects(const SpellDefinition &spell)
    {
        ResourceManager caster;
        const auto original = spellDefinitionToJson(spell);
        const auto result = exportSpellCast(spell, caster);
        assert(result.maxTargets == -1 || result.maxTargets > 0);
        assert(!result.metamagic);
        assert(result.effects.size() == spell.effects.size());
        assert(spellDefinitionToJson(spell) == original);

        CharacterSheet recipient;
        const auto baseline = persistence::toJson(recipient.toSaveData());
        recipient.addEnhancementGroup({.id = "catalog.cast", .name = spell.name, .enhancements = result.effects});
        static_cast<void>(recipient.toView());
        recipient.setEnhancementGroupActive("catalog.cast", false);
        static_cast<void>(recipient.toView());
        recipient.setEnhancementGroupActive("catalog.cast", true);
        static_cast<void>(recipient.toView());
        recipient.removeEnhancementGroup("catalog.cast");
        assert(persistence::toJson(recipient.toSaveData()) == baseline);
    }

    int armorClass(const CharacterSheetView &view, ArmorClassType type)
    {
        const auto &values = view.armorClass.abilityOptions.front().values;
        const auto value = std::ranges::find(values, type, &ArmorClassValueView::type);
        assert(value != values.end());
        return value->totalValue;
    }

    void applySpell(CharacterSheet &sheet, const char *id)
    {
        ResourceManager caster;
        const auto &spell = SpellDefinitionManager::instance().get(id);
        sheet.addEnhancementGroup({.id = id, .name = spell.name, .enhancements = exportSpellCast(spell, caster).effects});
    }

    void checkKnownExamples()
    {
        auto &manager = SpellDefinitionManager::instance();
        const auto &light = manager.get("light");
        assert(light.spellLevelByClass.at(CharacterClassType::Wizard) == 0);
        assert(light.school == SpellSchool::Evocation);
        assert(light.descriptors.contains(SpellDescriptor::Light));
        const auto &armor = manager.get("mageArmor");
        assert(armor.spellLevelByClass.at(CharacterClassType::Wizard) == 1);
        assert(armor.school == SpellSchool::Conjuration);
        assert(armor.subschool == SpellSubschool::Creation);
        const auto &spark = manager.get("spark");
        assert(spark.components.size() == 1);
        assert(spark.components.contains(SpellComponentType::VerbalOrSomatic));
        const auto &water = manager.get("blessWater");
        assert(water.materialComponentCostInGoldPieces == 25);
        assert(water.components.contains(SpellComponentType::Material));
        assert(water.castingTimeCategory == SpellCastingTimeCategory::Minute);
        assert(manager.get("whisperingLore").castingTimeCategory == SpellCastingTimeCategory::FullRoundAction);
        assert(manager.get("summonMonsterI").castingTimeCategory == SpellCastingTimeCategory::Round);
        assert(manager.get("detectThoughts").spellLevelByClass.at(CharacterClassType::Psychic) == 1);
        assert(manager.get("detectThoughts").spellLevelByClass.at(CharacterClassType::Wizard) == 2);

        CharacterSheet recipient;
        const auto baseline = recipient.toView();
        applySpell(recipient, "mageArmor");
        assert(armorClass(recipient.toView(), ArmorClassType::Normal) == armorClass(baseline, ArmorClassType::Normal) + 4);
        assert(armorClass(recipient.toView(), ArmorClassType::Touch) == armorClass(baseline, ArmorClassType::Touch));
        applySpell(recipient, "shield");
        assert(armorClass(recipient.toView(), ArmorClassType::Normal) == armorClass(baseline, ArmorClassType::Normal) + 8);
        recipient.removeEnhancementGroup("shield");
        recipient.removeEnhancementGroup("mageArmor");
        assert(armorClass(recipient.toView(), ArmorClassType::Normal) == armorClass(baseline, ArmorClassType::Normal));

        applySpell(recipient, "resistance");
        const auto resistant = recipient.toView();
        for (std::size_t index = 0; index < resistant.savingThrows.savingThrows.size(); ++index)
            assert(resistant.savingThrows.savingThrows[index].abilityOptions.front().totalValue == baseline.savingThrows.savingThrows[index].abilityOptions.front().totalValue + 1);
        recipient.removeEnhancementGroup("resistance");

        recipient.addEnhancementGroup({.id = "test.strike", .name = "Attacco di prova", .enhancements = {
            effectDefinitionFromJson(Json::parse(R"({"id":"claw","type":"strikeGrant","name":"Artiglio","mode":"melee","tags":["natural"],"naturalAttackClassification":"primary","usageChannels":["claw"],"ability":"str","damageAbilityRule":"full","reach":{"maximumUnits":1},"damageComponents":[{"id":"base","role":"base","dice":{"diceCount":1,"dieSize":4},"types":["slashing"],"typeMode":"all","criticalRule":"multiplied"}]})"))}});
        const int attack = recipient.toView().strikes.strikes.front().attackAbilityOptions.front().attackBonus;
        applySpell(recipient, "bless");
        assert(recipient.toView().strikes.strikes.front().attackAbilityOptions.front().attackBonus == attack + 1);
        recipient.removeEnhancementGroup("bless");
        assert(recipient.toView().strikes.strikes.front().attackAbilityOptions.front().attackBonus == attack);
        recipient.removeEnhancementGroup("test.strike");
    }

    void checkCastingModes()
    {
        ResourceManager resources;
        PreparedSpellcasting prepared(resources, "prepared", CharacterClassType::Wizard);
        SpontaneousSpellcasting spontaneous(resources, "spontaneous", CharacterClassType::Sorcerer);
        for (const char *id : {"prepared", "spontaneous"})
        {
            resources.addToCollection(spellSlotsResource(id), SpellSlotGrant{"base", "Test", {{1, 1}}});
            resources.addToCollection(repertoireGrantsResource(id), SpellRepertoireGrant{"known", "Test", {"mageArmor"}});
        }
        const SpellSlotId slot{"base", 1, 0};
        prepared.prepare(slot, {"mageArmor"});
        const auto preparedResult = prepared.consume(slot);
        const auto spontaneousResult = spontaneous.consume(slot, {"mageArmor"});
        assert(prepared.slotState(slot).state == PreparedSpellSlotState::Consumed);
        assert(spontaneous.slotState(slot) == SpontaneousSpellSlotState::Consumed);
        assert(preparedResult.maxTargets == 1 && spontaneousResult.maxTargets == 1);
        assert(preparedResult.metamagic->spellId == "mageArmor");
        assert(spontaneousResult.metamagic->spellId == "mageArmor");
        assert(preparedResult.effects.size() == SpellDefinitionManager::instance().get("mageArmor").effects.size());
        assert(spontaneousResult.effects.size() == preparedResult.effects.size());
    }

    void checkHigherLevelExamples()
    {
        auto &manager = SpellDefinitionManager::instance();
        // Each example is checked against its individual Golarion spell page.
        const auto &haste = manager.get("haste");
        assert(haste.school == SpellSchool::Transmutation);
        assert(haste.spellLevelByClass.at(CharacterClassType::Wizard) == 3);
        assert(haste.spellLevelByClass.at(CharacterClassType::Summoner) == 2);
        assert(haste.spellLevelByClass.at(CharacterClassType::UnchainedSummoner) == 3);

        const auto &windWalk = manager.get("windWalk");
        assert(windWalk.spellLevelByClass.at(CharacterClassType::Cleric) == 6);
        assert(windWalk.spellLevelByClass.at(CharacterClassType::Druid) == 7);
        assert(windWalk.components.contains(SpellComponentType::DivineFocus));
        assert(windWalk.descriptors.contains(SpellDescriptor::Air));
        assert(windWalk.spellResistanceType == SpellResistanceType::SeeText);

        const auto &teleport = manager.get("teleport");
        assert(teleport.spellLevelByClass.at(CharacterClassType::Wizard) == 5);
        assert(teleport.spellLevelByClass.at(CharacterClassType::Summoner) == 4);
        assert(teleport.subschool == SpellSubschool::Teleportation);
        assert(teleport.components == std::set{SpellComponentType::Verbal});
        assert(teleport.spellResistanceType == SpellResistanceType::SeeText);

        const auto &wish = manager.get("wish");
        assert(wish.school == SpellSchool::Universal);
        assert(wish.spellLevelByClass.at(CharacterClassType::Wizard) == 9);
        assert(wish.spellLevelByClass.at(CharacterClassType::Psychic) == 9);
        assert(wish.materialComponentCostInGoldPieces == 25000);
        assert(wish.castingTimeCategory == SpellCastingTimeCategory::StandardAction);
        assert(manager.get("bloodBiography").spellResistanceDescription == "no");

        CharacterSheet recipient;
        recipient.setRace("human");
        const auto baseline = recipient.toView();
        const auto intelligence = [](const CharacterSheetView &view) -> const AbilityView &
        {
            return *std::ranges::find(view.abilities, AbilityType::Intelligence, &AbilityView::type);
        };
        applySpell(recipient, "foxSCunning");
        const auto clever = recipient.toView();
        assert(intelligence(clever).totalValue == intelligence(baseline).totalValue + 4);
        assert(intelligence(clever).permanentValue == intelligence(baseline).permanentValue);
        assert(intelligence(clever).skillRankBudgetValue == intelligence(baseline).skillRankBudgetValue);
        recipient.removeEnhancementGroup("foxSCunning");

        const auto landSpeed = [](const CharacterSheetView &view)
        {
            return std::ranges::find(view.movement.grants, MovementType::Land, &MovementGrantView::type)->effectiveUnits;
        };
        assert(landSpeed(baseline) == 6);
        applySpell(recipient, "haste");
        assert(landSpeed(recipient.toView()) == 12);
        recipient.removeEnhancementGroup("haste");
        applySpell(recipient, "slow");
        assert(landSpeed(recipient.toView()) == 3);
        recipient.removeEnhancementGroup("slow");
        assert(landSpeed(recipient.toView()) == landSpeed(baseline));
    }
}

int main()
{
    const auto catalog = Json::parse(golarion::embedded::SpellDefinitionCatalogJson);
    assert(catalog.is_array() && catalog.size() == 2158);
    std::set<std::string> ids;
    for (const auto &entry : catalog)
    {
        const auto definition = spellDefinitionFromJson(entry);
        std::cerr << "Spell catalog: " << definition.id << '\n';
        assert(ids.insert(definition.id).second);
        assert(!definition.spellLevelByClass.empty());
        for (const auto &[characterClass, level] : definition.spellLevelByClass)
        {
            static_cast<void>(characterClass);
            assert(level >= 0 && level <= 9);
        }
        const auto canonical = spellDefinitionToJson(definition);
        assert(spellDefinitionToJson(spellDefinitionFromJson(canonical)) == canonical);
        const auto &loaded = SpellDefinitionManager::instance().get(definition.id);
        assert(spellDefinitionToJson(loaded) == canonical);
        checkEffectMetadata(canonical);
        checkCastEffects(loaded);
    }
    assert(ids.contains("fireball"));
    checkKnownExamples();
    checkHigherLevelExamples();
    checkCastingModes();
}
