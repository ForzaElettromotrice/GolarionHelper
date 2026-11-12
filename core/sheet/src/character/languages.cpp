#include "golarion/character/languages.hpp"
#include "golarion/character/ability.hpp"

#include "golarion/character/reminder.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace golarion
{
    Languages::Languages(ResourceManager &resources, std::function<int()> linguisticsRanks, std::function<bool(std::string_view, LanguageOrigin)> isAllowed)
        : resources_(resources), linguisticsRanks_(std::move(linguisticsRanks)), isAllowed_(std::move(isAllowed))
    {
        if (!linguisticsRanks_) throw std::invalid_argument("linguistics rank supplier must not be empty");
        resources_.registerCollectionResource<LanguageGrant>(SpokenLanguagesResource,
            [this](LanguageGrant entry) { grant(std::move(entry)); },
            [this](std::string_view id) { revoke(id); });
        try
        {
            resources_.registerCollectionResource<LanguageOptionsGrant>(LanguageOptionsResource,
                [this](LanguageOptionsGrant entry) { grantOptions(std::move(entry)); },
                [this](std::string_view id) { revokeOptions(id); });
        }
        catch (...) { resources_.unregisterCollectionResource(SpokenLanguagesResource); throw; }
    }

    Languages::~Languages()
    {
        for (const auto &id : reminderIds_) resources_.removeFromCollection(ReminderEntriesResource, id);
        resources_.unregisterCollectionResource(SpokenLanguagesResource);
        resources_.unregisterCollectionResource(LanguageOptionsResource);
    }

    int Languages::capacity(LanguageOrigin origin) const
    {
        if (origin == LanguageOrigin::Intelligence) return std::max(0, resources_.targetValue(PermanentIntelligenceModifierTarget));
        if (origin != LanguageOrigin::Linguistics) throw std::invalid_argument("unknown language origin");
        const int ranks = linguisticsRanks_();
        if (ranks < 0) throw std::invalid_argument("linguistics ranks must not be negative");
        return ranks;
    }

    std::size_t Languages::used(LanguageOrigin origin) const
    {
        return std::count_if(learned_.begin(), learned_.end(), [origin](const auto &entry) { return entry.second == origin; });
    }

    bool Languages::allowed(std::string_view id, LanguageOrigin origin) const
    {
        const bool listed = std::any_of(options_.begin(), options_.end(), [&](const auto &entry)
        {
            return entry.second.origin == origin && entry.second.languageIds.contains(std::string(id));
        });
        return listed && (!isAllowed_ || isAllowed_(id, origin));
    }

    void Languages::grantOptions(LanguageOptionsGrant entry)
    {
        entry.id = normalize(entry.id);
        entry.source = normalize(entry.source);
        if (entry.origin != LanguageOrigin::Intelligence && entry.origin != LanguageOrigin::Linguistics)
            throw std::invalid_argument("unknown language origin");
        std::set<std::string> ids;
        for (const auto &id : entry.languageIds)
            if (!ids.insert(normalize(id)).second) throw std::invalid_argument("duplicate language option");
        entry.languageIds = std::move(ids);
        const auto id = entry.id;
        if (!options_.emplace(id, std::move(entry)).second) throw std::invalid_argument("language options are already registered");
        refreshReminders();
    }

    void Languages::revokeOptions(std::string_view id)
    {
        if (!options_.erase(normalize(id))) throw std::invalid_argument("language options are not registered");
        refreshReminders();
    }

    void Languages::add(std::string_view languageId, LanguageOrigin origin)
    {
        const auto id = normalize(languageId);
        const auto available = capacity(origin);
        if (learned_.contains(id) || std::any_of(grants_.begin(), grants_.end(), [&](const auto &entry) { return entry.second.languageId == id; }))
            throw std::invalid_argument("language is already known: " + id);
        if (used(origin) >= static_cast<std::size_t>(available)) throw std::invalid_argument("no language choices remain for this origin");
        if (!allowed(id, origin)) throw std::invalid_argument("language is not allowed for this origin: " + id);
        learned_.emplace(id, origin);
        refreshReminders();
    }

    void Languages::remove(std::string_view languageId)
    {
        if (!learned_.erase(normalize(languageId))) throw std::invalid_argument("language is not learned");
        refreshReminders();
    }

    void Languages::grant(LanguageGrant entry)
    {
        entry.id = normalize(entry.id);
        entry.source = normalize(entry.source);
        entry.languageId = normalize(entry.languageId);
        const auto id = entry.id;
        if (!grants_.emplace(id, std::move(entry)).second) throw std::invalid_argument("language grant is already registered: " + id);
    }

    void Languages::revoke(std::string_view id)
    {
        if (!grants_.erase(normalize(id))) throw std::invalid_argument("language grant is not registered");
    }

    LanguagesView Languages::toView() const
    {
        std::map<std::string, LanguageView> entries;
        for (const auto &[id, origin] : learned_) entries.emplace(id, LanguageView{id, {}, origin});
        for (const auto &[grantId, grant] : grants_)
        {
            auto [entry, inserted] = entries.try_emplace(grant.languageId, LanguageView{grant.languageId, {}, std::nullopt});
            auto &sources = entry->second.grantedSources;
            if (std::find(sources.begin(), sources.end(), grant.source) == sources.end()) sources.push_back(grant.source);
        }
        const auto remaining = [&](LanguageOrigin origin)
        {
            const auto count = static_cast<std::size_t>(capacity(origin));
            return used(origin) >= count ? 0 : static_cast<int>(count - used(origin));
        };
        LanguagesView view{{}, remaining(LanguageOrigin::Intelligence), remaining(LanguageOrigin::Linguistics)};
        std::map<LanguageOrigin, std::set<std::string>> options;
        for (const auto &[key, grant] : options_) for (const auto &id : grant.languageIds)
            if (allowed(id, grant.origin)) options[grant.origin].insert(id);
        for (const auto &[origin, ids] : options) view.options.push_back({origin, {ids.begin(), ids.end()}});
        for (auto &[id, entry] : entries) view.spoken.push_back(std::move(entry));
        return view;
    }

    void Languages::refreshReminders()
    {
        std::map<std::string, std::string> messages;
        for (const auto origin : {LanguageOrigin::Intelligence, LanguageOrigin::Linguistics})
        {
            const auto total = static_cast<std::size_t>(capacity(origin));
            const auto spent = used(origin);
            const std::string key = origin == LanguageOrigin::Intelligence ? "languages.intelligence" : "languages.linguistics";
            const std::string label = origin == LanguageOrigin::Intelligence ? "Intelligenza" : "Linguistica";
            if (spent < total) messages[key] = "Lingue ancora da scegliere tramite " + label + ": " + std::to_string(total - spent) + ".";
            else if (spent > total) messages[key] = "Lingue scelte tramite " + label + " oltre la quota disponibile: " + std::to_string(spent - total) + ".";
        }
        for (const auto &[id, origin] : learned_)
            if (!allowed(id, origin)) messages["languages.invalid." + id] = "La lingua " + id + " non è più consentita per l'origine scelta.";
        for (const auto &id : reminderIds_) resources_.removeFromCollection(ReminderEntriesResource, id);
        reminderIds_.clear();
        for (const auto &[id, message] : messages)
        {
            resources_.addToCollection(ReminderEntriesResource, Reminder({id, message}));
            reminderIds_.insert(id);
        }
    }

    LanguagesData Languages::toData() const
    {
        LanguagesData data;
        for (const auto &[id, origin] : learned_) data.learned.push_back({id, origin});
        return data;
    }

    void Languages::load(const LanguagesData &data)
    {
        std::map<std::string, LanguageOrigin> learned;
        for (const auto &entry : data.learned)
        {
            if (entry.origin != LanguageOrigin::Intelligence && entry.origin != LanguageOrigin::Linguistics)
                throw std::invalid_argument("unknown language origin");
            if (!learned.emplace(normalize(entry.languageId), entry.origin).second)
                throw std::invalid_argument("duplicate learned language");
        }
        // Preserve selections after external quota/policy changes; report them instead of losing data.
        learned_ = std::move(learned);
        refreshReminders();
    }
    std::vector<PendingChoice> Languages::pendingChoices() const
    {
        std::vector<PendingChoice> result;
        for (auto origin : {LanguageOrigin::Intelligence, LanguageOrigin::Linguistics})
        {
            const auto maximum = static_cast<std::size_t>(capacity(origin));
            const auto selected = used(origin);
            if (selected < maximum)
                result.push_back({LanguageChoiceReference{origin},
                    origin == LanguageOrigin::Intelligence ? "Scegli le lingue aggiuntive da Intelligenza" : "Scegli le lingue apprese con Linguistica",
                    maximum - selected, false});
        }
        return result;
    }

}
