#pragma once
#include <string>
#include <vector>

namespace golarion
{
    struct GrantedSpellData { std::string id; int remaining; };
    struct GrantedSpellsData { std::vector<GrantedSpellData> spells; };
}
