#pragma once

namespace golarion
{
    class Condition;
    class ConditionManager;

    Condition makeBleedingCondition();
    Condition makeBlindedCondition();
    Condition makeCoweringCondition();
    Condition makeConfusedCondition();
    Condition makeDazedCondition();
    Condition makeDazzledCondition();
    Condition makeDeafenedCondition();
    Condition makeDisabledCondition();
    Condition makeDyingCondition();
    Condition makeDeadCondition();
    Condition makeEntangledCondition();
    Condition makeFascinatedCondition();
    Condition makeFatigueCondition();
    Condition makeFearCondition();
    Condition makeFlatFootedCondition();
    Condition makeGrappledCondition();
    Condition makeHelplessCondition();
    Condition makeIncorporealCondition();
    Condition makeInvisibleCondition();
    Condition makeNegativeLevelsCondition();
    Condition makeNauseatedCondition();
    Condition makeParalyzedCondition();
    Condition makePetrifiedCondition();
    Condition makePinnedCondition();
    Condition makeProneCondition();
    Condition makeSickenedCondition();
    Condition makeStableCondition();
    Condition makeStaggeredCondition();
    Condition makeStunnedCondition();
    Condition makeUnconsciousCondition();
    void registerCanonicalConditions(ConditionManager &manager);
}
