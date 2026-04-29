package org.golarion.model.character.ability;

import lombok.Getter;

import java.util.Collections;
import java.util.EnumMap;
import java.util.Map;
import java.util.UUID;

@Getter
public class AbilityScores
{
    private final EnumMap<Ability, AbilityScore> scores;

    public AbilityScores()
    {
        this.scores = new EnumMap<>(Ability.class);

        for (Ability ability : Ability.values())
        {
            scores.put(ability, new AbilityScore(10));
        }
    }

    public AbilityScore get(Ability ability)
    {
        if (ability == null)
        {
            throw new IllegalArgumentException("ability must not be null");
        }

        return scores.get(ability);
    }

    public void setBaseValue(Ability ability, int baseValue)
    {
        get(ability).setBaseValue(baseValue);
    }

    public void addBonus(Ability ability, AbilityBonus bonus)
    {
        get(ability).addBonus(bonus);
    }

    public void removeBonus(Ability ability, UUID bonusId)
    {
        get(ability).removeBonus(bonusId);
    }

    public void setBonusEnabled(Ability ability, UUID bonusId, boolean enabled)
    {
        get(ability).setBonusEnabled(bonusId, enabled);
    }

    public void addPenalty(Ability ability, AbilityPenalty penalty)
    {
        get(ability).addPenalty(penalty);
    }

    public void removePenalty(Ability ability, UUID penaltyId)
    {
        get(ability).removePenalty(penaltyId);
    }

    public void setPenaltyEnabled(Ability ability, UUID penaltyId, boolean enabled)
    {
        get(ability).setPenaltyEnabled(penaltyId, enabled);
    }

    public Map<Ability, AbilityScore> getScores()
    {
        return Collections.unmodifiableMap(scores);
    }
}
