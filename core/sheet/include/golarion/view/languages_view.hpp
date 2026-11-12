#pragma once

#include "golarion/data/languages_data.hpp"

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct LanguageView
    {
        std::string id;
        std::vector<std::string> grantedSources;
        std::optional<LanguageOrigin> learnedFrom;
    };

    struct LanguageOptionsView
    {
        LanguageOrigin origin;
        std::vector<std::string> languageIds;
    };

    struct LanguagesView
    {
        std::vector<LanguageView> spoken;
        int remainingIntelligence;
        int remainingLinguistics;
        std::vector<LanguageOptionsView> options{};
    };
}
