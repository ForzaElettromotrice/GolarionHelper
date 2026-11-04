#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"
#include "golarion/character/skill.hpp"

#include <string>

namespace golarion
{
    Condition makeDazzledCondition()
    {
        return Condition(ConditionDefinition{
            .id = "dazzled",
            .name = "Abbagliato",
            .stages = {
                ConditionStageDefinition{
                    .id = "dazzled",
                    .name = "Abbagliato",
                    .effects = {
                        conditionEffects::penalty("attackRolls", "–1 ai tiri per colpire", "Abbagliato", "attack.all", "1"),
                        conditionEffects::penalty("sightPerception", "–1 alle prove di Percezione basate sulla vista", "Abbagliato", std::string(resourceName(SkillType::Perception)), "1", "Basate sulla vista")
                    }
                }
            }
        });
    }
}
