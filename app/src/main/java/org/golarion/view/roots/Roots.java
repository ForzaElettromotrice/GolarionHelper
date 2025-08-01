package org.golarion.view.roots;

import lombok.Getter;

public enum Roots
{
    MAIN_MENU(0);

    @Getter
    private final int index;

    Roots(int index)
    {
        this.index = index;
    }
}
