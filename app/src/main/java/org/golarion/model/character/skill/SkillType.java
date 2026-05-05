package org.golarion.model.character.skill;

import lombok.Getter;
import org.golarion.model.character.ability.AbilityType;

@Getter
public enum SkillType
{
    ACROBATICS("Acrobazia", AbilityType.DEXTERITY, false, false),
    HANDLE_ANIMAL("Addestrare Animali", AbilityType.CHARISMA, true, false),
    CRAFT("Artigianato", AbilityType.INTELLIGENCE, false, true),
    ESCAPE_ARTIST("Artista della Fuga", AbilityType.DEXTERITY, false, false),
    DISGUISE("Camuffare", AbilityType.CHARISMA, false, false),
    RIDE("Cavalcare", AbilityType.DEXTERITY, false, false),
    KNOWLEDGE_ARCANA("Conoscenze (arcane)", AbilityType.INTELLIGENCE, true, false),
    KNOWLEDGE_DUNGEONEERING("Conoscenze (dungeon)", AbilityType.INTELLIGENCE, true, false),
    KNOWLEDGE_ENGINEERING("Conoscenze (ingegneria)", AbilityType.INTELLIGENCE, true, false),
    KNOWLEDGE_GEOGRAPHY("Conoscenze (geografia)", AbilityType.INTELLIGENCE, true, false),
    KNOWLEDGE_HISTORY("Conoscenze (storia)", AbilityType.INTELLIGENCE, true, false),
    KNOWLEDGE_LOCAL("Conoscenze (locali)", AbilityType.INTELLIGENCE, true, false),
    KNOWLEDGE_NATURE("Conoscenze (natura)", AbilityType.INTELLIGENCE, true, false),
    KNOWLEDGE_NOBILITY("Conoscenze (nobilta)", AbilityType.INTELLIGENCE, true, false),
    KNOWLEDGE_PLANES("Conoscenze (piani)", AbilityType.INTELLIGENCE, true, false),
    KNOWLEDGE_RELIGION("Conoscenze (religioni)", AbilityType.INTELLIGENCE, true, false),
    DIPLOMACY("Diplomazia", AbilityType.CHARISMA, false, false),
    DISABLE_DEVICE("Disattivare Congegni", AbilityType.DEXTERITY, true, false),
    STEALTH("Furtività", AbilityType.DEXTERITY, false, false),
    HEAL("Guarire", AbilityType.WISDOM, false, false),
    INTIMIDATE("Intimidire", AbilityType.CHARISMA, false, false),
    PERFORM("Intrattenere", AbilityType.CHARISMA, false, true),
    SENSE_MOTIVE("Intuizione", AbilityType.WISDOM, false, false),
    LINGUISTICS("Linguistica", AbilityType.INTELLIGENCE, true, false),
    SWIM("Nuotare", AbilityType.STRENGTH, false, false),
    PERCEPTION("Percezione", AbilityType.WISDOM, false, false),
    PROFESSION("Professione", AbilityType.WISDOM, true, true),
    BLUFF("Raggirare", AbilityType.CHARISMA, false, false),
    SLEIGHT_OF_HAND("Rapidità di Mano", AbilityType.DEXTERITY, true, false),
    SPELLCRAFT("Sapienza Magica", AbilityType.INTELLIGENCE, true, false),
    CLIMB("Scalare", AbilityType.STRENGTH, false, false),
    SURVIVAL("Sopravvivenza", AbilityType.WISDOM, false, false),
    USE_MAGIC_DEVICE("Utilizzare Congegni Magici", AbilityType.CHARISMA, true, false),
    APPRAISE("Valutare", AbilityType.INTELLIGENCE, false, false),
    FLY("Volare", AbilityType.DEXTERITY, false, false);

    private final String displayName;
    private final AbilityType keyAbility;
    private final boolean trainedOnly;
    private final boolean requiresSpecialization;

    SkillType(
            String displayName,
            AbilityType keyAbility,
            boolean trainedOnly,
            boolean requiresSpecialization
    )
    {
        this.displayName = displayName;
        this.keyAbility = keyAbility;
        this.trainedOnly = trainedOnly;
        this.requiresSpecialization = requiresSpecialization;
    }
}
