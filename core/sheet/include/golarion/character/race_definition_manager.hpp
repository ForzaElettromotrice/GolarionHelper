#pragma once

#include "golarion/character/race_definition.hpp"
#include "golarion/view/race_view.hpp"

#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

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
        std::vector<RaceSummaryView> listRaces() const;
        std::vector<AlternateRacialFeatureView> alternateFeaturesView(std::string_view raceDefinitionId);

    private:
        RaceDefinitionManager();

        std::map<std::string, RaceDefinition> definitions_;
        std::map<std::string, std::string> names_;
        std::mutex mutex_;
    };
}
