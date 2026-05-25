package org.golarion.model.character.skill;

import lombok.Getter;
import org.golarion.model.character.ability.AbilityType;

@Getter
public enum SkillType
{
    ACROBATICS("Acrobazia", AbilityType.DEXTERITY, false, false, true),
    HANDLE_ANIMAL("Addestrare Animali", AbilityType.CHARISMA, true, false, false),
    CRAFT("Artigianato", AbilityType.INTELLIGENCE, false, true, false),
    ESCAPE_ARTIST("Artista della Fuga", AbilityType.DEXTERITY, false, false, true),
    DISGUISE("Camuffare", AbilityType.CHARISMA, false, false, false),
    RIDE("Cavalcare", AbilityType.DEXTERITY, false, false, true),
    KNOWLEDGE_ARCANA("Conoscenze (arcane)", AbilityType.INTELLIGENCE, true, false, false),
    KNOWLEDGE_DUNGEONEERING("Conoscenze (dungeon)", AbilityType.INTELLIGENCE, true, false, false),
    KNOWLEDGE_ENGINEERING("Conoscenze (ingegneria)", AbilityType.INTELLIGENCE, true, false, false),
    KNOWLEDGE_GEOGRAPHY("Conoscenze (geografia)", AbilityType.INTELLIGENCE, true, false, false),
    KNOWLEDGE_HISTORY("Conoscenze (storia)", AbilityType.INTELLIGENCE, true, false, false),
    KNOWLEDGE_LOCAL("Conoscenze (locali)", AbilityType.INTELLIGENCE, true, false, false),
    KNOWLEDGE_NATURE("Conoscenze (natura)", AbilityType.INTELLIGENCE, true, false, false),
    KNOWLEDGE_NOBILITY("Conoscenze (nobilta)", AbilityType.INTELLIGENCE, true, false, false),
    KNOWLEDGE_PLANES("Conoscenze (piani)", AbilityType.INTELLIGENCE, true, false, false),
    KNOWLEDGE_RELIGION("Conoscenze (religioni)", AbilityType.INTELLIGENCE, true, false, false),
    DIPLOMACY("Diplomazia", AbilityType.CHARISMA, false, false, false),
    DISABLE_DEVICE("Disattivare Congegni", AbilityType.DEXTERITY, true, false, true),
    STEALTH("Furtività", AbilityType.DEXTERITY, false, false, true),
    HEAL("Guarire", AbilityType.WISDOM, false, false, false),
    INTIMIDATE("Intimidire", AbilityType.CHARISMA, false, false, false),
    PERFORM("Intrattenere", AbilityType.CHARISMA, false, true, false),
    SENSE_MOTIVE("Intuizione", AbilityType.WISDOM, false, false, false),
    LINGUISTICS("Linguistica", AbilityType.INTELLIGENCE, true, false, false),
    SWIM("Nuotare", AbilityType.STRENGTH, false, false, true),
    PERCEPTION("Percezione", AbilityType.WISDOM, false, false, false),
    PROFESSION("Professione", AbilityType.WISDOM, true, true, false),
    BLUFF("Raggirare", AbilityType.CHARISMA, false, false, false),
    SLEIGHT_OF_HAND("Rapidità di Mano", AbilityType.DEXTERITY, true, false, true),
    SPELLCRAFT("Sapienza Magica", AbilityType.INTELLIGENCE, true, false, false),
    CLIMB("Scalare", AbilityType.STRENGTH, false, false, true),
    SURVIVAL("Sopravvivenza", AbilityType.WISDOM, false, false, false),
    USE_MAGIC_DEVICE("Utilizzare Congegni Magici", AbilityType.CHARISMA, true, false, false),
    APPRAISE("Valutare", AbilityType.INTELLIGENCE, false, false, false),
    FLY("Volare", AbilityType.DEXTERITY, false, false, true);

    private final String displayName;
    private final AbilityType keyAbility;
    private final boolean trainedOnly;
    private final boolean requiresSpecialization;
    private final boolean armorCheckPenaltyApplied;

    SkillType(
            String displayName,
            AbilityType keyAbility,
            boolean trainedOnly,
            boolean requiresSpecialization,
            boolean armorCheckPenaltyApplied
    )
    {
        this.displayName = displayName;
        this.keyAbility = keyAbility;
        this.trainedOnly = trainedOnly;
        this.requiresSpecialization = requiresSpecialization;
        this.armorCheckPenaltyApplied = armorCheckPenaltyApplied;
    }

    public boolean isKnowledge()
    {
        return name().startsWith("KNOWLEDGE_");
    }

}
