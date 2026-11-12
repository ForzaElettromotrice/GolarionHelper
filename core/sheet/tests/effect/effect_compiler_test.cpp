#include "golarion/character/armor_class.hpp"
#include "golarion/character/feats.hpp"
#include "golarion/character/carrying_capacity.hpp"
#include "golarion/character/combat_maneuvers.hpp"
#include "golarion/character/movement.hpp"
#include "golarion/character/reminder.hpp"
#include "golarion/character/size.hpp"
#include "golarion/character/skills.hpp"
#include "golarion/character/strike.hpp"
#include "golarion/character/spellcastings.hpp"
#include "golarion/character/prepared_spellcasting.hpp"
#include "golarion/character/spontaneous_spellcasting.hpp"
#include "golarion/character/flexible_spellcasting.hpp"
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
    resourceManager.registerEnhanceableResource("initiative");
    Feats feats(resourceManager);
    feats.setLevel(1);
    auto anySlot = compileEffect(FeatSlotGrantEffectDefinition{"bonus", AnyFeat{}}, {"race.bonusFeat", "Razza"})(resourceManager);
    assert(feats.toView().slots.size() == 2);
    feats.selectFeat("race.bonusFeat", "improvedInitiative");
    assert(resourceManager.modifierTotal("initiative") == 4);
    anySlot();
    assert(feats.toView().slots.size() == 2);
    assert(!feats.toView().slots[1].available && !feats.toView().feats[0].active);
    assert(resourceManager.modifierTotal("initiative") == 0);
    feats.removeFeat("race.bonusFeat");
    auto combatSlot = compileEffect(FeatSlotGrantEffectDefinition{"bonus", FeatTypeFilter{FeatType::Combat}}, {"class.combat", "Classe"})(resourceManager);
    feats.selectFeat("class.combat", "improvedInitiative");
    feats.removeFeat("class.combat");
    assert(feats.toView().slots.size() == 2);
    combatSlot();
    auto listSlot = compileEffect(FeatSlotGrantEffectDefinition{"bonus", FeatIdFilter{{"other"}}}, {"class.list", "Classe"})(resourceManager);
    assert(throwsInvalidArgument([&] { feats.selectFeat("class.list", "improvedInitiative"); }));
    listSlot();
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

    Spellcastings castings(resourceManager);
    std::vector<EffectCleanup> castingCleanups;
    int castingIndex = 0;
    for (auto mode : {SpellcastingMode::Prepared, SpellcastingMode::Spontaneous, SpellcastingMode::Flexible})
    {
        const auto id = "effect.casting." + std::to_string(castingIndex++);
        const auto apply = compileEffect(SpellcastingGrantEffectDefinition{"casting", mode, CharacterClassType::Wizard}, {id, "Classe"});
        castingCleanups.push_back(apply(resourceManager));
        auto &casting = castings.get(id);
        if (mode == SpellcastingMode::Prepared) assert(dynamic_cast<PreparedSpellcasting *>(&casting));
        if (mode == SpellcastingMode::Spontaneous) assert(dynamic_cast<SpontaneousSpellcasting *>(&casting));
        if (mode == SpellcastingMode::Flexible) assert(dynamic_cast<FlexibleSpellcasting *>(&casting));
        casting.addToRepertoire("fireball");
        assert(throwsInvalidArgument([&] { apply(resourceManager); }));
        assert(&castings.get(id) == &casting); // A duplicate grant cannot replace live state.
    }
    assert(castings.toView().castings.size() == 3);
    auto savedCastings = castings.toData();
    for (auto &cleanup : castingCleanups) cleanup();
    assert(castings.toView().castings.empty());
    assert(throwsInvalidArgument([&] { castings.get("effect.casting.0"); }));
    // Reconstruct sources through effects before restoring mutable casting state.
    castingCleanups.clear();
    castingIndex = 0;
    for (auto mode : {SpellcastingMode::Prepared, SpellcastingMode::Spontaneous, SpellcastingMode::Flexible})
        castingCleanups.push_back(compileEffect(SpellcastingGrantEffectDefinition{"casting", mode, CharacterClassType::Wizard},
            {"effect.casting." + std::to_string(castingIndex++), "Classe"})(resourceManager));
    castings.load(savedCastings);
    for (const auto &casting : castings.toData().castings) assert(casting.repertoire == std::vector<std::string>{"fireball"});
    for (auto &cleanup : castingCleanups) cleanup();
    assert(throwsInvalidArgument([&] { compileEffect(SpellcastingGrantEffectDefinition{"casting", static_cast<SpellcastingMode>(999), CharacterClassType::Wizard}, {"invalid", "Classe"}); }));
    assert(throwsInvalidArgument([&] { compileEffect(SpellcastingGrantEffectDefinition{"casting", SpellcastingMode::Prepared, static_cast<CharacterClassType>(999)}, {"invalid", "Classe"}); }));
    for (auto mode : {SpellcastingMode::Prepared, SpellcastingMode::Spontaneous, SpellcastingMode::Flexible})
    {
        const auto castingCleanup = compileEffect(SpellcastingGrantEffectDefinition{"casting", mode, CharacterClassType::Wizard}, {"caster", "Classe"})(resourceManager);
        const auto slotsApply = compileEffect(SpellSlotGrantEffectDefinition{"slots", "caster", SpellSlotPoolType::Casting, {{0, 2}, {3, 1}}, std::set<std::string>{"fireball"}}, {"baseSlots", "Classe"});
        auto slotsCleanup = slotsApply(resourceManager);
        auto &casting = castings.get("caster");
        casting.addToRepertoire("fireball");
        assert(casting.slotPools().at("baseSlots").dailyCapacityByLevel.size() == 2);
        assert(casting.slotPools().at("baseSlots").allowedSpellIds->contains("fireball"));
        assert(throwsInvalidArgument([&] { slotsApply(resourceManager); }));
        const auto preparationApply = compileEffect(SpellSlotGrantEffectDefinition{"preparations", "caster", SpellSlotPoolType::Preparation, {{3, 1}}}, {"preparationSlots", "Classe"});
        EffectCleanup preparationCleanup;
        if (mode == SpellcastingMode::Flexible)
        {
            preparationCleanup = preparationApply(resourceManager);
            auto &flex = dynamic_cast<FlexibleSpellcasting &>(casting);
            flex.prepare({"preparationSlots", 3, 0}, "fireball");
        }
        else assert(throwsInvalidArgument([&] { preparationApply(resourceManager); }));
        if (mode == SpellcastingMode::Prepared)
        {
            auto &prepared = dynamic_cast<PreparedSpellcasting &>(casting);
            assert(throwsInvalidArgument([&] { prepared.prepare({"baseSlots", 0, 0}, "fireball"); }));
            prepared.prepare({"baseSlots", 3, 0}, "fireball");
            prepared.consume({"baseSlots", 3, 0});
        }
        slotsCleanup();
        assert(casting.slotPools().empty());
        slotsCleanup = slotsApply(resourceManager);
        if (mode == SpellcastingMode::Prepared)
        {
            auto &prepared = dynamic_cast<PreparedSpellcasting &>(casting);
            prepared.prepare({"baseSlots", 3, 0}, "fireball");
            prepared.consume({"baseSlots", 3, 0}); // Replacing a pool resets its state.
        }
        if (preparationCleanup) preparationCleanup();
        slotsCleanup();
        castingCleanup();
        assert(throwsInvalidArgument([&] { slotsApply(resourceManager); }));
    }
    assert(throwsInvalidArgument([&] { compileEffect(SpellSlotGrantEffectDefinition{"slots", "caster", static_cast<SpellSlotPoolType>(999), {{1, 1}}}, {"slots", "Classe"}); }));
    assert(throwsInvalidArgument([&] { compileEffect(SpellSlotGrantEffectDefinition{"slots", "caster", SpellSlotPoolType::Casting, {{10, 1}}}, {"slots", "Classe"}); }));
    assert(throwsInvalidArgument([&] { compileEffect(HandUsagePermissionEffectDefinition{"grip", "weapon", static_cast<HandUsage>(999)}, {"grip", "Fonte"}); }));
    assert(throwsInvalidArgument([&] { compileEffect(HandUsagePermissionEffectDefinition{"grip", "weapon", HandUsage::OneHanded, {}, std::nullopt, "slot"}, {"grip", "Fonte"}); }));
    assert(throwsInvalidArgument([&] { compileEffect(HandUsagePermissionEffectDefinition{"grip", " ", HandUsage::OneHanded}, {"grip", "Fonte"}); }));
    return 0;
}
