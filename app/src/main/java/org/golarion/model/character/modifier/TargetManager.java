package org.golarion.model.character.modifier;

import lombok.NonNull;

import java.util.ArrayList;
import java.util.ArrayDeque;
import java.util.Deque;
import java.util.LinkedHashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.function.IntSupplier;

public class TargetManager
{
    private final Map<String, ModifierTarget> modifierTargets;
    private final Map<String, DeltaTarget> deltaTargets;
    private final Map<String, IntSupplier> valueTargets;
    private final Map<String, Set<String>> forbiddenVariablesByTarget;
    private final ThreadLocal<Deque<String>> resolutionStack;

    public TargetManager()
    {
        this.modifierTargets = new LinkedHashMap<>();
        this.deltaTargets = new LinkedHashMap<>();
        this.valueTargets = new LinkedHashMap<>();
        this.forbiddenVariablesByTarget = new LinkedHashMap<>();
        this.resolutionStack = ThreadLocal.withInitial(ArrayDeque::new);
    }

    public void registerModifierTarget(@NonNull String name, @NonNull ModifierTarget target)
    {
        registerTarget(modifierTargets, name, target, "modifier target");
        registerForbiddenVariables(name, name);
    }

    public void registerDeltaTarget(@NonNull String name, @NonNull DeltaTarget target)
    {
        registerTarget(deltaTargets, name, target, "delta target");
        registerForbiddenVariables(name, name);
    }

    public void registerValueTarget(@NonNull String name, @NonNull IntSupplier valueResolver)
    {
        registerTarget(valueTargets, name, valueResolver, "value target");
    }

    public void registerForbiddenVariables(@NonNull String targetName, @NonNull String... variableNames)
    {
        String normalizedTargetName = normalizeName(targetName);
        Set<String> forbiddenVariables = forbiddenVariablesByTarget.computeIfAbsent(normalizedTargetName, ignored -> new LinkedHashSet<>());
        for (String variableName : variableNames)
        {
            forbiddenVariables.add(normalizeName(variableName));
        }
    }

    public ModifierTarget getModifierTarget(@NonNull String name)
    {
        return getTarget(modifierTargets, name, "Modifier target");
    }

    public DeltaTarget getDeltaTarget(@NonNull String name)
    {
        return getTarget(deltaTargets, name, "Delta target");
    }

    public int resolveValue(@NonNull String name)
    {
        String normalizedName = normalizeName(name);
        Deque<String> stack = resolutionStack.get();
        if (stack.contains(normalizedName))
        {
            throw new IllegalArgumentException("circular reference detected: " + buildCircularReferenceMessage(stack, normalizedName));
        }

        stack.addLast(normalizedName);
        try
        {
            return getTarget(valueTargets, normalizedName, "Value target").getAsInt();
        }
        finally
        {
            stack.removeLast();
            if (stack.isEmpty())
            {
                resolutionStack.remove();
            }
        }
    }

    public boolean hasModifierTarget(@NonNull String name)
    {
        return modifierTargets.containsKey(normalizeName(name));
    }

    public boolean hasDeltaTarget(@NonNull String name)
    {
        return deltaTargets.containsKey(normalizeName(name));
    }

    public void validateTargetExpression(@NonNull String targetName, @NonNull Expression expression)
    {
        for (String forbiddenVariable : forbiddenVariablesByTarget.getOrDefault(normalizeName(targetName), Set.of()))
        {
            if (expression.referencesVariable(forbiddenVariable))
            {
                throw new IllegalArgumentException("expression cannot contain variable " + forbiddenVariable + " as it depends on target " + targetName);
            }
        }
    }

    public List<String> getModifierTargetNames()
    {
        return new ArrayList<>(modifierTargets.keySet());
    }

    public List<String> getDeltaTargetNames()
    {
        return new ArrayList<>(deltaTargets.keySet());
    }

    public List<String> getValueTargetNames()
    {
        return new ArrayList<>(valueTargets.keySet());
    }

    private <T> void registerTarget(Map<String, T> targets, String name, T target, String targetType)
    {
        String normalizedName = normalizeName(name);
        if (targets.containsKey(normalizedName))
        {
            throw new IllegalArgumentException(targetType + " already exists: " + name);
        }

        targets.put(normalizedName, target);
    }

    private <T> T getTarget(Map<String, T> targets, String name, String targetType)
    {
        String normalizedName = normalizeName(name);
        T target = targets.get(normalizedName);
        if (target == null)
        {
            throw new IllegalArgumentException(targetType + " not found: " + name);
        }

        return target;
    }

    private String normalizeName(@NonNull String name)
    {
        String normalizedName = name.trim().toLowerCase();
        if (normalizedName.isEmpty())
        {
            throw new IllegalArgumentException("target name must not be blank");
        }

        return normalizedName;
    }

    private String buildCircularReferenceMessage(@NonNull Deque<String> stack, @NonNull String repeatedName)
    {
        List<String> chain = new ArrayList<>(stack);
        int repeatedIndex = chain.indexOf(repeatedName);
        List<String> cycle = chain.subList(repeatedIndex, chain.size());
        return String.join(" -> ", cycle) + " -> " + repeatedName;
    }
}
