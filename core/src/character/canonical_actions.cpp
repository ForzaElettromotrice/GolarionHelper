#include "golarion/character/action.hpp"

#include <utility>
#include <vector>

namespace
{
    constexpr char BaseCategoryId[] = "base";
    constexpr char AttacksCategoryId[] = "attacks";
    constexpr char MovementCategoryId[] = "movement";
    constexpr char ItemsCategoryId[] = "items";
    constexpr char CombatManeuversCategoryId[] = "combatManeuvers";
    constexpr char BaseRulesSource[] = "Regole base";

    std::vector<golarion::ActionDefinition> baseActionDefinitions()
    {
        using golarion::ActionCost;

        return {
            {.id = "base.attack", .source = BaseRulesSource, .categoryId = AttacksCategoryId, .name = "Attaccare", .description = "Effettua un singolo attacco in mischia, a distanza o senz'armi.", .cost = ActionCost::Standard, .tags = {"attack"}},
            {.id = "base.lightTorchWithEmber", .source = BaseRulesSource, .categoryId = ItemsCategoryId, .name = "Accendere una torcia con un tizzone ardente", .description = "Accende una torcia usando una fiamma già disponibile.", .cost = ActionCost::Standard, .tags = {"item"}},
            {.id = "base.aidAnother", .source = BaseRulesSource, .categoryId = BaseCategoryId, .name = "Aiutare un altro", .description = "Aiuta un alleato in una prova o in combattimento.", .cost = ActionCost::Standard, .tags = {"aid"}},
            {.id = "base.totalDefense", .source = BaseRulesSource, .categoryId = BaseCategoryId, .name = "Difesa totale", .description = "Si concentra esclusivamente sulla propria difesa.", .cost = ActionCost::Standard, .tags = {"defense"}},
            {.id = "base.drawHiddenWeapon", .source = BaseRulesSource, .categoryId = ItemsCategoryId, .name = "Estrarre un'arma occultata", .description = "Estrae un'arma nascosta usando Rapidità di Mano.", .cost = ActionCost::Standard, .tags = {"item", "weapon"}},
            {.id = "base.feint", .source = BaseRulesSource, .categoryId = AttacksCategoryId, .name = "Fintare", .description = "Tenta di sviare un avversario in combattimento.", .cost = ActionCost::Standard, .tags = {"attack", "feint"}},
            {.id = "base.escapeGrapple", .source = BaseRulesSource, .categoryId = CombatManeuversCategoryId, .name = "Liberarsi da una lotta", .description = "Tenta di liberarsi da una lotta con una prova appropriata.", .cost = ActionCost::Standard, .tags = {"combat-maneuver", "grapple"}},
            {.id = "base.ready", .source = BaseRulesSource, .categoryId = BaseCategoryId, .name = "Preparare", .description = "Prepara un'azione standard da eseguire al verificarsi di una condizione.", .cost = ActionCost::Standard, .tags = {"ready"}},
            {.id = "base.stabilizeDyingAlly", .source = BaseRulesSource, .categoryId = BaseCategoryId, .name = "Stabilizzare un alleato morente", .description = "Usa Guarire per stabilizzare un alleato morente.", .cost = ActionCost::Standard, .tags = {"skill"}},
            {.id = "base.useStandardSkill", .source = BaseRulesSource, .categoryId = BaseCategoryId, .name = "Usare un'abilità", .description = "Usa un'abilità che richiede un'azione standard.", .cost = ActionCost::Standard, .tags = {"skill"}},

            {.id = "base.move", .source = BaseRulesSource, .categoryId = MovementCategoryId, .name = "Muoversi", .description = "Si muove fino alla propria velocità.", .cost = ActionCost::Move, .tags = {"movement"}},
            {.id = "base.standUp", .source = BaseRulesSource, .categoryId = MovementCategoryId, .name = "Alzarsi da prono", .description = "Si alza dalla condizione Prono.", .cost = ActionCost::Move, .tags = {"movement"}},
            {.id = "base.openOrCloseDoor", .source = BaseRulesSource, .categoryId = ItemsCategoryId, .name = "Aprire o chiudere una porta", .description = "Apre oppure chiude una porta accessibile.", .cost = ActionCost::Move, .tags = {"object"}},
            {.id = "base.reloadLightOrHandCrossbow", .source = BaseRulesSource, .categoryId = ItemsCategoryId, .name = "Caricare una balestra a mano o leggera", .description = "Carica una balestra a mano o una balestra leggera.", .cost = ActionCost::Move, .tags = {"reload", "weapon"}},
            {.id = "base.controlFrightenedMount", .source = BaseRulesSource, .categoryId = MovementCategoryId, .name = "Controllare una cavalcatura spaventata", .description = "Tenta di controllare una cavalcatura spaventata.", .cost = ActionCost::Move, .tags = {"mount"}},
            {.id = "base.drawWeapon", .source = BaseRulesSource, .categoryId = ItemsCategoryId, .name = "Estrarre un'arma", .description = "Estrae un'arma pronta all'uso.", .cost = ActionCost::Move, .tags = {"item", "weapon"}},
            {.id = "base.mountOrDismount", .source = BaseRulesSource, .categoryId = MovementCategoryId, .name = "Montare o smontare da cavallo", .description = "Monta oppure smonta da una cavalcatura.", .cost = ActionCost::Move, .tags = {"mount", "movement"}},
            {.id = "base.readyOrDropShield", .source = BaseRulesSource, .categoryId = ItemsCategoryId, .name = "Preparare o lasciar cadere uno scudo", .description = "Prepara uno scudo oppure lo lascia cadere.", .cost = ActionCost::Move, .tags = {"item", "shield"}},
            {.id = "base.pickUpItem", .source = BaseRulesSource, .categoryId = ItemsCategoryId, .name = "Raccogliere un oggetto", .description = "Raccoglie da terra un oggetto accessibile.", .cost = ActionCost::Move, .tags = {"item"}},
            {.id = "base.retrieveStoredItem", .source = BaseRulesSource, .categoryId = ItemsCategoryId, .name = "Recuperare un oggetto custodito", .description = "Recupera un oggetto riposto in un contenitore o nell'equipaggiamento.", .cost = ActionCost::Move, .tags = {"item"}},
            {.id = "base.sheatheWeapon", .source = BaseRulesSource, .categoryId = ItemsCategoryId, .name = "Rinfoderare un'arma", .description = "Ripone un'arma nel suo fodero.", .cost = ActionCost::Move, .tags = {"item", "weapon"}},
            {.id = "base.pushHeavyObject", .source = BaseRulesSource, .categoryId = ItemsCategoryId, .name = "Spingere un oggetto pesante", .description = "Sposta un oggetto pesante accessibile.", .cost = ActionCost::Move, .tags = {"movement", "object"}},

            {.id = "base.fullAttack", .source = BaseRulesSource, .categoryId = AttacksCategoryId, .name = "Attacco completo", .description = "Effettua tutti gli attacchi concessi dalla routine scelta.", .cost = ActionCost::FullRound, .tags = {"attack", "full-attack"}},
            {.id = "base.lightTorch", .source = BaseRulesSource, .categoryId = ItemsCategoryId, .name = "Accendere una torcia", .description = "Accende una torcia senza disporre di una fiamma pronta.", .cost = ActionCost::FullRound, .tags = {"item"}},
            {.id = "base.lockWeaponToGauntlet", .source = BaseRulesSource, .categoryId = ItemsCategoryId, .name = "Assicurare o liberare un'arma dal guanto d'arme", .description = "Assicura un'arma a un guanto d'arme con sicura oppure la libera.", .cost = ActionCost::FullRound, .tags = {"item", "weapon"}},
            {.id = "base.charge", .source = BaseRulesSource, .categoryId = AttacksCategoryId, .name = "Caricare", .description = "Si muove in linea retta e attacca secondo le regole della carica; può diventare standard quando si dispone di una sola azione.", .cost = ActionCost::FullRound, .tags = {"attack", "charge", "movement"}},
            {.id = "base.reloadHeavyOrRepeatingCrossbow", .source = BaseRulesSource, .categoryId = ItemsCategoryId, .name = "Caricare una balestra pesante o a ripetizione", .description = "Carica una balestra pesante o una balestra a ripetizione.", .cost = ActionCost::FullRound, .tags = {"reload", "weapon"}},
            {.id = "base.run", .source = BaseRulesSource, .categoryId = MovementCategoryId, .name = "Correre", .description = "Si muove fino al multiplo di corsa consentito dalla modalità di movimento.", .cost = ActionCost::FullRound, .tags = {"movement", "run"}},
            {.id = "base.coupDeGrace", .source = BaseRulesSource, .categoryId = AttacksCategoryId, .name = "Dare il colpo di grazia", .description = "Infligge un colpo di grazia a un bersaglio indifeso.", .cost = ActionCost::FullRound, .tags = {"attack"}},
            {.id = "base.escapeNet", .source = BaseRulesSource, .categoryId = BaseCategoryId, .name = "Liberarsi da una rete", .description = "Tenta di liberarsi da una rete.", .cost = ActionCost::FullRound, .tags = {"escape"}},
            {.id = "base.prepareSplashWeapon", .source = BaseRulesSource, .categoryId = ItemsCategoryId, .name = "Preparare un'arma a spargimento", .description = "Prepara un'arma a spargimento prima del lancio.", .cost = ActionCost::FullRound, .tags = {"item", "weapon"}},
            {.id = "base.withdraw", .source = BaseRulesSource, .categoryId = MovementCategoryId, .name = "Ritirarsi", .description = "Si allontana dal combattimento; può diventare standard quando si dispone di una sola azione.", .cost = ActionCost::FullRound, .tags = {"movement", "withdraw"}},
            {.id = "base.extinguishFlames", .source = BaseRulesSource, .categoryId = BaseCategoryId, .name = "Spegnere le fiamme", .description = "Tenta di spegnere le fiamme che avvolgono il personaggio.", .cost = ActionCost::FullRound, .tags = {"fire"}},
            {.id = "base.useFullRoundSkill", .source = BaseRulesSource, .categoryId = BaseCategoryId, .name = "Usare un'abilità per un round", .description = "Usa un'abilità che richiede un intero round.", .cost = ActionCost::FullRound, .tags = {"skill"}},

            {.id = "base.dropProne", .source = BaseRulesSource, .categoryId = MovementCategoryId, .name = "Cadere a terra", .description = "Si lascia cadere volontariamente a terra Prono.", .cost = ActionCost::Free, .tags = {"movement"}},
            {.id = "base.dropItem", .source = BaseRulesSource, .categoryId = ItemsCategoryId, .name = "Lasciar cadere un oggetto", .description = "Lascia cadere un oggetto impugnato.", .cost = ActionCost::Free, .tags = {"item"}},
            {.id = "base.speak", .source = BaseRulesSource, .categoryId = BaseCategoryId, .name = "Parlare", .description = "Pronuncia una breve frase durante il proprio turno.", .cost = ActionCost::Free, .tags = {"speech"}},
            {.id = "base.fiveFootStep", .source = BaseRulesSource, .categoryId = MovementCategoryId, .name = "Passo di 1,5 metri", .description = "Compie un passo di 1,5 metri quando le altre azioni del round lo consentono.", .cost = ActionCost::NotAnAction, .tags = {"movement"}},
            {.id = "base.delay", .source = BaseRulesSource, .categoryId = BaseCategoryId, .name = "Ritardare", .description = "Ritarda volontariamente il proprio momento di iniziativa.", .cost = ActionCost::NotAnAction, .tags = {"initiative"}}
        };
    }

