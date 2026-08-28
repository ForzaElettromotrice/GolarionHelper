#include "golarion/character/action.hpp"
#include "golarion/character/character_sheet.hpp"
#include "golarion/character/condition.hpp"
#include "golarion/view/character_sheet_view.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <string>
#include <string_view>

namespace
{
    golarion::ActionView actionView(golarion::CharacterSheet &sheet, std::string_view actionId)
    {
        const golarion::CharacterSheetView sheetView = sheet.toView();
        for (const golarion::ActionCategoryView &category : sheetView.actions.categories)
        {
            const auto action = std::ranges::find(category.actions, actionId, &golarion::ActionView::id);
            if (action != category.actions.end())
            {
                return *action;
            }
        }
        assert(false);
        return {};
    }

    void addCondition(golarion::CharacterSheet &sheet, std::string_view conditionId)
    {
        sheet.addCondition(golarion::ConditionEntry(golarion::ConditionEntryDefinition{
            .id = "test.condition",
            .conditionId = std::string(conditionId),
            .source = "Test"
        }));
    }

    void removeCondition(golarion::CharacterSheet &sheet)
    {
        sheet.removeCondition("test.condition");
    }

    bool everyActionIsInhibited(golarion::CharacterSheet &sheet)
    {
        const golarion::CharacterSheetView sheetView = sheet.toView();
        return std::ranges::all_of(sheetView.actions.categories, [](const golarion::ActionCategoryView &category)
        {
            return std::ranges::all_of(category.actions, [](const golarion::ActionView &action)
            {
                return !action.usable;
            });
        });
    }
}

int main()
{
    using namespace golarion;

    CharacterSheet sheet;
    assert(actionView(sheet, "base.attack").usable);
    assert(actionView(sheet, "base.run").usable);

    for (const std::string_view conditionId : std::array{
        std::string_view("cowering"),
        std::string_view("fascinated"),
        std::string_view("dazed"),
        std::string_view("dead"),
        std::string_view("unconscious"),
        std::string_view("stunned"),
        std::string_view("dying"),
        std::string_view("stable"),
        std::string_view("petrified")
    })
    {
        addCondition(sheet, conditionId);
        assert(everyActionIsInhibited(sheet));
        removeCondition(sheet);
        assert(actionView(sheet, "base.attack").usable);
    }

    for (const std::string_view conditionId : std::array{std::string_view("fatigue"), std::string_view("entangled")})
    {
        addCondition(sheet, conditionId);
        assert(!actionView(sheet, "base.run").usable);
        assert(!actionView(sheet, "base.charge").usable);
        assert(actionView(sheet, "base.attack").usable);
        removeCondition(sheet);
    }

    for (const std::string_view conditionId : std::array{std::string_view("staggered"), std::string_view("disabled")})
    {
        addCondition(sheet, conditionId);
        assert(!actionView(sheet, "base.fullAttack").usable);
        assert(!actionView(sheet, "base.run").usable);
        assert(actionView(sheet, "base.attack").usable);
        assert(actionView(sheet, "base.move").usable);
        removeCondition(sheet);
    }

    addCondition(sheet, "nauseated");
    assert(!actionView(sheet, "base.attack").usable);
    assert(!actionView(sheet, "base.fullAttack").usable);
    assert(!actionView(sheet, "combatManeuvers.disarm").usable);
    assert(actionView(sheet, "base.move").usable);
    assert(!actionView(sheet, "base.speak").usable);
    assert(actionView(sheet, "base.fiveFootStep").usable);
    removeCondition(sheet);

    addCondition(sheet, "grappled");
    assert(!actionView(sheet, "base.move").usable);
    assert(!actionView(sheet, "base.run").usable);
    assert(!actionView(sheet, "base.charge").usable);
    assert(actionView(sheet, "base.attack").usable);
    assert(actionView(sheet, "base.escapeGrapple").usable);
    assert(actionView(sheet, "combatManeuvers.grapple").usable);
    removeCondition(sheet);

    addCondition(sheet, "pinned");
    assert(!actionView(sheet, "base.move").usable);
    assert(!actionView(sheet, "base.attack").usable);
    assert(!actionView(sheet, "base.drawWeapon").usable);
    assert(actionView(sheet, "base.escapeGrapple").usable);
    removeCondition(sheet);

    assert(actionView(sheet, "base.attack").usable);
    assert(actionView(sheet, "base.run").usable);
    assert(actionView(sheet, "base.drawWeapon").usable);

    return 0;
}
