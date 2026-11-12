#pragma once
#include "golarion/character/pending_choice.hpp"

#include "golarion/data/languages_data.hpp"
#include "golarion/view/languages_view.hpp"

#include <functional>
#include <map>
#include <set>
#include <string>
#include <string_view>

namespace golarion
{
    class ResourceManager;
    inline constexpr std::string_view SpokenLanguagesResource = "languages.spoken";
    inline constexpr std::string_view LanguageOptionsResource = "languages.options";

    struct LanguageOptionsGrant
    {
        std::string id;
        std::string source;
        LanguageOrigin origin;
        std::set<std::string> languageIds;
    };

    struct LanguageGrant
    {
        std::string id;
        std::string source;
        std::string languageId;
    };

    class Languages final
    {
    public:
        explicit Languages(ResourceManager &resources, std::function<int()> linguisticsRanks, std::function<bool(std::string_view, LanguageOrigin)> isAllowed = {});
        ~Languages();
        Languages(const Languages &) = delete;
        Languages &operator=(const Languages &) = delete;
        Languages(Languages &&) = delete;
        Languages &operator=(Languages &&) = delete;

        void add(std::string_view languageId, LanguageOrigin origin);
        void remove(std::string_view languageId);
        void refreshReminders();
        LanguagesView toView() const;
        LanguagesData toData() const;
        void load(const LanguagesData &data);

        std::vector<PendingChoice> pendingChoices() const;

    private:
        int capacity(LanguageOrigin origin) const;
        std::size_t used(LanguageOrigin origin) const;
        bool allowed(std::string_view languageId, LanguageOrigin origin) const;
        void grant(LanguageGrant entry);
        void revoke(std::string_view id);
        void grantOptions(LanguageOptionsGrant entry);
        void revokeOptions(std::string_view id);

        ResourceManager &resources_;
        std::function<int()> linguisticsRanks_;
        std::function<bool(std::string_view, LanguageOrigin)> isAllowed_;
        std::map<std::string, LanguageGrant> grants_;
        std::map<std::string, LanguageOptionsGrant> options_;
        std::map<std::string, LanguageOrigin> learned_;
        std::set<std::string> reminderIds_;
    };
}
