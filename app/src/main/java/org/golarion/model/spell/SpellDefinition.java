package org.golarion.model.spell;

import lombok.NonNull;
import org.golarion.model.api.SpellData;
import org.golarion.model.api.SpellFilter;
import org.golarion.model.api.SpellSavingThrowData;
import org.golarion.model.character.characterclass.CharacterClassType;

import java.util.*;

public class SpellDefinition
{
    private static Integer getClassSpellLevel(Map.Entry<CharacterClassType, Integer> entry, CharacterClassType characterClassType)
    {
        Integer spellLevel = entry.getValue();

        if (characterClassType == null)
        {
            throw new IllegalArgumentException("spellLevelByClass must not contain null class types");
        }
        if (spellLevel == null)
        {
            throw new IllegalArgumentException("spellLevelByClass must not contain null spell levels");
        }
        if (spellLevel < 0)
        {
            throw new IllegalArgumentException("spellLevel must not be negative");
        }
        return spellLevel;
    }

    private static Integer getDomainSpellLevel(Integer spellLevel)
    {
        if (spellLevel == null)
        {
            throw new IllegalArgumentException("spellLevelByDomain must not contain null spell levels");
        }
        if (spellLevel < 0)
        {
            throw new IllegalArgumentException("spellLevel must not be negative");
        }
        return spellLevel;
    }

    @NonNull
    private final String name;
    @NonNull
    private final SpellSchool school;
    private final SpellSubschool subschool;
    @NonNull
    private final EnumSet<SpellDescriptor> descriptors;
    @NonNull
    private final Map<CharacterClassType, Integer> spellLevelByClass;
    private final Map<DomainSpellListType, Integer> spellLevelByDomain;
    private final Map<BloodlineType, Integer> spellLevelByBloodline;
    @NonNull
    private final SpellCastingTimeCategory castingTimeCategory;
    @NonNull
    private final String castingTimeDescription;
    @NonNull
    private final EnumSet<SpellComponentType> components;
    private final String materialComponentDescription;
    private final Integer materialComponentCostInGoldPieces;
    private final String focusComponentDescription;
    @NonNull
    private final SpellRangeType rangeType;
    @NonNull
    private final String rangeDescription;
    private final String target;
    private final String effect;
    private final String area;
    @NonNull
    private final SpellDurationType durationType;
    @NonNull
    private final String durationDescription;
    private final boolean dismissible;
    private final List<SpellSavingThrowData> savingThrows;
    @NonNull
    private final SpellResistanceType spellResistanceType;
    @NonNull
    private final String spellResistanceDescription;
    @NonNull
    private final String description;

    private SpellDefinition(Builder builder)
    {
        this.name = normalizeRequired(builder.name, "name");
        this.school = Objects.requireNonNull(builder.school, "school must not be null");
        this.subschool = builder.subschool == null ? null : validateSubschool(this.school, builder.subschool);
        this.descriptors = EnumSet.copyOf(builder.descriptors);
        this.spellLevelByClass = validateSpellLevelByClass(builder.spellLevelByClass);
        this.spellLevelByDomain = builder.spellLevelByDomain == null ? null : validateSpellLevelByDomain(builder.spellLevelByDomain);
        this.spellLevelByBloodline = builder.spellLevelByBloodline == null ? null : validateSpellLevelByBloodline(builder.spellLevelByBloodline);
        this.castingTimeCategory = Objects.requireNonNull(builder.castingTimeCategory, "castingTimeCategory must not be null");
        this.castingTimeDescription = normalizeRequired(builder.castingTimeDescription, "castingTimeDescription");
        this.components = validateComponents(builder.components);
        this.materialComponentDescription = validateMaterialComponentDescription(this.components, builder.materialComponentDescription);
        this.materialComponentCostInGoldPieces = validateMaterialComponentCost(this.components, builder.materialComponentCostInGoldPieces);
        this.focusComponentDescription = validateFocusComponentDescription(this.components, builder.focusComponentDescription);
        this.rangeType = Objects.requireNonNull(builder.rangeType, "rangeType must not be null");
        this.rangeDescription = normalizeRequired(builder.rangeDescription, "rangeDescription");
        this.target = normalizeOptional(builder.target);
        this.effect = normalizeOptional(builder.effect);
        this.area = normalizeOptional(builder.area);
        this.durationType = Objects.requireNonNull(builder.durationType, "durationType must not be null");
        this.durationDescription = normalizeRequired(builder.durationDescription, "durationDescription");
        this.dismissible = validateDismissible(this.durationType, builder.dismissible);
        this.savingThrows = validateSavingThrows(builder.savingThrows);
        this.spellResistanceType = Objects.requireNonNull(builder.spellResistanceType, "spellResistanceType must not be null");
        this.spellResistanceDescription = normalizeRequired(builder.spellResistanceDescription, "spellResistanceDescription");
        this.description = normalizeRequired(builder.description, "description");
    }

