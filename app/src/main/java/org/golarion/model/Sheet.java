package org.golarion.model;

import javafx.scene.paint.Color;
import lombok.Getter;
import lombok.Setter;

@Setter
@Getter
public class Sheet
{

    private final Stats stats;
    private String name;
    private Alignment alignment;
    private int level;
    private Gender gender;
    private int age;
    private int height;
    private int weight;
    private Color hairColor;
    private Color eyeColor;
    private Color skinColor;

    public Sheet()
    {
        stats = new Stats();
    }
}
