#pragma once

namespace golarion { struct CharacterSheetView; }

// Inputs to the presentation projections, not a list of actions that might affect them.
struct SheetViewChanges
{
    bool identity, abilities, skills, saves, initiative, armor, hp, defenses, movement, conditions;
    bool attacks, bab, routines, actions, maneuvers;
    bool items, containers, equipment, weight, money, load;
    bool race, classes, feats, languages, reminders;
    bool castings, granted, customSpells, customFeats, enhancements, notes;
};

SheetViewChanges compareSheetViews(const golarion::CharacterSheetView *previous, const golarion::CharacterSheetView &next);
