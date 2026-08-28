#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/armor_class.hpp"
#include "golarion/character/condition.hpp"
#include "golarion/character/skill.hpp"

#include <string>

namespace golarion
{
    Condition makeBlindedCondition()
    {
        return Condition(ConditionDefinition{
            .id = "blinded",
            .name = "Accecato",
            .stages = {
                ConditionStageDefinition{
                    .id = "blinded",
                    .name = "Accecato",
                    .effects = {
                        conditionEffects::penalty("armorClassPenalty", "–2 alla Classe Armatura", "Accecato", std::string(ArmorClassAllResource), "2"),
                        conditionEffects::armorClassAbilitySuppression("armorClassAbilityBonusSuppression", "Perde il bonus di caratteristica alla Classe Armatura", "Accecato"),
                        conditionEffects::penalty("strengthSkillChecks", "–4 alle prove di abilità basate su Forza quando la cecità le ostacola", "Accecato", skillCheckResourceName(AbilityType::Strength), "4", "Quando la cecità ostacola la prova"),
                        conditionEffects::penalty("dexteritySkillChecks", "–4 alle prove di abilità basate su Destrezza quando la cecità le ostacola", "Accecato", skillCheckResourceName(AbilityType::Dexterity), "4", "Quando la cecità ostacola la prova"),
                        conditionEffects::penalty("opposedPerception", "–4 alle prove contrapposte di Percezione", "Accecato", std::string(resourceName(SkillType::Perception)), "4", "Prove contrapposte"),
                        conditionEffects::reminder("sightFailure", "Accecato: le prove e le attività basate sulla vista falliscono automaticamente."),
                        conditionEffects::reminder("totalConcealment", "Accecato: tutti gli avversari hanno Occultamento Totale, con probabilità del 50% di essere mancati, nei suoi confronti."),
                        conditionEffects::reminder("fastMovement", "Accecato: per muoversi più velocemente di metà velocità deve superare Acrobazia CD 10; se fallisce cade Prono.")
                    }
                }
            }
        });
    }
}
