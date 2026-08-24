#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

namespace golarion
{
    void registerCanonicalConditions(ConditionManager &manager)
    {
        manager.registerDefinition(makeBleedingCondition());
        manager.registerDefinition(makeHelplessCondition());
        manager.registerDefinition(makeFlatFootedCondition());
        manager.registerDefinition(makeUnconsciousCondition());
        manager.registerDefinition(makeBlindedCondition());
        manager.registerDefinition(makeCoweringCondition());
        manager.registerDefinition(makeConfusedCondition());
        manager.registerDefinition(makeDazedCondition());
        manager.registerDefinition(makeDazzledCondition());
        manager.registerDefinition(makeDeadCondition());
        manager.registerDefinition(makeDeafenedCondition());
        manager.registerDefinition(makeDisabledCondition());
        manager.registerDefinition(makeDyingCondition());
        manager.registerDefinition(makeEntangledCondition());
        manager.registerDefinition(makeFascinatedCondition());
        manager.registerDefinition(makeFatigueCondition());
        manager.registerDefinition(makeFearCondition());
        manager.registerDefinition(makeGrappledCondition());
        manager.registerDefinition(makeIncorporealCondition());
        manager.registerDefinition(makeInvisibleCondition());
        manager.registerDefinition(makeNegativeLevelsCondition());
        manager.registerDefinition(makeNauseatedCondition());
        manager.registerDefinition(makeParalyzedCondition());
        manager.registerDefinition(makePetrifiedCondition());
        manager.registerDefinition(makePinnedCondition());
        manager.registerDefinition(makeProneCondition());
        manager.registerDefinition(makeSickenedCondition());
        manager.registerDefinition(makeStableCondition());
        manager.registerDefinition(makeStaggeredCondition());
        manager.registerDefinition(makeStunnedCondition());
    }
}
