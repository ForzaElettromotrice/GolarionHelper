package org.golarion.model.api;

import org.golarion.model.character.characterclass.CharacterClassType;
import org.golarion.model.spell.*;

import java.util.LinkedHashSet;
import java.util.Set;

public record SpellFilter(
        String nameContains,
        Set<SpellSchool> schools,
        Set<SpellSubschool> subschools,
        Set<SpellCastingTimeCategory> castingTimeCategories,
        Set<SpellRangeType> rangeTypes,
        Set<SpellDurationType> durationTypes,
        Set<SpellResistanceType> spellResistanceTypes,
        Set<Integer> spellLevels,
        Boolean dismissible,
        SpellFilterCriterion<SpellDescriptor> descriptors,
        SpellFilterCriterion<SpellComponentType> components,
        SpellFilterCriterion<CharacterClassType> characterClasses,
        SpellFilterCriterion<DomainSpellListType> domainSpellListTypes,
        SpellFilterCriterion<BloodlineType> bloodlines
)
{
    public SpellFilter
    {
        nameContains = normalizeSearchText(nameContains);
        schools = copySet(schools);
        subschools = copySet(subschools);
        castingTimeCategories = copySet(castingTimeCategories);
        rangeTypes = copySet(rangeTypes);
        durationTypes = copySet(durationTypes);
        spellResistanceTypes = copySet(spellResistanceTypes);
        spellLevels = copySet(spellLevels);
        descriptors = descriptors == null ? new SpellFilterCriterion<>(Set.of(), SpellFilterMatchMode.ANY) : descriptors;
        components = components == null ? new SpellFilterCriterion<>(Set.of(), SpellFilterMatchMode.ANY) : components;
        characterClasses = characterClasses == null ? new SpellFilterCriterion<>(Set.of(), SpellFilterMatchMode.ANY) : characterClasses;
        domainSpellListTypes = domainSpellListTypes == null ? new SpellFilterCriterion<>(Set.of(), SpellFilterMatchMode.ANY) : domainSpellListTypes;
        bloodlines = bloodlines == null ? new SpellFilterCriterion<>(Set.of(), SpellFilterMatchMode.ANY) : bloodlines;
    }

    public static Builder builder()
    {
        return new Builder();
    }

    private static String normalizeSearchText(String value)
    {
        if (value == null)
        {
            return null;
        }

        String normalizedValue = value.trim().toLowerCase();
        return normalizedValue.isBlank() ? null : normalizedValue;
    }

    private static <T> Set<T> copySet(Set<T> values)
    {
        return values == null ? Set.of() : Set.copyOf(values);
    }

    public static class Builder
    {
        private String nameContains;
        private final Set<SpellSchool> schools = new LinkedHashSet<>();
        private final Set<SpellSubschool> subschools = new LinkedHashSet<>();
        private final Set<SpellCastingTimeCategory> castingTimeCategories = new LinkedHashSet<>();
        private final Set<SpellRangeType> rangeTypes = new LinkedHashSet<>();
        private final Set<SpellDurationType> durationTypes = new LinkedHashSet<>();
        private final Set<SpellResistanceType> spellResistanceTypes = new LinkedHashSet<>();
        private final Set<Integer> spellLevels = new LinkedHashSet<>();
        private Boolean dismissible;
        private SpellFilterCriterion<SpellDescriptor> descriptors;
        private SpellFilterCriterion<SpellComponentType> components;
        private SpellFilterCriterion<CharacterClassType> characterClasses;
        private SpellFilterCriterion<DomainSpellListType> domainSpellListTypes;
        private SpellFilterCriterion<BloodlineType> bloodlines;

        public Builder nameContains(String nameContains)
        {
            this.nameContains = nameContains;
            return this;
        }

        public Builder schools(Set<SpellSchool> schools)
        {
            this.schools.clear();
            if (schools != null)
            {
                this.schools.addAll(schools);
            }
            return this;
        }

        public Builder addSchool(SpellSchool school)
        {
            this.schools.add(school);
            return this;
        }

        public Builder subschools(Set<SpellSubschool> subschools)
        {
            this.subschools.clear();
            if (subschools != null)
            {
                this.subschools.addAll(subschools);
            }
            return this;
        }

        public Builder addSubschool(SpellSubschool subschool)
        {
            this.subschools.add(subschool);
            return this;
        }

        public Builder castingTimeCategories(Set<SpellCastingTimeCategory> castingTimeCategories)
        {
            this.castingTimeCategories.clear();
            if (castingTimeCategories != null)
            {
                this.castingTimeCategories.addAll(castingTimeCategories);
            }
            return this;
        }

        public Builder addCastingTimeCategory(SpellCastingTimeCategory castingTimeCategory)
        {
            this.castingTimeCategories.add(castingTimeCategory);
            return this;
        }

        public Builder rangeTypes(Set<SpellRangeType> rangeTypes)
        {
            this.rangeTypes.clear();
            if (rangeTypes != null)
            {
                this.rangeTypes.addAll(rangeTypes);
            }
            return this;
        }

        public Builder addRangeType(SpellRangeType rangeType)
        {
            this.rangeTypes.add(rangeType);
            return this;
        }

        public Builder durationTypes(Set<SpellDurationType> durationTypes)
        {
            this.durationTypes.clear();
            if (durationTypes != null)
            {
                this.durationTypes.addAll(durationTypes);
            }
            return this;
        }

        public Builder addDurationType(SpellDurationType durationType)
        {
            this.durationTypes.add(durationType);
            return this;
        }

        public Builder spellResistanceTypes(Set<SpellResistanceType> spellResistanceTypes)
        {
            this.spellResistanceTypes.clear();
            if (spellResistanceTypes != null)
            {
                this.spellResistanceTypes.addAll(spellResistanceTypes);
            }
            return this;
        }

        public Builder addSpellResistanceType(SpellResistanceType spellResistanceType)
        {
            this.spellResistanceTypes.add(spellResistanceType);
            return this;
        }

        public Builder spellLevels(Set<Integer> spellLevels)
        {
            this.spellLevels.clear();
            if (spellLevels != null)
            {
                this.spellLevels.addAll(spellLevels);
            }
            return this;
        }

        public Builder addSpellLevel(int spellLevel)
        {
            this.spellLevels.add(spellLevel);
            return this;
        }

        public Builder dismissible(Boolean dismissible)
        {
            this.dismissible = dismissible;
            return this;
        }

        public Builder descriptors(Set<SpellDescriptor> descriptors, SpellFilterMatchMode matchMode)
        {
            this.descriptors = new SpellFilterCriterion<>(descriptors, matchMode);
            return this;
        }

        public Builder components(Set<SpellComponentType> components, SpellFilterMatchMode matchMode)
        {
            this.components = new SpellFilterCriterion<>(components, matchMode);
            return this;
        }

        public Builder characterClasses(Set<CharacterClassType> characterClasses, SpellFilterMatchMode matchMode)
        {
            this.characterClasses = new SpellFilterCriterion<>(characterClasses, matchMode);
            return this;
        }

        public Builder domainSpellListTypes(Set<DomainSpellListType> domainSpellListTypes, SpellFilterMatchMode matchMode)
        {
            this.domainSpellListTypes = new SpellFilterCriterion<>(domainSpellListTypes, matchMode);
            return this;
        }

        public Builder bloodlines(Set<BloodlineType> bloodlines, SpellFilterMatchMode matchMode)
        {
            this.bloodlines = new SpellFilterCriterion<>(bloodlines, matchMode);
            return this;
        }

        public SpellFilter build()
        {
            return new SpellFilter(
                    nameContains,
                    schools,
                    subschools,
                    castingTimeCategories,
                    rangeTypes,
                    durationTypes,
                    spellResistanceTypes,
                    spellLevels,
                    dismissible,
                    descriptors,
                    components,
                    characterClasses,
                    domainSpellListTypes,
                    bloodlines
            );
        }
    }
}
