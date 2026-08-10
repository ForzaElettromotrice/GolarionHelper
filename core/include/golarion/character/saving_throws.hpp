#pragma once

#include "golarion/character/saving_throw.hpp"
#include "golarion/data/saving_throws_save_data.hpp"
#include "golarion/view/saving_throws_view.hpp"

#include <array>

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

        void setBaseValue(SavingThrowType type, int baseValue);
        void setAbilityType(SavingThrowType type, AbilityType abilityType);
        SavingThrowsView toView();
        SavingThrowsSaveData toSaveData() const;

    private:
        friend class CharacterSheet;

        static constexpr std::size_t SavingThrowCount = 3;

        SavingThrow &savingThrow(SavingThrowType type);
        void load(const SavingThrowsSaveData &data);

        ResourceManager &resourceManager_;
        std::array<SavingThrow, SavingThrowCount> savingThrows_;
    };
}
