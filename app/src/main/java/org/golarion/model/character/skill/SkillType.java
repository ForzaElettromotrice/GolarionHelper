package org.golarion.model.character.skill;

import lombok.Getter;
import org.golarion.model.character.ability.Ability;

@Getter
public enum SkillType
{
    ACROBATICS("Acrobazia", Ability.DEXTERITY, false, true, false),
    HANDLE_ANIMAL("Addestrare Animali", Ability.CHARISMA, true, false, false),
    CRAFT("Artigianato", Ability.INTELLIGENCE, false, false, true),
    ESCAPE_ARTIST("Artista della Fuga", Ability.DEXTERITY, false, true, false),
    DISGUISE("Camuffare", Ability.CHARISMA, false, false, false),
    RIDE("Cavalcare", Ability.DEXTERITY, false, true, false),
    KNOWLEDGE_ARCANA("Conoscenze (arcane)", Ability.INTELLIGENCE, true, false, false),
    KNOWLEDGE_DUNGEONEERING("Conoscenze (dungeon)", Ability.INTELLIGENCE, true, false, false),
    KNOWLEDGE_ENGINEERING("Conoscenze (ingegneria)", Ability.INTELLIGENCE, true, false, false),
    KNOWLEDGE_GEOGRAPHY("Conoscenze (geografia)", Ability.INTELLIGENCE, true, false, false),
    KNOWLEDGE_HISTORY("Conoscenze (storia)", Ability.INTELLIGENCE, true, false, false),
    KNOWLEDGE_LOCAL("Conoscenze (locali)", Ability.INTELLIGENCE, true, false, false),
    KNOWLEDGE_NATURE("Conoscenze (natura)", Ability.INTELLIGENCE, true, false, false),
    KNOWLEDGE_NOBILITY("Conoscenze (nobilta)", Ability.INTELLIGENCE, true, false, false),
    KNOWLEDGE_PLANES("Conoscenze (piani)", Ability.INTELLIGENCE, true, false, false),
    KNOWLEDGE_RELIGION("Conoscenze (religioni)", Ability.INTELLIGENCE, true, false, false),
    DIPLOMACY("Diplomazia", Ability.CHARISMA, false, false, false),
    DISABLE_DEVICE("Disattivare Congegni", Ability.DEXTERITY, true, true, false),
    STEALTH("Furtività", Ability.DEXTERITY, false, true, false),
    HEAL("Guarire", Ability.WISDOM, false, false, false),
    INTIMIDATE("Intimidire", Ability.CHARISMA, false, false, false),
    PERFORM("Intrattenere", Ability.CHARISMA, false, false, true),
    SENSE_MOTIVE("Intuizione", Ability.WISDOM, false, false, false),
    LINGUISTICS("Linguistica", Ability.INTELLIGENCE, true, false, false),
    SWIM("Nuotare", Ability.STRENGTH, false, true, false),
    PERCEPTION("Percezione", Ability.WISDOM, false, false, false),
    PROFESSION("Professione", Ability.WISDOM, true, false, true),
    BLUFF("Raggirare", Ability.CHARISMA, false, false, false),
    SLEIGHT_OF_HAND("Rapidità di Mano", Ability.DEXTERITY, true, true, false),
    SPELLCRAFT("Sapienza Magica", Ability.INTELLIGENCE, true, false, false),
    CLIMB("Scalare", Ability.STRENGTH, false, true, false),
    SURVIVAL("Sopravvivenza", Ability.WISDOM, false, false, false),
    USE_MAGIC_DEVICE("Utilizzare Congegni Magici", Ability.CHARISMA, true, false, false),
    APPRAISE("Valutare", Ability.INTELLIGENCE, false, false, false),
    FLY("Volare", Ability.DEXTERITY, false, true, false);

    private final String displayName;
    private final Ability keyAbility;
    private final boolean trainedOnly;
    private final boolean armorCheckPenaltyApplies;
    private final boolean requiresSpecialization;

    SkillType(
        String displayName,
        Ability keyAbility,
        boolean trainedOnly,
        boolean armorCheckPenaltyApplies,
        boolean requiresSpecialization
    )
    {
        this.displayName = displayName;
        this.keyAbility = keyAbility;
        this.trainedOnly = trainedOnly;
        this.armorCheckPenaltyApplies = armorCheckPenaltyApplies;
        this.requiresSpecialization = requiresSpecialization;
    }
}
