#include "golarion/golarion.h"

#include "golarion/character_sheet.hpp"

#include <new>

struct GhCharacterSheet
{
    golarion::CharacterSheet value;
};

extern "C" GhCharacterSheet *gh_character_sheet_create(void)
{
    try
    {
        return new GhCharacterSheet();
    }
    catch (...)
    {
        return nullptr;
    }
}

extern "C" void gh_character_sheet_destroy(GhCharacterSheet *sheet)
{
    delete sheet;
}

