#include "golarion/character/spell_preparation_cost.hpp"
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
    const auto rules = preparationCostRulesResource("wizard");
    const auto exemptions = preparationCostExemptionsResource("wizard");
    const SpellPreparationCostRule opposition{"opposition.evocation", "Wizard", SpellSchool::Evocation, 2};
    {
        SpellPreparationCostRegistry registry(resources, " wizard ");
        SpellPreparationCostRegistry other(resources, "other");
        assert(registry.calculate(SpellSchool::Evocation, 3).minimumSlotLevel == 3);
        assert(registry.calculate(SpellSchool::Evocation, 3).requiredSlots == 1);
        resources.addToCollection(rules, opposition);
        assert(registry.calculate(SpellSchool::Evocation, 3).requiredSlots == 2);
        assert(registry.calculate(SpellSchool::Evocation, 0).requiredSlots == 2);
        assert(registry.calculate(SpellSchool::Conjuration, 3).requiredSlots == 1);
        assert(other.calculate(SpellSchool::Evocation, 3).requiredSlots == 1);
        resources.addToCollection(rules, SpellPreparationCostRule{"opposition.necromancy", "Wizard", SpellSchool::Necromancy, 2});
        resources.addToCollection(exemptions, SpellPreparationCostExemption{" research ", " Research ", " opposition.evocation "});
        assert(registry.calculate(SpellSchool::Evocation, 3).requiredSlots == 1);
        assert(registry.calculate(SpellSchool::Necromancy, 3).requiredSlots == 2);
        resources.removeFromCollection(exemptions, " research ");
        assert(registry.calculate(SpellSchool::Evocation, 3).requiredSlots == 2);

        // Requirements combine independently of their insertion order, by maximum.
        resources.addToCollection(rules, SpellPreparationCostRule{"another", "Other", SpellSchool::Evocation, 3});
        assert(registry.calculate(SpellSchool::Evocation, 3).requiredSlots == 3);
        resources.addToCollection(exemptions, SpellPreparationCostExemption{"research", "Research", opposition.id});
        assert(registry.calculate(SpellSchool::Evocation, 3).requiredSlots == 3);
        resources.removeFromCollection(rules, "another");
        assert(registry.calculate(SpellSchool::Evocation, 3).requiredSlots == 1);
        resources.removeFromCollection(rules, opposition.id);
        resources.addToCollection(rules, opposition);
        assert(registry.calculate(SpellSchool::Evocation, 3).requiredSlots == 1);
        resources.addToCollection(exemptions, SpellPreparationCostExemption{"research2", "Second source", opposition.id});
        resources.removeFromCollection(exemptions, "research");
        assert(registry.calculate(SpellSchool::Evocation, 3).requiredSlots == 1);
        resources.removeFromCollection(exemptions, "research2");
        assert(registry.calculate(SpellSchool::Evocation, 3).requiredSlots == 2);

        // An exemption can arrive before its source rule.
        resources.addToCollection(preparationCostExemptionsResource("other"), SpellPreparationCostExemption{"research", "Research", opposition.id});
        resources.addToCollection(preparationCostRulesResource("other"), opposition);
        assert(other.calculate(SpellSchool::Evocation, 3).requiredSlots == 1);

        rejects([&] { resources.addToCollection(rules, opposition); });
        rejects([&] { SpellPreparationCostRegistry duplicate(resources, "wizard"); });
        rejects([&] { resources.addToCollection(rules, 1); });
        rejects([&] { resources.addToCollection(exemptions, opposition); });
        rejects([&] { resources.removeFromCollection(rules, "missing"); });
        rejects([&] { resources.removeFromCollection(exemptions, "missing"); });
        for (int count : {0, -1})
            rejects([&] { resources.addToCollection(rules, SpellPreparationCostRule{"bad", "Source", SpellSchool::Evocation, count}); });
        for (int invalid : {-1, 999})
        {
            rejects([&] { resources.addToCollection(rules, SpellPreparationCostRule{"bad", "Source", static_cast<SpellSchool>(invalid), 2}); });
            rejects([&] { registry.calculate(static_cast<SpellSchool>(invalid), 3); });
        }
        for (int invalid : {-1, 10}) rejects([&] { registry.calculate(SpellSchool::Evocation, invalid); });
        rejects([&] { resources.addToCollection(rules, SpellPreparationCostRule{" ", "Source", SpellSchool::Evocation, 2}); });
        rejects([&] { resources.addToCollection(rules, SpellPreparationCostRule{"bad", " ", SpellSchool::Evocation, 2}); });
        rejects([&] { resources.addToCollection(exemptions, SpellPreparationCostExemption{"bad", "Source", " "}); });
        rejects([&] { resources.addToCollection(exemptions, SpellPreparationCostExemption{" ", "Source", opposition.id}); });
        rejects([&] { resources.addToCollection(exemptions, SpellPreparationCostExemption{"bad", " ", opposition.id}); });
        resources.addToCollection(exemptions, SpellPreparationCostExemption{"research", "Research", opposition.id});
        rejects([&] { resources.addToCollection(exemptions, SpellPreparationCostExemption{" research ", "Research", opposition.id}); });
        assert(registry.calculate(SpellSchool::Evocation, 3).requiredSlots == 1);
    }
    rejects([&] { resources.addToCollection(rules, opposition); });
    rejects([&] { resources.addToCollection(exemptions, SpellPreparationCostExemption{"research", "Research", opposition.id}); });
    {
        SpellPreparationCostRegistry reused(resources, "wizard");
        assert(reused.calculate(SpellSchool::Evocation, 3).requiredSlots == 1);
    }
    resources.registerCollectionResource<int>(exemptions, [](int) {}, [](std::string_view) {});
    rejects([&] { SpellPreparationCostRegistry failed(resources, "wizard"); });
    // Failed construction must roll back the first endpoint without deleting the collision.
    rejects([&] { resources.addToCollection(rules, opposition); });
    resources.addToCollection(exemptions, 1);
    resources.unregisterCollectionResource(exemptions);
    SpellPreparationCostRegistry recovered(resources, "wizard");
    resources.addToCollection(rules, opposition);
    assert(recovered.calculate(SpellSchool::Evocation, 3).requiredSlots == 2);
}
