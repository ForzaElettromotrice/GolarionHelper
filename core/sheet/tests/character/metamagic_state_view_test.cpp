#include "golarion/character/character_sheet.hpp"
#include "golarion/character/flexible_spellcasting.hpp"
#include "golarion/character/prepared_spellcasting.hpp"
#include "golarion/persistence/json.hpp"

#include <cassert>
#include <stdexcept>

namespace
{
    template<class Action> void rejects(Action action)
    {
        bool rejected = false;
        try { action(); } catch (const std::exception &) { rejected = true; }
        assert(rejected);
    }
}

int main()
{
    using namespace golarion;
    ResourceManager resources;
    Spellcastings castings(resources);
    auto pool = std::make_shared<MetamagicUsagePool>("source", 3);
    resources.addToCollection(Spellcastings::UsagePoolsResource, pool);
    resources.addToCollection(Spellcastings::Resource, SpellcastingGrant{"flex", "Classe", SpellcastingMode::Flexible, CharacterClassType::Wizard});
    resources.addToCollection(Spellcastings::Resource, SpellcastingGrant{"prepared", "Classe", SpellcastingMode::Prepared, CharacterClassType::Wizard});
    MetamagicGrant grant{.id = "silent", .source = "Talento", .type = MetamagicType::Silent,
        .slotCost = MetamagicSlotCostPolicy::Waived, .usageCost = MetamagicUsageCost{"source", 1, pool}};
    for (const auto id : {"flex", "prepared"})
    {
        castings.get(id).addToRepertoire("fireball");
        resources.addToCollection(spellSlotsResource(id), SpellSlotGrant{"slots", "Classe", {{3, 1}}});
        resources.addToCollection(preparationMetamagicGrantsResource(id), grant);
        resources.addToCollection(castingMetamagicGrantsResource(id), grant);
    }
    resources.addToCollection(preparationSlotsResource("flex"), SpellSlotGrant{"preparations", "Classe", {{3, 1}}});
    resources.addToCollection(metamagicMixingPermissionsResource("flex"), MetamagicMixingPermission{
        "mix", "Privilegio", [](const auto &) { return true; }, MetamagicUsageCost{"source", 2, pool}});
    auto &flex = dynamic_cast<FlexibleSpellcasting &>(castings.get("flex"));
    auto &prepared = dynamic_cast<PreparedSpellcasting &>(castings.get("prepared"));
    flex.prepare({"preparations", 3, 0}, {"fireball", {{MetamagicType::Silent, "silent", {}}}});
    prepared.prepare({"slots", 3, 0}, {"fireball", {{MetamagicType::Silent, "silent", {}}}});
    const auto view = castings.toView();
    assert(view.metamagicPools.size() == 1 && view.metamagicPools[0].remaining == 1);
    const auto &flexView = std::get<FlexibleSpellcastingView>(view.castings[0].state);
    assert(flexView.metamagic.size() == 2);
    assert(flexView.metamagic[0].phase == MetamagicPhase::Preparation);
    assert(flexView.metamagic[1].phase == MetamagicPhase::Casting);
    assert(flexView.metamagic[0].slotCost == MetamagicSlotCostPolicy::Waived);
    assert(flexView.metamagic[0].usageCost->poolId == "source" && flexView.metamagic[0].usageCost->remaining == 1);
    assert(flexView.mixingPermissions.size() == 1 && flexView.mixingPermissions[0].usageCost->uses == 2);
    assert(flexView.preparationGroups[0].preparations[0].evaluation->nominalSlotLevel == 4);
    auto saved = castings.toData();
    assert(saved.metamagicUses->size() == 1 && saved.metamagicUses->at("source") == 1);
    for (const auto id : {"flex", "prepared"})
    {
        resources.removeFromCollection(preparationMetamagicGrantsResource(id), "silent");
        resources.removeFromCollection(castingMetamagicGrantsResource(id), "silent");
    }
    resources.removeFromCollection(metamagicMixingPermissionsResource("flex"), "mix");
    auto sheetData = CharacterSheet().toSaveData();
    sheetData.spellcastings = saved;
    const auto decoded = persistence::fromJson(persistence::toJson(sheetData));
    {
        // Fresh owners restore historical preparations without ever registering their old grants.
        ResourceManager cloneResources;
        Spellcastings clone(cloneResources);
        auto clonePool = std::make_shared<MetamagicUsagePool>("source", 9);
        cloneResources.addToCollection(Spellcastings::UsagePoolsResource, clonePool);
        for (const auto &[id, mode] : {std::pair{"flex", SpellcastingMode::Flexible}, {"prepared", SpellcastingMode::Prepared}})
        {
            cloneResources.addToCollection(Spellcastings::Resource, SpellcastingGrant{id, "Classe", mode, CharacterClassType::Wizard});
            cloneResources.addToCollection(spellSlotsResource(id), SpellSlotGrant{"slots", "Classe", {{3, 1}}});
        }
        cloneResources.addToCollection(preparationSlotsResource("flex"), SpellSlotGrant{"preparations", "Classe", {{3, 1}}});
        clone.load(decoded.spellcastings);
        assert(clonePool->remaining() == 1);
        assert(dynamic_cast<PreparedSpellcasting &>(clone.get("prepared")).consume({"slots", 3, 0}).metamagic->nominalSlotLevel == 4);
        assert(dynamic_cast<FlexibleSpellcasting &>(clone.get("flex")).consume({"slots", 3, 0}, {{"preparations", 3, 0}}).metamagic->requiredSlotLevel == 3);
        assert(clonePool->remaining() == 1);
    }
    pool->reset(9);
    castings.load(decoded.spellcastings);
    assert(pool->remaining() == 1);
    assert(flex.toView().metamagic.empty() && flex.toView().mixingPermissions.empty());
    const auto flexibleCast = flex.consume({"slots", 3, 0}, {{"preparations", 3, 0}});
    const auto preparedCast = prepared.consume({"slots", 3, 0});
    assert(flexibleCast.metamagic->requiredSlotLevel == 3 && preparedCast.metamagic->requiredSlotLevel == 3);
    assert(flexibleCast.metamagic->nominalSlotLevel == 4 && preparedCast.metamagic->nominalSlotLevel == 4);
    assert(pool->remaining() == 1); // Restoring/casting never pays a preparation cost again.
    flex.clearPreparation({"preparations", 3, 0});
    rejects([&] { flex.prepare({"preparations", 3, 0}, {"fireball", {{MetamagicType::Silent, "silent", {}}}}); });
    castings.load(saved);

    for (int error = 0; error < 4; ++error)
    {
        auto invalid = saved;
        if (error == 0) invalid.metamagicUses->at("source") = -1;
        if (error == 1) invalid.metamagicUses->clear();
        if (error == 2) invalid.metamagicUses->emplace("unknown", 2);
        if (error == 3) std::get<FlexibleSpellcastingData>(invalid.castings[0].state).preparations[0].evaluation->requiredSlotLevel = 9;
        pool->reset(7);
        rejects([&] { castings.load(invalid); });
        assert(pool->remaining() == 7);
        assert(!prepared.toData().preparations.empty());
    }
    auto conflicting = std::make_shared<MetamagicUsagePool>("source", 2);
    rejects([&] { resources.addToCollection(Spellcastings::UsagePoolsResource, conflicting); });
    grant.usageCost->pool = conflicting;
    resources.addToCollection(castingMetamagicGrantsResource("flex"), grant);
    rejects([&] { castings.toData(); });
    rejects([&] { castings.load(saved); });
    resources.removeFromCollection(castingMetamagicGrantsResource("flex"), "silent");
    grant.usageCost->pool.reset();
    resources.addToCollection(castingMetamagicGrantsResource("flex"), grant);
    assert(!flex.toView().metamagic.front().usageCost->remaining);
    assert(castings.toView().metamagicPools.size() == 1);
    resources.removeFromCollection(castingMetamagicGrantsResource("flex"), "silent");
    resources.removeFromCollection(Spellcastings::Resource, "flex");
    resources.removeFromCollection(Spellcastings::Resource, "prepared");
    assert(castings.toData().metamagicUses->at("source") == 7); // Source can outlive every casting grant.
    ResourceManager restoredResources;
    Spellcastings restored(restoredResources);
    auto restoredPool = std::make_shared<MetamagicUsagePool>("source", 0);
    restoredResources.addToCollection(Spellcastings::UsagePoolsResource, restoredPool);
    restored.load(castings.toData());
    assert(restoredPool->remaining() == 7);
}