    std::vector<golarion::ActionDefinition> combatManeuverDefinitions()
    {
        using golarion::ActionCost;

        return {
            {.id = "combatManeuvers.disarm", .source = BaseRulesSource, .categoryId = CombatManeuversCategoryId, .name = "Disarmare", .description = "Tenta di far cadere un oggetto impugnato dal bersaglio al posto di un attacco.", .cost = ActionCost::ReplacesAttack, .tags = {"attack", "combat-maneuver", "disarm"}},
            {.id = "combatManeuvers.grapple", .source = BaseRulesSource, .categoryId = CombatManeuversCategoryId, .name = "Lottare", .description = "Tenta di afferrare un avversario o mantiene una presa già stabilita.", .cost = ActionCost::Standard, .tags = {"attack", "combat-maneuver", "grapple"}},
            {.id = "combatManeuvers.overrun", .source = BaseRulesSource, .categoryId = CombatManeuversCategoryId, .name = "Oltrepassare", .description = "Tenta di attraversare lo spazio di un avversario durante il movimento o una carica.", .cost = ActionCost::Standard, .tags = {"attack", "combat-maneuver", "movement", "overrun"}},
            {.id = "combatManeuvers.reposition", .source = BaseRulesSource, .categoryId = CombatManeuversCategoryId, .name = "Riposizionare", .description = "Costringe un avversario a spostarsi in una nuova posizione.", .cost = ActionCost::Standard, .tags = {"attack", "combat-maneuver", "reposition"}},
            {.id = "combatManeuvers.steal", .source = BaseRulesSource, .categoryId = CombatManeuversCategoryId, .name = "Rubare", .description = "Tenta di sottrarre un oggetto accessibile a un avversario.", .cost = ActionCost::Standard, .tags = {"attack", "combat-maneuver", "steal"}},
            {.id = "combatManeuvers.trip", .source = BaseRulesSource, .categoryId = CombatManeuversCategoryId, .name = "Sbilanciare", .description = "Tenta di rendere Prono un avversario al posto di un attacco in mischia.", .cost = ActionCost::ReplacesAttack, .tags = {"attack", "combat-maneuver", "trip"}},
            {.id = "combatManeuvers.sunder", .source = BaseRulesSource, .categoryId = CombatManeuversCategoryId, .name = "Spezzare", .description = "Tenta di danneggiare un oggetto posseduto dal bersaglio al posto di un attacco in mischia.", .cost = ActionCost::ReplacesAttack, .tags = {"attack", "combat-maneuver", "sunder"}},
            {.id = "combatManeuvers.bullRush", .source = BaseRulesSource, .categoryId = CombatManeuversCategoryId, .name = "Spingere", .description = "Tenta di respingere un avversario come azione standard o al posto dell'attacco di una carica.", .cost = ActionCost::Standard, .tags = {"attack", "combat-maneuver", "movement", "bull-rush"}},
            {.id = "combatManeuvers.dirtyTrick", .source = BaseRulesSource, .categoryId = CombatManeuversCategoryId, .name = "Sporco trucco", .description = "Impiega una manovra improvvisata per imporre temporaneamente una condizione.", .cost = ActionCost::Standard, .tags = {"attack", "combat-maneuver", "dirty-trick"}},
            {.id = "combatManeuvers.drag", .source = BaseRulesSource, .categoryId = CombatManeuversCategoryId, .name = "Trascinare", .description = "Tenta di trascinare un avversario in linea retta insieme a sé.", .cost = ActionCost::Standard, .tags = {"attack", "combat-maneuver", "movement", "drag"}}
        };
    }
}

