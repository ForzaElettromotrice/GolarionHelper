#include "golarion/character/special_defenses.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/special_defenses_view.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>

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

    const golarion::EnergyResistanceView &resistanceView(const golarion::SpecialDefensesView &view, golarion::DamageType energy)
    {
        const auto resistance = std::ranges::find(view.energyResistances, energy, &golarion::EnergyResistanceView::energy);
        if (resistance == view.energyResistances.end())
        {
            throw std::invalid_argument("energy resistance is not available");
        }
        return *resistance;
    }

    const golarion::EnergyResistanceGrantView &grantView(const golarion::EnergyResistanceView &view, std::string_view grantId)
    {
        const auto grant = std::ranges::find(view.grants, grantId, &golarion::EnergyResistanceGrantView::id);
        if (grant == view.grants.end())
        {
            throw std::invalid_argument("energy resistance grant is not available");
        }
        return *grant;
    }

    const golarion::DamageReductionGrantView &damageReductionView(const golarion::SpecialDefensesView &view, std::string_view grantId)
    {
        const auto grant = std::ranges::find(view.damageReductions, grantId, &golarion::DamageReductionGrantView::id);
        if (grant == view.damageReductions.end())
        {
            throw std::invalid_argument("damage reduction grant is not available");
        }
        return *grant;
    }

    const golarion::DamageReductionAdjustmentApplicationView &damageReductionAdjustmentView(const golarion::DamageReductionGrantView &view, std::string_view adjustmentId)
    {
        const auto adjustment = std::ranges::find(view.adjustments, adjustmentId, &golarion::DamageReductionAdjustmentApplicationView::id);
        if (adjustment == view.adjustments.end())
        {
            throw std::invalid_argument("damage reduction adjustment is not available");
        }
        return *adjustment;
    }

    const golarion::DamageReductionCombinationView &damageReductionCombinationView(const golarion::SpecialDefensesView &view, std::vector<std::string> grantIds)
    {
        std::ranges::sort(grantIds);
        const auto combination = std::ranges::find(view.damageReductionCombinations, grantIds, &golarion::DamageReductionCombinationView::grantIds);
        if (combination == view.damageReductionCombinations.end())
        {
            throw std::invalid_argument("damage reduction combination is not available");
        }
        return *combination;
    }

    const golarion::ImmunityView &immunityView(const golarion::SpecialDefensesView &view, std::string_view targetId)
    {
        const auto immunity = std::ranges::find(view.immunities, targetId, &golarion::ImmunityView::targetId);
        if (immunity == view.immunities.end())
        {
            throw std::invalid_argument("immunity is not available");
        }
        return *immunity;
    }

    const golarion::ImmunityGrantView &immunityGrantView(const golarion::ImmunityView &view, std::string_view grantId)
    {
        const auto grant = std::ranges::find(view.grants, grantId, &golarion::ImmunityGrantView::id);
        if (grant == view.grants.end())
        {
            throw std::invalid_argument("immunity grant is not available");
        }
        return *grant;
    }

    const golarion::SpellResistanceContextView &generalSpellResistanceView(const golarion::SpecialDefensesView &view)
    {
        const auto context = std::ranges::find_if(view.spellResistances, [](const golarion::SpellResistanceContextView &candidate)
        {
            return !candidate.applicability.has_value();
        });
        if (context == view.spellResistances.end())
        {
            throw std::invalid_argument("general spell resistance is not available");
        }
        return *context;
    }

    const golarion::SpellResistanceContextView &conditionalSpellResistanceView(const golarion::SpecialDefensesView &view, std::string_view applicability)
    {
        const auto context = std::ranges::find_if(view.spellResistances, [applicability](const golarion::SpellResistanceContextView &candidate)
        {
            return candidate.applicability == applicability;
        });
        if (context == view.spellResistances.end())
        {
            throw std::invalid_argument("conditional spell resistance is not available");
        }
        return *context;
    }

    const golarion::SpellResistanceGrantView &spellResistanceGrantView(const golarion::SpellResistanceContextView &view, std::string_view grantId)
    {
        const auto grant = std::ranges::find(view.grants, grantId, &golarion::SpellResistanceGrantView::id);
        if (grant == view.grants.end())
        {
            throw std::invalid_argument("spell resistance grant is not available");
        }
        return *grant;
    }

    const golarion::SpellResistanceAdjustmentApplicationView &spellResistanceAdjustmentView(const golarion::SpellResistanceGrantView &view, std::string_view adjustmentId)
    {
        const auto adjustment = std::ranges::find(view.adjustments, adjustmentId, &golarion::SpellResistanceAdjustmentApplicationView::id);
        if (adjustment == view.adjustments.end())
        {
            throw std::invalid_argument("spell resistance adjustment is not available");
        }
        return *adjustment;
    }

    const golarion::FastHealingContextView &generalFastHealingView(const golarion::SpecialDefensesView &view)
    {
        const auto context = std::ranges::find_if(view.fastHealing, [](const golarion::FastHealingContextView &candidate)
        {
            return !candidate.applicability.has_value();
        });
        if (context == view.fastHealing.end())
        {
            throw std::invalid_argument("general fast healing is not available");
        }
        return *context;
    }

    const golarion::FastHealingContextView &conditionalFastHealingView(const golarion::SpecialDefensesView &view, std::string_view applicability)
    {
        const auto context = std::ranges::find_if(view.fastHealing, [applicability](const golarion::FastHealingContextView &candidate)
        {
            return candidate.applicability == applicability;
        });
        if (context == view.fastHealing.end())
        {
            throw std::invalid_argument("conditional fast healing is not available");
        }
        return *context;
    }

    const golarion::FastHealingGrantView &fastHealingGrantView(const golarion::FastHealingContextView &view, std::string_view grantId)
    {
        const auto grant = std::ranges::find(view.grants, grantId, &golarion::FastHealingGrantView::id);
        if (grant == view.grants.end())
        {
            throw std::invalid_argument("fast healing grant is not available");
        }
        return *grant;
    }

    const golarion::RegenerationGrantView &regenerationGrantView(const golarion::SpecialDefensesView &view, std::string_view grantId)
    {
        const auto grant = std::ranges::find(view.regeneration, grantId, &golarion::RegenerationGrantView::id);
        if (grant == view.regeneration.end())
        {
            throw std::invalid_argument("regeneration grant is not available");
        }
        return *grant;
    }
}

