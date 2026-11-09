#include "golarion/character/metamagic.hpp"
#include "golarion/character/prepared_spellcasting.hpp"
#include "golarion/character/spontaneous_spellcasting.hpp"
#include "golarion/character/flexible_spellcasting.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <cassert>
#include <stdexcept>

namespace
{
    template<typename Function> void rejects(Function function)
    {
        bool rejected = false;
        try { function(); }
        catch (const std::invalid_argument &) { rejected = true; }
        assert(rejected);
    }
}

int main()
{
    using namespace golarion;
    ResourceManager resources;
    const auto preparation = preparationMetamagicGrantsResource("wizard");
    const auto casting = castingMetamagicGrantsResource("wizard");
    const MetamagicApplication empowered{MetamagicType::Empower, "empower", {}};
    const MetamagicApplication heightened{MetamagicType::Heighten, "heighten", HeightenMetamagicOptions{5}};
    const MetamagicApplication reach{MetamagicType::Reach, "reach", ReachMetamagicOptions{2}};
    const MetamagicApplication elemental{MetamagicType::Elemental, "cold", ElementalMetamagicOptions{DamageType::Cold, ElementalDamageMode::Split}};
    const MetamagicGrant feat{"empower", "Feat", MetamagicType::Empower};
    MetamagicGrant rod{"rod", "Rod instance", MetamagicType::Empower};
    rod.slotCost = MetamagicSlotCostPolicy::Waived;
    rod.rodGrade = MetamagicRodGrade::Lesser;
    rod.usageCost = MetamagicUsageCost{" item.rod.uses ", 1};
    {
        MetamagicRegistry registry(resources, "wizard");
        rejects([&] { registry.resolve(MetamagicPhase::Preparation, empowered); });
        rejects([&] { registry.resolve(MetamagicPhase::Casting, empowered); });
        resources.addToCollection(preparation, feat);
        assert(registry.resolve(MetamagicPhase::Preparation, empowered).type == MetamagicType::Empower);
        rejects([&] { registry.resolve(MetamagicPhase::Casting, empowered); });
        auto conversionFeat = feat;
        conversionFeat.castingScope = CastingMetamagicScope::SpontaneousConversionOnly;
        resources.addToCollection(casting, conversionFeat);
        assert(registry.resolve(MetamagicPhase::Casting, empowered).castingScope == CastingMetamagicScope::SpontaneousConversionOnly);
        rejects([&] { resources.addToCollection(preparation, feat); });
        resources.removeFromCollection(preparation, " empower ");
        rejects([&] { registry.resolve(MetamagicPhase::Preparation, empowered); });
        assert(registry.resolve(MetamagicPhase::Casting, empowered).id == "empower");
        rejects([&] { resources.addToCollection(preparation, conversionFeat); });
        rejects([&] { resources.addToCollection(preparation, rod); });
        resources.addToCollection(casting, rod);
        const auto resolvedRod = registry.resolve(MetamagicPhase::Casting, {MetamagicType::Empower, " rod ", {}});
        assert(resolvedRod.usageCost->poolId == "item.rod.uses");
        assert(resolvedRod.slotCost == MetamagicSlotCostPolicy::Waived);
        assert(metamagicRodMaximumSpellLevel(*resolvedRod.rodGrade) == 3);
        rod.id = "second-rod";
        rod.usageCost->poolId = "item.second.uses";
        resources.addToCollection(casting, rod);
        assert(registry.resolve(MetamagicPhase::Casting, {MetamagicType::Empower, "second-rod", {}}).usageCost->poolId == "item.second.uses");

        MetamagicGrant cold{"cold", "Feat", MetamagicType::Elemental};
        rejects([&] { resources.addToCollection(preparation, cold); });
        cold.elementalEnergy = DamageType::Cold;
        resources.addToCollection(preparation, cold);
        assert(registry.resolve(MetamagicPhase::Preparation, elemental).elementalEnergy == DamageType::Cold);
        rejects([&] { registry.resolve(MetamagicPhase::Preparation, {MetamagicType::Elemental, "cold", ElementalMetamagicOptions{DamageType::Fire, ElementalDamageMode::Replace}}); });
        rejects([&] { registry.resolve(MetamagicPhase::Preparation, {MetamagicType::Empower, "cold", {}}); });
        rejects([&] { registry.resolve(static_cast<MetamagicPhase>(99), elemental); });
        auto invalid = feat;
        invalid.id = "bad";
        invalid.type = static_cast<MetamagicType>(99);
        rejects([&] { resources.addToCollection(casting, invalid); });
        invalid = feat;
        invalid.id = " ";
        rejects([&] { resources.addToCollection(casting, invalid); });
        invalid.id = "bad";
        invalid.source = " ";
        rejects([&] { resources.addToCollection(casting, invalid); });
        invalid.source = "Test";
        invalid.elementalEnergy = DamageType::Fire;
        rejects([&] { resources.addToCollection(casting, invalid); });
        invalid.elementalEnergy.reset();
        invalid.slotCost = static_cast<MetamagicSlotCostPolicy>(99);
        rejects([&] { resources.addToCollection(casting, invalid); });
        invalid.slotCost = MetamagicSlotCostPolicy::Standard;
        invalid.castingScope = static_cast<CastingMetamagicScope>(99);
        rejects([&] { resources.addToCollection(casting, invalid); });
        invalid = rod;
        invalid.id = "bad";
        invalid.usageCost.reset();
        rejects([&] { resources.addToCollection(casting, invalid); });
        invalid.usageCost = MetamagicUsageCost{"pool", 0};
        rejects([&] { resources.addToCollection(casting, invalid); });
        invalid.usageCost = MetamagicUsageCost{" ", 1};
        rejects([&] { resources.addToCollection(casting, invalid); });
        invalid.usageCost = MetamagicUsageCost{"pool", 1};
        invalid.rodGrade = static_cast<MetamagicRodGrade>(99);
        rejects([&] { resources.addToCollection(casting, invalid); });
        invalid.rodGrade = MetamagicRodGrade::Normal;
        invalid.slotCost = MetamagicSlotCostPolicy::Standard;
        rejects([&] { resources.addToCollection(casting, invalid); });
        rejects([&] { resources.addToCollection(casting, 1); });
        rejects([&] { resources.removeFromCollection(casting, "missing"); });
        MetamagicRegistry other(resources, "other");
        rejects([&] { other.resolve(MetamagicPhase::Casting, empowered); });
    }

    for (const auto &[type, increase] : std::map<MetamagicType, int>{{MetamagicType::Empower, 2}, {MetamagicType::Enlarge, 1}, {MetamagicType::Extend, 1}, {MetamagicType::Maximize, 3}, {MetamagicType::Quicken, 4}, {MetamagicType::Silent, 1}, {MetamagicType::Still, 1}, {MetamagicType::Widen, 3}, {MetamagicType::Intensified, 1}, {MetamagicType::Selective, 1}})
    {
        const auto adjustment = standardMetamagicAdjustment({type, "grant", {}}, 3);
        assert(adjustment.slotLevelIncrease == increase);
        assert(adjustment.effectiveLevelIncrease == 0);
    }
    const auto heightenCost = standardMetamagicAdjustment(heightened, 3);
    assert(heightenCost.slotLevelIncrease == 2 && heightenCost.effectiveLevelIncrease == 2);
    assert(standardMetamagicAdjustment(reach, 3).slotLevelIncrease == 2);
    assert(standardMetamagicAdjustment(elemental, 3).slotLevelIncrease == 1);
    rejects([&] { standardMetamagicAdjustment(heightened, 5); });
    rejects([&] { standardMetamagicAdjustment(empowered, -1); });
    rejects([&] { standardMetamagicAdjustment(empowered, 10); });
    for (const MetamagicApplication invalid : {
        MetamagicApplication{static_cast<MetamagicType>(99), "id", {}},
        {MetamagicType::Empower, " ", {}}, {MetamagicType::Empower, "id", HeightenMetamagicOptions{5}},
        {MetamagicType::Heighten, "id", {}}, {MetamagicType::Heighten, "id", HeightenMetamagicOptions{10}},
        {MetamagicType::Reach, "id", ReachMetamagicOptions{0}}, {MetamagicType::Reach, "id", ReachMetamagicOptions{4}},
        {MetamagicType::Elemental, "id", ElementalMetamagicOptions{static_cast<DamageType>(99), ElementalDamageMode::Split}},
        {MetamagicType::Elemental, "id", ElementalMetamagicOptions{DamageType::Cold, static_cast<ElementalDamageMode>(99)}}})
        rejects([&] { validateMetamagicApplication(invalid); });
    assert(metamagicRodMaximumSpellLevel(MetamagicRodGrade::Normal) == 6);
    assert(metamagicRodMaximumSpellLevel(MetamagicRodGrade::Greater) == 9);

    // All casting modes own both endpoints, but neither endpoint grants anything itself.
    {
        PreparedSpellcasting prepared(resources, "wizard", CharacterClassType::Wizard);
        resources.addToCollection(preparation, feat);
        resources.addToCollection(casting, rod);
        SpontaneousSpellcasting spontaneous(resources, "sorcerer", CharacterClassType::Sorcerer);
        FlexibleSpellcasting flexible(resources, "arcanist", CharacterClassType::Arcanist);
        resources.addToCollection(castingMetamagicGrantsResource("sorcerer"), feat);
        resources.addToCollection(preparationMetamagicGrantsResource("arcanist"), feat);
    }
    for (const auto &endpoint : {preparation, casting})
    {
        rejects([&] { resources.addToCollection(endpoint, feat); });
        resources.registerCollectionResource<int>(endpoint, [](int) {}, [](std::string_view) {});
        rejects([&] { PreparedSpellcasting fail(resources, "wizard", CharacterClassType::Wizard); });
        resources.addToCollection(endpoint, 1);
        resources.unregisterCollectionResource(endpoint);
        PreparedSpellcasting retry(resources, "wizard", CharacterClassType::Wizard);
    }
    resources.registerCollectionResource<int>(spellSlotsResource("wizard"), [](int) {}, [](std::string_view) {});
    rejects([&] { PreparedSpellcasting fail(resources, "wizard", CharacterClassType::Wizard); });
    // Failure after metamagic construction must release its endpoints too.
    MetamagicRegistry retry(resources, "wizard");
}
