#pragma once

#include "golarion/character/spell_definition.hpp"

#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    class SpellDefinitionManager final
    {
    public:
        struct CatalogEntry
        {
            std::string id;
            std::string name;
            std::map<CharacterClassType, int> levels;
        };
        static SpellDefinitionManager &instance();

        SpellDefinitionManager(const SpellDefinitionManager &) = delete;
        SpellDefinitionManager &operator=(const SpellDefinitionManager &) = delete;
        SpellDefinitionManager(SpellDefinitionManager &&) = delete;
        SpellDefinitionManager &operator=(SpellDefinitionManager &&) = delete;

        bool contains(std::string_view id);
        const SpellDefinition &get(std::string_view spellDefinitionId);
        std::vector<CatalogEntry> catalogEntries();

    private:
        SpellDefinitionManager() = default;

        void loadCatalog();

        std::map<std::string, SpellDefinition> definitions_;
        bool loaded_ = false;
        std::mutex mutex_;
    };
}
