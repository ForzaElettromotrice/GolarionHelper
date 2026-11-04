#include "golarion/ffi/golarion.h"

#include <cassert>

int main()
{
    GhCharacterSheet *sheet = gh_character_sheet_create();

    assert(sheet != nullptr);

    gh_character_sheet_destroy(sheet);
    return 0;
}
