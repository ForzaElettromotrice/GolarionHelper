#pragma once

#include <array>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace golarion
{
    enum class FeatType
    {
        General, Combat, Critical, ItemCreation, Metamagic, Teamwork, Style,
        Achievement, Channeling, Grit, Panache, Performance, Racial, Story,
        Mythic, Monster, Stare, HeroPoint, Alignment
    };

    inline FeatType featTypeFromName(std::string_view name)
    {
        static constexpr std::array names{
            std::pair{"general", FeatType::General}, std::pair{"combat", FeatType::Combat},
            std::pair{"critical", FeatType::Critical}, std::pair{"itemCreation", FeatType::ItemCreation},
            std::pair{"metamagic", FeatType::Metamagic}, std::pair{"teamwork", FeatType::Teamwork},
            std::pair{"style", FeatType::Style}, std::pair{"achievement", FeatType::Achievement},
            std::pair{"channeling", FeatType::Channeling}, std::pair{"grit", FeatType::Grit},
            std::pair{"panache", FeatType::Panache}, std::pair{"performance", FeatType::Performance},
            std::pair{"racial", FeatType::Racial}, std::pair{"story", FeatType::Story},
            std::pair{"mythic", FeatType::Mythic}, std::pair{"monster", FeatType::Monster},
            std::pair{"stare", FeatType::Stare}, std::pair{"heroPoint", FeatType::HeroPoint},
            std::pair{"alignment", FeatType::Alignment}
        };
        for (const auto &[key, type] : names) if (name == key) return type;
        throw std::invalid_argument("unknown feat type: " + std::string(name));
    }
}
