#pragma once

#include "golarion/character/spell_definition.hpp"

#include <map>
#include <mutex>
#include <string>
#include <string_view>

namespace golarion
{
    class SpellDefinitionManager final
    {
    public:
        static SpellDefinitionManager &instance();

        SpellDefinitionManager(const SpellDefinitionManager &) = delete;
        SpellDefinitionManager &operator=(const SpellDefinitionManager &) = delete;
        SpellDefinitionManager(SpellDefinitionManager &&) = delete;
        SpellDefinitionManager &operator=(SpellDefinitionManager &&) = delete;

        const SpellDefinition &get(std::string_view spellDefinitionId);

    private:
        SpellDefinitionManager() = default;

        void loadCatalog();

        std::map<std::string, SpellDefinition> definitions_;
        bool loaded_ = false;
        std::mutex mutex_;
    };
}
