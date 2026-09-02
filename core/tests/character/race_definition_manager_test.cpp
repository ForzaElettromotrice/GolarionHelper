#include "golarion/character/race_definition_json.hpp"
#include "golarion/character/race_definition_manager.hpp"

#include <nlohmann/json.hpp>

#include <cassert>
#include <stdexcept>
#include <variant>
#include <vector>

namespace
{
    template<typename Function>
    bool throwsInvalidArgument(Function function)
    {
        try
        {
            function();
            return false;
        }
        catch (const std::invalid_argument &)
        {
            return true;
        }
    }

    nlohmann::json minimalRaceJson()
    {
        using Json = nlohmann::json;
        return Json{
            {"id", "test"},
            {"name", "Razza di prova"},
            {"qualities", Json::array({Json{
                {"id", "test.quality"},
                {"name", "Qualità"},
                {"description", "Qualità di prova."}
            }})},
            {"standardFeatures", Json::array({Json{
                {"id", "test.standard"},
                {"name", "Tratto standard"},
                {"description", "Tratto standard di prova."}
            }})},
            {"alternateFeatures", Json::array()}
        };
    }
}

int main()
{
    using namespace golarion;
    using Json = nlohmann::json;

    RaceDefinitionManager &manager = RaceDefinitionManager::instance();
    assert(&manager == &RaceDefinitionManager::instance());

    const RaceDefinition &human = manager.get(" human ");
    assert(&human == &manager.get("human"));
    assert(human.id == "human");
    assert(human.name == "Umano");
    assert(human.qualities.size() == 5);
    assert(human.standardFeatures.size() == 2);
    assert(human.alternateFeatures.size() == 1);

    assert(human.qualities[0].id == "human.type");
    assert(human.qualities[0].effects.size() == 2);
    assert(std::get<CarryingBodyTypeEffectDefinition>(human.qualities[0].effects[0]).type == CarryingBodyType::Biped);
    assert(std::get<SizeBaseEffectDefinition>(human.qualities[1].effects[0]).category == SizeCategory::Medium);
    const MovementGrantEffectDefinition &speed = std::get<MovementGrantEffectDefinition>(human.qualities[2].effects[0]);
    assert(speed.type == MovementType::Land);
    assert(speed.baseSpeedExpression == "6");
    assert(speed.affectedByArmor);
    assert(speed.affectedByLoad);
    assert(speed.supportsRunning);

    const RacialChoiceDefinition &abilityChoice = human.qualities[3].choices[0];
    assert(abilityChoice.id == "ability");
    assert(abilityChoice.selectionCount == 1);
    assert(abilityChoice.options.size() == 6);
    const ModifierEffectDefinition &strengthBonus = std::get<ModifierEffectDefinition>(abilityChoice.options[0].effects[0]);
    assert(strengthBonus.resource == "str");
    assert(strengthBonus.bonusType == BonusType::Racial);
    assert(strengthBonus.expression == "2");

    const RacialElementDefinition &dualTalent = human.alternateFeatures[0];
    assert(dualTalent.id == "human.dualTalent");
    assert(dualTalent.choices[0].selectionCount == 2);
    assert((dualTalent.replaces == std::vector<std::string>{"human.abilityScores", "human.skilled", "human.bonusFeat"}));

    assert(throwsInvalidArgument([&manager]
    {
        static_cast<void>(manager.get("missing"));
    }));

    Json duplicateElement = minimalRaceJson();
    duplicateElement["standardFeatures"][0]["id"] = "test.quality";
    assert(throwsInvalidArgument([&duplicateElement]
    {
        static_cast<void>(raceDefinitionFromJson(duplicateElement));
    }));

    Json unknownReplacement = minimalRaceJson();
    unknownReplacement["alternateFeatures"].push_back(Json{
        {"id", "test.alternate"},
        {"name", "Tratto alternativo"},
        {"description", "Tratto alternativo di prova."},
        {"replaces", {"test.missing"}}
    });
    assert(throwsInvalidArgument([&unknownReplacement]
    {
        static_cast<void>(raceDefinitionFromJson(unknownReplacement));
    }));

    Json invalidChoice = minimalRaceJson();
    invalidChoice["qualities"][0]["choices"] = Json::array({Json{
        {"id", "choice"},
        {"prompt", "Scegli due opzioni"},
        {"selectionCount", 2},
        {"options", Json::array({Json{
            {"id", "only"},
            {"name", "Unica opzione"}
        }})}
    }});
    assert(throwsInvalidArgument([&invalidChoice]
    {
        static_cast<void>(raceDefinitionFromJson(invalidChoice));
    }));

    Json duplicateEffects = minimalRaceJson();
    duplicateEffects["qualities"][0]["effects"] = Json::array({
        Json{{"id", "reminder"}, {"type", "reminder"}, {"message", "Primo."}},
        Json{{"id", "reminder"}, {"type", "reminder"}, {"message", "Secondo."}}
    });
    assert(throwsInvalidArgument([&duplicateEffects]
    {
        static_cast<void>(raceDefinitionFromJson(duplicateEffects));
    }));

    return 0;
}