    public static Builder builder()
    {
        return new Builder();
    }

    public SpellData toData()
    {
        return new SpellData(
                name,
                school,
                subschool,
                EnumSet.copyOf(descriptors),
                Map.copyOf(spellLevelByClass),
                spellLevelByDomain == null ? null : Map.copyOf(spellLevelByDomain),
                spellLevelByBloodline == null ? null : Map.copyOf(spellLevelByBloodline),
                castingTimeCategory,
                castingTimeDescription,
                EnumSet.copyOf(components),
                materialComponentDescription,
                materialComponentCostInGoldPieces,
                focusComponentDescription,
                rangeType,
                rangeDescription,
                target,
                effect,
                area,
                durationType,
                durationDescription,
                dismissible,
                savingThrows == null ? null : List.copyOf(savingThrows),
                spellResistanceType,
                spellResistanceDescription,
                description
        );
    }

    public boolean matches(@NonNull SpellFilter filter)
    {
        return matchesName(filter.nameContains())
                && matchesSingleValueFilter(filter.schools(), school)
                && matchesSingleValueFilter(filter.subschools(), subschool)
                && matchesSingleValueFilter(filter.castingTimeCategories(), castingTimeCategory)
                && matchesSingleValueFilter(filter.rangeTypes(), rangeType)
                && matchesSingleValueFilter(filter.durationTypes(), durationType)
                && matchesSingleValueFilter(filter.spellResistanceTypes(), spellResistanceType)
                && matchesDismissible(filter.dismissible())
                && filter.descriptors().matches(descriptors)
                && filter.components().matches(components)
                && filter.characterClasses().matches(spellLevelByClass.keySet())
                && filter.domainSpellListTypes().matches(spellLevelByDomain == null ? Set.of() : spellLevelByDomain.keySet())
                && filter.bloodlines().matches(spellLevelByBloodline == null ? Set.of() : spellLevelByBloodline.keySet())
                && matchesSpellLevels(filter);
    }

    private boolean matchesName(String nameContains)
    {
        return nameContains == null || name.toLowerCase().contains(nameContains);
    }

    private <T> boolean matchesSingleValueFilter(Set<T> filterValues, T value)
    {
        return filterValues.isEmpty() || filterValues.contains(value);
    }

    private boolean matchesDismissible(Boolean filterDismissible)
    {
        return filterDismissible == null || dismissible == filterDismissible;
    }

    private boolean matchesSpellLevels(SpellFilter filter)
    {
        if (filter.spellLevels().isEmpty())
        {
            return true;
        }

        Set<Integer> candidateLevels = new LinkedHashSet<>();
        collectSelectedSpellLevels(candidateLevels, filter);

        if (candidateLevels.isEmpty())
        {
            collectAllSpellLevels(candidateLevels);
        }

        return filter.spellLevels().stream().anyMatch(candidateLevels::contains);
    }