namespace golarion
{
    void ActionManager::registerCanonicalActions()
    {
        addCategory(ActionCategory(ActionCategoryDefinition{
            .id = std::string(BaseCategoryId),
            .source = std::string(BaseRulesSource),
            .name = "Base"
        }));
        addCategory(ActionCategory(ActionCategoryDefinition{
            .id = std::string(AttacksCategoryId),
            .source = std::string(BaseRulesSource),
            .name = "Attacchi"
        }));
        addCategory(ActionCategory(ActionCategoryDefinition{
            .id = std::string(MovementCategoryId),
            .source = std::string(BaseRulesSource),
            .name = "Movimento"
        }));
        addCategory(ActionCategory(ActionCategoryDefinition{
            .id = std::string(ItemsCategoryId),
            .source = std::string(BaseRulesSource),
            .name = "Oggetti"
        }));
        addCategory(ActionCategory(ActionCategoryDefinition{
            .id = std::string(CombatManeuversCategoryId),
            .source = std::string(BaseRulesSource),
            .name = "Manovre di combattimento"
        }));

        for (ActionDefinition &definition : baseActionDefinitions())
        {
            addAction(Action(std::move(definition)));
        }
        for (ActionDefinition &definition : combatManeuverDefinitions())
        {
            addAction(Action(std::move(definition)));
        }
    }
}
