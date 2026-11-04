#include "golarion/character/character_sheet.hpp"
#include "golarion/character/condition.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <optional>
#include <string>
#include <string_view>

namespace
{
    bool hasMessage(const golarion::CharacterSheetView &view, std::string_view text)
    {
        return std::ranges::any_of(view.reminders.messages, [text](const std::string &message)
        {
            return message.find(text) != std::string::npos;
        });
    }

    void addCondition(golarion::CharacterSheet &sheet, std::string_view id, std::string_view conditionId, int severity = 1, std::optional<std::string> parameter = std::nullopt)
    {
        sheet.addCondition(golarion::ConditionEntry(golarion::ConditionEntryDefinition{
            .id = std::string(id),
            .conditionId = std::string(conditionId),
            .source = "Test",
            .severity = severity,
            .parameter = std::move(parameter)
        }));
    }
}

int main()
{
    using namespace golarion;

    CharacterSheet sheet;
    assert(sheet.toView().reminders.messages.empty());

    addCondition(sheet, "test.sickened", "sickened");
    assert(sheet.toView().reminders.messages.empty());
    sheet.removeCondition("test.sickened");

    addCondition(sheet, "test.blinded", "blinded");
    CharacterSheetView view = sheet.toView();
    assert(view.reminders.messages.size() == 3);
    assert(hasMessage(view, "falliscono automaticamente"));
    assert(hasMessage(view, "Occultamento Totale"));
    assert(hasMessage(view, "Acrobazia CD 10"));
    sheet.removeCondition("test.blinded");
    assert(sheet.toView().reminders.messages.empty());

    addCondition(sheet, "test.bleeding.one", "bleeding", 1, "2d6 PF");
    addCondition(sheet, "test.bleeding.two", "bleeding", 1, "1 Forza");
    view = sheet.toView();
    assert(view.reminders.messages.size() == 2);
    assert(hasMessage(view, "2d6 PF"));
    assert(hasMessage(view, "1 Forza"));
    const auto bleeding = std::ranges::find(view.conditions.conditions, "bleeding", &ConditionView::id);
    assert(bleeding != view.conditions.conditions.end());
    assert(bleeding->stackingMode == ConditionStackingMode::PerEntry);
    assert(bleeding->stages[0].effects[0].activeInstances.size() == 2);
    sheet.removeCondition("test.bleeding.one");
    assert(sheet.toView().reminders.messages.size() == 1);
    sheet.removeCondition("test.bleeding.two");
    assert(sheet.toView().reminders.messages.empty());

    addCondition(sheet, "test.fear", "fear", 3);
    view = sheet.toView();
    assert(view.reminders.messages.size() == 2);
    assert(hasMessage(view, "Spaventato"));
    assert(hasMessage(view, "In preda al panico"));
    sheet.removeCondition("test.fear");

    addCondition(sheet, "test.negativeLevel", "negativeLevels");
    view = sheet.toView();
    assert(view.reminders.messages.size() == 2);
    assert(hasMessage(view, "Dadi Vita"));
    sheet.removeCondition("test.negativeLevel");

    for (std::string_view conditionId : std::array{
        std::string_view("confused"),
        std::string_view("dead"),
        std::string_view("deafened"),
        std::string_view("disabled"),
        std::string_view("dying"),
        std::string_view("entangled"),
        std::string_view("fascinated"),
        std::string_view("fatigue"),
        std::string_view("flatFooted"),
        std::string_view("grappled"),
        std::string_view("helpless"),
        std::string_view("incorporeal"),
        std::string_view("invisible"),
        std::string_view("nauseated"),
        std::string_view("paralyzed"),
        std::string_view("petrified"),
        std::string_view("pinned"),
        std::string_view("prone"),
        std::string_view("stable"),
        std::string_view("staggered"),
        std::string_view("stunned"),
        std::string_view("unconscious")
    })
    {
        const std::string entryId = "test." + std::string(conditionId);
        addCondition(sheet, entryId, conditionId);
        assert(!sheet.toView().reminders.messages.empty());
        sheet.removeCondition(entryId);
        assert(sheet.toView().reminders.messages.empty());
    }

    return 0;
}
