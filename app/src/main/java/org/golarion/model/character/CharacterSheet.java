package org.golarion.model.character;

import lombok.Getter;

@Getter
public class CharacterSheet
{
    private String characterName;
    private final AbilityScores abilityScores;

    public CharacterSheet(String characterName)
    {
        this.abilityScores = new AbilityScores();
        setCharacterName(characterName);
    }

    public void setCharacterName(String characterName)
    {
        String normalizedName = normalize(characterName);
        if (normalizedName.isEmpty())
        {
            throw new IllegalArgumentException("characterName must not be blank");
        }

        this.characterName = normalizedName;
    }

    private String normalize(String value)
    {
        return value == null ? "" : value.trim();
    }
}
