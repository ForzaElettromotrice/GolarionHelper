package org.golarion.model.character;

import org.golarion.model.character.ability.Ability;
import org.golarion.model.character.modifier.BonusType;
import org.golarion.model.character.skill.SkillBonus;
import org.golarion.model.character.skill.SkillPenalty;
import org.golarion.model.character.skill.SkillType;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;

class CharacterSheetSkillCalculationTest
{
    @Test
    void calculatesGenericSkillModifierWithStackingPenaltyAndArmorCheckPenalty()
    {
        CharacterSheet sheet = new CharacterSheet("Valeros");

        sheet.getAbilityScores().setBaseValue(Ability.DEXTERITY, 18);
        sheet.getSkills().setArmorCheckPenalty(1);
        sheet.getSkills().setRanks(SkillType.ACROBATICS, 2);
        sheet.getSkills().setClassSkill(SkillType.ACROBATICS, true);

        sheet.getSkills().addBonus(SkillType.ACROBATICS, new SkillBonus("Belt", BonusType.COMPETENCE, 5));
        sheet.getSkills().addBonus(SkillType.ACROBATICS, new SkillBonus("Training", BonusType.COMPETENCE, 3));
        sheet.getSkills().addBonus(SkillType.ACROBATICS, new SkillBonus("Flanking", BonusType.CIRCUMSTANCE, 2));
        sheet.getSkills().addBonus(SkillType.ACROBATICS, new SkillBonus("Flanking", BonusType.CIRCUMSTANCE, 1));
        sheet.getSkills().addBonus(SkillType.ACROBATICS, new SkillBonus("Terrain", BonusType.CIRCUMSTANCE, 1));
        sheet.getSkills().addFamilyBonus(SkillType.ACROBATICS, new SkillBonus("Prayer", BonusType.LUCK, 1));
        sheet.getSkills().addBonus(SkillType.ACROBATICS, new SkillBonus("Disabled", BonusType.DODGE, 2, false, ""));

        sheet.getSkills().addPenalty(SkillType.ACROBATICS, new SkillPenalty("Fatigue", 2));
        sheet.getSkills().addFamilyPenalty(SkillType.ACROBATICS, new SkillPenalty("Encumbrance", 1));

        assertEquals(14, sheet.getSkillModifier(SkillType.ACROBATICS));
    }

    @Test
    void calculatesSpecializedSkillModifierUsingFamilyAndEntryAdjustments()
    {
        CharacterSheet sheet = new CharacterSheet("Merisiel");

        sheet.getAbilityScores().setBaseValue(Ability.INTELLIGENCE, 16);
        sheet.getSkills().setClassSkill(SkillType.CRAFT, true);
        sheet.getSkills().setRanks(SkillType.CRAFT, "Alchemy", 1);

        sheet.getSkills().addFamilyBonus(SkillType.CRAFT, new SkillBonus("Crafter's Fortune", BonusType.COMPETENCE, 2));
        sheet.getSkills().addBonus(SkillType.CRAFT, "Alchemy", new SkillBonus("Lab", BonusType.COMPETENCE, 4));
        sheet.getSkills().addFamilyBonus(SkillType.CRAFT, new SkillBonus("Masterwork Tools", BonusType.CIRCUMSTANCE, 2));
        sheet.getSkills().addBonus(SkillType.CRAFT, "Alchemy", new SkillBonus("Masterwork Tools", BonusType.CIRCUMSTANCE, 1));
        sheet.getSkills().addBonus(SkillType.CRAFT, "Alchemy", new SkillBonus("Assistant", BonusType.CIRCUMSTANCE, 1));
        sheet.getSkills().addPenalty(SkillType.CRAFT, "Alchemy", new SkillPenalty("Distraction", 1));

        assertEquals(13, sheet.getSkillModifier(SkillType.CRAFT, "Alchemy"));
    }

    @Test
    void calculatesGenericModifierForSpecializedSkillUsingFamilyOnly()
    {
        CharacterSheet sheet = new CharacterSheet("Merisiel");

        sheet.getAbilityScores().setBaseValue(Ability.INTELLIGENCE, 16);
        sheet.getSkills().setClassSkill(SkillType.CRAFT, true);
        sheet.getSkills().setRanks(SkillType.CRAFT, "Alchemy", 1);

        sheet.getSkills().addFamilyBonus(SkillType.CRAFT, new SkillBonus("Crafter's Fortune", BonusType.COMPETENCE, 2));
        sheet.getSkills().addFamilyBonus(SkillType.CRAFT, new SkillBonus("Masterwork Tools", BonusType.CIRCUMSTANCE, 2));
        sheet.getSkills().addBonus(SkillType.CRAFT, "Alchemy", new SkillBonus("Lab", BonusType.COMPETENCE, 4));
        sheet.getSkills().addPenalty(SkillType.CRAFT, "Alchemy", new SkillPenalty("Distraction", 1));

        assertEquals(7, sheet.getSkillModifier(SkillType.CRAFT));
    }

    @Test
    void calculatesKnowledgeArcanaAsRegularSkill()
    {
        CharacterSheet sheet = new CharacterSheet("Ezren");

        sheet.getAbilityScores().setBaseValue(Ability.INTELLIGENCE, 16);
        sheet.getSkills().setRanks(SkillType.KNOWLEDGE_ARCANA, 1);
        sheet.getSkills().setClassSkill(SkillType.KNOWLEDGE_ARCANA, true);

        assertEquals(7, sheet.getSkillModifier(SkillType.KNOWLEDGE_ARCANA));
    }

}
