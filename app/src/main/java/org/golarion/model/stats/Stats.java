package org.golarion.model.stats;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;

public class Stats
{
    private final HashMap<String, Integer> strength;
    private final HashMap<String, Integer> dexterity;
    private final HashMap<String, Integer> constitution;
    private final HashMap<String, Integer> intelligence;
    private final HashMap<String, Integer> wisdom;
    private final HashMap<String, Integer> charisma;


    public Stats()
    {
        strength = new HashMap<>();
        dexterity = new HashMap<>();
        constitution = new HashMap<>();
        intelligence = new HashMap<>();
        wisdom = new HashMap<>();
        charisma = new HashMap<>();

        strength.put("base", 10);
        dexterity.put("base", 10);
        constitution.put("base", 10);
        intelligence.put("base", 10);
        wisdom.put("base", 10);
        charisma.put("base", 10);
    }

    public int getCharacteristic(Characteristic characteristic)
    {
        int sum = 0;
        for (int val : switch (characteristic)
        {
            case STRENGTH -> strength.values();
            case DEXTERITY -> dexterity.values();
            case CONSTITUTION -> constitution.values();
            case INTELLIGENCE -> intelligence.values();
            case WISDOM -> wisdom.values();
            case CHARISMA -> charisma.values();
        })
        {
            sum += val;
        }
        return sum;
    }

    public int getBaseCharacteristic(Characteristic characteristic)
    {
        return switch (characteristic)
        {
            case STRENGTH -> strength.get("base");
            case DEXTERITY -> dexterity.get("base");
            case CONSTITUTION -> constitution.get("base");
            case INTELLIGENCE -> intelligence.get("base");
            case WISDOM -> wisdom.get("base");
            case CHARISMA -> charisma.get("base");
        };
    }

    public void updateCharacteristic(Characteristic characteristic, String key, int value)
    {
        HashMap<String, Integer> target = switch (characteristic)
        {
            case STRENGTH -> strength;
            case DEXTERITY -> dexterity;
            case CONSTITUTION -> constitution;
            case INTELLIGENCE -> intelligence;
            case WISDOM -> wisdom;
            case CHARISMA -> charisma;
        };

        if (value == 0)
            target.remove(key);
        else
            target.put(key, value);


    }

    public Set<Map.Entry<String, Integer>> getCharacteristicMap(Characteristic characteristic)
    {
        return switch (characteristic)
        {
            case STRENGTH -> strength.entrySet();
            case DEXTERITY -> dexterity.entrySet();
            case CONSTITUTION -> constitution.entrySet();
            case INTELLIGENCE -> intelligence.entrySet();
            case WISDOM -> wisdom.entrySet();
            case CHARISMA -> charisma.entrySet();
        };
    }
}
