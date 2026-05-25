package org.golarion.model.item;

import com.google.gson.JsonObject;
import lombok.NonNull;
import org.golarion.model.character.action.Action;
import org.golarion.model.character.action.ActionJsonParser;
import org.golarion.model.character.action.NoAction;
import org.golarion.model.json.JsonResourceParser;

import java.util.LinkedHashMap;
import java.util.Map;

public class ItemRepository
{
    private static final String ITEMS_RESOURCE_PATH = "/org/golarion/item/items.jsonl";
    private static final ItemRepository INSTANCE = new ItemRepository();

    public static ItemRepository getInstance()
    {
        return INSTANCE;
    }

    private final Map<String, ItemDefinition> itemsByName = new LinkedHashMap<>();
    private boolean loaded;

    private ItemRepository()
    {
    }

    public ItemDefinition getByName(@NonNull String name)
    {
        ensureLoaded();
        return itemsByName.get(normalizeName(name));
    }

    private synchronized void ensureLoaded()
    {
        if (loaded)
        {
            return;
        }

        itemsByName.putAll(loadAllItems());
        loaded = true;
    }

    private Map<String, ItemDefinition> loadAllItems()
    {
        Map<String, ItemDefinition> loadedItems = new LinkedHashMap<>();

        for (JsonObject jsonObject : JsonResourceParser.parseJsonLines(ITEMS_RESOURCE_PATH))
        {
            registerItem(loadedItems, jsonObject);
        }

        return loadedItems;
    }

    private void registerItem(Map<String, ItemDefinition> loadedItems, JsonObject jsonObject)
    {
        String itemName = readRequiredString(jsonObject);
        String normalizedName = normalizeName(itemName);
        ItemDefinition itemDefinition = parseItemDefinition(jsonObject);

        if (loadedItems.put(normalizedName, itemDefinition) != null)
        {
            throw new IllegalArgumentException("duplicated item name: " + itemName);
        }
    }

    private ItemDefinition parseItemDefinition(JsonObject jsonObject)
    {
        String itemName = readRequiredString(jsonObject, "name");
        return ItemDefinition.builder()
                .name(itemName)
                .weightGrams(readRequiredLong(jsonObject))
                .priceInCopperPieces(readOptionalInteger(jsonObject))
                .equipmentSlot(readOptionalEnum(jsonObject, "equipmentSlot", EquipmentSlot.class))
                .handUsage(readOptionalEnum(jsonObject, "handUsage", HandUsage.class))
                .equippedAction(readOptionalAction(jsonObject, "equippedAction", itemName))
                .activatedAction(readOptionalAction(jsonObject, "activatedAction", itemName))
                .description(readOptionalString(jsonObject))
                .build();
    }

    private String readRequiredString(JsonObject jsonObject)
    {
        return readRequiredString(jsonObject, "name");
    }

    private String readRequiredString(JsonObject jsonObject, String fieldName)
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

    private String readOptionalString(JsonObject jsonObject)
    {
        if (!jsonObject.has("description") || jsonObject.get("description").isJsonNull())
        {
            return null;
        }

        String value = jsonObject.get("description").getAsString().trim();
        return value.isBlank() ? null : value;
    }

    private long readRequiredLong(JsonObject jsonObject)
    {
        if (!jsonObject.has("weightGrams") || jsonObject.get("weightGrams").isJsonNull())
        {
            throw new IllegalArgumentException("weightGrams" + " is required");
        }

        return jsonObject.get("weightGrams").getAsLong();
    }

    private Integer readOptionalInteger(JsonObject jsonObject)
    {
        if (!jsonObject.has("priceInCopperPieces") || jsonObject.get("priceInCopperPieces").isJsonNull())
        {
            return null;
        }

        return jsonObject.get("priceInCopperPieces").getAsInt();
    }

    private <T extends Enum<T>> T readOptionalEnum(JsonObject jsonObject, String fieldName, Class<T> enumClass)
    {
        if (!jsonObject.has(fieldName) || jsonObject.get(fieldName).isJsonNull())
        {
            return null;
        }

        return Enum.valueOf(enumClass, jsonObject.get(fieldName).getAsString());
    }

    private Action readOptionalAction(JsonObject jsonObject, String fieldName, String itemName)
    {
        if (!jsonObject.has(fieldName) || jsonObject.get(fieldName).isJsonNull())
        {
            return NoAction.INSTANCE;
        }

        return ActionJsonParser.parse(jsonObject.getAsJsonObject(fieldName), itemName);
    }

    private String readOptionalString(JsonObject jsonObject, String fieldName, String defaultValue)
    {
        if (!jsonObject.has(fieldName) || jsonObject.get(fieldName).isJsonNull())
        {
            return defaultValue;
        }

        String value = jsonObject.get(fieldName).getAsString().trim();
        return value.isBlank() ? defaultValue : value;
    }

    private String normalizeName(@NonNull String name)
    {
        String normalizedName = name.trim().toLowerCase();
        if (normalizedName.isBlank())
        {
            throw new IllegalArgumentException("name must not be blank");
        }

        return normalizedName;
    }
}
