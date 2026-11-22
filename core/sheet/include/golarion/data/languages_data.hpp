#pragma once

#include <string>
#include <vector>

namespace golarion
{
    enum class LanguageOrigin { Intelligence, Linguistics, Extra, CustomLinguistics };

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
