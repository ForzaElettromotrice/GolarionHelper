#include "golarion/character/armor_class.hpp"
#include "golarion/character/carrying_capacity.hpp"
#include "golarion/character/combat_maneuvers.hpp"
#include "golarion/character/size.hpp"
#include "golarion/character/skill.hpp"
#include "golarion/character/skills.hpp"
#include "golarion/character/strike.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/size_view.hpp"
#include "golarion/view/carrying_capacity_view.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
    struct ExpectedSizeModifiers
    {
        golarion::SizeCategory category;
        int attack;
        int combatManeuver;
        int stealth;
        int fly;
    };

    constexpr std::array ExpectedModifiers{
        ExpectedSizeModifiers{.category = golarion::SizeCategory::Fine, .attack = 8, .combatManeuver = -8, .stealth = 16, .fly = 8},
        ExpectedSizeModifiers{.category = golarion::SizeCategory::Diminutive, .attack = 4, .combatManeuver = -4, .stealth = 12, .fly = 6},
        ExpectedSizeModifiers{.category = golarion::SizeCategory::Tiny, .attack = 2, .combatManeuver = -2, .stealth = 8, .fly = 4},
        ExpectedSizeModifiers{.category = golarion::SizeCategory::Small, .attack = 1, .combatManeuver = -1, .stealth = 4, .fly = 2},
        ExpectedSizeModifiers{.category = golarion::SizeCategory::Medium, .attack = 0, .combatManeuver = 0, .stealth = 0, .fly = 0},
        ExpectedSizeModifiers{.category = golarion::SizeCategory::Large, .attack = -1, .combatManeuver = 1, .stealth = -4, .fly = -2},
        ExpectedSizeModifiers{.category = golarion::SizeCategory::Huge, .attack = -2, .combatManeuver = 2, .stealth = -8, .fly = -4},
        ExpectedSizeModifiers{.category = golarion::SizeCategory::Gargantuan, .attack = -4, .combatManeuver = 4, .stealth = -12, .fly = -6},
        ExpectedSizeModifiers{.category = golarion::SizeCategory::Colossal, .attack = -8, .combatManeuver = 8, .stealth = -16, .fly = -8}
    };

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
}

