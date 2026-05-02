package org.golarion.model.character.skill;

import lombok.Getter;
import org.golarion.model.character.ability.Ability;

@Getter
public enum SkillType
{
    ACROBATICS("Acrobazia", Ability.DEXTERITY, false, false),
    HANDLE_ANIMAL("Addestrare Animali", Ability.CHARISMA, true, false),
    CRAFT("Artigianato", Ability.INTELLIGENCE, false, true),
    ESCAPE_ARTIST("Artista della Fuga", Ability.DEXTERITY, false, false),
    DISGUISE("Camuffare", Ability.CHARISMA, false, false),
    RIDE("Cavalcare", Ability.DEXTERITY, false, false),
    KNOWLEDGE_ARCANA("Conoscenze (arcane)", Ability.INTELLIGENCE, true, false),
    KNOWLEDGE_DUNGEONEERING("Conoscenze (dungeon)", Ability.INTELLIGENCE, true, false),
    KNOWLEDGE_ENGINEERING("Conoscenze (ingegneria)", Ability.INTELLIGENCE, true, false),
    KNOWLEDGE_GEOGRAPHY("Conoscenze (geografia)", Ability.INTELLIGENCE, true, false),
    KNOWLEDGE_HISTORY("Conoscenze (storia)", Ability.INTELLIGENCE, true, false),
    KNOWLEDGE_LOCAL("Conoscenze (locali)", Ability.INTELLIGENCE, true, false),
    KNOWLEDGE_NATURE("Conoscenze (natura)", Ability.INTELLIGENCE, true, false),
    KNOWLEDGE_NOBILITY("Conoscenze (nobilta)", Ability.INTELLIGENCE, true, false),
    KNOWLEDGE_PLANES("Conoscenze (piani)", Ability.INTELLIGENCE, true, false),
    KNOWLEDGE_RELIGION("Conoscenze (religioni)", Ability.INTELLIGENCE, true, false),
    DIPLOMACY("Diplomazia", Ability.CHARISMA, false, false),
    DISABLE_DEVICE("Disattivare Congegni", Ability.DEXTERITY, true, false),
    STEALTH("Furtività", Ability.DEXTERITY, false, false),
    HEAL("Guarire", Ability.WISDOM, false, false),
    INTIMIDATE("Intimidire", Ability.CHARISMA, false, false),
    PERFORM("Intrattenere", Ability.CHARISMA, false, true),
    SENSE_MOTIVE("Intuizione", Ability.WISDOM, false, false),
    LINGUISTICS("Linguistica", Ability.INTELLIGENCE, true, false),
    SWIM("Nuotare", Ability.STRENGTH, false, false),
    PERCEPTION("Percezione", Ability.WISDOM, false, false),
    PROFESSION("Professione", Ability.WISDOM, true, true),
    BLUFF("Raggirare", Ability.CHARISMA, false, false),
    SLEIGHT_OF_HAND("Rapidità di Mano", Ability.DEXTERITY, true, false),
    SPELLCRAFT("Sapienza Magica", Ability.INTELLIGENCE, true, false),
    CLIMB("Scalare", Ability.STRENGTH, false, false),
    SURVIVAL("Sopravvivenza", Ability.WISDOM, false, false),
    USE_MAGIC_DEVICE("Utilizzare Congegni Magici", Ability.CHARISMA, true, false),
    APPRAISE("Valutare", Ability.INTELLIGENCE, false, false),
    FLY("Volare", Ability.DEXTERITY, false, false);

    private final String displayName;
    private final Ability keyAbility;
    private final boolean trainedOnly;
    private final boolean requiresSpecialization;

    SkillType(
        String displayName,
        Ability keyAbility,
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
