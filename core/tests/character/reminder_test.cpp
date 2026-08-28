#include "golarion/character/reminder.hpp"
#include "golarion/resource/resource_manager.hpp"

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
}

int main()
{
    using namespace golarion;

    ResourceManager resourceManager;
    ReminderManager reminders(resourceManager);

    assert(reminders.toView().messages.empty());
    const ResourceManagerView resourceView = resourceManager.toView();
    assert(std::ranges::find(resourceView.collections, ReminderEntriesResource) != resourceView.collections.end());

    resourceManager.addToCollection(ReminderEntriesResource, Reminder(ReminderDefinition{
        .id = "blinded.movement",
        .message = "Muoversi oltre metà velocità richiede una prova di Acrobazia."
    }));
    resourceManager.addToCollection(ReminderEntriesResource, Reminder(ReminderDefinition{
        .id = "bleeding.damage",
        .message = "Applica il danno da sanguinamento all'inizio del turno."
    }));

    const RemindersView view = reminders.toView();
    assert(view.messages.size() == 2);
    assert(view.messages[0] == "Applica il danno da sanguinamento all'inizio del turno.");
    assert(view.messages[1] == "Muoversi oltre metà velocità richiede una prova di Acrobazia.");

    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.addToCollection(ReminderEntriesResource, Reminder(ReminderDefinition{
            .id = "blinded.movement",
            .message = "Duplicato"
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        Reminder invalid(ReminderDefinition{.id = " ", .message = "Messaggio"});
    }));
    assert(throwsInvalidArgument([]
    {
        Reminder invalid(ReminderDefinition{.id = "invalid", .message = " "});
    }));

    resourceManager.removeFromCollection(ReminderEntriesResource, "blinded.movement");
    assert(reminders.toView().messages == std::vector<std::string>{"Applica il danno da sanguinamento all'inizio del turno."});
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.removeFromCollection(ReminderEntriesResource, "blinded.movement");
    }));

    return 0;
}
