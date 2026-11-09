#pragma once

#include <string>
#include <vector>

namespace golarion
{
    enum class LanguageOrigin { Intelligence, Linguistics };

    struct LearnedLanguageData
    {
        std::string languageId;
        LanguageOrigin origin;
    };

    struct LanguagesData
    {
        // Initial entitlement, not the current Intelligence modifier.
        int intelligenceBonus = 0;
        std::vector<LearnedLanguageData> learned;
    };
}