    private void collectSelectedSpellLevels(Set<Integer> candidateLevels, SpellFilter filter)
    {
        for (CharacterClassType characterClassType : filter.characterClasses().values())
        {
            if (spellLevelByClass.containsKey(characterClassType))
            {
                candidateLevels.add(spellLevelByClass.get(characterClassType));
            }
        }
        if (spellLevelByDomain != null)
        {
            for (DomainSpellListType domainSpellListType : filter.domainSpellListTypes().values())
            {
                if (spellLevelByDomain.containsKey(domainSpellListType))
                {
                    candidateLevels.add(spellLevelByDomain.get(domainSpellListType));
                }
            }
        }
        if (spellLevelByBloodline != null)
        {
            for (BloodlineType bloodlineType : filter.bloodlines().values())
            {
                if (spellLevelByBloodline.containsKey(bloodlineType))
                {
                    candidateLevels.add(spellLevelByBloodline.get(bloodlineType));
                }
            }
        }
    }

    private void collectAllSpellLevels(Set<Integer> candidateLevels)
    {
        candidateLevels.addAll(spellLevelByClass.values());
        if (spellLevelByDomain != null)
        {
            candidateLevels.addAll(spellLevelByDomain.values());
        }
        if (spellLevelByBloodline != null)
        {
            candidateLevels.addAll(spellLevelByBloodline.values());
        }
    }

    private SpellSubschool validateSubschool(@NonNull SpellSchool school, SpellSubschool subschool)
    {
        if (subschool == null)
        {
            return null;
        }
        if (subschool.getSpellSchool() != school)
        {
            throw new IllegalArgumentException("subschool " + subschool + " does not match school " + school);
        }

        return subschool;
    }

    private Map<CharacterClassType, Integer> validateSpellLevelByClass(@NonNull Map<CharacterClassType, Integer> spellLevelByClass)
    {
        Map<CharacterClassType, Integer> validatedMap = new LinkedHashMap<>();
        for (Map.Entry<CharacterClassType, Integer> entry : spellLevelByClass.entrySet())
        {
            CharacterClassType characterClassType = entry.getKey();
            Integer spellLevel = getClassSpellLevel(entry, characterClassType);

            validatedMap.put(characterClassType, spellLevel);
        }

        return Map.copyOf(validatedMap);
    }

    private Map<DomainSpellListType, Integer> validateSpellLevelByDomain(@NonNull Map<DomainSpellListType, Integer> spellLevelByDomain)
    {
        Map<DomainSpellListType, Integer> validatedMap = new LinkedHashMap<>();
        for (Map.Entry<DomainSpellListType, Integer> entry : spellLevelByDomain.entrySet())
        {
            DomainSpellListType domainSpellListType = entry.getKey();
            Integer spellLevel = getDomainSpellLevel(entry.getValue());

            if (domainSpellListType == null)
            {
                throw new IllegalArgumentException("spellLevelByDomain must not contain null domain spell list types");
            }

            validatedMap.put(domainSpellListType, spellLevel);
        }

        return Map.copyOf(validatedMap);
    }

    private Map<BloodlineType, Integer> validateSpellLevelByBloodline(@NonNull Map<BloodlineType, Integer> spellLevelByBloodline)
    {
        Map<BloodlineType, Integer> validatedMap = new LinkedHashMap<>();
        for (Map.Entry<BloodlineType, Integer> entry : spellLevelByBloodline.entrySet())
        {
            BloodlineType bloodlineType = entry.getKey();
            Integer spellLevel = getDomainSpellLevel(entry.getValue());

            if (bloodlineType == null)
            {
                throw new IllegalArgumentException("spellLevelByBloodline must not contain null bloodline types");
            }

            validatedMap.put(bloodlineType, spellLevel);
        }

        return Map.copyOf(validatedMap);
    }

    private EnumSet<SpellComponentType> validateComponents(EnumSet<SpellComponentType> components)
    {
        if (components.isEmpty())
        {
            throw new IllegalArgumentException("components must not be empty");
        }

        if (components.contains(SpellComponentType.MATERIAL_OR_DIVINE_FOCUS)
                && (components.contains(SpellComponentType.MATERIAL)
                || components.contains(SpellComponentType.DIVINE_FOCUS)))
        {
            throw new IllegalArgumentException("MATERIAL_OR_DIVINE_FOCUS must not be combined with MATERIAL or DIVINE_FOCUS");
        }
        if (components.contains(SpellComponentType.FOCUS_OR_DIVINE_FOCUS)
                && (components.contains(SpellComponentType.FOCUS)
                || components.contains(SpellComponentType.DIVINE_FOCUS)))
        {
            throw new IllegalArgumentException("FOCUS_OR_DIVINE_FOCUS must not be combined with FOCUS or DIVINE_FOCUS");
        }

        return components;
    }

