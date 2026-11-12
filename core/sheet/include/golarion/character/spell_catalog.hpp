#pragma once

#include "golarion/character/spell_definition_manager.hpp"
#include <vector>

namespace golarion
{
    // Owned by the sheet; must outlive every consumer of its definitions.
    class SpellCatalog final
    {
    public:
        void add(SpellDefinition definition);
        const SpellDefinition &get(std::string_view id) const;
        std::vector<SpellDefinition> customDefinitions() const;

    private:
        std::map<std::string, SpellDefinition> definitions_;
    };
}
