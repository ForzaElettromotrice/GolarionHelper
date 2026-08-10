#include "golarion/character/saving_throws.hpp"

#include "golarion/resource/resource_manager.hpp"

#include <array>
#include <stdexcept>
#include <utility>

namespace
{
    std::size_t savingThrowIndex(golarion::SavingThrowType type)
    {
        switch (type)
        {
            case golarion::SavingThrowType::Fortitude:
                return 0;
            case golarion::SavingThrowType::Reflex:
                return 1;
            case golarion::SavingThrowType::Will:
                return 2;
        }
        throw std::invalid_argument("unknown saving throw type");
    }
}

namespace golarion
{
    SavingThrows::SavingThrows(ResourceManager &resourceManager)
        : resourceManager_(resourceManager),
          savingThrows_{SavingThrow(SavingThrowType::Fortitude), SavingThrow(SavingThrowType::Reflex), SavingThrow(SavingThrowType::Will)}
    {
        resourceManager_.registerEnhanceableResource("savingThrow.all");
        for (const SavingThrow &entry : savingThrows_)
        {
            entry.registerResources(resourceManager_, {"savingThrow.all"});
        }
    }

    void SavingThrows::setBaseValue(SavingThrowType type, int baseValue)
    {
        savingThrow(type).setBaseValue(baseValue);
    }

    void SavingThrows::setAbilityType(SavingThrowType type, AbilityType abilityType)
    {
        savingThrow(type).setAbilityType(abilityType);
    }

    SavingThrowsView SavingThrows::toView()
    {
        std::vector<SavingThrowView> views;
        views.reserve(savingThrows_.size());
        for (const SavingThrow &entry : savingThrows_)
        {
            views.push_back(entry.toView(resourceManager_));
        }
        return SavingThrowsView{.savingThrows = std::move(views)};
    }

    SavingThrowsSaveData SavingThrows::toSaveData() const
    {
        std::vector<SavingThrowSaveData> data;
        data.reserve(savingThrows_.size());
        for (const SavingThrow &entry : savingThrows_)
        {
            data.push_back(entry.toSaveData());
        }
        return SavingThrowsSaveData{.savingThrows = std::move(data)};
    }

    void SavingThrows::load(const SavingThrowsSaveData &data)
    {
        if (data.savingThrows.size() != SavingThrowCount)
        {
            throw std::invalid_argument("saving throw save must contain exactly three entries");
        }

        std::array<bool, SavingThrowCount> loaded{};
        for (const SavingThrowSaveData &entry : data.savingThrows)
        {
            const std::size_t index = savingThrowIndex(entry.type);
            if (loaded[index])
            {
                throw std::invalid_argument("saving throw is duplicated in save data");
            }
            savingThrows_[index].setBaseValue(entry.baseValue);
            savingThrows_[index].setAbilityType(entry.abilityType);
            loaded[index] = true;
        }
    }

    SavingThrow &SavingThrows::savingThrow(SavingThrowType type)
    {
        return savingThrows_[savingThrowIndex(type)];
    }
}