int main()
{
    using namespace golarion;

    int level = 7;
    ResourceManager resourceManager;
    resourceManager.registerTarget("level", [&level]
    {
        return level;
    });
    SpecialDefenses specialDefenses(resourceManager);
    assert(specialDefenses.toView().energyResistances.empty());
    assert(specialDefenses.toView().damageReductions.empty());
    assert(specialDefenses.toView().damageReductionCombinations.empty());
    assert(specialDefenses.toView().immunities.empty());
    assert(specialDefenses.toView().spellResistances.empty());
    assert(specialDefenses.toView().fastHealing.empty());
    assert(specialDefenses.toView().regeneration.empty());

    resourceManager.addToCollection(ImmunityGrantsResource, Immunity(ImmunityDefinition{
        .id = "subtype.fire",
        .source = "Sottotipo Fuoco",
        .targetId = "damage.fire",
        .name = "Fuoco",
        .applicability = std::nullopt
    }));
    resourceManager.addToCollection(ImmunityGrantsResource, Immunity(ImmunityDefinition{
        .id = "spell.protectionFromFire",
        .source = "Protezione dall'Energia",
        .targetId = "damage.fire",
        .name = " Fuoco ",
        .applicability = "Finché l'incantesimo non viene scaricato"
    }));
    resourceManager.addToCollection(ImmunityGrantsResource, Immunity(ImmunityDefinition{
        .id = "daemon.poison",
        .source = "Tratti dei Daemon",
        .targetId = "affliction.poison",
        .name = "Veleno",
        .applicability = std::nullopt
    }));
    resourceManager.addToCollection(ImmunityGrantsResource, Immunity(ImmunityDefinition{
        .id = "construct.mindAffecting",
        .source = "Tratti dei Costrutti",
        .targetId = "effect.mindAffecting",
        .name = "Effetti di influenza mentale",
        .applicability = std::nullopt
    }));
    resourceManager.addToCollection(ImmunityGrantsResource, Immunity(ImmunityDefinition{
        .id = "spell.magicMissile",
        .source = "Immunità agli Incantesimi",
        .targetId = "spell.magicMissile",
        .name = "Dardo Incantato",
        .applicability = std::nullopt
    }));
    resourceManager.addToCollection(ImmunityGrantsResource, Immunity(ImmunityDefinition{
        .id = "construct.traits",
        .source = "Tipo Costrutto",
        .targetId = "trait.construct",
        .name = "Tratti dei Costrutti",
        .applicability = std::nullopt
    }));

    SpecialDefensesView immunityState = specialDefenses.toView();
    assert(immunityState.immunities.size() == 5);
    const ImmunityView &fireImmunity = immunityView(immunityState, "damage.fire");
    assert(fireImmunity.name == "Fuoco");
    assert(fireImmunity.grants.size() == 2);
    assert(immunityGrantView(fireImmunity, "subtype.fire").source == "Sottotipo Fuoco");
    assert(immunityGrantView(fireImmunity, "spell.protectionFromFire").applicability == "Finché l'incantesimo non viene scaricato");
    assert(immunityView(immunityState, "affliction.poison").name == "Veleno");
    assert(immunityView(immunityState, "effect.mindAffecting").name == "Effetti di influenza mentale");
    assert(immunityView(immunityState, "spell.magicMissile").name == "Dardo Incantato");
    assert(immunityView(immunityState, "trait.construct").name == "Tratti dei Costrutti");

    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.addToCollection(ImmunityGrantsResource, Immunity(ImmunityDefinition{
            .id = "duplicate.subtype.fire",
            .source = "Nome incoerente",
            .targetId = "damage.fire",
            .name = "Fiamme",
            .applicability = std::nullopt
        }));
    }));
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.addToCollection(ImmunityGrantsResource, Immunity(ImmunityDefinition{
            .id = "subtype.fire",
            .source = "Duplicata",
            .targetId = "damage.fire",
            .name = "Fuoco",
            .applicability = std::nullopt
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        Immunity(ImmunityDefinition{
            .id = "invalid.blankTarget",
            .source = "Non valida",
            .targetId = "   ",
            .name = "Niente",
            .applicability = std::nullopt
        });
    }));

    resourceManager.removeFromCollection(ImmunityGrantsResource, "spell.protectionFromFire");
    assert(immunityView(specialDefenses.toView(), "damage.fire").grants.size() == 1);
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.removeFromCollection(ImmunityGrantsResource, "missing.immunity");
    }));

    resourceManager.addToCollection(SpellResistanceGrantsResource, SpellResistance(SpellResistanceDefinition{
        .id = "spell.spellResistance",
        .source = "Resistenza agli Incantesimi",
        .expression = "12 + @level",
        .applicability = std::nullopt,
        .canBeLowered = true,
        .tags = {"spell"}
    }));
    resourceManager.addToCollection(SpellResistanceGrantsResource, SpellResistance(SpellResistanceDefinition{
        .id = "armor.spellResistance",
        .source = "Armatura della resistenza agli incantesimi",
        .expression = "15",
        .applicability = std::nullopt,
        .canBeLowered = true,
        .tags = {"equipment"}
    }));
    resourceManager.addToCollection(SpellResistanceGrantsResource, SpellResistance(SpellResistanceDefinition{
        .id = "archmage.spellResistance",
        .source = "Vero Arcimago",
        .expression = "15 + @level",
        .applicability = " Contro incantesimi arcani ",
        .canBeLowered = false,
        .tags = {"mythic"}
    }));

    SpecialDefensesView spellResistanceState = specialDefenses.toView();
    assert(spellResistanceState.spellResistances.size() == 2);
    const SpellResistanceContextView &generalSpellResistance = generalSpellResistanceView(spellResistanceState);
    assert(generalSpellResistance.effectiveValue == 19);
    assert(generalSpellResistance.grants.size() == 2);
    const SpellResistanceGrantView &spellGrant = spellResistanceGrantView(generalSpellResistance, "spell.spellResistance");
    assert(spellGrant.baseValue == 19);
    assert(spellGrant.effectiveValue == 19);
    assert(spellGrant.canBeLowered);
    assert(spellGrant.determinesEffectiveValue);
    const SpellResistanceGrantView &armorGrant = spellResistanceGrantView(generalSpellResistance, "armor.spellResistance");
    assert(!armorGrant.determinesEffectiveValue);
    assert(armorGrant.notAppliedReason.has_value());
    const SpellResistanceContextView &arcaneSpellResistance = conditionalSpellResistanceView(spellResistanceState, "Contro incantesimi arcani");
    assert(arcaneSpellResistance.effectiveValue == 22);
    assert(arcaneSpellResistance.grants.size() == 3);
    const SpellResistanceGrantView &archmageGrant = spellResistanceGrantView(arcaneSpellResistance, "archmage.spellResistance");
    assert(archmageGrant.applicability == "Contro incantesimi arcani");
    assert(!archmageGrant.canBeLowered);
    assert(archmageGrant.determinesEffectiveValue);

    resourceManager.addToCollection(SpellResistanceAdjustmentsResource, SpellResistanceAdjustment(SpellResistanceAdjustmentDefinition{
        .id = "symbol.vulnerability",
        .source = "Simbolo di Vulnerabilità",
        .selector = SpellResistanceSelector(),
        .expression = "-4",
        .stackingGroup = "symbol.vulnerability",
        .applicability = std::nullopt
    }));
    resourceManager.addToCollection(SpellResistanceAdjustmentsResource, SpellResistanceAdjustment(SpellResistanceAdjustmentDefinition{
        .id = "symbol.vulnerability.weaker",
        .source = "Simbolo di Vulnerabilità inferiore",
        .selector = SpellResistanceSelector(),
        .expression = "-2",
        .stackingGroup = "symbol.vulnerability",
        .applicability = std::nullopt
    }));
    resourceManager.addToCollection(SpellResistanceAdjustmentsResource, SpellResistanceAdjustment(SpellResistanceAdjustmentDefinition{
        .id = "mesmerist.planarBinding",
        .source = "Legame Anatema dei Diavoli",
        .selector = SpellResistanceSelector(),
        .expression = "-3",
        .stackingGroup = "mesmerist.hypnoticStare",
        .applicability = "Contro Legame Planare"
    }));

    spellResistanceState = specialDefenses.toView();
    assert(spellResistanceState.spellResistances.size() == 3);
    assert(generalSpellResistanceView(spellResistanceState).effectiveValue == 15);
    assert(conditionalSpellResistanceView(spellResistanceState, "Contro incantesimi arcani").effectiveValue == 18);
    const SpellResistanceContextView &planarBindingResistance = conditionalSpellResistanceView(spellResistanceState, "Contro Legame Planare");
    assert(planarBindingResistance.effectiveValue == 12);
    const SpellResistanceGrantView &planarBindingSpellGrant = spellResistanceGrantView(planarBindingResistance, "spell.spellResistance");
    assert(planarBindingSpellGrant.effectiveValue == 12);
    assert(planarBindingSpellGrant.adjustments.size() == 3);
    assert(spellResistanceAdjustmentView(planarBindingSpellGrant, "symbol.vulnerability").applied);
    assert(!spellResistanceAdjustmentView(planarBindingSpellGrant, "symbol.vulnerability.weaker").applied);
    assert(spellResistanceAdjustmentView(planarBindingSpellGrant, "symbol.vulnerability.weaker").notAppliedReason.has_value());
    assert(spellResistanceAdjustmentView(planarBindingSpellGrant, "mesmerist.planarBinding").applicability == "Contro Legame Planare");

    resourceManager.addToCollection(SpellResistanceAdjustmentsResource, SpellResistanceAdjustment(SpellResistanceAdjustmentDefinition{
        .id = "archmage.increase",
        .source = "Potere mitico",
        .selector = SpellResistanceSelector(SpellResistanceSelectorDefinition{
            .grantId = "archmage.spellResistance",
            .anyTags = {"mythic"},
            .excludedGrantIds = {}
        }),
        .expression = "2",
        .stackingGroup = "archmage.increase",
        .applicability = std::nullopt
    }));
    assert(conditionalSpellResistanceView(specialDefenses.toView(), "Contro incantesimi arcani").effectiveValue == 20);
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.removeFromCollection(SpellResistanceGrantsResource, "archmage.spellResistance");
    }));
    resourceManager.removeFromCollection(SpellResistanceAdjustmentsResource, "archmage.increase");

    resourceManager.addToCollection(SpellResistanceAdjustmentsResource, SpellResistanceAdjustment(SpellResistanceAdjustmentDefinition{
        .id = "ambiguous.spellResistance.increase",
        .source = "Incremento ambiguo",
        .selector = SpellResistanceSelector(SpellResistanceSelectorDefinition{
            .grantId = "spell.spellResistance",
            .anyTags = {},
            .excludedGrantIds = {}
        }),
        .expression = "1",
        .stackingGroup = "ambiguous.spellResistance",
        .applicability = std::nullopt
    }));
    resourceManager.addToCollection(SpellResistanceAdjustmentsResource, SpellResistanceAdjustment(SpellResistanceAdjustmentDefinition{
        .id = "ambiguous.spellResistance.decrease",
        .source = "Riduzione ambigua",
        .selector = SpellResistanceSelector(SpellResistanceSelectorDefinition{
            .grantId = "spell.spellResistance",
            .anyTags = {},
            .excludedGrantIds = {}
        }),
        .expression = "-1",
        .stackingGroup = "ambiguous.spellResistance",
        .applicability = std::nullopt
    }));
    assert(throwsInvalidArgument([&specialDefenses]
    {
        specialDefenses.toView();
    }));
    resourceManager.removeFromCollection(SpellResistanceAdjustmentsResource, "ambiguous.spellResistance.increase");
    resourceManager.removeFromCollection(SpellResistanceAdjustmentsResource, "ambiguous.spellResistance.decrease");

    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.addToCollection(SpellResistanceAdjustmentsResource, SpellResistanceAdjustment(SpellResistanceAdjustmentDefinition{
            .id = "missing.spellResistance",
            .source = "Bersaglio mancante",
            .selector = SpellResistanceSelector(SpellResistanceSelectorDefinition{
                .grantId = "missing.grant",
                .anyTags = {},
                .excludedGrantIds = {}
            }),
            .expression = "1",
            .stackingGroup = "missing.spellResistance",
            .applicability = std::nullopt
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        SpellResistanceSelector(SpellResistanceSelectorDefinition{
            .grantId = "same.grant",
            .anyTags = {},
            .excludedGrantIds = {"same.grant"}
        });
    }));

    resourceManager.addToCollection(SpellResistanceGrantsResource, SpellResistance(SpellResistanceDefinition{
        .id = "invalid.negativeSpellResistance",
        .source = "RI non valida",
        .expression = "-1",
        .applicability = std::nullopt,
        .canBeLowered = true,
        .tags = {}
    }));
    assert(throwsInvalidArgument([&specialDefenses]
    {
        specialDefenses.toView();
    }));
    resourceManager.removeFromCollection(SpellResistanceGrantsResource, "invalid.negativeSpellResistance");

    resourceManager.removeFromCollection(SpellResistanceAdjustmentsResource, "symbol.vulnerability");
    resourceManager.removeFromCollection(SpellResistanceAdjustmentsResource, "symbol.vulnerability.weaker");
    resourceManager.removeFromCollection(SpellResistanceAdjustmentsResource, "mesmerist.planarBinding");
    resourceManager.removeFromCollection(SpellResistanceGrantsResource, "spell.spellResistance");
    resourceManager.removeFromCollection(SpellResistanceGrantsResource, "armor.spellResistance");
    resourceManager.removeFromCollection(SpellResistanceGrantsResource, "archmage.spellResistance");
    assert(specialDefenses.toView().spellResistances.empty());

    resourceManager.addToCollection(FastHealingGrantsResource, FastHealing(FastHealingDefinition{
        .id = "race.fastHealing",
        .source = "Tratto razziale",
        .expression = "1",
        .applicability = std::nullopt,
        .stackingGroup = "fastHealing",
        .tags = {"race"}
    }));
    resourceManager.addToCollection(FastHealingGrantsResource, FastHealing(FastHealingDefinition{
        .id = "spell.rapidRepair",
        .source = "Riparazione Rapida",
        .expression = "5",
        .applicability = std::nullopt,
        .stackingGroup = "fastHealing",
        .tags = {"spell"}
    }));
    resourceManager.addToCollection(FastHealingGrantsResource, FastHealing(FastHealingDefinition{
        .id = "mythic.fastHealing",
        .source = "Guarigione Rapida Mitica",
        .expression = "2",
        .applicability = std::nullopt,
        .stackingGroup = "mythic.fastHealing",
        .tags = {"mythic"}
    }));
    resourceManager.addToCollection(FastHealingGrantsResource, FastHealing(FastHealingDefinition{
        .id = "rage.fastHealing",
        .source = "Sangue di Vita",
        .expression = "8",
        .applicability = " Mentre è in ira ",
        .stackingGroup = "fastHealing",
        .tags = {"class"}
    }));

    SpecialDefensesView recoveryState = specialDefenses.toView();
    assert(recoveryState.fastHealing.size() == 2);
    const FastHealingContextView &generalFastHealing = generalFastHealingView(recoveryState);
    assert(generalFastHealing.effectiveValue == 7);
    assert(generalFastHealing.grants.size() == 3);
    assert(!fastHealingGrantView(generalFastHealing, "race.fastHealing").contributesToEffectiveValue);
    assert(fastHealingGrantView(generalFastHealing, "race.fastHealing").notAppliedReason.has_value());
    assert(fastHealingGrantView(generalFastHealing, "spell.rapidRepair").contributesToEffectiveValue);
    assert(fastHealingGrantView(generalFastHealing, "mythic.fastHealing").contributesToEffectiveValue);
    const FastHealingContextView &rageFastHealing = conditionalFastHealingView(recoveryState, "Mentre è in ira");
    assert(rageFastHealing.effectiveValue == 10);
    assert(rageFastHealing.grants.size() == 4);
    assert(fastHealingGrantView(rageFastHealing, "rage.fastHealing").applicability == "Mentre è in ira");

    resourceManager.removeFromCollection(FastHealingGrantsResource, "spell.rapidRepair");
    assert(generalFastHealingView(specialDefenses.toView()).effectiveValue == 3);
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.addToCollection(FastHealingGrantsResource, FastHealing(FastHealingDefinition{
            .id = "race.fastHealing",
            .source = "Duplicata",
            .expression = "2",
            .applicability = std::nullopt,
            .stackingGroup = "fastHealing",
            .tags = {}
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        FastHealing(FastHealingDefinition{
            .id = "invalid.stackingGroup",
            .source = "Non valida",
            .expression = "1",
            .applicability = std::nullopt,
            .stackingGroup = "   ",
            .tags = {}
        });
    }));
    resourceManager.addToCollection(FastHealingGrantsResource, FastHealing(FastHealingDefinition{
        .id = "invalid.negativeFastHealing",
        .source = "Non valida",
        .expression = "-1",
        .applicability = std::nullopt,
        .stackingGroup = "fastHealing",
        .tags = {}
    }));
    assert(throwsInvalidArgument([&specialDefenses]
    {
        specialDefenses.toView();
    }));
    resourceManager.removeFromCollection(FastHealingGrantsResource, "invalid.negativeFastHealing");
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.removeFromCollection(FastHealingGrantsResource, "missing.fastHealing");
    }));

    resourceManager.addToCollection(RegenerationGrantsResource, Regeneration(RegenerationDefinition{
        .id = "troll.regeneration",
        .source = "Tratto da Troll",
        .expression = "5",
        .interruption = " Acido o fuoco ",
        .applicability = std::nullopt,
        .tags = {"creature"}
    }));
    resourceManager.addToCollection(RegenerationGrantsResource, Regeneration(RegenerationDefinition{
        .id = "artifact.regeneration",
        .source = "Artefatto rigenerante",
        .expression = "@level",
        .interruption = std::nullopt,
        .applicability = "Mentre è indossato",
        .tags = {"item"}
    }));

    recoveryState = specialDefenses.toView();
    assert(recoveryState.regeneration.size() == 2);
    const RegenerationGrantView &trollRegeneration = regenerationGrantView(recoveryState, "troll.regeneration");
    assert(trollRegeneration.resolvedValue == 5);
    assert(trollRegeneration.interruption == "Acido o fuoco");
    assert(trollRegeneration.tags == std::vector<std::string>{"creature"});
    const RegenerationGrantView &artifactRegeneration = regenerationGrantView(recoveryState, "artifact.regeneration");
    assert(artifactRegeneration.resolvedValue == 7);
    assert(!artifactRegeneration.interruption.has_value());
    assert(artifactRegeneration.applicability == "Mentre è indossato");
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.addToCollection(RegenerationGrantsResource, Regeneration(RegenerationDefinition{
            .id = "troll.regeneration",
            .source = "Duplicata",
            .expression = "10",
            .interruption = "Fuoco",
            .applicability = std::nullopt,
            .tags = {}
        }));
    }));
    resourceManager.addToCollection(RegenerationGrantsResource, Regeneration(RegenerationDefinition{
        .id = "invalid.negativeRegeneration",
        .source = "Non valida",
        .expression = "-1",
        .interruption = std::nullopt,
        .applicability = std::nullopt,
        .tags = {}
    }));
    assert(throwsInvalidArgument([&specialDefenses]
    {
        specialDefenses.toView();
    }));
    resourceManager.removeFromCollection(RegenerationGrantsResource, "invalid.negativeRegeneration");
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.removeFromCollection(RegenerationGrantsResource, "missing.regeneration");
    }));

    resourceManager.removeFromCollection(FastHealingGrantsResource, "race.fastHealing");
    resourceManager.removeFromCollection(FastHealingGrantsResource, "mythic.fastHealing");
    resourceManager.removeFromCollection(FastHealingGrantsResource, "rage.fastHealing");
    resourceManager.removeFromCollection(RegenerationGrantsResource, "troll.regeneration");
    resourceManager.removeFromCollection(RegenerationGrantsResource, "artifact.regeneration");
    assert(specialDefenses.toView().fastHealing.empty());
    assert(specialDefenses.toView().regeneration.empty());

    resourceManager.addToCollection(DamageReductionGrantsResource, DamageReduction(DamageReductionDefinition{
        .id = "barbarian.damageReduction",
        .source = "Riduzione del danno del barbaro",
        .expression = "@level",
        .bypass = DamageReductionBypass{.anyOf = {}},
        .applicability = "Mentre è in ira",
        .tags = {"class"}
    }));
    resourceManager.addToCollection(DamageReductionGrantsResource, DamageReduction(DamageReductionDefinition{
        .id = "daemon.damageReduction",
        .source = "Resistenza immonda",
        .expression = "10",
        .bypass = DamageReductionBypass{.anyOf = {
            DamageReductionBypassAlternative{.allOf = {DamageReductionBypassTrait::Good}},
            DamageReductionBypassAlternative{.allOf = {DamageReductionBypassTrait::Silver}}
        }},
        .applicability = std::nullopt,
        .tags = {"creature", "outsider"}
    }));
    resourceManager.addToCollection(DamageReductionGrantsResource, DamageReduction(DamageReductionDefinition{
        .id = "devil.damageReduction",
        .source = "Difesa diabolica",
        .expression = "10",
        .bypass = DamageReductionBypass{.anyOf = {
            DamageReductionBypassAlternative{.allOf = {DamageReductionBypassTrait::Good, DamageReductionBypassTrait::Silver}}
        }},
        .applicability = std::nullopt,
        .tags = {"creature"}
    }));

    SpecialDefensesView damageReductionState = specialDefenses.toView();
    assert(damageReductionState.damageReductions.size() == 3);
    const DamageReductionGrantView &barbarianReduction = damageReductionView(damageReductionState, "barbarian.damageReduction");
    assert(barbarianReduction.baseValue == 7);
    assert(barbarianReduction.effectiveValue == 7);
    assert(barbarianReduction.adjustments.empty());
    assert(barbarianReduction.bypass.anyOf.empty());
    assert(barbarianReduction.applicability == "Mentre è in ira");
    const DamageReductionGrantView &daemonReduction = damageReductionView(damageReductionState, "daemon.damageReduction");
    assert(daemonReduction.bypass.anyOf.size() == 2);
    assert(daemonReduction.bypass.anyOf[0].allOf == std::vector{DamageReductionBypassTrait::Silver});
    assert(daemonReduction.bypass.anyOf[1].allOf == std::vector{DamageReductionBypassTrait::Good});
    const DamageReductionGrantView &devilReduction = damageReductionView(damageReductionState, "devil.damageReduction");
    assert(devilReduction.bypass.anyOf.size() == 1);
    assert(devilReduction.bypass.anyOf[0].allOf == std::vector({DamageReductionBypassTrait::Silver, DamageReductionBypassTrait::Good}));
    assert(displayName(DamageReductionBypassTrait::ColdIron) == "Ferro freddo");
    assert(displayName(DamageReductionBypassTrait::Chaotic) == "Caotico");

    resourceManager.addToCollection(DamageReductionAdjustmentsResource, DamageReductionAdjustment(DamageReductionAdjustmentDefinition{
        .id = "eidolon.damageReduction",
        .source = "Evoluzione Riduzione del Danno",
        .selector = DamageReductionSelector(DamageReductionSelectorDefinition{
            .grantId = std::nullopt,
            .anyTags = {"class"},
            .excludedGrantIds = {}
        }),
        .expression = "@level - 2",
        .stackingGroup = "eidolon.damageReduction"
    }));
    resourceManager.addToCollection(DamageReductionAdjustmentsResource, DamageReductionAdjustment(DamageReductionAdjustmentDefinition{
        .id = "eidolon.damageReduction.weaker",
        .source = "Evoluzione Riduzione del Danno inferiore",
        .selector = DamageReductionSelector(DamageReductionSelectorDefinition{
            .grantId = std::nullopt,
            .anyTags = {"class"},
            .excludedGrantIds = {}
        }),
        .expression = "3",
        .stackingGroup = "eidolon.damageReduction"
    }));
    resourceManager.addToCollection(DamageReductionAdjustmentsResource, DamageReductionAdjustment(DamageReductionAdjustmentDefinition{
        .id = "class.damageReduction.blessing",
        .source = "Benedizione della resistenza",
        .selector = DamageReductionSelector(DamageReductionSelectorDefinition{
            .grantId = std::nullopt,
            .anyTags = {"class"},
            .excludedGrantIds = {}
        }),
        .expression = "2",
        .stackingGroup = "class.damageReduction.blessing"
    }));

    damageReductionState = specialDefenses.toView();
    const DamageReductionGrantView &adjustedBarbarian = damageReductionView(damageReductionState, "barbarian.damageReduction");
    assert(adjustedBarbarian.baseValue == 7);
    assert(adjustedBarbarian.effectiveValue == 14);
    assert(adjustedBarbarian.adjustments.size() == 3);
    assert(damageReductionAdjustmentView(adjustedBarbarian, "eidolon.damageReduction").resolvedDelta == 5);
    assert(damageReductionAdjustmentView(adjustedBarbarian, "eidolon.damageReduction").applied);
    assert(!damageReductionAdjustmentView(adjustedBarbarian, "eidolon.damageReduction.weaker").applied);
    assert(damageReductionAdjustmentView(adjustedBarbarian, "eidolon.damageReduction.weaker").notAppliedReason.has_value());
    assert(damageReductionAdjustmentView(adjustedBarbarian, "class.damageReduction.blessing").applied);
    assert(damageReductionView(damageReductionState, "daemon.damageReduction").adjustments.empty());

    resourceManager.addToCollection(DamageReductionAdjustmentsResource, DamageReductionAdjustment(DamageReductionAdjustmentDefinition{
        .id = "weather.heat",
        .source = "Caldo intenso",
        .selector = DamageReductionSelector(DamageReductionSelectorDefinition{
            .grantId = "barbarian.damageReduction",
            .anyTags = {},
            .excludedGrantIds = {}
        }),
        .expression = "-20",
        .stackingGroup = "weather.heat"
    }));
    resourceManager.addToCollection(DamageReductionAdjustmentsResource, DamageReductionAdjustment(DamageReductionAdjustmentDefinition{
        .id = "weather.heat.weaker",
        .source = "Caldo moderato",
        .selector = DamageReductionSelector(DamageReductionSelectorDefinition{
            .grantId = "barbarian.damageReduction",
            .anyTags = {},
            .excludedGrantIds = {}
        }),
        .expression = "-2",
        .stackingGroup = "weather.heat"
    }));
    damageReductionState = specialDefenses.toView();
    assert(damageReductionView(damageReductionState, "barbarian.damageReduction").effectiveValue == 0);
    assert(damageReductionAdjustmentView(damageReductionView(damageReductionState, "barbarian.damageReduction"), "weather.heat").applied);
    assert(!damageReductionAdjustmentView(damageReductionView(damageReductionState, "barbarian.damageReduction"), "weather.heat.weaker").applied);
    assert(damageReductionAdjustmentView(damageReductionView(damageReductionState, "barbarian.damageReduction"), "weather.heat.weaker").notAppliedReason.has_value());
    resourceManager.removeFromCollection(DamageReductionAdjustmentsResource, "weather.heat");
    resourceManager.removeFromCollection(DamageReductionAdjustmentsResource, "weather.heat.weaker");

    resourceManager.addToCollection(DamageReductionAdjustmentsResource, DamageReductionAdjustment(DamageReductionAdjustmentDefinition{
        .id = "ambiguous.positive",
        .source = "Modifica positiva ambigua",
        .selector = DamageReductionSelector(DamageReductionSelectorDefinition{
            .grantId = "barbarian.damageReduction",
            .anyTags = {},
            .excludedGrantIds = {}
        }),
        .expression = "2",
        .stackingGroup = "ambiguous"
    }));
    resourceManager.addToCollection(DamageReductionAdjustmentsResource, DamageReductionAdjustment(DamageReductionAdjustmentDefinition{
        .id = "ambiguous.negative",
        .source = "Modifica negativa ambigua",
        .selector = DamageReductionSelector(DamageReductionSelectorDefinition{
            .grantId = "barbarian.damageReduction",
            .anyTags = {},
            .excludedGrantIds = {}
        }),
        .expression = "-2",
        .stackingGroup = "ambiguous"
    }));
    assert(throwsInvalidArgument([&specialDefenses]
    {
        specialDefenses.toView();
    }));
    resourceManager.removeFromCollection(DamageReductionAdjustmentsResource, "ambiguous.positive");
    resourceManager.removeFromCollection(DamageReductionAdjustmentsResource, "ambiguous.negative");

    resourceManager.addToCollection(DamageReductionAdjustmentsResource, DamageReductionAdjustment(DamageReductionAdjustmentDefinition{
        .id = "daemon.exactIncrease",
        .source = "Incremento immondo",
        .selector = DamageReductionSelector(DamageReductionSelectorDefinition{
            .grantId = "daemon.damageReduction",
            .anyTags = {"outsider"},
            .excludedGrantIds = {}
        }),
        .expression = "1",
        .stackingGroup = "daemon.exactIncrease"
    }));
    assert(damageReductionView(specialDefenses.toView(), "daemon.damageReduction").effectiveValue == 11);
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.removeFromCollection(DamageReductionGrantsResource, "daemon.damageReduction");
    }));
    resourceManager.removeFromCollection(DamageReductionAdjustmentsResource, "daemon.exactIncrease");

    resourceManager.addToCollection(DamageReductionAdjustmentsResource, DamageReductionAdjustment(DamageReductionAdjustmentDefinition{
        .id = "faerieRobe.increase",
        .source = "Veste della Regina Fatata",
        .selector = DamageReductionSelector(DamageReductionSelectorDefinition{
            .grantId = std::nullopt,
            .anyTags = {"fey"},
            .excludedGrantIds = {}
        }),
        .expression = "10",
        .stackingGroup = "faerieRobe.increase"
    }));
    resourceManager.addToCollection(DamageReductionGrantsResource, DamageReduction(DamageReductionDefinition{
        .id = "fey.damageReduction",
        .source = "Tratto da folletto",
        .expression = "10",
        .bypass = DamageReductionBypass{.anyOf = {
            DamageReductionBypassAlternative{.allOf = {DamageReductionBypassTrait::ColdIron}}
        }},
        .applicability = std::nullopt,
        .tags = {"fey"}
    }));
    assert(damageReductionView(specialDefenses.toView(), "fey.damageReduction").effectiveValue == 20);

    resourceManager.addToCollection(DamageReductionAdjustmentsResource, DamageReductionAdjustment(DamageReductionAdjustmentDefinition{
        .id = "creature.excludingDaemon",
        .source = "Protezione selettiva",
        .selector = DamageReductionSelector(DamageReductionSelectorDefinition{
            .grantId = std::nullopt,
            .anyTags = {"creature"},
            .excludedGrantIds = {"daemon.damageReduction"}
        }),
        .expression = "4",
        .stackingGroup = "creature.excludingDaemon"
    }));
    damageReductionState = specialDefenses.toView();
    assert(damageReductionView(damageReductionState, "daemon.damageReduction").effectiveValue == 10);
    assert(damageReductionView(damageReductionState, "devil.damageReduction").effectiveValue == 14);
    resourceManager.removeFromCollection(DamageReductionAdjustmentsResource, "creature.excludingDaemon");

    resourceManager.addToCollection(DamageReductionGrantsResource, DamageReduction(DamageReductionDefinition{
        .id = "stack.class",
        .source = "Riduzione del danno di classe",
        .expression = "5",
        .bypass = DamageReductionBypass{.anyOf = {}},
        .applicability = std::nullopt,
        .tags = {"stack.class"},
        .stacksWithTags = {"stack.armor"}
    }));
    resourceManager.addToCollection(DamageReductionGrantsResource, DamageReduction(DamageReductionDefinition{
        .id = "stack.armor",
        .source = "Armatura di adamantio",
        .expression = "3",
        .bypass = DamageReductionBypass{.anyOf = {}},
        .applicability = std::nullopt,
        .tags = {"stack.armor"},
        .stacksWithTags = {}
    }));
    resourceManager.addToCollection(DamageReductionGrantsResource, DamageReduction(DamageReductionDefinition{
        .id = "stack.feat",
        .source = "Prestante",
        .expression = "2",
        .bypass = DamageReductionBypass{.anyOf = {}},
        .applicability = std::nullopt,
        .tags = {"stack.feat"},
        .stacksWithTags = {"stack.class"}
    }));
    resourceManager.addToCollection(DamageReductionAdjustmentsResource, DamageReductionAdjustment(DamageReductionAdjustmentDefinition{
        .id = "stack.class.increase",
        .source = "Incremento di classe",
        .selector = DamageReductionSelector(DamageReductionSelectorDefinition{
            .grantId = "stack.class",
            .anyTags = {},
            .excludedGrantIds = {}
        }),
        .expression = "2",
        .stackingGroup = "stack.class.increase"
    }));

    damageReductionState = specialDefenses.toView();
    assert(damageReductionState.damageReductionCombinations.size() == 2);
    const DamageReductionGrantView &stackingClass = damageReductionView(damageReductionState, "stack.class");
    assert(stackingClass.stacksWithTags == std::vector<std::string>{"stack.armor"});
    assert(stackingClass.stackableWithGrantIds == std::vector<std::string>({"stack.armor", "stack.feat"}));
    assert(damageReductionView(damageReductionState, "stack.armor").stackableWithGrantIds == std::vector<std::string>{"stack.class"});
    assert(damageReductionView(damageReductionState, "stack.feat").stackableWithGrantIds == std::vector<std::string>{"stack.class"});
    const DamageReductionCombinationView &classAndArmor = damageReductionCombinationView(damageReductionState, {"stack.class", "stack.armor"});
    assert(classAndArmor.maximumValue == 10);
    assert(classAndArmor.flattenedBypass.has_value());
    assert(classAndArmor.flattenedBypass->anyOf.empty());
    assert(!classAndArmor.flattenedApplicability.has_value());
    assert(classAndArmor.authorizations.size() == 1);
    assert(classAndArmor.authorizations[0].declaringGrantId == "stack.class");
    assert(classAndArmor.authorizations[0].compatibleGrantId == "stack.armor");
    assert(classAndArmor.authorizations[0].matchedTags == std::vector<std::string>{"stack.armor"});
    const DamageReductionCombinationView &classAndFeat = damageReductionCombinationView(damageReductionState, {"stack.class", "stack.feat"});
    assert(classAndFeat.maximumValue == 9);
    assert(classAndFeat.flattenedBypass.has_value());
    assert(std::ranges::none_of(damageReductionState.damageReductionCombinations, [](const DamageReductionCombinationView &combination)
    {
        return combination.grantIds.size() == 3;
    }));

    resourceManager.addToCollection(DamageReductionGrantsResource, DamageReduction(DamageReductionDefinition{
        .id = "stack.magic",
        .source = "Protezione dalla magia",
        .expression = "5",
        .bypass = DamageReductionBypass{.anyOf = {
            DamageReductionBypassAlternative{.allOf = {DamageReductionBypassTrait::Magic}}
        }},
        .applicability = std::nullopt,
        .tags = {"stack.magic"},
        .stacksWithTags = {"stack.untyped"}
    }));
    resourceManager.addToCollection(DamageReductionGrantsResource, DamageReduction(DamageReductionDefinition{
        .id = "stack.untyped",
        .source = "Protezione universale",
        .expression = "2",
        .bypass = DamageReductionBypass{.anyOf = {}},
        .applicability = std::nullopt,
        .tags = {"stack.untyped"},
        .stacksWithTags = {}
    }));
    damageReductionState = specialDefenses.toView();
    const DamageReductionCombinationView &differentBypasses = damageReductionCombinationView(damageReductionState, {"stack.magic", "stack.untyped"});
    assert(differentBypasses.maximumValue == 7);
    assert(!differentBypasses.flattenedBypass.has_value());
    assert(!differentBypasses.flattenedApplicability.has_value());

    resourceManager.addToCollection(DamageReductionGrantsResource, DamageReduction(DamageReductionDefinition{
        .id = "stack.tripleA",
        .source = "Prima protezione cumulabile",
        .expression = "1",
        .bypass = DamageReductionBypass{.anyOf = {}},
        .applicability = std::nullopt,
        .tags = {"stack.tripleA"},
        .stacksWithTags = {"stack.tripleB", "stack.tripleC"}
    }));
    resourceManager.addToCollection(DamageReductionGrantsResource, DamageReduction(DamageReductionDefinition{
        .id = "stack.tripleB",
        .source = "Seconda protezione cumulabile",
        .expression = "2",
        .bypass = DamageReductionBypass{.anyOf = {}},
        .applicability = std::nullopt,
        .tags = {"stack.tripleB"},
        .stacksWithTags = {"stack.tripleC"}
    }));
    resourceManager.addToCollection(DamageReductionGrantsResource, DamageReduction(DamageReductionDefinition{
        .id = "stack.tripleC",
        .source = "Terza protezione cumulabile",
        .expression = "3",
        .bypass = DamageReductionBypass{.anyOf = {}},
        .applicability = std::nullopt,
        .tags = {"stack.tripleC"},
        .stacksWithTags = {}
    }));
    damageReductionState = specialDefenses.toView();
    const DamageReductionCombinationView &completeTriple = damageReductionCombinationView(damageReductionState, {"stack.tripleA", "stack.tripleB", "stack.tripleC"});
    assert(completeTriple.maximumValue == 6);
    assert(completeTriple.flattenedBypass.has_value());
    assert(completeTriple.authorizations.size() == 3);
    assert(std::ranges::none_of(damageReductionState.damageReductionCombinations, [](const DamageReductionCombinationView &combination)
    {
        return combination.grantIds == std::vector<std::string>({"stack.tripleA", "stack.tripleB"})
            || combination.grantIds == std::vector<std::string>({"stack.tripleA", "stack.tripleC"})
            || combination.grantIds == std::vector<std::string>({"stack.tripleB", "stack.tripleC"});
    }));

    assert(throwsInvalidArgument([]
    {
        DamageReduction(DamageReductionDefinition{
            .id = "invalid.duplicateStackingTag",
            .source = "Non valida",
            .expression = "5",
            .bypass = DamageReductionBypass{.anyOf = {}},
            .applicability = std::nullopt,
            .tags = {},
            .stacksWithTags = {"armor", "armor"}
        });
    }));

    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.addToCollection(DamageReductionAdjustmentsResource, DamageReductionAdjustment(DamageReductionAdjustmentDefinition{
            .id = "missing.damageReduction",
            .source = "Bersaglio mancante",
            .selector = DamageReductionSelector(DamageReductionSelectorDefinition{
                .grantId = "missing.damageReduction",
                .anyTags = {},
                .excludedGrantIds = {}
            }),
            .expression = "5",
            .stackingGroup = "missing.damageReduction"
        }));
    }));
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.addToCollection(DamageReductionAdjustmentsResource, DamageReductionAdjustment(DamageReductionAdjustmentDefinition{
            .id = "mismatched.damageReduction",
            .source = "Bersaglio non compatibile",
            .selector = DamageReductionSelector(DamageReductionSelectorDefinition{
                .grantId = "daemon.damageReduction",
                .anyTags = {"class"},
                .excludedGrantIds = {}
            }),
            .expression = "5",
            .stackingGroup = "mismatched.damageReduction"
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        DamageReductionSelector(DamageReductionSelectorDefinition{
            .grantId = "same.damageReduction",
            .anyTags = {},
            .excludedGrantIds = {"same.damageReduction"}
        });
    }));
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.addToCollection(DamageReductionAdjustmentsResource, DamageReductionAdjustment(DamageReductionAdjustmentDefinition{
            .id = "eidolon.damageReduction",
            .source = "Duplicata",
            .selector = DamageReductionSelector{},
            .expression = "5",
            .stackingGroup = "duplicate"
        }));
    }));

    assert(throwsInvalidArgument([]
    {
        DamageReduction(DamageReductionDefinition{
            .id = "invalid.emptyAlternative",
            .source = "Non valida",
            .expression = "5",
            .bypass = DamageReductionBypass{.anyOf = {DamageReductionBypassAlternative{.allOf = {}}}},
            .applicability = std::nullopt,
            .tags = {}
        });
    }));
    assert(throwsInvalidArgument([]
    {
        DamageReduction(DamageReductionDefinition{
            .id = "invalid.duplicateTrait",
            .source = "Non valida",
            .expression = "5",
            .bypass = DamageReductionBypass{.anyOf = {DamageReductionBypassAlternative{.allOf = {
                DamageReductionBypassTrait::Magic,
                DamageReductionBypassTrait::Magic
            }}}},
            .applicability = std::nullopt,
            .tags = {}
        });
    }));
    assert(throwsInvalidArgument([]
    {
        DamageReduction(DamageReductionDefinition{
            .id = "invalid.duplicateAlternative",
            .source = "Non valida",
            .expression = "5",
            .bypass = DamageReductionBypass{.anyOf = {
                DamageReductionBypassAlternative{.allOf = {DamageReductionBypassTrait::Magic}},
                DamageReductionBypassAlternative{.allOf = {DamageReductionBypassTrait::Magic}}
            }},
            .applicability = std::nullopt,
            .tags = {}
        });
    }));
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.addToCollection(DamageReductionGrantsResource, DamageReduction(DamageReductionDefinition{
            .id = "daemon.damageReduction",
            .source = "Duplicata",
            .expression = "5",
            .bypass = DamageReductionBypass{.anyOf = {}},
            .applicability = std::nullopt,
            .tags = {}
        }));
    }));
    resourceManager.addToCollection(DamageReductionGrantsResource, DamageReduction(DamageReductionDefinition{
        .id = "invalid.negative",
        .source = "Non valida",
        .expression = "-1",
        .bypass = DamageReductionBypass{.anyOf = {}},
        .applicability = std::nullopt,
        .tags = {}
    }));
    assert(throwsInvalidArgument([&specialDefenses]
    {
        specialDefenses.toView();
    }));
    resourceManager.removeFromCollection(DamageReductionGrantsResource, "invalid.negative");
    resourceManager.removeFromCollection(DamageReductionGrantsResource, "devil.damageReduction");
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.removeFromCollection(DamageReductionGrantsResource, "devil.damageReduction");
    }));

    resourceManager.addToCollection(EnergyResistanceGrantsResource, EnergyResistance(EnergyResistanceDefinition{
        .id = "tiefling.fire",
        .source = "Resistenze immonde",
        .energy = DamageType::Fire,
        .expression = "5",
        .tags = {"race"}
    }));
    resourceManager.addToCollection(EnergyResistanceGrantsResource, EnergyResistance(EnergyResistanceDefinition{
        .id = "ring.fire",
        .source = "Anello della resistenza all'energia",
        .energy = DamageType::Fire,
        .expression = "10",
        .tags = {"item"}
    }));
    resourceManager.addToCollection(EnergyResistanceGrantsResource, EnergyResistance(EnergyResistanceDefinition{
        .id = "class.cold",
        .source = "Privilegio di classe",
        .energy = DamageType::Cold,
        .expression = "@level",
        .tags = {"class"}
    }));
    resourceManager.addToCollection(EnergyResistanceGrantsResource, EnergyResistance(EnergyResistanceDefinition{
        .id = "void.negative",
        .source = "Vacuità",
        .energy = DamageType::NegativeEnergy,
        .expression = "2",
        .tags = {"class"}
    }));

    SpecialDefensesView view = specialDefenses.toView();
    assert(view.energyResistances.size() == 3);
    const EnergyResistanceView &initialFire = resistanceView(view, DamageType::Fire);
    assert(initialFire.effectiveValue == 10);
    assert(initialFire.grants.size() == 2);
    assert(grantView(initialFire, "ring.fire").determinesEffectiveValue);
    assert(!grantView(initialFire, "tiefling.fire").determinesEffectiveValue);
    assert(grantView(initialFire, "tiefling.fire").notAppliedReason.has_value());
    assert(resistanceView(view, DamageType::Cold).effectiveValue == 7);
    assert(resistanceView(view, DamageType::NegativeEnergy).effectiveValue == 2);

    resourceManager.addToCollection(EnergyResistanceAdjustmentsResource, EnergyResistanceAdjustment(EnergyResistanceAdjustmentDefinition{
        .id = "feat.improvedResistance",
        .source = "Resistenza migliorata",
        .selector = EnergyResistanceSelector(EnergyResistanceSelectorDefinition{
            .energy = DamageType::Fire,
            .grantId = std::nullopt,
            .anyTags = {"race"},
            .excludedGrantIds = {}
        }),
        .expression = "5",
        .stackingGroup = "feat.improvedResistance"
    }));
    resourceManager.addToCollection(EnergyResistanceAdjustmentsResource, EnergyResistanceAdjustment(EnergyResistanceAdjustmentDefinition{
        .id = "feat.improvedResistance.weaker",
        .source = "Resistenza migliorata inferiore",
        .selector = EnergyResistanceSelector(EnergyResistanceSelectorDefinition{
            .energy = DamageType::Fire,
            .grantId = std::nullopt,
            .anyTags = {"race"},
            .excludedGrantIds = {}
        }),
        .expression = "3",
        .stackingGroup = "feat.improvedResistance"
    }));

    view = specialDefenses.toView();
    const EnergyResistanceView &tiedFire = resistanceView(view, DamageType::Fire);
    assert(tiedFire.effectiveValue == 10);
    const EnergyResistanceGrantView &improvedTiefling = grantView(tiedFire, "tiefling.fire");
    assert(improvedTiefling.baseValue == 5);
    assert(improvedTiefling.effectiveValue == 10);
    assert(improvedTiefling.determinesEffectiveValue);
    assert(improvedTiefling.adjustments.size() == 2);
    assert(improvedTiefling.adjustments[0].applied);
    assert(!improvedTiefling.adjustments[1].applied);
    assert(improvedTiefling.adjustments[1].notAppliedReason.has_value());
    assert(grantView(tiedFire, "ring.fire").adjustments.empty());

    resourceManager.addToCollection(EnergyResistanceAdjustmentsResource, EnergyResistanceAdjustment(EnergyResistanceAdjustmentDefinition{
        .id = "blessing.tiefling",
        .source = "Benedizione razziale",
        .selector = EnergyResistanceSelector(EnergyResistanceSelectorDefinition{
            .energy = DamageType::Fire,
            .grantId = "tiefling.fire",
            .anyTags = {},
            .excludedGrantIds = {}
        }),
        .expression = "2",
        .stackingGroup = "blessing.tiefling"
    }));
    view = specialDefenses.toView();
    assert(resistanceView(view, DamageType::Fire).effectiveValue == 12);
    assert(grantView(resistanceView(view, DamageType::Fire), "tiefling.fire").effectiveValue == 12);
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.removeFromCollection(EnergyResistanceGrantsResource, "tiefling.fire");
    }));
    resourceManager.removeFromCollection(EnergyResistanceAdjustmentsResource, "blessing.tiefling");
    resourceManager.removeFromCollection(EnergyResistanceGrantsResource, "tiefling.fire");
    assert(resistanceView(specialDefenses.toView(), DamageType::Fire).effectiveValue == 10);

    resourceManager.addToCollection(EnergyResistanceGrantsResource, EnergyResistance(EnergyResistanceDefinition{
        .id = "heritage.fire",
        .source = "Retaggio elementale",
        .energy = DamageType::Fire,
        .expression = "8",
        .tags = {"race"}
    }));
    assert(resistanceView(specialDefenses.toView(), DamageType::Fire).effectiveValue == 13);

    resourceManager.addToCollection(EnergyResistanceGrantsResource, EnergyResistance(EnergyResistanceDefinition{
        .id = "elementalist.fallback",
        .source = "Evocazione elementalista",
        .energy = DamageType::Fire,
        .expression = "10",
        .tags = {"fallback"}
    }));
    resourceManager.addToCollection(EnergyResistanceAdjustmentsResource, EnergyResistanceAdjustment(EnergyResistanceAdjustmentDefinition{
        .id = "elementalist.increase",
        .source = "Evocazione elementalista",
        .selector = EnergyResistanceSelector(EnergyResistanceSelectorDefinition{
            .energy = DamageType::Fire,
            .grantId = std::nullopt,
            .anyTags = {},
            .excludedGrantIds = {"elementalist.fallback"}
        }),
        .expression = "5",
        .stackingGroup = "elementalist.increase"
    }));
    view = specialDefenses.toView();
    assert(resistanceView(view, DamageType::Fire).effectiveValue == 18);
    assert(grantView(resistanceView(view, DamageType::Fire), "elementalist.fallback").effectiveValue == 10);
    assert(grantView(resistanceView(view, DamageType::Fire), "elementalist.fallback").adjustments.empty());

    assert(throwsInvalidArgument([]
    {
        EnergyResistance(EnergyResistanceDefinition{
            .id = "invalid.physical",
            .source = "Non valido",
            .energy = DamageType::Slashing,
            .expression = "5",
            .tags = {}
        });
    }));
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.addToCollection(EnergyResistanceAdjustmentsResource, EnergyResistanceAdjustment(EnergyResistanceAdjustmentDefinition{
            .id = "missing.exact",
            .source = "Bersaglio mancante",
            .selector = EnergyResistanceSelector(EnergyResistanceSelectorDefinition{
                .energy = DamageType::Fire,
                .grantId = "missing.grant",
                .anyTags = {},
                .excludedGrantIds = {}
            }),
            .expression = "5",
            .stackingGroup = "missing.exact"
        }));
    }));
    assert(throwsInvalidArgument([&resourceManager]
    {
        resourceManager.addToCollection(EnergyResistanceAdjustmentsResource, EnergyResistanceAdjustment(EnergyResistanceAdjustmentDefinition{
            .id = "mismatched.exact",
            .source = "Energia errata",
            .selector = EnergyResistanceSelector(EnergyResistanceSelectorDefinition{
                .energy = DamageType::Cold,
                .grantId = "heritage.fire",
                .anyTags = {},
                .excludedGrantIds = {}
            }),
            .expression = "5",
            .stackingGroup = "mismatched.exact"
        }));
    }));

    level = 12;
    assert(resistanceView(specialDefenses.toView(), DamageType::Cold).effectiveValue == 12);
    const SpecialDefensesView leveledView = specialDefenses.toView();
    const DamageReductionGrantView &leveledDamageReduction = damageReductionView(leveledView, "barbarian.damageReduction");
    assert(leveledDamageReduction.baseValue == 12);
    assert(leveledDamageReduction.effectiveValue == 24);
    assert(damageReductionAdjustmentView(leveledDamageReduction, "eidolon.damageReduction").resolvedDelta == 10);
}
