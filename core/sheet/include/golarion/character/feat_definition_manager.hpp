#pragma once

#include "golarion/character/feat_definition.hpp"
#include "golarion/view/feat_definition_view.hpp"

#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    class FeatDefinitionManager final
    {
    public:
        struct CatalogEntry
        {
            std::string id;
            std::string name;
            std::vector<FeatType> types;
        };
        static FeatDefinitionManager &instance();
        std::vector<CatalogEntry> catalogEntries() const;

        FeatDefinitionManager(const FeatDefinitionManager &) = delete;
        FeatDefinitionManager &operator=(const FeatDefinitionManager &) = delete;
        FeatDefinitionManager(FeatDefinitionManager &&) = delete;
        FeatDefinitionManager &operator=(FeatDefinitionManager &&) = delete;

        bool contains(std::string_view id);
        const FeatDefinition &get(std::string_view featDefinitionId);
        std::vector<FeatDefinitionView> toView();

    private:
        FeatDefinitionManager();

        std::map<std::string, FeatDefinition> definitions_;
        std::map<std::string, CatalogEntry> entries_;
        std::mutex mutex_;
    };
}
