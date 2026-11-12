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
        const auto restore = [&]
        {
            for (const auto &id : temporaryFeats) sheet.removeFeat(id);
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
                sheet.setRacialChoice("human.abilityScores", "ability", {"charisma"});
            }
            if (temporaryFavoredClass) sheet.setFavoredClass(type);
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
