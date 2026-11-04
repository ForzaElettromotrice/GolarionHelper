#pragma once

#include "golarion/character/race_definition.hpp"

#include <map>
#include <mutex>
#include <string>
#include <string_view>

namespace golarion
{
    class RaceDefinitionManager final
    {
    public:
        static RaceDefinitionManager &instance();

        RaceDefinitionManager(const RaceDefinitionManager &) = delete;
        RaceDefinitionManager &operator=(const RaceDefinitionManager &) = delete;
        RaceDefinitionManager(RaceDefinitionManager &&) = delete;
        RaceDefinitionManager &operator=(RaceDefinitionManager &&) = delete;

        const RaceDefinition &get(std::string_view raceDefinitionId);

    private:
        RaceDefinitionManager() = default;

        void loadCatalog();

        std::map<std::string, RaceDefinition> definitions_;
        bool loaded_ = false;
        std::mutex mutex_;
    };
}
