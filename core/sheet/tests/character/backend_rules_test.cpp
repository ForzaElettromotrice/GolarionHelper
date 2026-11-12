#include "golarion/character/character_sheet.hpp"
#include "golarion/effect/effect_compiler.hpp"
#include "golarion/effect/effect_json.hpp"
#include <nlohmann/json.hpp>
#include <cassert>

namespace
{
    using namespace golarion;
    template<class F> void rejects(F action)
    {
        bool rejected = false;
        try { action(); } catch (const std::exception &) { rejected = true; }
        assert(rejected);
    }
    EffectCleanup apply(ResourceManager &resources, const char *json, const char *id)
    {
        return compileEffect(effectDefinitionFromJson(nlohmann::json::parse(json)), {id, id})(resources);
    }
}

int main()
{
    using namespace golarion;
    ResourceManager resources;
    int dexterity = 4;
    resources.registerTarget("dexMod", [&] { return dexterity; });
    ArmorClass armor(resources);
    auto suppressed = apply(resources, R"({"id":"local","type":"armorClassAbilitySuppression","cause":"flatFooted"})", "flat");
    assert(armor.toView().abilityOptions[0].values[0].appliedAbilityModifier == 0);
    auto retain = apply(resources, R"({"id":"local","type":"armorClassDexterityRetention","causes":["flatFooted","invisibleAttacker"]})", "retention");
    auto view = armor.toView();
    assert(!view.abilityBonusSuppressed && view.abilitySuppressions[0].dexterityRetained);
    assert(view.abilityOptions[0].values[0].appliedAbilityModifier == 4);
    assert(view.abilityOptions[0].values[2].appliedAbilityModifier == 4);
    auto pinned = apply(resources, R"({"id":"local","type":"armorClassAbilitySuppression","cause":"pinned"})", "pinned");
    assert(armor.toView().abilityBonusSuppressed);
    pinned();
    auto feint = apply(resources, R"({"id":"local","type":"armorClassAbilitySuppression","cause":"feint"})", "feint");
    assert(armor.toView().abilityBonusSuppressed);
    feint();
    dexterity = -2;
    assert(armor.toView().abilityOptions[0].values[0].appliedAbilityModifier == -2);
    retain();
    suppressed();
    assert(armor.toView().dexterityRetentions.empty());
    rejects([&] { apply(resources, R"({"id":"x","type":"armorClassDexterityRetention","causes":["pinned"]})", "bad"); });

    Movement movement(resources);
    int base = 2;
    resources.registerTarget("baseSpeed", [&] { return base; });
    auto grant = apply(resources, R"({"id":"local","type":"movementGrant","movementType":"land","baseSpeedExpression":"@baseSpeed","affectedByArmor":true,"affectedByLoad":true,"supportsRunning":true})", "walk");
    auto haste = apply(resources, R"({"id":"local","type":"movementBoundedBonus","description":"Velocità","bonusType":"enhancement","expression":"6","maximumPercent":200,"selector":{"grantId":"walk"}})", "haste");
    assert(movement.toView().grants[0].effectiveUnits == 4);
    assert(movement.toView().grants[0].boundedBonuses[0].cappedUnits == 2);
    base = 6;
    assert(movement.toView().grants[0].effectiveUnits == 12);
    auto second = apply(resources, R"({"id":"x","type":"movementBoundedBonus","description":"Velocità","bonusType":"enhancement","expression":"6"})", "second");
    assert(movement.toView().grants[0].effectiveUnits == 12);
    auto armorSlow = apply(resources, R"({"id":"x","type":"movementAdjustment","description":"Armatura","adjustment":"reducedByArmorOrLoad","selector":{"affectedByArmorOnly":true}})", "armor");
    assert(movement.toView().grants[0].effectiveUnits == 8);
    base = 4;
    assert(movement.toView().grants[0].effectiveUnits == 6);
    auto bonus = apply(resources, R"({"id":"x","type":"modifier","resource":"speed.land","description":"Altro bonus","modifierType":"bonus","bonusType":"enhancement","expression":"8"})", "speedBonus");
    assert(movement.toView().grants[0].effectiveUnits == 8);
    bonus();
    rejects([&] { grant(); });
    haste(); second(); armorSlow(); grant();
    assert(movement.toView().grants.empty());

    // Class requirements bind their own level and update without rebuilding the grant.
    ResourceManager classResources;
    for (const auto *name : {"baseAttackBonus", "hp.max", "skill.ranks.max"}) classResources.registerAccumulatedResource(name);
    for (auto type : {SavingThrowType::Fortitude, SavingThrowType::Reflex, SavingThrowType::Will}) classResources.registerAccumulatedResource(baseResourceName(type));
    for (const auto *target : {"conMod", "intSkillRanksMod"}) classResources.registerTarget(target, [] { return 0; });
    classResources.registerCollectionResource<SkillClassSkillGrant>(SkillClassSkillGrantsResource, [](auto) {}, [](auto) {});
    std::optional<HandUsagePermission> permission;
    classResources.registerCollectionResource<HandUsagePermission>(HandUsagePermissionsResource, [&](auto entry) { permission = entry; }, [&](auto) { permission.reset(); });
    ClassDefinition definition{.type = CharacterClassType::Fighter, .name = "Test", .description = "Test", .hitDie = ClassHitDie::D10, .skillRanksPerLevel = 2,
        .progression = {{1, 1, {{SavingThrowType::Fortitude, 2}, {SavingThrowType::Reflex, 0}, {SavingThrowType::Will, 0}}},
            {2, 2, {{SavingThrowType::Fortitude, 3}, {SavingThrowType::Reflex, 0}, {SavingThrowType::Will, 0}}}}, .features = {{"grip", "Presa", "Test", {{.id = "grip", .level = 1, .effects = {
            effectDefinitionFromJson(nlohmann::json::parse(R"({"id":"permission","type":"handUsagePermission","strikeGrantId":"weapon","handUsage":"oneHanded","requirements":[{"expression":"@classLevel >= 2","failureReason":"Livello insufficiente"}]})"))
        }}}}}};
    {
        CharacterClass characterClass(classResources, "fighter", definition);
        assert(permission && !permission->requirements[0].isSatisfied(classResources));
        characterClass.setLevel(2);
        assert(permission->requirements[0].isSatisfied(classResources));
        characterClass.setLevel(1);
        assert(!permission->requirements[0].isSatisfied(classResources));
    }
    assert(!permission);
}
