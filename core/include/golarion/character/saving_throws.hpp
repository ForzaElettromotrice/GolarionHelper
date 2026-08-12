#pragma once

#include "golarion/character/saving_throw.hpp"
#include "golarion/view/saving_throws_view.hpp"

#include <array>
#include <map>
#include <string>
#include <string_view>

namespace golarion
{
    class CharacterSheet;
    class ResourceManager;

    class SavingThrows final
    {
    public:
        explicit SavingThrows(ResourceManager &resourceManager);

        SavingThrows(const SavingThrows &) = delete;
        SavingThrows &operator=(const SavingThrows &) = delete;
        SavingThrows(SavingThrows &&) = delete;
        SavingThrows &operator=(SavingThrows &&) = delete;

        SavingThrowsView toView();

    private:
        friend class CharacterSheet;

        static constexpr std::size_t SavingThrowCount = 3;

        void addAbilityReplacement(SavingThrowAbilityReplacement replacement);
        void removeAbilityReplacement(std::string_view replacementId);

        ResourceManager &resourceManager_;
        std::array<SavingThrow, SavingThrowCount> savingThrows_;
        std::map<std::string, SavingThrowAbilityReplacement> abilityReplacements_;
    };
}
