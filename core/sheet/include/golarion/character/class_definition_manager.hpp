#pragma once

#include "golarion/character/class_definition.hpp"
#include "golarion/character/archetype_definition.hpp"

#include <map>
#include <vector>

namespace golarion
{
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

    private:
        std::map<CharacterClassType, ClassDefinition> definitions_;
        std::map<std::string, ArchetypeDefinition> archetypes_;
    };
}
