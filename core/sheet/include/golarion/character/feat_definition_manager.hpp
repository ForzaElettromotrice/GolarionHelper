#pragma once

#include "golarion/character/feat_definition.hpp"

#include <map>
#include <mutex>
#include <string>
#include <string_view>

namespace golarion
{
    class FeatDefinitionManager final
    {
    public:
        static FeatDefinitionManager &instance();

        FeatDefinitionManager(const FeatDefinitionManager &) = delete;
        FeatDefinitionManager &operator=(const FeatDefinitionManager &) = delete;
        FeatDefinitionManager(FeatDefinitionManager &&) = delete;
        FeatDefinitionManager &operator=(FeatDefinitionManager &&) = delete;

        bool contains(std::string_view id);
        const FeatDefinition &get(std::string_view featDefinitionId);

    private:
        FeatDefinitionManager() = default;

        void loadCatalog();

        std::map<std::string, FeatDefinition> definitions_;
        bool loaded_ = false;
        std::mutex mutex_;
    };
}
