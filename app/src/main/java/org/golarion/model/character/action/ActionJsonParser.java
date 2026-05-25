package org.golarion.model.character.action;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import lombok.NonNull;
import org.golarion.model.character.modifier.BonusType;
import org.golarion.model.character.modifier.ModifierType;

import java.util.ArrayList;
import java.util.List;

public final class ActionJsonParser
{
    private ActionJsonParser()
    {
    }

    public static Action parse(@NonNull JsonObject actionObject, @NonNull String defaultSource)
    {
        if (!actionObject.has("actions") || actionObject.get("actions").isJsonNull())
        {
            return readAction(actionObject, defaultSource);
        }

        if (!actionObject.get("actions").isJsonArray())
        {
            throw new IllegalArgumentException("actions must be an array");
        }

        List<Action> actions = new ArrayList<>();
        JsonArray actionsArray = actionObject.getAsJsonArray("actions");
        for (JsonElement element : actionsArray)
        {
            actions.add(readAction(element.getAsJsonObject(), defaultSource));
        }

        return actions.isEmpty() ? NoAction.INSTANCE : new ActionGroup(actions);
    }

    private static Action readAction(JsonObject actionObject, String defaultSource)
    {
        String actionType = readOptionalString(actionObject, "type", actionObject.has("effects") ? "ADD_EFFECTS" : null);
        if (actionType == null)
        {
            throw new IllegalArgumentException("action type is required");
        }

        return switch (actionType)
        {
            case "ADD_EFFECTS" -> readAddEffectsAction(actionObject, defaultSource);
            case "CHANGE_CARRYING_CAPACITY_MULTIPLIER" -> new ChangeCarryingCapacityMultiplierAction(
                    readRequiredDouble(actionObject, "multiplier")
            );
            default -> throw new IllegalArgumentException("unsupported action type: " + actionType);
        };
    }

    private static AddEffectsAction readAddEffectsAction(JsonObject actionObject, String defaultSource)
    {
        String effectGroupName = readOptionalString(actionObject, "effectGroupName", defaultSource);
        return new AddEffectsAction(effectGroupName, readEffects(actionObject, defaultSource));
    }

    private static List<AddEffectsAction.Effect> readEffects(JsonObject actionObject, String defaultSource)
    {
        if (!actionObject.has("effects") || !actionObject.get("effects").isJsonArray())
        {
            throw new IllegalArgumentException("effects is required");
        }

        List<AddEffectsAction.Effect> effects = new ArrayList<>();
        JsonArray effectsArray = actionObject.getAsJsonArray("effects");
        for (JsonElement element : effectsArray)
        {
            effects.add(readEffect(element.getAsJsonObject(), defaultSource));
        }

        return effects;
    }

    private static AddEffectsAction.Effect readEffect(JsonObject effectObject, String defaultSource)
    {
        ModifierType modifierType = readRequiredEnum(effectObject, "modifierType", ModifierType.class);
        BonusType bonusType = readOptionalEnum(effectObject, "bonusType", BonusType.class);
        String expression = readRequiredString(effectObject, "expression");
        String target = readRequiredString(effectObject, "target");
        String source = readOptionalString(effectObject, "source", defaultSource);
        String description = readOptionalString(effectObject, "description", source);
        String condition = readOptionalString(effectObject, "condition", null);

        if (condition == null)
        {
            return AddEffectsAction.Effect.modifier(
                    modifierType,
                    bonusType,
                    expression,
                    target,
                    source,
                    description
            );
        }

        return AddEffectsAction.Effect.conditionalModifier(
                modifierType,
                bonusType,
                expression,
                target,
                source,
                description,
                condition
        );
    }

    private static String readRequiredString(JsonObject jsonObject, String fieldName)
    {
        if (!jsonObject.has(fieldName) || jsonObject.get(fieldName).isJsonNull())
        {
            throw new IllegalArgumentException(fieldName + " is required");
        }

        String value = jsonObject.get(fieldName).getAsString().trim();
        if (value.isBlank())
        {
            throw new IllegalArgumentException(fieldName + " must not be blank");
        }

        return value;
    }

    private static double readRequiredDouble(JsonObject jsonObject, String fieldName)
    {
        if (!jsonObject.has(fieldName) || jsonObject.get(fieldName).isJsonNull())
        {
            throw new IllegalArgumentException(fieldName + " is required");
        }

        return jsonObject.get(fieldName).getAsDouble();
    }

    private static <T extends Enum<T>> T readRequiredEnum(JsonObject jsonObject, String fieldName, Class<T> enumClass)
    {
        if (!jsonObject.has(fieldName) || jsonObject.get(fieldName).isJsonNull())
        {
            throw new IllegalArgumentException(fieldName + " is required");
        }

        return Enum.valueOf(enumClass, jsonObject.get(fieldName).getAsString());
    }

    private static <T extends Enum<T>> T readOptionalEnum(JsonObject jsonObject, String fieldName, Class<T> enumClass)
    {
        if (!jsonObject.has(fieldName) || jsonObject.get(fieldName).isJsonNull())
        {
            return null;
        }

        return Enum.valueOf(enumClass, jsonObject.get(fieldName).getAsString());
    }

    private static String readOptionalString(JsonObject jsonObject, String fieldName, String defaultValue)
    {
        if (!jsonObject.has(fieldName) || jsonObject.get(fieldName).isJsonNull())
        {
            return defaultValue;
        }

        String value = jsonObject.get(fieldName).getAsString().trim();
        return value.isBlank() ? defaultValue : value;
    }
}
