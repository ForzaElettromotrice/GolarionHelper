package org.golarion.view;

import lombok.NonNull;
import org.golarion.model.api.ModifierData;
import org.golarion.model.character.modifier.ModifierType;

public final class ModifierDisplayFormatter
{
    private ModifierDisplayFormatter()
    {
    }

    public static String format(@NonNull ModifierData modifier)
    {
        String status = modifier.enabled() ? "" : " [disattivo]";
        String prefix = modifier.modifierType() == ModifierType.BONUS ? "+ " : "- ";
        String expression = normalizeExpression(modifier);
        String bonusType = modifier.bonusType() == null ? "" : modifier.bonusType().getDisplayName() + " ";

        return prefix + expression + " " + bonusType + "- " + modifier.source() + status;
    }

    private static String normalizeExpression(@NonNull ModifierData modifier)
    {
        String expression = modifier.expression() == null ? "" : modifier.expression().trim();
        if (expression.isEmpty())
        {
            return expression;
        }

        if (modifier.modifierType() == ModifierType.BONUS && expression.startsWith("+"))
        {
            return expression.substring(1).trim();
        }

        if (modifier.modifierType() == ModifierType.PENALTY)
        {
            if (expression.startsWith("+") || expression.startsWith("-"))
            {
                return expression.substring(1).trim();
            }
        }

        return expression;
    }
}
