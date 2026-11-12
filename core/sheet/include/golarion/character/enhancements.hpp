#pragma once

#include "golarion/view/enhancements_view.hpp"
#include "golarion/effect/effect_compiler.hpp"
#include <memory>

namespace golarion
{
    class EnhancementGroup final
    {
    public:
        EnhancementGroup(ResourceManager &resources, EnhancementGroupDefinition definition);
        ~EnhancementGroup();
        EnhancementGroup(const EnhancementGroup &) = delete;
        EnhancementGroup &operator=(const EnhancementGroup &) = delete;
        void add(EffectDefinition definition);
        void remove(std::string_view id);
        void setActive(bool active);
        EnhancementGroupData toData() const;
        EnhancementGroupView toView() const;

    private:
        void deactivate();
        ResourceManager &resources_;
        EnhancementGroupData data_;
        std::vector<EffectCleanup> cleanups_;
    };

    class Enhancements final
    {
    public:
        explicit Enhancements(ResourceManager &resources);
        ~Enhancements();
        void addGroup(EnhancementGroupDefinition definition);
        void removeGroup(std::string_view id);
        void add(std::string_view groupId, EffectDefinition definition);
        void remove(std::string_view groupId, std::string_view id);
        void setGroupActive(std::string_view groupId, bool active);
        EnhancementsData toData() const;
        EnhancementsView toView() const;

    private:
        struct Group { std::string id; std::unique_ptr<EnhancementGroup> instance; };
        EnhancementGroup &group(std::string_view id);
        ResourceManager &resources_;
        std::vector<Group> groups_;
    };
}
