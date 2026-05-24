package org.golarion.model.character.action;

import lombok.NonNull;
import org.golarion.model.character.CharacterSheet;
import org.golarion.model.character.modifier.BonusType;
import org.golarion.model.character.modifier.ModifierType;
import org.golarion.model.character.size.CharacterSize;
import org.golarion.model.character.skill.SkillType;

import java.util.UUID;

public class ApplySizeAction implements Action
{
    private static final String SOURCE = "Taglia";
    private final CharacterSize size;

    public ApplySizeAction(@NonNull CharacterSize size)
    {
        this.size = size;
    }

    @Override
    public @NonNull ReverseAction apply(@NonNull CharacterSheet characterSheet)
    {
        if (hasNoEffects())
        {
            return NoAction.INSTANCE;
        }

        UUID effectGroupId = characterSheet.createEffectGroup("Taglia: " + size.getDisplayName());
        try
        {
            characterSheet.changeCarryingCapacityMultiplier(size.getCarryingCapacityMultiplier());
            addSizeModifier(characterSheet, effectGroupId, "armorClass", size.getCombatModifier());
            addSizeModifier(characterSheet, effectGroupId, "Attack", size.getCombatModifier());
            addSizeModifier(characterSheet, effectGroupId, SkillType.STEALTH.toString(), size.getStealthModifier());
            addSizeModifier(characterSheet, effectGroupId, SkillType.FLY.toString(), size.getFlyModifier());
            addSizeModifier(characterSheet, effectGroupId, "CMB", size.getCombatManeuverModifier());
            addSizeModifier(characterSheet, effectGroupId, "CMD", size.getCombatManeuverModifier());
        }
        catch (RuntimeException exception)
        {
            characterSheet.changeCarryingCapacityMultiplier(1 / size.getCarryingCapacityMultiplier());
            characterSheet.removeEffectGroup(effectGroupId);
            throw exception;
        }

        return () ->
        {
            characterSheet.changeCarryingCapacityMultiplier(1 / size.getCarryingCapacityMultiplier());
            characterSheet.removeEffectGroup(effectGroupId);
        };
    }

    private boolean hasNoEffects()
    {
        return size.getCombatModifier() == 0
                && size.getCombatManeuverModifier() == 0
                && size.getStealthModifier() == 0
                && size.getFlyModifier() == 0
                && size.getCarryingCapacityMultiplier() == 1.0;
    }

    private void addSizeModifier(@NonNull CharacterSheet characterSheet, @NonNull UUID effectGroupId, @NonNull String target, int modifier)
    {
        if (modifier == 0)
        {
            return;
        }

        ModifierType modifierType = modifier >= 0 ? ModifierType.BONUS : ModifierType.PENALTY;
        characterSheet.addEffect(
                effectGroupId,
                modifierType,
                modifierType == ModifierType.BONUS ? BonusType.SIZE : null,
                Integer.toString(modifier),
                target,
                SOURCE,
                size.getDisplayName()
        );
    }

}
