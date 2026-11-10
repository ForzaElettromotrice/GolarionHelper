#pragma once

#include "golarion/character/class_definition.hpp"

namespace golarion::test
{
    inline ClassDefinition levelOnlyClassDefinition()
    {
        return {
            .type = CharacterClassType::Fighter, .name = "Livello di prova", .description = "Solo livello, senza bonus.",
            .hitDie = ClassHitDie::D8, .skillRanksPerLevel = 2, .classSkills = {},
            .progression = {{1, 0, {{SavingThrowType::Fortitude, 0}, {SavingThrowType::Reflex, 0}, {SavingThrowType::Will, 0}}}}
        };
    }

    inline ClassDefinition classDefinition(CharacterClassType type)
    {
        // Synthetic progression, not an official class catalog entry.
        return {
            .type = type, .name = "Classe di prova", .description = "Progressione sintetica.",
            .hitDie = ClassHitDie::D8, .skillRanksPerLevel = 2, .classSkills = {},
            .progression = {
                {1, 1, {{SavingThrowType::Fortitude, 2}, {SavingThrowType::Reflex, 0}, {SavingThrowType::Will, 0}}},
                {2, 2, {{SavingThrowType::Fortitude, 3}, {SavingThrowType::Reflex, 0}, {SavingThrowType::Will, 0}}},
                {3, 3, {{SavingThrowType::Fortitude, 3}, {SavingThrowType::Reflex, 1}, {SavingThrowType::Will, 1}}}
            }
        };
    }
}
