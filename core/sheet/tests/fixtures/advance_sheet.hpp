#pragma once

#include "golarion/character/character_sheet.hpp"
#include <algorithm>
#include <stdexcept>

namespace golarion::test
{
    // Legacy subsystem tests intentionally inspect incomplete sheets. Fulfill the
    // public advancement contract, then undo only the temporary fixture selections.
    // The production gate is exercised separately without this helper.
    inline void advanceSheetFixture(CharacterSheet &sheet, CharacterClassType type, std::optional<int> hitDieResult = std::nullopt)
    {
        const auto before = sheet.toSaveData();
        const bool temporaryRace = !before.race.raceDefinitionId;
        const bool temporaryFavoredClass = before.classes.favoredClasses.selectedClasses.empty();
        std::vector<std::string> temporaryFeats;
        std::vector<std::pair<CharacterClassType, ClassOptionDefinition>> temporaryClassOptions;
        const auto restore = [&]
        {
            for (const auto &id : temporaryFeats) sheet.removeFeat(id);
            for (auto it = temporaryClassOptions.rbegin(); it != temporaryClassOptions.rend(); ++it)
                sheet.unselectClassOption(it->first, it->second);
            if (temporaryRace) sheet.clearRace();
            if (temporaryFavoredClass) sheet.setFavoredClasses({});
            sheet.setAlignment(before.identity.alignment);
        };
        try
        {
            if (!before.identity.alignment) sheet.setAlignment(Alignment::Neutral);
            if (temporaryRace)
            {
                sheet.setRace("human");
                sheet.selectAlternateRacialFeature("human.dualTalent");
                sheet.setRacialChoice("human.dualTalent", "abilities", {"wisdom", "charisma"});
            }
            if (temporaryFavoredClass) sheet.setFavoredClass(type);
            for (;;)
            {
                bool selected = false;
                const auto view = sheet.toView();
                for (const auto &characterClass : view.classes.classes)
                {
                    for (const auto &feature : characterClass.features)
                    {
                        for (const auto &acquisition : feature.acquisitions)
                        {
                            if (!acquisition.active) continue;
                            for (const auto &choice : acquisition.choices)
                            {
                                if (!choice.blocksAdvancement || choice.remainingSelections == 0) continue;
                                const auto option = std::find_if(choice.options.begin(), choice.options.end(), [](const auto &entry) { return !entry.selected; });
                                if (option == choice.options.end()) throw std::logic_error("fixture class choice has no available option");
                                ClassOptionDefinition selection{acquisition.id, choice.id, option->id};
                                sheet.selectClassOption(characterClass.type, selection);
                                temporaryClassOptions.emplace_back(characterClass.type, std::move(selection));
                                selected = true;
                                break;
                            }
                            if (selected) break;
                        }
                        if (selected) break;
                    }
                    if (selected) break;
                }
                if (!selected) break;
            }
            for (const auto &slot : sheet.toView().feats.slots)
            {
                if (!slot.available || slot.featId || slot.origin == FeatSlotOrigin::Extra) continue;
                if (!slot.allowedFeatIds.empty()) throw std::logic_error("fixture must explicitly fill restricted feat slots");
                const auto featType = slot.allowedType.value_or(FeatType::General);
                const auto featId = "test.fixture.feat." + std::to_string(static_cast<int>(featType));
                const auto definitions = sheet.toSaveData().customFeatDefinitions;
                if (std::none_of(definitions.begin(), definitions.end(), [&](const auto &definition) { return definition.id == featId; }))
                    sheet.addCustomFeatDefinition({.id = featId, .name = "Talento di prova", .description = "Nessun effetto.",
                        .types = {featType}, .repeatability = FeatRepeatability::Unlimited});
                sheet.selectFeat(slot.id, featId);
                temporaryFeats.push_back(slot.id);
            }
            sheet.addClassLevel(type, hitDieResult);
        }
        catch (...)
        {
            restore();
            throw;
        }
        restore();
    }
}
