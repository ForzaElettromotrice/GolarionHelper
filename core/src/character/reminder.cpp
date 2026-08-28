#include "golarion/character/reminder.hpp"

#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace golarion
{
    Reminder::Reminder(ReminderDefinition definition)
        : id_(normalize(definition.id)),
          message_(normalize(definition.message))
    {
    }

    ReminderManager::ReminderManager(ResourceManager &resourceManager)
    {
        resourceManager.registerCollectionResource<Reminder>(ReminderEntriesResource, [this](Reminder reminder)
        {
            addReminder(std::move(reminder));
        }, [this](std::string_view reminderId)
        {
            removeReminder(reminderId);
        });
    }

    RemindersView ReminderManager::toView() const
    {
        std::vector<std::string> messages;
        messages.reserve(reminders_.size());
        for (const auto &[id, reminder] : reminders_)
        {
            static_cast<void>(id);
            messages.push_back(reminder.message_);
        }
        return RemindersView{.messages = std::move(messages)};
    }

    void ReminderManager::addReminder(Reminder reminder)
    {
        const std::string id = reminder.id_;
        if (!reminders_.emplace(id, std::move(reminder)).second)
        {
            throw std::invalid_argument("reminder is already registered: " + id);
        }
    }

    void ReminderManager::removeReminder(std::string_view reminderId)
    {
        const std::string id = normalize(reminderId);
        if (reminders_.erase(id) == 0)
        {
            throw std::invalid_argument("reminder is not registered: " + id);
        }
    }
}
