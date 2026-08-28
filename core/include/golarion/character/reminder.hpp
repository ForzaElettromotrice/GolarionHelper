#pragma once

#include "golarion/view/reminders_view.hpp"

#include <map>
#include <string>
#include <string_view>

namespace golarion
{
    inline constexpr std::string_view ReminderEntriesResource = "reminder.entries";

    class ResourceManager;

    struct ReminderDefinition
    {
        std::string id;
        std::string message;
    };

    class Reminder final
    {
    public:
        explicit Reminder(ReminderDefinition definition);

    private:
        friend class ReminderManager;

        std::string id_;
        std::string message_;
    };

    class ReminderManager final
    {
    public:
        explicit ReminderManager(ResourceManager &resourceManager);

        RemindersView toView() const;

    private:
        void addReminder(Reminder reminder);
        void removeReminder(std::string_view reminderId);

        std::map<std::string, Reminder> reminders_;
    };
}
