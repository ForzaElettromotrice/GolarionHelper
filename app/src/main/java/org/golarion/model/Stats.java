package org.golarion.model;

import lombok.Setter;

public class Stats
{
    @Setter
    private int baseStrength;
    @Setter
    private int baseDexterity;
    @Setter
    private int baseConstitution;
    @Setter
    private int baseIntelligence;
    @Setter
    private int baseWisdom;
    @Setter
    private int baseCharisma;

    @Setter
    private int racialStrength;
    @Setter
    private int racialDexterity;
    @Setter
    private int racialConstitution;
    @Setter
    private int racialIntelligence;
    @Setter
    private int racialWisdom;
    @Setter
    private int racialCharisma;

    private int otherStrength;
    private int otherDexterity;
    private int otherConstitution;
    private int otherIntelligence;
    private int otherWisdom;
    private int otherCharisma;


    public Stats()
    {
        baseStrength = 10;
        baseDexterity = 10;
        baseConstitution = 10;
        baseIntelligence = 10;
        baseWisdom = 10;
        baseCharisma = 10;
    }

    public int getStrength()
    {
        return baseStrength + racialStrength + otherStrength;
    }

    public int getDexterity()
    {
        return baseDexterity + racialDexterity + otherDexterity;
    }

    public int getConstitution()
    {
        return baseConstitution + racialConstitution + otherConstitution;
    }

    public int getIntelligence()
    {
        return baseIntelligence + racialIntelligence + otherIntelligence;
    }

    public int getWisdom()
    {
        return baseWisdom + racialWisdom + otherWisdom;
    }

    public int getCharisma()
    {
        return baseCharisma + racialCharisma + otherCharisma;
    }

    public void addOtherStrength(int value)
    {
        otherStrength += value;
    }

    public void addOtherDexterity(int value)
    {
        otherDexterity += value;
    }

    public void addOtherConstitution(int value)
    {
        otherConstitution += value;
    }

    public void addOtherIntelligence(int value)
    {
        otherIntelligence += value;
    }

    public void addOtherWisdom(int value)
    {
        otherWisdom += value;
    }

    public void addOtherCharisma(int value)
    {
        otherCharisma += value;
    }

    public void subtractOtherStrength(int value)
    {
        otherStrength -= value;
    }

    public void subtractOtherDexterity(int value)
    {
        otherDexterity -= value;
    }

    public void subtractOtherConstitution(int value)
    {
        otherConstitution -= value;
    }

    public void subtractOtherIntelligence(int value)
    {
        otherIntelligence -= value;
    }

    public void subtractOtherWisdom(int value)
    {
        otherWisdom -= value;
    }

    public void subtractOtherCharisma(int value)
    {
        otherCharisma -= value;
    }

}