int main()
{
    using namespace golarion;

    ResourceManager resourceManager;
    Strikes strikes(resourceManager);
    CombatManeuvers combatManeuvers(resourceManager);
    ArmorClass armorClass(resourceManager);
    Skills skills(resourceManager);
    resourceManager.registerTarget("str", []
    {
        return 10;
    });
    CarryingCapacity carryingCapacity(resourceManager);
    SizeManager sizeManager(resourceManager);

    static_cast<void>(strikes);
    static_cast<void>(combatManeuvers);
    static_cast<void>(armorClass);
    static_cast<void>(skills);

    SizeView view = sizeManager.toView();
    assert(view.baseCategory == SizeCategory::Medium);
    assert(!view.replacementCategory.has_value());
    assert(view.effectiveCategory == SizeCategory::Medium);
    assert(view.attackModifier == 0);
    assert(view.combatManeuverModifier == 0);
    assert(resourceManager.modifierTotal(attackResourceName(AttackMode::Melee)) == 0);
    assert(resourceManager.modifierTotal(ArmorClassAllResource) == 0);
    assert(resourceManager.modifierTotal(CombatManeuverBonusAllResource) == 0);
    assert(resourceManager.modifierTotal(CombatManeuverDefenseAllResource) == 0);
    const ResourceManagerView resourceView = resourceManager.toView();
    assert(resourceView.targets.size() == 2);
    assert(std::ranges::any_of(resourceView.targets, [](const ResourceManagerView::TargetView &target)
    {
        return target.name == ArmorCheckPenaltyTarget;
    }));
    assert(std::ranges::any_of(resourceView.targets, [](const ResourceManagerView::TargetView &target)
    {
        return target.name == "str";
    }));
    assert(carryingCapacity.toView().size == SizeCategory::Medium);

    resourceManager.addToCollection(SizeBaseResource, SizeBase(SizeBaseDefinition{
        .id = "ancestry.halfling",
        .source = "Halfling",
        .category = SizeCategory::Small
    }));
    view = sizeManager.toView();
    assert(view.baseCategory == SizeCategory::Small);
    assert(view.effectiveCategory == SizeCategory::Small);
    assert(view.armorClassModifier == 1);
    assert(view.attackModifier == 1);
    assert(view.combatManeuverModifier == -1);
    assert(view.stealthModifier == 4);
    assert(view.flyModifier == 2);
    assert(resourceManager.modifierTotal(attackResourceName(AttackMode::Melee)) == 1);
    assert(resourceManager.modifierTotal(attackResourceName(AttackMode::Ranged)) == 1);
    assert(resourceManager.modifierTotal(ArmorClassAllResource) == 1);
    assert(resourceManager.modifierTotal(CombatManeuverBonusAllResource) == -1);
    assert(resourceManager.modifierTotal(CombatManeuverDefenseAllResource) == -1);
    assert(resourceManager.modifierTotal(resourceName(SkillType::Stealth)) == 4);
    assert(resourceManager.modifierTotal(resourceName(SkillType::Fly)) == 2);
    assert(carryingCapacity.toView().size == SizeCategory::Small);
    assert(carryingCapacity.toView().heavyLoadMaxGrams == 37500);

    resourceManager.addToCollection(SizeReplacementsResource, SizeReplacement(SizeReplacementDefinition{
        .id = "spell.form",
        .source = "Forma",
        .category = SizeCategory::Large,
        .acceptsAdjustments = true
    }));
    resourceManager.addToCollection(SizeAdjustmentsResource, SizeAdjustment(SizeAdjustmentDefinition{
        .id = "spell.growth",
        .source = "Crescita",
        .steps = 1
    }));
    view = sizeManager.toView();
    assert(view.replacementCategory == SizeCategory::Large);
    assert(view.selectedAdjustmentSteps == 1);
    assert(view.appliedAdjustmentSteps == 1);
    assert(view.effectiveCategory == SizeCategory::Huge);
    assert(view.attackModifier == -2);
    assert(resourceManager.modifierTotal(attackResourceName(AttackMode::Melee)) == -2);
    assert(resourceManager.modifierTotal(ArmorClassAllResource) == -2);
    assert(resourceManager.modifierTotal(CombatManeuverBonusAllResource) == 2);
    assert(resourceManager.modifierTotal(CombatManeuverDefenseAllResource) == 2);
    assert(carryingCapacity.toView().size == SizeCategory::Huge);
    assert(carryingCapacity.toView().heavyLoadMaxGrams == 200000);

    resourceManager.removeFromCollection(SizeReplacementsResource, "spell.form");
    view = sizeManager.toView();
    assert(!view.replacementCategory.has_value());
    assert(view.baseCategory == SizeCategory::Small);
    assert(view.effectiveCategory == SizeCategory::Medium);

    resourceManager.addToCollection(SizeAdjustmentsResource, SizeAdjustment(SizeAdjustmentDefinition{
        .id = "spell.greaterGrowth",
        .source = "Crescita superiore",
        .steps = 2
    }));
    view = sizeManager.toView();
    assert(view.effectiveCategory == SizeCategory::Large);
    assert(view.selectedAdjustmentSteps == 2);
    assert(view.adjustments.size() == 2);
    const auto weakerAdjustment = std::ranges::find(view.adjustments, "spell.growth", &SizeAdjustmentView::id);
    const auto strongerAdjustment = std::ranges::find(view.adjustments, "spell.greaterGrowth", &SizeAdjustmentView::id);
    assert(weakerAdjustment != view.adjustments.end());
    assert(strongerAdjustment != view.adjustments.end());
    assert(!weakerAdjustment->applied);
    assert(weakerAdjustment->notAppliedReason == "Superato da un aggiustamento di taglia più forte");
    assert(strongerAdjustment->applied);

    assert(throwsInvalidArgument([&]
    {
        resourceManager.addToCollection(SizeAdjustmentsResource, SizeAdjustment(SizeAdjustmentDefinition{
            .id = "spell.reduction",
            .source = "Riduzione",
            .steps = -1
        }));
    }));
    assert(sizeManager.toView().adjustments.size() == 2);

    resourceManager.addToCollection(SizeReplacementsResource, SizeReplacement(SizeReplacementDefinition{
        .id = "spell.polymorph",
        .source = "Metamorfosi",
        .category = SizeCategory::Huge,
        .acceptsAdjustments = false
    }));
    view = sizeManager.toView();
    assert(view.effectiveCategory == SizeCategory::Huge);
    assert(!view.acceptsAdjustments);
    assert(view.appliedAdjustmentSteps == 0);
    assert(!view.adjustments[0].applied);
    assert(view.adjustments[0].notAppliedReason == "La sostituzione di taglia attiva non accetta aggiustamenti");

    assert(throwsInvalidArgument([&]
    {
        resourceManager.addToCollection(SizeReplacementsResource, SizeReplacement(SizeReplacementDefinition{
            .id = "spell.otherForm",
            .source = "Altra forma",
            .category = SizeCategory::Large,
            .acceptsAdjustments = true
        }));
    }));
    assert(sizeManager.toView().replacements.size() == 1);

    assert(throwsInvalidArgument([&]
    {
        resourceManager.addToCollection(SizeBaseResource, SizeBase(SizeBaseDefinition{
            .id = "ancestry.other",
            .source = "Altra razza",
            .category = SizeCategory::Small
        }));
    }));
    assert(sizeManager.toView().base.has_value());
    assert(sizeManager.toView().base->id == "ancestry.halfling");

    resourceManager.removeFromCollection(SizeReplacementsResource, "spell.polymorph");
    resourceManager.removeFromCollection(SizeAdjustmentsResource, "spell.growth");
    resourceManager.removeFromCollection(SizeAdjustmentsResource, "spell.greaterGrowth");
    resourceManager.removeFromCollection(SizeBaseResource, "ancestry.halfling");
    for (std::size_t index = 0; index < ExpectedModifiers.size(); ++index)
    {
        const ExpectedSizeModifiers &expected = ExpectedModifiers[index];
        const std::string id = "test.base." + std::to_string(index);
        resourceManager.addToCollection(SizeBaseResource, SizeBase(SizeBaseDefinition{
            .id = id,
            .source = "Test",
            .category = expected.category
        }));
        view = sizeManager.toView();
        assert(view.effectiveCategory == expected.category);
        assert(view.attackModifier == expected.attack);
        assert(view.armorClassModifier == expected.attack);
        assert(view.combatManeuverModifier == expected.combatManeuver);
        assert(view.stealthModifier == expected.stealth);
        assert(view.flyModifier == expected.fly);
        assert(resourceManager.modifierTotal(attackResourceName(AttackMode::Melee)) == expected.attack);
        assert(resourceManager.modifierTotal(attackResourceName(AttackMode::Ranged)) == expected.attack);
        assert(resourceManager.modifierTotal(ArmorClassAllResource) == expected.attack);
        assert(resourceManager.modifierTotal(CombatManeuverBonusAllResource) == expected.combatManeuver);
        assert(resourceManager.modifierTotal(CombatManeuverDefenseAllResource) == expected.combatManeuver);
        assert(resourceManager.modifierTotal(resourceName(SkillType::Stealth)) == expected.stealth);
        assert(resourceManager.modifierTotal(resourceName(SkillType::Fly)) == expected.fly);
        resourceManager.removeFromCollection(SizeBaseResource, id);
    }

    assert(displayName(SizeCategory::Fine) == std::string_view("Piccolissima"));
    assert(displayName(SizeCategory::Colossal) == std::string_view("Colossale"));
}
