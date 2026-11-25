#include "sheet_view_changes.hpp"
#include "view_comparison.hpp"
#include "golarion/view/character_sheet_view.hpp"

namespace sheet_view_compare
{
    template<> bool equal(const golarion::GameDuration &a, const golarion::GameDuration &b)
    {
        return a.roundCount() == b.roundCount();
    }

    // Definitions contain rules that are not displayed by the catalogue pickers.
    template<> bool equal(const golarion::SpellDefinition &a, const golarion::SpellDefinition &b)
    {
        return a.id == b.id && a.name == b.name && a.spellLevelByClass == b.spellLevelByClass;
    }

    template<> bool equal(const golarion::FeatChoiceOptionDefinition &a, const golarion::FeatChoiceOptionDefinition &b)
    {
        return a.id == b.id && a.name == b.name;
    }

    template<> bool equal(const golarion::FeatDefinition &a, const golarion::FeatDefinition &b)
    {
        return a.id == b.id && a.name == b.name && a.description == b.description && a.types == b.types && equal(a.choices, b.choices);
    }

    template<> bool equal(const golarion::EffectDefinition &a, const golarion::EffectDefinition &b)
    {
        if (a.index() != b.index()) return false;
        return std::visit([&](const auto &x) {
            using T = std::decay_t<decltype(x)>;
            const auto &y = std::get<T>(b);
            if (x.id != y.id) return false;
            if constexpr (requires { x.description.empty(); })
                if (x.description != y.description) return false;
            if constexpr (std::is_same_v<T, golarion::ModifierEffectDefinition> || std::is_same_v<T, golarion::ContributionEffectDefinition>)
                return equal(x, y);
            return true;
        }, a);
    }

}

SheetViewChanges compareSheetViews(const golarion::CharacterSheetView *previous, const golarion::CharacterSheetView &next)
{
    using sheet_view_compare::equal;
    return {
        .identity = !previous || !equal(previous->identity, next.identity),
        .abilities = !previous || !equal(previous->abilities, next.abilities),
        .skills = !previous || !equal(previous->skills, next.skills),
        .saves = !previous || !equal(previous->savingThrows, next.savingThrows),
        .initiative = !previous || !equal(previous->initiative, next.initiative),
        .armor = !previous || !equal(previous->armorClass, next.armorClass),
        .hp = !previous || !equal(previous->hitPoints, next.hitPoints),
        .defenses = !previous || !equal(previous->specialDefenses, next.specialDefenses),
        .movement = !previous || !equal(previous->movement, next.movement),
        .conditions = !previous || !equal(previous->conditions, next.conditions),
        .attacks = !previous || !equal(previous->attacks, next.attacks),
        .bab = !previous || !equal(previous->baseAttackBonus.total, next.baseAttackBonus.total),
        .routines = !previous || !equal(previous->attackRoutines, next.attackRoutines),
        .actions = !previous || !equal(previous->actions, next.actions),
        .maneuvers = !previous || !equal(previous->combatManeuvers, next.combatManeuvers),
        .items = !previous || !equal(previous->inventory.items, next.inventory.items),
        .containers = !previous || !equal(previous->inventory.containers, next.inventory.containers),
        .equipment = !previous || !equal(previous->inventory.equipmentSlots, next.inventory.equipmentSlots),
        .weight = !previous || !equal(previous->inventory.totalCarriedWeightGrams, next.inventory.totalCarriedWeightGrams),
        .money = !previous || !equal(previous->inventory.money, next.inventory.money),
        .load = !previous || !equal(previous->encumbrance, next.encumbrance),
        .race = !previous || !equal(previous->race, next.race),
        .classes = !previous || !equal(previous->classes, next.classes),
        .feats = !previous || !equal(previous->feats, next.feats),
        .languages = !previous || !equal(previous->languages, next.languages),
        .reminders = !previous || !equal(previous->reminders, next.reminders),
        .castings = !previous || !equal(previous->spellcastings.castings, next.spellcastings.castings),
        .granted = !previous || !equal(previous->grantedSpells, next.grantedSpells),
        .customSpells = !previous || !equal(previous->customSpellDefinitions, next.customSpellDefinitions),
        .customFeats = !previous || !equal(previous->customFeatDefinitions, next.customFeatDefinitions),
        .enhancements = !previous || !equal(previous->enhancements, next.enhancements),
        .notes = !previous || !equal(previous->notes, next.notes),
    };
}
