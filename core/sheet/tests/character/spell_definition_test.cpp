#include "golarion/character/spell_definition.hpp"

#include <cassert>
#include <stdexcept>
#include <type_traits>

namespace
{
    template<typename Function> void rejects(Function function)
    {
        bool rejected = false;
        try { function(); }
        catch (const std::invalid_argument &) { rejected = true; }
        assert(rejected);
    }
}

int main()
{
    using namespace golarion;
    static_assert(std::is_enum_v<SpellSchool>);
    static_assert(std::is_enum_v<SpellDescriptor>);
    static_assert(std::is_enum_v<CharacterClassType>);
    static_assert(!std::is_convertible_v<std::string, SpellSchool>);

    const SpellDefinition fireball{
        .id = "fireball",
        .name = "Palla di Fuoco",
        .school = SpellSchool::Evocation,
        .descriptors = {SpellDescriptor::Fire},
        .spellLevelByClass = {{CharacterClassType::Wizard, 3}, {CharacterClassType::Sorcerer, 3}},
        .spellLevelByDomain = {{DomainType::Fire, 3}},
        .castingTimeCategory = SpellCastingTimeCategory::StandardAction,
        .castingTimeDescription = "Un'azione standard",
        .components = {SpellComponentType::Verbal, SpellComponentType::Somatic, SpellComponentType::Material},
        .materialComponentDescription = "Guano di pipistrello con zolfo.",
        .rangeType = SpellRangeType::Long,
        .rangeDescription = "120 metri, più 12 per livello dell'incantatore.",
        .area = "Propagazione con raggio di 6 metri.",
        .durationType = SpellDurationType::Instantaneous,
        .durationDescription = "Effetto istantaneo",
        .savingThrows = {{SavingThrowType::Reflex, SpellSavingThrowEffect::Half, "Un successo dimezza i danni."}},
        .spellResistanceType = SpellResistanceType::Yes,
        .spellResistanceDescription = "Applicabile",
        .description = "Esplosione che infligge danni da fuoco nell'area."
    };
    validateSpellDefinition(fireball);
    assert(fireball.spellLevelByClass.at(CharacterClassType::Wizard) == 3);
    assert(fireball.spellLevelByDomain.at(DomainType::Fire) == 3);
    assert(!fireball.subschool && !fireball.target && !fireball.effect);

    auto spell = fireball;
    spell.id = "test";
    spell.name = "Incantesimo di prova";
    spell.school = SpellSchool::Conjuration;
    spell.subschool = SpellSubschool::Creation;
    spell.descriptors = {SpellDescriptor::Water, SpellDescriptor::Cold};
    spell.spellLevelByClass = {{CharacterClassType::Cleric, 0}, {CharacterClassType::Druid, 1}};
    spell.spellLevelByDomain = {{DomainType::Water, 1}, {SubdomainType::Ice, 2}};
    spell.spellLevelByBloodline = {{BloodlineType::Aquatic, 1}};
    spell.components = {SpellComponentType::MaterialOrDivineFocus, SpellComponentType::Somatic};
    spell.materialComponentDescription = "Componente materiale di prova.";
    spell.materialComponentCostInGoldPieces = 100;
    spell.castingTimeCategory = SpellCastingTimeCategory::Round;
    spell.castingTimeDescription = "Un round";
    spell.rangeType = SpellRangeType::Other;
    spell.rangeDescription = "Vedi descrizione";
    spell.target = "Una creatura";
    spell.effect = "Manifestazione di prova";
    spell.durationType = SpellDurationType::Concentration;
    spell.durationDescription = "Concentrazione entro il limite indicato.";
    spell.dismissible = true;
    spell.savingThrows = {
        {SavingThrowType::Will, SpellSavingThrowEffect::Negates, "Volontà nega il primo effetto."},
        {SavingThrowType::Fortitude, SpellSavingThrowEffect::Partial, "Tempra riduce il secondo effetto."}
    };
    spell.spellResistanceType = SpellResistanceType::SeeText;
    spell.spellResistanceDescription = "Dipende dal bersaglio.";
    validateSpellDefinition(spell);
    assert(spell.spellLevelByClass.at(CharacterClassType::Cleric) == 0);
    assert(spell.spellLevelByDomain.at(SubdomainType::Ice) == 2);
    assert(spell.spellLevelByBloodline.at(BloodlineType::Aquatic) == 1);
    assert(spell.savingThrows.size() == 2);
    const auto rejectChange = [&](auto change)
    {
        auto invalid = spell;
        change(invalid);
        rejects([&] { validateSpellDefinition(invalid); });
    };
    rejectChange([](auto &value) { value.school = SpellSchool::Evocation; });
    rejectChange([](auto &value) { value.components.clear(); });
    rejectChange([](auto &value) { value.components.insert(SpellComponentType::Material); });
    rejectChange([](auto &value) { value.components.insert(SpellComponentType::DivineFocus); });
    rejectChange([](auto &value) { value.components = {SpellComponentType::Verbal}; });
    rejectChange([](auto &value) { value.materialComponentCostInGoldPieces = -1; });
    rejectChange([](auto &value) { value.focusComponentDescription = "Focus privo di componente"; });
    rejectChange([](auto &value) { value.dismissible = false; });
    rejectChange([](auto &value) { value.spellLevelByClass[CharacterClassType::Wizard] = -1; });
    rejectChange([](auto &value) { value.spellLevelByDomain[DomainType::Water] = -1; });
    rejectChange([](auto &value) { value.spellLevelByBloodline[BloodlineType::Arcane] = -1; });
    rejectChange([](auto &value) { value.id = " "; });
    rejectChange([](auto &value) { value.savingThrows[0].description = " "; });
    rejectChange([](auto &value) { value.school = static_cast<SpellSchool>(999); });
    rejectChange([](auto &value) { value.descriptors.insert(static_cast<SpellDescriptor>(999)); });
    rejectChange([](auto &value) { value.spellLevelByClass[static_cast<CharacterClassType>(999)] = 1; });

    auto copy = spell;
    copy.spellLevelByClass[CharacterClassType::Cleric] = 2;
    copy.descriptors.clear();
    assert(spell.spellLevelByClass.at(CharacterClassType::Cleric) == 0);
    assert(spell.descriptors.size() == 2);
    copy = fireball;
    copy.materialComponentDescription.reset();
    copy.components = {SpellComponentType::FocusOrDivineFocus};
    copy.focusComponentDescription = "Focus di prova.";
    copy.savingThrows.clear();
    validateSpellDefinition(copy);
    copy.components.insert(SpellComponentType::Focus);
    rejects([&] { validateSpellDefinition(copy); });
}
