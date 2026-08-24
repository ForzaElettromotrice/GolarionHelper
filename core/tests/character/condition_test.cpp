#include "golarion/character/condition.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/resource_manager_view.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

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

    template<typename Function>
    bool throwsRuntimeError(Function function)
    {
        try
        {
            function();
            return false;
        }
        catch (const std::runtime_error &)
        {
            return true;
        }
    }

    golarion::ConditionEffectDefinition penaltyEffect(std::string id, std::string expression, int &applications, int &cleanups)
    {
        return golarion::ConditionEffectDefinition{
            .id = std::move(id),
            .description = "Penalità di prova",
            .apply = [expression = std::move(expression), &applications, &cleanups](golarion::ResourceManager &resourceManager, const golarion::ConditionEffectContext &context)
            {
                ++applications;
                golarion::Modifier modifier(golarion::ModifierType::Penalty, context.conditionId, "Penalità di prova", std::nullopt, expression);
                const std::string modifierId = modifier.id();
                resourceManager.addModifier("condition.test", std::move(modifier));
                return [&resourceManager, modifierId, &cleanups]
                {
                    resourceManager.removeModifier("condition.test", modifierId);
                    ++cleanups;
                };
            }
        };
    }
}

int main()
{
    using namespace golarion;

    assert(displayName(ConditionStackingMode::Shared) == "Condivisa");
    assert(displayName(ConditionStackingMode::Escalating) == "Progressiva");
    assert(displayName(ConditionStackingMode::PerEntry) == "Per fonte");
    assert(displayName(ConditionEntryOrigin::Manual) == "Manuale");
    assert(displayName(ConditionEntryOrigin::SourceControlled) == "Controllata dalla fonte");
    assert(displayName(ConditionEntryOrigin::Derived) == "Derivata");

    ResourceManager resourceManager;
    ConditionManager conditionManager(resourceManager);
    const ResourceManagerView resources = resourceManager.toView();
    assert(std::ranges::find(resources.collections, ConditionEntriesResource) != resources.collections.end());

    conditionManager.registerDefinition(Condition(ConditionDefinition{
        .id = "sickened",
        .name = "Infermo",
        .stackingMode = ConditionStackingMode::Shared,
        .stages = {
            ConditionStageDefinition{.id = "sickened", .name = "Infermo"}
        }
    }));
    conditionManager.registerDefinition(Condition(ConditionDefinition{
        .id = "fear",
        .name = "Paura",
        .stackingMode = ConditionStackingMode::Escalating,
        .stages = {
            ConditionStageDefinition{.id = "shaken", .name = "Scosso"},
            ConditionStageDefinition{.id = "frightened", .name = "Spaventato"},
            ConditionStageDefinition{.id = "panicked", .name = "In preda al panico"}
        }
    }));
    assert(!conditionManager.isActive("sickened"));
    assert(conditionManager.entryCount("sickened") == 0);
    assert(conditionManager.effectiveSeverity("sickened") == 0);

    conditionManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
        .id = "table.sickened",
        .conditionId = "sickened",
        .source = "Incantesimo nemico"
    }));
    assert(conditionManager.isActive("sickened"));
    assert(conditionManager.entryCount("sickened") == 1);
    assert(conditionManager.effectiveSeverity("sickened") == 1);
    assert(conditionManager.entryOrigin("table.sickened") == ConditionEntryOrigin::Manual);

    resourceManager.addToCollection(ConditionEntriesResource, ConditionEntry(ConditionEntryDefinition{
        .id = "classFeature.sickened",
        .conditionId = "sickened",
        .source = "Capacità di classe"
    }));
    assert(conditionManager.entryCount("sickened") == 2);
    assert(conditionManager.effectiveSeverity("sickened") == 1);
    assert(conditionManager.entryOrigin("classFeature.sickened") == ConditionEntryOrigin::SourceControlled);

    assert(throwsInvalidArgument([&]
    {
        conditionManager.removeManualEntry("classFeature.sickened");
    }));
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(ConditionEntriesResource, "table.sickened");
    }));

    conditionManager.removeManualEntry("table.sickened");
    assert(conditionManager.entryCount("sickened") == 1);
    resourceManager.removeFromCollection(ConditionEntriesResource, "classFeature.sickened");
    assert(!conditionManager.isActive("sickened"));

    assert(throwsInvalidArgument([&]
    {
        conditionManager.registerDefinition(Condition(ConditionDefinition{
            .id = "sickened",
            .name = "Infermo duplicato",
            .stages = {
                ConditionStageDefinition{.id = "sickened", .name = "Infermo"}
            }
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(Condition(ConditionDefinition{
            .id = "empty",
            .name = "Senza stadi",
            .stages = {}
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(Condition(ConditionDefinition{
            .id = "duplicateStages",
            .name = "Stadi duplicati",
            .stackingMode = ConditionStackingMode::Escalating,
            .stages = {
                ConditionStageDefinition{.id = "same", .name = "Primo"},
                ConditionStageDefinition{.id = "same", .name = "Secondo"}
            }
        }));
    }));
    assert(throwsInvalidArgument([&]
    {
        conditionManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
            .id = "unknown",
            .conditionId = "missing",
            .source = "Test"
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(ConditionEntry(ConditionEntryDefinition{
            .id = "invalidSeverity",
            .conditionId = "fear",
            .source = "Test",
            .severity = 0
        }));
    }));
    conditionManager.registerDefinition(Condition(ConditionDefinition{
        .id = "parameterized",
        .name = "Parametrizzata",
        .stages = {
            ConditionStageDefinition{.id = "parameterized", .name = "Parametrizzata"}
        },
        .entryParameterName = "Valore"
    }));
    assert(throwsInvalidArgument([&]
    {
        conditionManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
            .id = "parameterized.missing",
            .conditionId = "parameterized",
            .source = "Test"
        }));
    }));
    assert(throwsInvalidArgument([&]
    {
        conditionManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
            .id = "sickened.unexpectedParameter",
            .conditionId = "sickened",
            .source = "Test",
            .parameter = "Valore"
        }));
    }));
    conditionManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
        .id = "parameterized.valid",
        .conditionId = "parameterized",
        .source = "Test",
        .parameter = "1d6"
    }));
    const ConditionManagerView parameterizedManagerView = conditionManager.toView();
    const auto stableParameterizedView = std::ranges::find_if(parameterizedManagerView.conditions, [](const ConditionView &condition)
    {
        return condition.id == "parameterized";
    });
    assert(stableParameterizedView != parameterizedManagerView.conditions.end());
    assert(stableParameterizedView->entryParameterName == "Valore");
    assert(stableParameterizedView->entries[0].parameter == "1d6");
    assert(conditionManager.toSaveData().manualEntries[0].parameter == "1d6");
    conditionManager.removeManualEntry("parameterized.valid");

    conditionManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
        .id = "fear.one",
        .conditionId = "fear",
        .source = "Prima fonte",
        .severity = 1,
        .contributesToEscalation = true,
        .stackingGroup = "same effect"
    }));
    conditionManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
        .id = "fear.two",
        .conditionId = "fear",
        .source = "Seconda applicazione della stessa fonte",
        .severity = 2,
        .contributesToEscalation = true,
        .stackingGroup = "same effect"
    }));
    conditionManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
        .id = "fear.three",
        .conditionId = "fear",
        .source = "Fonte indipendente",
        .severity = 1
    }));
    assert(conditionManager.effectiveSeverity("fear") == 3);

    conditionManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
        .id = "fear.minimum",
        .conditionId = "fear",
        .source = "Gravità minima imposta",
        .severity = 4,
        .contributesToEscalation = false
    }));
    assert(conditionManager.effectiveSeverity("fear") == 3);
    conditionManager.removeManualEntry("fear.minimum");
    assert(conditionManager.effectiveSeverity("fear") == 3);
    conditionManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
        .id = "fear.extreme",
        .conditionId = "fear",
        .source = "Fonte di gravità estrema",
        .severity = std::numeric_limits<int>::max()
    }));
    assert(conditionManager.effectiveSeverity("fear") == 3);

    assert(throwsInvalidArgument([&]
    {
        resourceManager.addToCollection(ConditionEntriesResource, ConditionEntry(ConditionEntryDefinition{
            .id = "fear.one",
            .conditionId = "fear",
            .source = "Seconda fonte"
        }));
    }));

    conditionManager.registerDefinition(Condition(ConditionDefinition{
        .id = "negativeLevels",
        .name = "Livelli negativi",
        .stackingMode = ConditionStackingMode::PerEntry,
        .stages = {
            ConditionStageDefinition{.id = "negativeLevel", .name = "Livello negativo"}
        }
    }));
    conditionManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
        .id = "negativeLevels.one",
        .conditionId = "negativeLevels",
        .source = "Prima fonte"
    }));
    resourceManager.addToCollection(ConditionEntriesResource, ConditionEntry(ConditionEntryDefinition{
        .id = "negativeLevels.two",
        .conditionId = "negativeLevels",
        .source = "Seconda fonte",
        .severity = 2
    }));
    assert(conditionManager.entryCount("negativeLevels") == 2);
    assert(conditionManager.effectiveSeverity("negativeLevels") == 3);

    ResourceManager serializedResources;
    ConditionManager serializedManager(serializedResources);
    serializedManager.registerDefinition(Condition(ConditionDefinition{
        .id = "savedCondition",
        .name = "Condizione salvata",
        .stackingMode = ConditionStackingMode::Escalating,
        .stages = {
            ConditionStageDefinition{
                .id = "first",
                .name = "Primo stadio",
                .effects = {
                    ConditionEffectDefinition{
                        .id = "displayedEffect",
                        .description = "Effetto mostrato",
                        .apply = [](ResourceManager &, const ConditionEffectContext &)
                        {
                            return ConditionCleanup([] {});
                        }
                    }
                }
            },
            ConditionStageDefinition{.id = "second", .name = "Secondo stadio"}
        }
    }));
    serializedManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
        .id = "savedCondition.manual",
        .conditionId = "savedCondition",
        .source = "Interfaccia",
        .severity = 2,
        .contributesToEscalation = false,
        .stackingGroup = "same source"
    }));
    serializedResources.addToCollection(ConditionEntriesResource, ConditionEntry(ConditionEntryDefinition{
        .id = "savedCondition.source",
        .conditionId = "savedCondition",
        .source = "Componente"
    }));

    const ConditionManagerView serializedView = serializedManager.toView();
    assert(serializedView.conditions.size() == 1);
    assert(serializedView.conditions[0].id == "savedCondition");
    assert(serializedView.conditions[0].effectiveSeverity == 2);
    assert(serializedView.conditions[0].entries.size() == 2);
    assert(serializedView.conditions[0].stages.size() == 2);
    assert(serializedView.conditions[0].stages[0].active);
    assert(serializedView.conditions[0].stages[1].active);
    assert(serializedView.conditions[0].stages[0].effects.size() == 1);
    assert(serializedView.conditions[0].stages[0].effects[0].description == "Effetto mostrato");
    assert(serializedView.conditions[0].stages[0].effects[0].activeInstances.size() == 1);
    assert(!serializedView.conditions[0].stages[0].effects[0].activeInstances[0].entryId.has_value());

    const ConditionManagerSaveData serializedData = serializedManager.toSaveData();
    assert(serializedData.manualEntries.size() == 1);
    assert(serializedData.manualEntries[0].id == "savedCondition.manual");
    assert(serializedData.manualEntries[0].conditionId == "savedCondition");
    assert(serializedData.manualEntries[0].source == "Interfaccia");
    assert(serializedData.manualEntries[0].severity == 2);
    assert(!serializedData.manualEntries[0].contributesToEscalation);
    assert(serializedData.manualEntries[0].stackingGroup == "same source");

    ResourceManager loadedConditionResources;
    ConditionManager loadedConditionManager(loadedConditionResources);
    loadedConditionManager.registerDefinition(Condition(ConditionDefinition{
        .id = "savedCondition",
        .name = "Condizione salvata",
        .stackingMode = ConditionStackingMode::Escalating,
        .stages = {
            ConditionStageDefinition{.id = "first", .name = "Primo stadio"},
            ConditionStageDefinition{.id = "second", .name = "Secondo stadio"}
        }
    }));
    loadedConditionManager.load(serializedData);
    assert(loadedConditionManager.entryCount("savedCondition") == 1);
    assert(loadedConditionManager.entryOrigin("savedCondition.manual") == ConditionEntryOrigin::Manual);
    assert(loadedConditionManager.effectiveSeverity("savedCondition") == 2);
    assert(throwsInvalidArgument([&]
    {
        loadedConditionManager.load(ConditionManagerSaveData{
            .manualEntries = {
                ConditionEntrySaveData{
                    .id = "savedCondition.additional",
                    .conditionId = "savedCondition",
                    .source = "Valida",
                    .severity = 1,
                    .contributesToEscalation = true,
                    .stackingGroup = std::nullopt
                },
                ConditionEntrySaveData{
                    .id = "missing.entry",
                    .conditionId = "missing",
                    .source = "Non valida",
                    .severity = 1,
                    .contributesToEscalation = true,
                    .stackingGroup = std::nullopt
                }
            }
        });
    }));
    assert(loadedConditionManager.entryCount("savedCondition") == 1);

    ResourceManager derivedResources;
    ConditionManager derivedManager(derivedResources);
    int derivedApplications = 0;
    int derivedCleanups = 0;
    derivedManager.registerDefinition(Condition(ConditionDefinition{
        .id = "helpless",
        .name = "Indifeso",
        .stages = {
            ConditionStageDefinition{
                .id = "helpless",
                .name = "Indifeso",
                .effects = {
                    ConditionEffectDefinition{
                        .id = "test",
                        .description = "Effetto derivato di prova",
                        .apply = [&derivedApplications, &derivedCleanups](ResourceManager &, const ConditionEffectContext &)
                        {
                            ++derivedApplications;
                            return ConditionCleanup([&derivedCleanups]
                            {
                                ++derivedCleanups;
                            });
                        }
                    }
                }
            }
        }
    }));
    derivedManager.registerDefinition(Condition(ConditionDefinition{
        .id = "unconscious",
        .name = "Privo di sensi",
        .stages = {
            ConditionStageDefinition{
                .id = "unconscious",
                .name = "Privo di sensi",
                .derivedConditions = {
                    DerivedConditionDefinition{.conditionId = "helpless"}
                }
            }
        }
    }));
    derivedManager.registerDefinition(Condition(ConditionDefinition{
        .id = "dying",
        .name = "Morente",
        .stages = {
            ConditionStageDefinition{
                .id = "dying",
                .name = "Morente",
                .derivedConditions = {
                    DerivedConditionDefinition{.conditionId = "unconscious"}
                }
            }
        }
    }));

    derivedManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
        .id = "table.dying",
        .conditionId = "dying",
        .source = "Danni"
    }));
    assert(derivedManager.isActive("dying"));
    assert(derivedManager.isActive("unconscious"));
    assert(derivedManager.isActive("helpless"));
    assert(derivedApplications == 1);
    assert(derivedManager.entryOrigin("condition.derived.dying.dying.unconscious") == ConditionEntryOrigin::Derived);
    assert(derivedManager.entryOrigin("condition.derived.unconscious.unconscious.helpless") == ConditionEntryOrigin::Derived);
    assert(throwsInvalidArgument([&]
    {
        derivedManager.removeManualEntry("condition.derived.dying.dying.unconscious");
    }));
    assert(throwsInvalidArgument([&]
    {
        derivedResources.removeFromCollection(ConditionEntriesResource, "condition.derived.dying.dying.unconscious");
    }));

    derivedManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
        .id = "table.unconscious",
        .conditionId = "unconscious",
        .source = "Sonno"
    }));
    derivedManager.removeManualEntry("table.dying");
    assert(!derivedManager.isActive("dying"));
    assert(derivedManager.isActive("unconscious"));
    assert(derivedManager.isActive("helpless"));
    assert(derivedApplications == 1);
    assert(derivedCleanups == 0);
    derivedManager.removeManualEntry("table.unconscious");
    assert(!derivedManager.isActive("unconscious"));
    assert(!derivedManager.isActive("helpless"));
    assert(derivedCleanups == 1);

    derivedManager.registerDefinition(Condition(ConditionDefinition{
        .id = "stageOneResult",
        .name = "Risultato del primo stadio",
        .stages = {
            ConditionStageDefinition{.id = "stageOneResult", .name = "Risultato del primo stadio"}
        }
    }));
    derivedManager.registerDefinition(Condition(ConditionDefinition{
        .id = "stageTwoResult",
        .name = "Risultato del secondo stadio",
        .stages = {
            ConditionStageDefinition{.id = "stageTwoResult", .name = "Risultato del secondo stadio"}
        }
    }));
    derivedManager.registerDefinition(Condition(ConditionDefinition{
        .id = "progressive",
        .name = "Progressiva",
        .stackingMode = ConditionStackingMode::Escalating,
        .stages = {
            ConditionStageDefinition{
                .id = "first",
                .name = "Primo",
                .derivedConditions = {
                    DerivedConditionDefinition{.conditionId = "stageOneResult"}
                }
            },
            ConditionStageDefinition{
                .id = "second",
                .name = "Secondo",
                .derivedConditions = {
                    DerivedConditionDefinition{.conditionId = "stageTwoResult"}
                }
            }
        }
    }));
    derivedManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
        .id = "progressive.one",
        .conditionId = "progressive",
        .source = "Prima fonte"
    }));
    assert(derivedManager.isActive("stageOneResult"));
    assert(!derivedManager.isActive("stageTwoResult"));
    derivedManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
        .id = "progressive.two",
        .conditionId = "progressive",
        .source = "Seconda fonte"
    }));
    assert(derivedManager.isActive("stageOneResult"));
    assert(derivedManager.isActive("stageTwoResult"));
    derivedManager.removeManualEntry("progressive.two");
    assert(derivedManager.isActive("stageOneResult"));
    assert(!derivedManager.isActive("stageTwoResult"));

    ResourceManager invalidDerivedResources;
    ConditionManager invalidDerivedManager(invalidDerivedResources);
    invalidDerivedManager.registerDefinition(Condition(ConditionDefinition{
        .id = "unknownDerivation",
        .name = "Derivazione sconosciuta",
        .stages = {
            ConditionStageDefinition{
                .id = "unknownDerivation",
                .name = "Derivazione sconosciuta",
                .derivedConditions = {
                    DerivedConditionDefinition{.conditionId = "missing"}
                }
            }
        }
    }));
    assert(throwsInvalidArgument([&]
    {
        invalidDerivedManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
            .id = "unknownDerivation.entry",
            .conditionId = "unknownDerivation",
            .source = "Test"
        }));
    }));
    assert(!invalidDerivedManager.isActive("unknownDerivation"));

    invalidDerivedManager.registerDefinition(Condition(ConditionDefinition{
        .id = "cycleA",
        .name = "Ciclo A",
        .stages = {
            ConditionStageDefinition{
                .id = "cycleA",
                .name = "Ciclo A",
                .derivedConditions = {
                    DerivedConditionDefinition{.conditionId = "cycleB"}
                }
            }
        }
    }));
    assert(throwsInvalidArgument([&]
    {
        invalidDerivedManager.registerDefinition(Condition(ConditionDefinition{
            .id = "cycleB",
            .name = "Ciclo B",
            .stages = {
                ConditionStageDefinition{
                    .id = "cycleB",
                    .name = "Ciclo B",
                    .derivedConditions = {
                        DerivedConditionDefinition{.conditionId = "cycleA"}
                    }
                }
            }
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(Condition(ConditionDefinition{
            .id = "duplicateDerivations",
            .name = "Derivazioni duplicate",
            .stages = {
                ConditionStageDefinition{
                    .id = "duplicateDerivations",
                    .name = "Derivazioni duplicate",
                    .derivedConditions = {
                        DerivedConditionDefinition{.conditionId = "same"},
                        DerivedConditionDefinition{.conditionId = "same"}
                    }
                }
            }
        }));
    }));

    assert(throwsInvalidArgument([]
    {
        static_cast<void>(Condition(ConditionDefinition{
            .id = "duplicateEffects",
            .name = "Effetti duplicati",
            .stages = {
                ConditionStageDefinition{
                    .id = "duplicateEffects",
                    .name = "Effetti duplicati",
                    .effects = {
                        ConditionEffectDefinition{.id = "same", .description = "Primo", .apply = [](ResourceManager &, const ConditionEffectContext &)
                        {
                            return ConditionCleanup([] {});
                        }},
                        ConditionEffectDefinition{.id = "same", .description = "Secondo", .apply = [](ResourceManager &, const ConditionEffectContext &)
                        {
                            return ConditionCleanup([] {});
                        }}
                    }
                }
            }
        }));
    }));

    ResourceManager effectResources;
    effectResources.registerEnhanceableResource("condition.test");
    ConditionManager effectManager(effectResources);
    int sharedApplications = 0;
    int sharedCleanups = 0;
    effectManager.registerDefinition(Condition(ConditionDefinition{
        .id = "sharedEffect",
        .name = "Effetto condiviso",
        .stages = {
            ConditionStageDefinition{
                .id = "active",
                .name = "Attivo",
                .effects = {
                    penaltyEffect("penalty", "2", sharedApplications, sharedCleanups)
                }
            }
        }
    }));
    effectManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
        .id = "sharedEffect.one",
        .conditionId = "sharedEffect",
        .source = "Prima fonte"
    }));
    assert(sharedApplications == 1);
    assert(sharedCleanups == 0);
    assert(effectResources.modifierTotal("condition.test") == -2);
    effectManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
        .id = "sharedEffect.two",
        .conditionId = "sharedEffect",
        .source = "Seconda fonte"
    }));
    assert(sharedApplications == 1);
    effectManager.removeManualEntry("sharedEffect.one");
    assert(sharedCleanups == 0);
    assert(effectResources.modifierTotal("condition.test") == -2);
    effectManager.removeManualEntry("sharedEffect.two");
    assert(sharedCleanups == 1);
    assert(effectResources.modifierTotal("condition.test") == 0);

    int escalatingApplications = 0;
    int escalatingCleanups = 0;
    effectManager.registerDefinition(Condition(ConditionDefinition{
        .id = "escalatingEffect",
        .name = "Effetto progressivo",
        .stackingMode = ConditionStackingMode::Escalating,
        .stages = {
            ConditionStageDefinition{
                .id = "first",
                .name = "Primo",
                .effects = {
                    penaltyEffect("firstPenalty", "1", escalatingApplications, escalatingCleanups)
                }
            },
            ConditionStageDefinition{
                .id = "second",
                .name = "Secondo",
                .effects = {
                    penaltyEffect("secondPenalty", "2", escalatingApplications, escalatingCleanups)
                }
            }
        }
    }));
    effectManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
        .id = "escalatingEffect.one",
        .conditionId = "escalatingEffect",
        .source = "Prima fonte"
    }));
    assert(escalatingApplications == 1);
    assert(effectResources.modifierTotal("condition.test") == -1);
    effectManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
        .id = "escalatingEffect.two",
        .conditionId = "escalatingEffect",
        .source = "Seconda fonte"
    }));
    assert(escalatingApplications == 2);
    assert(effectResources.modifierTotal("condition.test") == -3);
    effectManager.removeManualEntry("escalatingEffect.two");
    assert(escalatingCleanups == 1);
    assert(effectResources.modifierTotal("condition.test") == -1);
    effectManager.removeManualEntry("escalatingEffect.one");
    assert(escalatingCleanups == 2);
    assert(effectResources.modifierTotal("condition.test") == 0);

    int perEntryApplications = 0;
    int perEntryCleanups = 0;
    effectManager.registerDefinition(Condition(ConditionDefinition{
        .id = "perEntryEffect",
        .name = "Effetto per fonte",
        .stackingMode = ConditionStackingMode::PerEntry,
        .stages = {
            ConditionStageDefinition{
                .id = "active",
                .name = "Attivo",
                .effects = {
                    penaltyEffect("penalty", "1", perEntryApplications, perEntryCleanups)
                }
            }
        }
    }));
    effectManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
        .id = "perEntryEffect.one",
        .conditionId = "perEntryEffect",
        .source = "Prima fonte"
    }));
    effectResources.addToCollection(ConditionEntriesResource, ConditionEntry(ConditionEntryDefinition{
        .id = "perEntryEffect.two",
        .conditionId = "perEntryEffect",
        .source = "Seconda fonte"
    }));
    assert(perEntryApplications == 2);
    assert(effectResources.modifierTotal("condition.test") == -2);
    effectManager.removeManualEntry("perEntryEffect.one");
    assert(perEntryCleanups == 1);
    assert(effectResources.modifierTotal("condition.test") == -1);
    effectResources.removeFromCollection(ConditionEntriesResource, "perEntryEffect.two");
    assert(perEntryCleanups == 2);
    assert(effectResources.modifierTotal("condition.test") == 0);

    ResourceManager rollbackResources;
    ConditionManager rollbackManager(rollbackResources);
    int rollbackState = 0;
    rollbackManager.registerDefinition(Condition(ConditionDefinition{
        .id = "rollback",
        .name = "Rollback",
        .stackingMode = ConditionStackingMode::Escalating,
        .stages = {
            ConditionStageDefinition{
                .id = "a",
                .name = "Primo",
                .effects = {
                    ConditionEffectDefinition{
                        .id = "apply",
                        .description = "Applicazione riuscita",
                        .apply = [&rollbackState](ResourceManager &, const ConditionEffectContext &)
                        {
                            ++rollbackState;
                            return ConditionCleanup([&rollbackState]
                            {
                                --rollbackState;
                            });
                        }
                    }
                }
            },
            ConditionStageDefinition{
                .id = "b",
                .name = "Secondo",
                .effects = {
                    ConditionEffectDefinition{
                        .id = "fail",
                        .description = "Applicazione fallita",
                        .apply = [](ResourceManager &, const ConditionEffectContext &) -> ConditionCleanup
                        {
                            throw std::runtime_error("effect failed");
                        }
                    }
                }
            }
        }
    }));
    assert(throwsRuntimeError([&]
    {
        rollbackManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
            .id = "rollback.entry",
            .conditionId = "rollback",
            .source = "Test",
            .severity = 2
        }));
    }));
    assert(rollbackState == 0);
    assert(!rollbackManager.isActive("rollback"));

    ResourceManager lifetimeResources;
    lifetimeResources.registerEnhanceableResource("condition.test");
    int lifetimeApplications = 0;
    int lifetimeCleanups = 0;
    {
        ConditionManager lifetimeManager(lifetimeResources);
        lifetimeManager.registerDefinition(Condition(ConditionDefinition{
            .id = "lifetime",
            .name = "Durata del manager",
            .stages = {
                ConditionStageDefinition{
                    .id = "active",
                    .name = "Attivo",
                    .effects = {
                        penaltyEffect("penalty", "1", lifetimeApplications, lifetimeCleanups)
                    }
                }
            }
        }));
        lifetimeManager.addManualEntry(ConditionEntry(ConditionEntryDefinition{
            .id = "lifetime.entry",
            .conditionId = "lifetime",
            .source = "Test"
        }));
        assert(lifetimeResources.modifierTotal("condition.test") == -1);
    }
    assert(lifetimeCleanups == 1);
    assert(lifetimeResources.modifierTotal("condition.test") == 0);
    const ResourceManagerView lifetimeResourceView = lifetimeResources.toView();
    assert(std::ranges::find(lifetimeResourceView.collections, ConditionEntriesResource) == lifetimeResourceView.collections.end());
}