    private String validateMaterialComponentDescription(
            EnumSet<SpellComponentType> components,
            String materialComponentDescription
    )
    {
        String normalizedDescription = normalizeOptional(materialComponentDescription);
        if (normalizedDescription == null)
        {
            return null;
        }

        if (!components.contains(SpellComponentType.MATERIAL)
                && !components.contains(SpellComponentType.MATERIAL_OR_DIVINE_FOCUS))
        {
            throw new IllegalArgumentException("materialComponentDescription requires a material component");
        }

        return normalizedDescription;
    }

    private Integer validateMaterialComponentCost(
            EnumSet<SpellComponentType> components,
            Integer materialComponentCostInGoldPieces
    )
    {
        if (materialComponentCostInGoldPieces == null)
        {
            return null;
        }

        if (materialComponentCostInGoldPieces < 0)
        {
            throw new IllegalArgumentException("materialComponentCostInGoldPieces must not be negative");
        }

        if (!components.contains(SpellComponentType.MATERIAL)
                && !components.contains(SpellComponentType.MATERIAL_OR_DIVINE_FOCUS))
        {
            throw new IllegalArgumentException("materialComponentCostInGoldPieces requires a material component");
        }

        return materialComponentCostInGoldPieces;
    }

    private String validateFocusComponentDescription(
            EnumSet<SpellComponentType> components,
            String focusComponentDescription
    )
    {
        String normalizedDescription = normalizeOptional(focusComponentDescription);
        if (normalizedDescription == null)
        {
            return null;
        }

        if (!components.contains(SpellComponentType.FOCUS)
                && !components.contains(SpellComponentType.FOCUS_OR_DIVINE_FOCUS))
        {
            throw new IllegalArgumentException("focusComponentDescription requires a focus component");
        }

        return normalizedDescription;
    }

    private boolean validateDismissible(@NonNull SpellDurationType durationType, boolean dismissible)
    {
        if (durationType == SpellDurationType.CONCENTRATION && !dismissible)
        {
            throw new IllegalArgumentException("CONCENTRATION spells must be dismissible");
        }

        return dismissible;
    }

    private List<SpellSavingThrowData> validateSavingThrows(List<SpellSavingThrowData> savingThrows)
    {
        if (savingThrows == null)
        {
            return null;
        }
        if (savingThrows.stream().anyMatch(Objects::isNull))
        {
            throw new IllegalArgumentException("savingThrows must not contain null values");
        }

        return List.copyOf(savingThrows);
    }

    private String normalizeRequired(@NonNull String value, @NonNull String fieldName)
    {
        String normalizedValue = value.trim();
        if (normalizedValue.isBlank())
        {
            throw new IllegalArgumentException(fieldName + " must not be blank");
        }

        return normalizedValue;
    }

    private String normalizeOptional(String value)
    {
        if (value == null)
        {
            return null;
        }

        String normalizedValue = value.trim();
        return normalizedValue.isBlank() ? null : normalizedValue;
    }

    public static class Builder
    {
        private String name;
        private SpellSchool school;
        private SpellSubschool subschool;
        private EnumSet<SpellDescriptor> descriptors = EnumSet.noneOf(SpellDescriptor.class);
        private Map<CharacterClassType, Integer> spellLevelByClass = new LinkedHashMap<>();
        private Map<DomainSpellListType, Integer> spellLevelByDomain;
        private Map<BloodlineType, Integer> spellLevelByBloodline;
        private SpellCastingTimeCategory castingTimeCategory;
        private String castingTimeDescription;
        private EnumSet<SpellComponentType> components = EnumSet.noneOf(SpellComponentType.class);
        private String materialComponentDescription;
        private Integer materialComponentCostInGoldPieces;
        private String focusComponentDescription;
        private SpellRangeType rangeType;
        private String rangeDescription;
        private String target;
        private String effect;
        private String area;
        private SpellDurationType durationType;
        private String durationDescription;
        private boolean dismissible;
        private List<SpellSavingThrowData> savingThrows;
        private SpellResistanceType spellResistanceType;
        private String spellResistanceDescription;
        private String description;

