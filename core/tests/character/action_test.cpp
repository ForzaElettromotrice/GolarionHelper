#include "golarion/character/action.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/actions_view.hpp"
#include "golarion/view/resource_manager_view.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <string>

namespace
{
    template<typename Function>
    bool throwsInvalidArgument(Function function)
    {
        try
        {
            function();
            return false;
        }
        catch (const std::invalid_argument &)
        {
            return true;
        }
    }

    const golarion::ActionCategoryView &findCategory(const golarion::ActionsView &view, const std::string &id)
    {
        const auto category = std::ranges::find(view.categories, id, &golarion::ActionCategoryView::id);
        assert(category != view.categories.end());
        return *category;
    }

    const golarion::ActionView &findAction(const golarion::ActionCategoryView &category, const std::string &id)
    {
        const auto action = std::ranges::find(category.actions, id, &golarion::ActionView::id);
        assert(action != category.actions.end());
        return *action;
    }
}

int main()
{
    using namespace golarion;

    ResourceManager resourceManager;
    ActionManager actionManager(resourceManager);

    const ResourceManagerView resourceView = resourceManager.toView();
    assert(std::ranges::find(resourceView.collections, ActionCategoriesResource) != resourceView.collections.end());
    assert(std::ranges::find(resourceView.collections, ActionGrantsResource) != resourceView.collections.end());
    assert(std::ranges::find(resourceView.collections, ActionInhibitionsResource) != resourceView.collections.end());
    assert(std::ranges::find(resourceView.collections, ActionNotesResource) != resourceView.collections.end());
    assert(std::ranges::find(resourceView.collections, ActionCostReplacementsResource) != resourceView.collections.end());

    ActionsView view = actionManager.toView();
    assert(view.categories.size() == 5);
    const ActionCategoryView &base = findCategory(view, "base");
    const ActionCategoryView &attacks = findCategory(view, "attacks");
    const ActionCategoryView &movement = findCategory(view, "movement");
    const ActionCategoryView &items = findCategory(view, "items");
    const ActionCategoryView &maneuvers = findCategory(view, "combatManeuvers");
    assert(base.name == "Base");
    assert(base.actions.size() == 10);
    assert(attacks.name == "Attacchi");
    assert(attacks.actions.size() == 5);
    assert(movement.name == "Movimento");
    assert(movement.actions.size() == 8);
    assert(items.name == "Oggetti");
    assert(items.actions.size() == 15);
    assert(maneuvers.name == "Manovre di combattimento");
    assert(maneuvers.actions.size() == 11);
    assert(findAction(attacks, "base.attack").effectiveCost == ActionCost::Standard);
    assert(findAction(attacks, "base.fullAttack").effectiveCost == ActionCost::FullRound);
    assert(findAction(movement, "base.fiveFootStep").effectiveCost == ActionCost::NotAnAction);
    assert(findAction(items, "base.drawWeapon").effectiveCost == ActionCost::Move);
    assert(findAction(maneuvers, "combatManeuvers.disarm").effectiveCost == ActionCost::ReplacesAttack);
    assert(findAction(maneuvers, "combatManeuvers.grapple").effectiveCost == ActionCost::Standard);
    assert(!actionManager.actionView("base.castSpell").has_value());
    assert(!actionManager.actionView("base.activateMagicItem").has_value());

    resourceManager.addToCollection(ActionCategoriesResource, ActionCategory(ActionCategoryDefinition{
        .id = "hexes",
        .source = "Fattucchiere",
        .name = "Fatture"
    }));
    resourceManager.addToCollection(ActionGrantsResource, Action(ActionDefinition{
        .id = "hex.slumber",
        .source = "Fattucchiere",
        .categoryId = "hexes",
        .name = "Sonno",
        .description = "Può far cadere addormentato il bersaglio.",
        .cost = ActionCost::Standard,
        .tags = {"hex", "supernatural"}
    }));

    resourceManager.addToCollection(ActionInhibitionsResource, ActionInhibition(ActionInhibitionDefinition{
        .id = "staggeredFullRound",
        .source = "Barcollante",
        .selector = ActionSelector(ActionSelectorDefinition{.cost = ActionCost::FullRound}),
        .reason = "Non può compiere azioni di round completo"
    }));
    assert(!actionManager.actionView("base.run")->usable);
    assert(!actionManager.actionView("base.fullAttack")->usable);

    resourceManager.addToCollection(ActionNotesResource, ActionNote(ActionNoteDefinition{
        .id = "runWithoutStraightLine",
        .source = "Talento di prova",
        .actionId = "base.run",
        .text = "Può cambiare direzione una volta durante la corsa."
    }));
    resourceManager.addToCollection(ActionCostReplacementsResource, ActionCostReplacement(ActionCostReplacementDefinition{
        .id = "fastRun",
        .source = "Capacità di prova",
        .actionId = "base.run",
        .cost = ActionCost::Move
    }));
    resourceManager.addToCollection(ActionCostReplacementsResource, ActionCostReplacement(ActionCostReplacementDefinition{
        .id = "anotherFastRun",
        .source = "Seconda capacità di prova",
        .actionId = "base.run",
        .cost = ActionCost::Move
    }));

    const ActionView changedRun = *actionManager.actionView("base.run");
    assert(changedRun.baseCost == ActionCost::FullRound);
    assert(changedRun.effectiveCost == ActionCost::Move);
    assert(changedRun.notes.size() == 1);
    assert(changedRun.notes[0].source == "Talento di prova");
    assert(changedRun.costReplacements.size() == 2);
    assert(changedRun.usable);
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.addToCollection(ActionCostReplacementsResource, ActionCostReplacement(ActionCostReplacementDefinition{
            .id = "conflictingRun",
            .source = "Conflitto",
            .actionId = "base.run",
            .cost = ActionCost::Swift
        }));
    }));
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.removeFromCollection(ActionGrantsResource, "base.run");
    }));

    resourceManager.removeFromCollection(ActionNotesResource, "runWithoutStraightLine");
    resourceManager.removeFromCollection(ActionCostReplacementsResource, "fastRun");
    resourceManager.removeFromCollection(ActionCostReplacementsResource, "anotherFastRun");
    assert(actionManager.actionView("base.run")->effectiveCost == ActionCost::FullRound);
    assert(!actionManager.actionView("base.run")->usable);
    resourceManager.removeFromCollection(ActionInhibitionsResource, "staggeredFullRound");
    assert(actionManager.actionView("base.run")->usable);

    resourceManager.addToCollection(ActionNotesResource, ActionNote(ActionNoteDefinition{
        .id = "hexNote",
        .source = "Fattucchiere",
        .actionId = "hex.slumber",
        .text = "Una creatura può essere influenzata una sola volta al giorno."
    }));
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.removeFromCollection(ActionGrantsResource, "hex.slumber");
    }));
    resourceManager.removeFromCollection(ActionNotesResource, "hexNote");
    resourceManager.removeFromCollection(ActionGrantsResource, "hex.slumber");
    resourceManager.removeFromCollection(ActionCategoriesResource, "hexes");
    assert(actionManager.toView().categories.size() == 5);
    assert(!actionManager.actionView("missing").has_value());

    assert(displayName(ActionCost::Standard) == "Standard");
    assert(displayName(ActionCost::FullRound) == "Round completo");
    assert(displayName(ActionCost::ReplacesAttack) == "Sostituisce un attacco");
    assert(displayName(ActionCost::NotAnAction) == "Non è un'azione");

    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.addToCollection(ActionGrantsResource, Action(ActionDefinition{
            .id = "base.run",
            .source = "Duplicata",
            .categoryId = "base",
            .name = "Correre ancora",
            .description = "Duplicata",
            .cost = ActionCost::FullRound,
            .tags = {}
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        Action(ActionDefinition{
            .id = "invalid",
            .source = "Test",
            .categoryId = "base",
            .name = "Non valida",
            .description = "Non valida",
            .cost = ActionCost::Standard,
            .tags = {"attack", "attack"}
        });
    }));
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.addToCollection(ActionNotesResource, ActionNote(ActionNoteDefinition{
            .id = "missingNote",
            .source = "Test",
            .actionId = "missing",
            .text = "Non valida"
        }));
    }));
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.addToCollection(ActionCostReplacementsResource, ActionCostReplacement(ActionCostReplacementDefinition{
            .id = "missingReplacement",
            .source = "Test",
            .actionId = "missing",
            .cost = ActionCost::Free
        }));
    }));
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.removeFromCollection(ActionCategoriesResource, "base");
    }));

    return 0;
}
