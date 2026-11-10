#pragma once

#include "golarion/character/character_class_type.hpp"
#include "golarion/character/class_feature_definition.hpp"

#include <string>
#include <vector>

namespace golarion
{
    enum class ArchetypeFeatureChangeType { Alter, Replace };

    struct ArchetypeFeatureChangeDefinition
    {
        // Original class feature ID, used for compatibility across all levels.
        std::string featureId;
        ArchetypeFeatureChangeType type;
    };

    struct ArchetypeDefinition
    {
        std::string id;
        CharacterClassType classType;
        std::string name;
        std::string description;

        // Rules-level impact, independent of which acquisitions change mechanically.
        std::vector<ArchetypeFeatureChangeDefinition> affectedFeatures{};

        // Structural edits applied before evaluating level thresholds. Removing an
        // entire feature also removes its acquisitions and nested choice branches.
        std::vector<std::string> removedFeatureIds{};
        std::vector<std::string> removedAcquisitionIds{};

        // Replacement benefits and additional features reuse ordinary progression,
        // effects and choices. They must have their own stable IDs.
        std::vector<ClassFeatureDefinition> addedFeatures{};
    };
}