        public Builder name(String name)
        {
            this.name = name;
            return this;
        }

        public Builder school(SpellSchool school)
        {
            this.school = school;
            return this;
        }

        public Builder subschool(SpellSubschool subschool)
        {
            this.subschool = subschool;
            return this;
        }

        public Builder descriptors(EnumSet<SpellDescriptor> descriptors)
        {
            this.descriptors = descriptors == null ? EnumSet.noneOf(SpellDescriptor.class) : EnumSet.copyOf(descriptors);
            return this;
        }

        public Builder spellLevelByClass(Map<CharacterClassType, Integer> spellLevelByClass)
        {
            this.spellLevelByClass = spellLevelByClass == null ? new LinkedHashMap<>() : new LinkedHashMap<>(spellLevelByClass);
            return this;
        }

        public Builder spellLevelByDomain(Map<DomainSpellListType, Integer> spellLevelByDomain)
        {
            this.spellLevelByDomain = spellLevelByDomain == null ? null : new LinkedHashMap<>(spellLevelByDomain);
            return this;
        }

        public Builder spellLevelByBloodline(Map<BloodlineType, Integer> spellLevelByBloodline)
        {
            this.spellLevelByBloodline = spellLevelByBloodline == null ? null : new LinkedHashMap<>(spellLevelByBloodline);
            return this;
        }

        public Builder castingTimeCategory(SpellCastingTimeCategory castingTimeCategory)
        {
            this.castingTimeCategory = castingTimeCategory;
            return this;
        }

        public Builder castingTimeDescription(String castingTimeDescription)
        {
            this.castingTimeDescription = castingTimeDescription;
            return this;
        }

        public Builder components(EnumSet<SpellComponentType> components)
        {
            this.components = components == null ? EnumSet.noneOf(SpellComponentType.class) : EnumSet.copyOf(components);
            return this;
        }

        public Builder materialComponentDescription(String materialComponentDescription)
        {
            this.materialComponentDescription = materialComponentDescription;
            return this;
        }

        public Builder materialComponentCostInGoldPieces(Integer materialComponentCostInGoldPieces)
        {
            this.materialComponentCostInGoldPieces = materialComponentCostInGoldPieces;
            return this;
        }

        public Builder focusComponentDescription(String focusComponentDescription)
        {
            this.focusComponentDescription = focusComponentDescription;
            return this;
        }

        public Builder rangeType(SpellRangeType rangeType)
        {
            this.rangeType = rangeType;
            return this;
        }

        public Builder rangeDescription(String rangeDescription)
        {
            this.rangeDescription = rangeDescription;
            return this;
        }

        public Builder target(String target)
        {
            this.target = target;
            return this;
        }

        public Builder effect(String effect)
        {
            this.effect = effect;
            return this;
        }

        public Builder area(String area)
        {
            this.area = area;
            return this;
        }

        public Builder durationType(SpellDurationType durationType)
        {
            this.durationType = durationType;
            return this;
        }

        public Builder durationDescription(String durationDescription)
        {
            this.durationDescription = durationDescription;
            return this;
        }

        public Builder dismissible(boolean dismissible)
        {
            this.dismissible = dismissible;
            return this;
        }

        public Builder savingThrows(List<SpellSavingThrowData> savingThrows)
        {
            this.savingThrows = savingThrows == null ? null : List.copyOf(savingThrows);
            return this;
        }

        public Builder spellResistanceType(SpellResistanceType spellResistanceType)
        {
            this.spellResistanceType = spellResistanceType;
            return this;
        }

        public Builder spellResistanceDescription(String spellResistanceDescription)
        {
            this.spellResistanceDescription = spellResistanceDescription;
            return this;
        }

        public Builder description(String description)
        {
            this.description = description;
            return this;
        }

        public SpellDefinition build()
        {
            return new SpellDefinition(this);
        }
    }
}
