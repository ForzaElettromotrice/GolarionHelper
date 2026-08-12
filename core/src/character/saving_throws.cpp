#include "golarion/character/saving_throws.hpp"

#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <stdexcept>
#include <string>
#include <utility>

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
        resourceManager_.registerCollectionResource<SavingThrowAbilityReplacement>(SavingThrowAbilityReplacementsResource, [this](SavingThrowAbilityReplacement replacement)
        {
            addAbilityReplacement(std::move(replacement));
        }, [this](std::string_view replacementId)
        {
            removeAbilityReplacement(replacementId);
        });
    }

    SavingThrowsView SavingThrows::toView()
    {
        std::vector<SavingThrowView> views;
        views.reserve(savingThrows_.size());
        for (const SavingThrow &entry : savingThrows_)
        {
            views.push_back(entry.toView(resourceManager_, abilityReplacements_));
        }
        return SavingThrowsView{.savingThrows = std::move(views)};
    }

    void SavingThrows::addAbilityReplacement(SavingThrowAbilityReplacement replacement)
    {
        const std::string id = replacement.id_;
        if (!abilityReplacements_.emplace(id, std::move(replacement)).second)
        {
            throw std::invalid_argument("saving throw ability replacement is already registered: " + id);
        }
    }

    void SavingThrows::removeAbilityReplacement(std::string_view replacementId)
    {
        const std::string id = normalize(replacementId);
        if (abilityReplacements_.erase(id) == 0)
        {
            throw std::invalid_argument("saving throw ability replacement is not registered: " + id);
        }
    }
}
