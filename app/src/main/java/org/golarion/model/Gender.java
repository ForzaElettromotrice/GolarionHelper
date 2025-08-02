package org.golarion.model;

import lombok.Getter;

public enum Gender
{
    MALE("Male"),
    FEMALE("Female"),
    NEUTER("Neuter"),
    OTHER("Other");

    @Getter
    private final String toString;

    Gender(String toString)
    {
        this.toString = toString;
    }
}
