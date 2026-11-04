#include "golarion/character/armor_class.hpp"
#include "golarion/character/carrying_capacity.hpp"
#include "golarion/character/combat_maneuvers.hpp"
#include "golarion/character/movement.hpp"
#include "golarion/character/reminder.hpp"
#include "golarion/character/size.hpp"
#include "golarion/character/skills.hpp"
#include "golarion/character/strike.hpp"
#include "golarion/effect/effect_compiler.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/carrying_capacity_view.hpp"
#include "golarion/view/movement_view.hpp"
#include "golarion/view/size_view.hpp"

#include <cassert>
#include <stdexcept>
#include <string>
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
}

int main()
{
    using namespace golarion;

    ResourceManager resourceManager;
    resourceManager.registerEnhanceableResource("effect.test");
    resourceManager.registerAccumulatedResource("effect.total");
    resourceManager.registerTarget("str", []
    {
        return 10;
    });

    Strikes strikes(resourceManager);
    CombatManeuvers combatManeuvers(resourceManager);
    ArmorClass armorClass(resourceManager);
    Skills skills(resourceManager);
    ReminderManager reminderManager(resourceManager);
    Movement movement(resourceManager);
    CarryingCapacity carryingCapacity(resourceManager);
    SizeManager sizeManager(resourceManager);

    static_cast<void>(strikes);
    static_cast<void>(combatManeuvers);
    static_cast<void>(armorClass);
    static_cast<void>(skills);

    EffectCleanup modifierCleanup = compileEffect(ModifierEffectDefinition{
        .id = "bonus",
        .resource = "effect.test",
        .description = "Bonus di prova",
        .type = ModifierType::Bonus,
        .bonusType = BonusType::Generic,
        .expression = "2"
    }, EffectContext{
        .instanceId = "effect.test.modifier",
        .source = "Fonte di prova"
    })(resourceManager);
    const ModifierSetView modifierView = resourceManager.modifierSetView("effect.test");
    assert(modifierView.total == 2);
    assert(modifierView.modifiers.size() == 1);
    assert(modifierView.modifiers[0].source == "Fonte di prova");
    modifierCleanup();
    assert(resourceManager.modifierTotal("effect.test") == 0);

    EffectCleanup contributionCleanup = compileEffect(ContributionEffectDefinition{
        .id = "contribution",
        .resource = "effect.total",
        .expression = "3"
    }, EffectContext{
        .instanceId = "effect.test.contribution",
        .source = "Fonte di prova"
    })(resourceManager);
    assert(resourceManager.contributionTotal("effect.total") == 3);
    contributionCleanup();
    assert(resourceManager.contributionTotal("effect.total") == 0);

    EffectCleanup sizeCleanup = compileEffect(SizeBaseEffectDefinition{
        .id = "size",
        .category = SizeCategory::Small
    }, EffectContext{
        .instanceId = "effect.test.size",
        .source = "Fonte della taglia"
    })(resourceManager);
    assert(sizeManager.toView().base.has_value());
    assert(sizeManager.toView().base->id == "effect.test.size");
    assert(sizeManager.toView().base->source == "Fonte della taglia");
    assert(sizeManager.toView().effectiveCategory == SizeCategory::Small);
    sizeCleanup();
    assert(!sizeManager.toView().base.has_value());
    assert(sizeManager.toView().effectiveCategory == SizeCategory::Medium);

    EffectCleanup movementCleanup = compileEffect(MovementGrantEffectDefinition{
        .id = "speed",
        .type = MovementType::Land,
        .baseSpeedExpression = "6",
        .affectedByArmor = true,
        .affectedByLoad = true,
        .supportsRunning = true
    }, EffectContext{
        .instanceId = "effect.test.speed",
        .source = "Fonte della velocità"
    })(resourceManager);
    assert(movement.toView().grants.size() == 1);
    assert(movement.toView().grants[0].id == "effect.test.speed");
    assert(movement.toView().grants[0].source == "Fonte della velocità");
    assert(movement.toView().grants[0].effectiveUnits == 6);
    movementCleanup();
    assert(movement.toView().grants.empty());

    EffectCleanup bodyTypeCleanup = compileEffect(CarryingBodyTypeEffectDefinition{
        .id = "bodyType",
        .type = CarryingBodyType::Quadruped
    }, EffectContext{
        .instanceId = "effect.test.bodyType",
        .source = "Fonte dell'anatomia"
    })(resourceManager);
    assert(carryingCapacity.toView().bodyType == CarryingBodyType::Quadruped);
    assert(carryingCapacity.toView().bodyTypeBase.has_value());
    assert(carryingCapacity.toView().bodyTypeBase->id == "effect.test.bodyType");
    bodyTypeCleanup();
    assert(carryingCapacity.toView().bodyType == CarryingBodyType::Biped);
    assert(!carryingCapacity.toView().bodyTypeBase.has_value());

    EffectCleanup reminderCleanup = compileEffect(ReminderEffectDefinition{
        .id = "reminder",
        .message = "Ricorda questa regola."
    }, EffectContext{
        .instanceId = "effect.test.reminder",
        .source = "Fonte del promemoria"
    })(resourceManager);
    assert(reminderManager.toView().messages == std::vector<std::string>{"Ricorda questa regola."});
    reminderCleanup();
    assert(reminderManager.toView().messages.empty());

    assert(throwsInvalidArgument([]
    {
        static_cast<void>(compileEffect(ReminderEffectDefinition{
            .id = " ",
            .message = "Promemoria"
        }, EffectContext{
            .instanceId = "effect.test.invalidEffect",
            .source = "Fonte di prova"
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(compileEffect(ReminderEffectDefinition{
            .id = "reminder",
            .message = "Promemoria"
        }, EffectContext{
            .instanceId = " ",
            .source = "Fonte di prova"
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(compileEffect(ReminderEffectDefinition{
            .id = "reminder",
            .message = "Promemoria"
        }, EffectContext{
            .instanceId = "effect.test.invalidSource",
            .source = " "
        }));
    }));

    return 0;
}
