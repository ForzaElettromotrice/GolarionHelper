package org.golarion.model.spell;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import lombok.NonNull;
import org.golarion.model.api.SpellFilter;
import org.golarion.model.api.SpellSavingThrowData;
import org.golarion.model.character.characterclass.CharacterClassType;
import org.golarion.model.character.savingthrow.SavingThrowType;
import org.golarion.model.json.JsonResourceParser;

import java.util.EnumSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class SpellRepository
{
    private static final String SPELLS_RESOURCE_PATH = "/org/golarion/spell/spells.jsonl";
    private static final SpellRepository INSTANCE = new SpellRepository();

    public static SpellRepository getInstance()
    {
        return INSTANCE;
    }

    private final Map<String, SpellDefinition> spellsByName = new LinkedHashMap<>();
    private boolean loaded;

    private SpellRepository()
    {
    }

    public SpellDefinition getByName(@NonNull String name)
    {
        ensureLoaded();
        return spellsByName.get(normalizeName(name));
    }

    public List<SpellDefinition> getAll()
    {
        ensureLoaded();
        return List.copyOf(spellsByName.values());
    }

    public List<SpellDefinition> find(@NonNull SpellFilter filter)
    {
        ensureLoaded();
        return spellsByName.values()
                .stream()
                .filter(spellDefinition -> spellDefinition.matches(filter))
                .toList();
    }

    private synchronized void ensureLoaded()
    {
        if (loaded)
        {
            return;
        }

        spellsByName.putAll(loadAllSpells());
        loaded = true;
    }

    private Map<String, SpellDefinition> loadAllSpells()
    {
        Map<String, SpellDefinition> loadedSpells = new LinkedHashMap<>();

        for (JsonObject jsonObject : JsonResourceParser.parseJsonLines(SPELLS_RESOURCE_PATH))
        {
            registerSpell(loadedSpells, jsonObject);
        }

        return loadedSpells;
    }

    private void registerSpell(Map<String, SpellDefinition> loadedSpells, JsonObject jsonObject)
    {
        String spellName = readRequiredString(jsonObject, "name");
        String normalizedName = normalizeName(spellName);
        SpellDefinition spellDefinition = parseSpellDefinition(jsonObject);

        if (loadedSpells.put(normalizedName, spellDefinition) != null)
        {
            throw new IllegalArgumentException("duplicated spell name: " + spellName);
        }
    }

    private SpellDefinition parseSpellDefinition(JsonObject jsonObject)
    {
        return SpellDefinition.builder()
                .name(readRequiredString(jsonObject, "name"))
                .school(readRequiredEnum(jsonObject, "school", SpellSchool.class))
                .subschool(readOptionalEnum(jsonObject))
                .descriptors(readEnumSet(jsonObject, "descriptors", SpellDescriptor.class))
                .spellLevelByClass(readSpellLevelByClass(jsonObject))
                .spellLevelByDomain(readSpellLevelByDomain(jsonObject))
                .spellLevelByBloodline(readSpellLevelByBloodline(jsonObject))
                .castingTimeCategory(readRequiredEnum(jsonObject, "castingTimeCategory", SpellCastingTimeCategory.class))
                .castingTimeDescription(readRequiredString(jsonObject, "castingTimeDescription"))
                .components(readEnumSet(jsonObject, "components", SpellComponentType.class))
                .materialComponentDescription(readOptionalString(jsonObject, "materialComponentDescription"))
                .materialComponentCostInGoldPieces(readOptionalInteger(jsonObject))
                .focusComponentDescription(readOptionalString(jsonObject, "focusComponentDescription"))
                .rangeType(readRequiredEnum(jsonObject, "rangeType", SpellRangeType.class))
                .rangeDescription(readRequiredString(jsonObject, "rangeDescription"))
                .target(readOptionalString(jsonObject, "target"))
                .effect(readOptionalString(jsonObject, "effect"))
                .area(readOptionalString(jsonObject, "area"))
                .durationType(readRequiredEnum(jsonObject, "durationType", SpellDurationType.class))
                .durationDescription(readRequiredString(jsonObject, "durationDescription"))
                .dismissible(readOptionalBoolean(jsonObject))
                .savingThrows(readSavingThrows(jsonObject))
                .spellResistanceType(readRequiredEnum(jsonObject, "spellResistanceType", SpellResistanceType.class))
                .spellResistanceDescription(readRequiredString(jsonObject, "spellResistanceDescription"))
                .description(readRequiredString(jsonObject, "description"))
                .build();
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

    private String readOptionalString(JsonObject jsonObject, String fieldName)
    {
        if (!jsonObject.has(fieldName) || jsonObject.get(fieldName).isJsonNull())
        {
            return null;
        }

        String value = jsonObject.get(fieldName).getAsString().trim();
        return value.isBlank() ? null : value;
    }

    private Integer readOptionalInteger(JsonObject jsonObject)
    {
        if (!jsonObject.has("materialComponentCostInGoldPieces") || jsonObject.get("materialComponentCostInGoldPieces").isJsonNull())
        {
            return null;
        }

        return jsonObject.get("materialComponentCostInGoldPieces").getAsInt();
    }

    private boolean readOptionalBoolean(JsonObject jsonObject)
    {
        if (!jsonObject.has("dismissible") || jsonObject.get("dismissible").isJsonNull())
        {
            return false;
        }

        return jsonObject.get("dismissible").getAsBoolean();
    }

    private <T extends Enum<T>> T readRequiredEnum(JsonObject jsonObject, String fieldName, Class<T> enumClass)
    {
        return Enum.valueOf(enumClass, readRequiredString(jsonObject, fieldName));
    }

    private SpellSubschool readOptionalEnum(JsonObject jsonObject)
    {
        String value = readOptionalString(jsonObject, "subschool");
        return value == null ? null : SpellSubschool.valueOf(value);
    }

    private <T extends Enum<T>> EnumSet<T> readEnumSet(JsonObject jsonObject, String fieldName, Class<T> enumClass)
    {
        EnumSet<T> values = EnumSet.noneOf(enumClass);
        if (!jsonObject.has(fieldName) || jsonObject.get(fieldName).isJsonNull())
        {
            return values;
        }

        JsonArray jsonArray = jsonObject.getAsJsonArray(fieldName);
        for (JsonElement jsonElement : jsonArray)
        {
            values.add(Enum.valueOf(enumClass, jsonElement.getAsString()));
        }

        return values;
    }

    private Map<CharacterClassType, Integer> readSpellLevelByClass(JsonObject jsonObject)
    {
        Map<CharacterClassType, Integer> spellLevelByClass = new LinkedHashMap<>();
        if (!jsonObject.has("spellLevelByClass") || jsonObject.get("spellLevelByClass").isJsonNull())
        {
            return spellLevelByClass;
        }

        JsonObject spellLevelByClassObject = jsonObject.getAsJsonObject("spellLevelByClass");
        for (Map.Entry<String, JsonElement> entry : spellLevelByClassObject.entrySet())
        {
            spellLevelByClass.put(CharacterClassType.valueOf(entry.getKey()), entry.getValue().getAsInt());
        }

        return spellLevelByClass;
    }

    private Map<DomainSpellListType, Integer> readSpellLevelByDomain(JsonObject jsonObject)
    {
        if (!jsonObject.has("spellLevelByDomain") || jsonObject.get("spellLevelByDomain").isJsonNull())
        {
            return null;
        }

        Map<DomainSpellListType, Integer> spellLevelByDomain = new LinkedHashMap<>();
        JsonObject spellLevelByDomainObject = jsonObject.getAsJsonObject("spellLevelByDomain");
        for (Map.Entry<String, JsonElement> entry : spellLevelByDomainObject.entrySet())
        {
            spellLevelByDomain.put(readDomainSpellListType(entry.getKey()), entry.getValue().getAsInt());
        }

        return spellLevelByDomain;
    }

    private DomainSpellListType readDomainSpellListType(String value)
    {
        try
        {
            return DomainType.valueOf(value);
        } catch (IllegalArgumentException ignored)
        {
            return SubdomainType.valueOf(value);
        }
    }

    private Map<BloodlineType, Integer> readSpellLevelByBloodline(JsonObject jsonObject)
    {
        if (!jsonObject.has("spellLevelByBloodline") || jsonObject.get("spellLevelByBloodline").isJsonNull())
        {
            return null;
        }

        Map<BloodlineType, Integer> spellLevelByBloodline = new LinkedHashMap<>();
        JsonObject spellLevelByBloodlineObject = jsonObject.getAsJsonObject("spellLevelByBloodline");
        for (Map.Entry<String, JsonElement> entry : spellLevelByBloodlineObject.entrySet())
        {
            spellLevelByBloodline.put(BloodlineType.valueOf(entry.getKey()), entry.getValue().getAsInt());
        }

        return spellLevelByBloodline;
    }

    private List<SpellSavingThrowData> readSavingThrows(JsonObject jsonObject)
    {
        if (!jsonObject.has("savingThrows") || jsonObject.get("savingThrows").isJsonNull())
        {
            return null;
        }

        return jsonObject.getAsJsonArray("savingThrows")
                .asList()
                .stream()
                .map(JsonElement::getAsJsonObject)
                .map(this::readSavingThrow)
                .toList();
    }

    private SpellSavingThrowData readSavingThrow(JsonObject jsonObject)
    {
        return new SpellSavingThrowData(
                readRequiredEnum(jsonObject, "savingThrowType", SavingThrowType.class),
                readRequiredEnum(jsonObject, "effect", SpellSavingThrowEffect.class),
                readRequiredString(jsonObject, "description")
        );
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
