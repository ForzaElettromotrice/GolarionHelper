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
        std::vector<LearnedLanguageData> learned;
    };
}
