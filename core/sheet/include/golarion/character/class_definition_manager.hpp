#pragma once

#include "golarion/character/class_definition.hpp"
#include "golarion/character/archetype_definition.hpp"
#include "golarion/view/classes_view.hpp"

#include <map>
#include <mutex>
#include <vector>

namespace golarion
{
    struct ClassSummary { CharacterClassType type; std::string name; };
    struct ArchetypeSummary { std::string id, name; };

    class ClassDefinitionManager final
    {
    public:
        static const ClassDefinitionManager &instance();
        explicit ClassDefinitionManager(std::vector<ClassDefinition> definitions, std::vector<ArchetypeDefinition> archetypes = {});

        ClassDefinitionManager(const ClassDefinitionManager &) = delete;
        ClassDefinitionManager &operator=(const ClassDefinitionManager &) = delete;
        ClassDefinitionManager(ClassDefinitionManager &&) = delete;
        ClassDefinitionManager &operator=(ClassDefinitionManager &&) = delete;

        const ClassDefinition &get(CharacterClassType type) const;
        const ArchetypeDefinition &getArchetype(const std::string &id) const;
        std::vector<ClassSummary> listClasses() const;
        std::vector<ArchetypeSummary> listArchetypes(CharacterClassType type) const;

    private:
        ClassDefinitionManager();
        struct Record { ClassDefinition definition; std::vector<ArchetypeSummary> archetypes; };
        const Record &record(CharacterClassType type) const;
        std::map<CharacterClassType, std::string> names_;
        bool diskBacked_ = false;
        mutable std::mutex mutex_;
        mutable std::map<CharacterClassType, Record> classes_;
        mutable std::map<std::string, ArchetypeDefinition> archetypes_;
    };
}
