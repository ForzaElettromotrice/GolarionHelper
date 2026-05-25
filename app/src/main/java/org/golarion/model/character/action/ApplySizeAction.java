package org.golarion.model.character.action;

import lombok.NonNull;
import org.golarion.model.character.CharacterSheet;
import org.golarion.model.character.modifier.BonusType;
import org.golarion.model.character.modifier.ModifierType;
import org.golarion.model.character.size.CharacterSize;
import org.golarion.model.character.skill.SkillType;

import java.util.ArrayList;
import java.util.List;

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

        return buildAction().apply(characterSheet);
    }

    private boolean hasNoEffects()
    {
        return size.getCombatModifier() == 0
                && size.getCombatManeuverModifier() == 0
                && size.getStealthModifier() == 0
                && size.getFlyModifier() == 0
                && size.getCarryingCapacityMultiplier() == 1.0;
    }

    private Action buildAction()
    {
        List<Action> actions = new ArrayList<>();
        if (size.getCarryingCapacityMultiplier() != 1.0)
        {
            actions.add(new ChangeCarryingCapacityMultiplierAction(size.getCarryingCapacityMultiplier()));
        }

        List<AddEffectsAction.Effect> effects = buildEffects();
        if (!effects.isEmpty())
        {
            actions.add(new AddEffectsAction("Taglia: " + size.getDisplayName(), effects));
        }

        return new ActionGroup(actions);
    }

    private List<AddEffectsAction.Effect> buildEffects()
    {
        List<AddEffectsAction.Effect> effects = new ArrayList<>();
        addSizeModifier(effects, "armorClass", size.getCombatModifier());
        addSizeModifier(effects, "Attack", size.getCombatModifier());
        addSizeModifier(effects, SkillType.STEALTH.toString(), size.getStealthModifier());
        addSizeModifier(effects, SkillType.FLY.toString(), size.getFlyModifier());
        addSizeModifier(effects, "CMB", size.getCombatManeuverModifier());
        addSizeModifier(effects, "CMD", size.getCombatManeuverModifier());
        return effects;
    }

    private void addSizeModifier(@NonNull List<AddEffectsAction.Effect> effects, @NonNull String target, int modifier)
    {
        if (modifier == 0)
        {
            return;
        }

        ModifierType modifierType = modifier >= 0 ? ModifierType.BONUS : ModifierType.PENALTY;
        effects.add(AddEffectsAction.Effect.modifier(
                modifierType,
                modifierType == ModifierType.BONUS ? BonusType.SIZE : null,
                Integer.toString(modifier),
                target,
                SOURCE,
                size.getDisplayName()
        ));
    }

}
