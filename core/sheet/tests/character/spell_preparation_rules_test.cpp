#include "golarion/character/prepared_spellcasting.hpp"
#include "golarion/character/spell_definition_manager.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <cassert>
#include <stdexcept>

namespace
{
    template<typename Exception = std::invalid_argument, typename Function> void rejects(Function function)
    {
        bool rejected = false;
        try { function(); }
        catch (const Exception &) { rejected = true; }
        assert(rejected);
    }
}

int main()
{
    using namespace golarion;
    ResourceManager resources;
    const auto levels = spellLevelOverridesResource("test");
    const auto checks = preparationChecksResource("test");
    const auto exemptions = preparationCheckExemptionsResource("test");
    const auto consumption = consumptionExemptionsResource("test");
    const auto conversions = spontaneousConversionsResource("test");
    const auto slots = spellSlotsResource("test");
    const SpellSlotId base{"base", 3, 0}, special{"special", 3, 0};
    const auto never = [](const SpellPreparationContext &) { return false; };
    const auto always = [](const SpellPreparationContext &) { return true; };
    const auto preserve = [](const SpellConsumptionContext &) { return true; };
    bool bookAvailable = false;
    int abilityScore = 12;

    {
        // Cleric has no ordinary fireball level; the special group supplies it.
        PreparedSpellcasting casting(resources, "test", CharacterClassType::Cleric);
        resources.addToCollection(slots, SpellSlotGrant{"base", "Class", {{3, 2}, {4, 1}}});
        resources.addToCollection(slots, SpellSlotGrant{"special", "Domain", {{3, 1}}, std::set<std::string>{"fireball"}});
        resources.addToCollection(slots, SpellSlotGrant{"blocked", "Class", {{3, 1}}, std::set<std::string>{}});
        const SpellLevelOverride domain{"domain", "Domain", "special", "fireball", 3};
        resources.addToCollection(levels, domain);
        rejects([&] { casting.prepare(special, {"fireball"}); });
        resources.addToCollection(repertoireGrantsResource("test"), SpellRepertoireGrant{"domain", "Domain", {"fireball"}});
        rejects([&] { casting.prepare(base, {"fireball"}); });
        casting.prepare(special, {"fireball"});
        casting.clear(special);
        resources.addToCollection(levels, SpellLevelOverride{"blocked", "Test", "blocked", "fireball", 3});
        rejects([&] { casting.prepare({"blocked", 3, 0}, {"fireball"}); });

        // Normalization, compatible duplicate sources, and conflicting overrides.
        resources.addToCollection(levels, SpellLevelOverride{" duplicate ", "Domain", " special ", " fireball ", 3});
        resources.removeFromCollection(levels, "domain");
        casting.prepare(special, {"fireball"});
        casting.clear(special);
        resources.addToCollection(levels, SpellLevelOverride{"conflict", "Test", "special", "fireball", 2});
        rejects([&] { casting.prepare(special, {"fireball"}); });
        assert(casting.toData().preparations.empty());
        resources.removeFromCollection(levels, "conflict");
        resources.removeFromCollection(levels, " duplicate ");
        rejects([&] { casting.prepare(special, {"fireball"}); });
        resources.addToCollection(levels, domain);

        resources.addToCollection(checks, SpellPreparationCheck{"book", "Class", [&](const auto &context)
        {
            assert(context.spell.id == "fireball" && context.spellLevel == 3);
            assert(context.spellListClass == CharacterClassType::Cleric);
            assert(context.group.id == context.slot.groupId);
            return bookAvailable;
        }});
        resources.addToCollection(checks, SpellPreparationCheck{"ability", "Class", [&](const auto &context)
        {
            return abilityScore >= 10 + context.spellLevel;
        }});
        rejects([&] { casting.prepare(special, {"fireball"}); });
        resources.addToCollection(exemptions, SpellPreparationCheckExemption{"mastery", "Feat", "book", always});
        rejects([&] { casting.prepare(special, {"fireball"}); }); // Does not exempt ability.
        abilityScore = 13;
        casting.prepare(special, {"fireball"});
        const auto saved = casting.toData();
        casting.clear(special);
        resources.removeFromCollection(exemptions, "mastery");
        rejects([&] { casting.prepare(special, {"fireball"}); });
        casting.load(saved); // Restoring is not preparing again; book may be absent.
        assert(casting.slotState(special).state == PreparedSpellSlotState::Prepared);
        casting.clear(special);
        bookAvailable = true;
        casting.prepare(special, {"fireball"});
        casting.clear(special);
        resources.addToCollection(exemptions, SpellPreparationCheckExemption{"early", "Feat", "future", always});
        resources.addToCollection(checks, SpellPreparationCheck{"future", "Class", never});
        casting.prepare(special, {"fireball"});
        casting.clear(special);
        resources.addToCollection(exemptions, SpellPreparationCheckExemption{"second", "Feat", "future", always});
        resources.removeFromCollection(exemptions, "early");
        casting.prepare(special, {"fireball"});
        casting.clear(special);
        resources.removeFromCollection(exemptions, "second");
        resources.addToCollection(exemptions, SpellPreparationCheckExemption{"conditional", "Feat", "future", never});
        rejects([&] { casting.prepare(special, {"fireball"}); });
        resources.removeFromCollection(checks, "future");

        // A failure in a later linked slot must not reserve the first one.
        resources.addToCollection(levels, SpellLevelOverride{"base", "Test", "base", "fireball", 3});
        resources.addToCollection(preparationCostRulesResource("test"), SpellPreparationCostRule{"opposition", "Test", SpellSchool::Evocation, 2});
        const std::vector<SpellSlotId> pair{base, special};
        resources.addToCollection(checks, SpellPreparationCheck{"restricted", "Test", [](const auto &context) { return context.group.id == "base"; }});
        rejects([&] { casting.prepare(pair, {"fireball"}); });
        assert(casting.toData().preparations.empty());
        resources.removeFromCollection(checks, "restricted");
        resources.addToCollection(checks, SpellPreparationCheck{"throws", "Test", [](const auto &) -> bool { throw std::runtime_error("check failed"); }});
        rejects<std::runtime_error>([&] { casting.prepare(pair, {"fireball"}); });
        assert(casting.toData().preparations.empty());
        resources.removeFromCollection(checks, "throws");
        casting.prepare(pair, {"fireball"});

        resources.addToCollection(consumption, SpellConsumptionExemption{"throws", "Test", [](const auto &) -> bool { throw std::runtime_error("consumption failed"); }});
        rejects<std::runtime_error>([&] { casting.consume(base); });
        assert(casting.slotState(special).state == PreparedSpellSlotState::Prepared);
        resources.removeFromCollection(consumption, "throws");
        resources.addToCollection(consumption, SpellConsumptionExemption{"atWill", "Test", [&](const auto &context)
        {
            assert(context.slots.size() == 2 && context.slots.front() == base);
            return context.spell.id == "fireball";
        }});
        casting.consume(special);
        casting.consume(base);
        assert(casting.slotState(base).state == PreparedSpellSlotState::Prepared);
        assert(casting.slotState(special).state == PreparedSpellSlotState::Prepared);
        resources.addToCollection(consumption, SpellConsumptionExemption{"second", "Test", preserve});
        resources.removeFromCollection(consumption, "atWill");
        casting.consume(base);
        assert(casting.slotState(base).state == PreparedSpellSlotState::Prepared);
        resources.removeFromCollection(consumption, "second");
        resources.addToCollection(consumption, SpellConsumptionExemption{"inapplicable", "Test", [](const auto &) { return false; }});
        casting.consume(base);
        assert(casting.slotState(special).state == PreparedSpellSlotState::Consumed);
        resources.addToCollection(consumption, SpellConsumptionExemption{"atWill", "Test", preserve});
        rejects([&] { casting.consume(special); }); // An exemption cannot revive a spent spell.
        casting.clear(special);
        rejects([&] { casting.consume(base); });

        PreparedSpellcasting other(resources, "otherRules", CharacterClassType::Cleric);
        resources.addToCollection(spellSlotsResource("otherRules"), SpellSlotGrant{"special", "Test", {{3, 1}}, std::set<std::string>{"fireball"}});
        resources.addToCollection(repertoireGrantsResource("otherRules"), SpellRepertoireGrant{"gift", "Test", {"fireball"}});
        rejects([&] { other.prepare(special, {"fireball"}); }); // Overrides do not leak.

        rejects([&] { resources.addToCollection(levels, domain); });
        rejects([&] { resources.addToCollection(levels, SpellLevelOverride{"bad", "Test", "special", "fireball", 10}); });
        rejects([&] { resources.addToCollection(levels, SpellLevelOverride{"bad", "Test", "special", "fireball", -1}); });
        rejects([&] { resources.addToCollection(levels, SpellLevelOverride{"bad", "Test", " ", "fireball", 3}); });
        rejects([&] { resources.addToCollection(levels, SpellLevelOverride{"bad", "Test", "special", " ", 3}); });
        rejects([&] { resources.addToCollection(checks, SpellPreparationCheck{" ", "Test", always}); });
        rejects([&] { resources.addToCollection(checks, SpellPreparationCheck{"bad", " ", always}); });
        rejects([&] { resources.addToCollection(checks, SpellPreparationCheck{"bad", "Test", {}}); });
        rejects([&] { resources.addToCollection(exemptions, SpellPreparationCheckExemption{"bad", "Test", "book", {}}); });
        rejects([&] { resources.addToCollection(exemptions, SpellPreparationCheckExemption{"bad", "Test", " ", always}); });
        rejects([&] { resources.addToCollection(consumption, SpellConsumptionExemption{"bad", "Test", {}}); });
        rejects([&] { resources.addToCollection(checks, domain); });
        rejects([&] { resources.removeFromCollection(checks, "unknown"); });
    }

    // Destruction and partial construction release only owned endpoints.
    for (const auto &endpoint : {levels, checks, exemptions, consumption, conversions})
        rejects([&] { resources.removeFromCollection(endpoint, "anything"); });
    for (const auto &collision : {levels, checks, exemptions, consumption, conversions})
    {
        resources.registerCollectionResource<int>(collision, [](int) {}, [](std::string_view) {});
        rejects([&] { PreparedSpellcasting fails(resources, "test", CharacterClassType::Wizard); });
        resources.addToCollection(collision, 1);
        resources.unregisterCollectionResource(collision);
        PreparedSpellcasting retry(resources, "test", CharacterClassType::Wizard);
    }

    {
        PreparedSpellcasting casting(resources, "test", CharacterClassType::Wizard);
        resources.addToCollection(slots, SpellSlotGrant{"base", "Class", {{3, 2}, {4, 1}}});
        resources.addToCollection(slots, SpellSlotGrant{"domain", "Domain", {{3, 1}}});
        casting.addToRepertoire("fireball");
        casting.prepare(base, {"fireball"});
        rejects([&] { casting.consume(base, {"fireball"}); }); // Explicit conversion needs a grant, even to the same spell.
        rejects([&] { casting.consume(base, {" "}); });
        rejects([&] { casting.consume(base, {"unknown"}); });
        assert(casting.slotState(base).state == PreparedSpellSlotState::Prepared);
        int requiredLevel = 4;
        SpontaneousConversion conversion{"convert", "Test", [&](const SpellConversionContext &context) -> std::optional<int>
        {
            assert(context.preparedSpell.id == "fireball");
            if (context.replacementSpell.id != "fireball") return std::nullopt;
            for (const auto &slot : context.slots)
                if (context.slotPools.at(slot.groupId).source == "Domain") return std::nullopt;
            return requiredLevel;
        }};
        resources.addToCollection(conversions, conversion);
        rejects([&] { casting.consume(base, {"fireball"}); });
        requiredLevel = -1;
        rejects([&] { casting.consume(base, {"fireball"}); });
        requiredLevel = 10;
        rejects([&] { casting.consume(base, {"fireball"}); });
        requiredLevel = 3;
        resources.addToCollection(consumption, SpellConsumptionExemption{"atWill", "Test", preserve});
        casting.consume(base); // Ordinary consumption still honors exemptions.
        assert(casting.slotState(base).state == PreparedSpellSlotState::Prepared);
        casting.consume(base, {" fireball "}); // Sacrifice does not honor normal-use exemptions.
        assert(casting.slotState(base).state == PreparedSpellSlotState::Consumed);
        assert(casting.toData().preparations[0].spellId == "fireball");
        rejects([&] { casting.consume(base, {"fireball"}); });
        casting.clear(base);
        rejects([&] { casting.consume(base, {"fireball"}); });
        resources.addToCollection(preparationCostRulesResource("test"), SpellPreparationCostRule{"opposition", "Test", SpellSchool::Evocation, 2});
        const SpellSlotId second{"base", 3, 1}, domainSlot{"domain", 3, 0};
        casting.prepare(std::vector<SpellSlotId>{base, domainSlot}, {"fireball"});
        rejects([&] { casting.consume(base, {"fireball"}); });
        assert(casting.slotState(domainSlot).state == PreparedSpellSlotState::Prepared);
        casting.clear(base);
        casting.prepare(std::vector<SpellSlotId>{base, second}, {"fireball"});
        resources.removeFromCollection(conversions, "convert");
        rejects([&] { casting.consume(second, {"fireball"}); });
        resources.addToCollection(conversions, SpontaneousConversion{"throws", "Test", [](const auto &) -> std::optional<int> { throw std::runtime_error("conversion failed"); }});
        rejects<std::runtime_error>([&] { casting.consume(base, {"fireball"}); });
        assert(casting.slotState(second).state == PreparedSpellSlotState::Prepared);
        resources.removeFromCollection(conversions, "throws");
        resources.addToCollection(conversions, SpontaneousConversion{"denied", "Test", [](const auto &) -> std::optional<int> { return std::nullopt; }});
        resources.addToCollection(conversions, conversion);
        casting.consume(second, {"fireball"});
        assert(casting.slotState(base).state == PreparedSpellSlotState::Consumed);
        assert(casting.slotState(second).state == PreparedSpellSlotState::Consumed);
        rejects([&] { resources.addToCollection(conversions, conversion); });
        rejects([&] { resources.addToCollection(conversions, SpontaneousConversion{"empty", "Test", {}}); });
        conversion.id = " ";
        rejects([&] { resources.addToCollection(conversions, conversion); });

        PreparedSpellcasting other(resources, "isolatedConversion", CharacterClassType::Wizard);
        resources.addToCollection(spellSlotsResource("isolatedConversion"), SpellSlotGrant{"base", "Class", {{3, 1}}});
        other.addToRepertoire("fireball");
        other.prepare(base, {"fireball"});
        rejects([&] { other.consume(base, {"fireball"}); });
    }

    // Distinct spell definitions exercise replacement identity without expanding the catalog.
    {
        SpellPreparationRules rules(resources, "test");
        const auto &prepared = SpellDefinitionManager::instance().get("fireball");
        auto replacement = prepared;
        replacement.id = "replacement";
        const std::vector<SpellSlotId> payment{base};
        const std::map<std::string, SpellSlotGrant> pools{{"base", {"base", "Class", {{3, 1}}}}};
        const SpellConversionContext context{CharacterClassType::Wizard, prepared, replacement, payment, pools};
        resources.addToCollection(conversions, SpontaneousConversion{"grant", "Test", [](const auto &query) -> std::optional<int>
        {
            if (query.preparedSpell.id == "fireball" && query.replacementSpell.id == "replacement") return 2;
            return std::nullopt;
        }});
        assert(rules.allowsConversion(context));
        replacement.id = "not-granted";
        assert(!rules.allowsConversion(context));
    }
}
