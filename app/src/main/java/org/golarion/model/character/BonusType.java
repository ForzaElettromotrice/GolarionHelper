package org.golarion.model.character;

import lombok.Getter;

@Getter
public enum BonusType
{
    ALCHEMICAL("Alchemico", StackingRule.HIGHEST_ONLY),
    ARMOR("Armatura", StackingRule.HIGHEST_ONLY),
    NATURAL_ARMOR("Armatura Naturale", StackingRule.HIGHEST_ONLY),
    CIRCUMSTANCE("Circostanza", StackingRule.STACKS_UNLESS_SAME_SOURCE),
    INSIGHT("Cognitivo", StackingRule.HIGHEST_ONLY),
    COMPETENCE("Competenza", StackingRule.HIGHEST_ONLY),
    DEFLECTION("Deviazione", StackingRule.HIGHEST_ONLY),
    LUCK("Fortuna", StackingRule.HIGHEST_ONLY),
    INHERENT("Intrinseco", StackingRule.HIGHEST_ONLY),
    MORALE("Morale", StackingRule.HIGHEST_ONLY),
    ENHANCEMENT("Potenziamento", StackingRule.HIGHEST_ONLY),
    PROFANE("Profano", StackingRule.HIGHEST_ONLY),
    RACIAL("Razziale", StackingRule.STACKS),
    RESISTANCE("Resistenza", StackingRule.HIGHEST_ONLY),
    SACRED("Sacro", StackingRule.HIGHEST_ONLY),
    DODGE("Schivare", StackingRule.STACKS),
    SHIELD("Scudo", StackingRule.HIGHEST_ONLY),
    SIZE("Taglia", StackingRule.HIGHEST_ONLY);

    private final String displayName;
    private final StackingRule stackingRule;

    BonusType(String displayName, StackingRule stackingRule)
    {
        this.displayName = displayName;
        this.stackingRule = stackingRule;
    }
}
