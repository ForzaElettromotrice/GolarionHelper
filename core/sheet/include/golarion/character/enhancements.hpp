#pragma once

#include "golarion/view/enhancements_view.hpp"
#include "golarion/effect/effect_compiler.hpp"

#include <map>
#include <memory>

namespace golarion
{
    class EnhancementGroup final
    {
    public:
        EnhancementGroup(ResourceManager &resources, std::string id, std::string name);
        ~EnhancementGroup();
        EnhancementGroup(const EnhancementGroup &) = delete;
        EnhancementGroup &operator=(const EnhancementGroup &) = delete;
        void add(EnhancementDefinition data);
        void remove(std::string_view id);
        void setActive(std::string_view id, bool active);
        EnhancementGroupData toData() const;
        EnhancementGroupView toView() const;

    private:
        struct Entry
        {
            EnhancementData data;
            EffectCleanup cleanup;
        };
        void activate(Entry &entry);
        ResourceManager &resources_;
        std::string id_;
        std::string name_;
        std::map<std::string, Entry> entries_;
    };

    class Enhancements final
    {
    public:
        explicit Enhancements(ResourceManager &resources);
        void addGroup(EnhancementGroupDefinition data);
        void removeGroup(std::string_view id);
        void add(std::string_view groupId, EnhancementDefinition data);
        void remove(std::string_view groupId, std::string_view id);
        void setActive(std::string_view groupId, std::string_view id, bool active);
        EnhancementsData toData() const;
        EnhancementsView toView() const;

    private:
        EnhancementGroup &group(std::string_view id);
        ResourceManager &resources_;
        std::map<std::string, std::unique_ptr<EnhancementGroup>> groups_;
    };
}
