#include "golarion/character/ability.hpp"
#include "golarion/character/character_sheet.hpp"
#include "golarion/view/condition_view.hpp"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <string_view>

namespace
{
    bool conditionIsActive(const golarion::CharacterSheetView &view, std::string_view conditionId)
    {
        const auto condition = std::ranges::find_if(view.conditions.conditions, [conditionId](const golarion::ConditionView &candidate)
        {
            return candidate.id == conditionId;
        });
        assert(condition != view.conditions.conditions.end());
        return condition->effectiveSeverity > 0;
    }
}

int main()
{
    using namespace golarion;

    CharacterSheet sheet;
    assert(!conditionIsActive(sheet.toView(), "disabled"));

    sheet.setAbilityBaseValue(AbilityType::Constitution, 30);
    assert(sheet.toView().hitPoints.current == 10);
    assert(!conditionIsActive(sheet.toView(), "disabled"));

    sheet.damage(10, DamageLethality::NonLethal);
    assert(conditionIsActive(sheet.toView(), "staggered"));
    assert(!conditionIsActive(sheet.toView(), "unconscious"));

    sheet.damage(1, DamageLethality::NonLethal);
    assert(!conditionIsActive(sheet.toView(), "staggered"));
    assert(conditionIsActive(sheet.toView(), "unconscious"));

    sheet.heal(10);
    assert(!conditionIsActive(sheet.toView(), "staggered"));
    assert(!conditionIsActive(sheet.toView(), "unconscious"));

    sheet.damage(10, DamageLethality::Lethal);
    assert(sheet.toView().hitPoints.current == 0);
    assert(conditionIsActive(sheet.toView(), "disabled"));

    sheet.damage(9, DamageLethality::Lethal);
    assert(sheet.toView().hitPoints.current == -9);
    assert(conditionIsActive(sheet.toView(), "disabled"));
    assert(!conditionIsActive(sheet.toView(), "dying"));
    assert(!conditionIsActive(sheet.toView(), "stable"));
    assert(!conditionIsActive(sheet.toView(), "unconscious"));

    sheet.damage(21, DamageLethality::Lethal);
    assert(sheet.toView().hitPoints.current == -30);
    assert(conditionIsActive(sheet.toView(), "dead"));
    assert(!conditionIsActive(sheet.toView(), "disabled"));
    assert(sheet.toSaveData().hitPoints.dead);

    sheet.heal(100);
    assert(sheet.toView().hitPoints.current == -30);
    assert(conditionIsActive(sheet.toView(), "dead"));

    sheet.setAbilityBaseValue(AbilityType::Constitution, 40);
    assert(sheet.toView().hitPoints.current == -25);
    assert(conditionIsActive(sheet.toView(), "dead"));

    const std::filesystem::path savePath = "hit_point_conditions_test_save.json";
    sheet.save(savePath);
    CharacterSheet loaded = CharacterSheet::load(savePath);
    assert(loaded.toView().hitPoints.current == -25);
    assert(conditionIsActive(loaded.toView(), "dead"));
    loaded.heal(100);
    assert(loaded.toView().hitPoints.current == -25);
    std::filesystem::remove(savePath);

    return 0;
}
