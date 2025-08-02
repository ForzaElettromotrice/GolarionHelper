package org.golarion.view.roots;

import javafx.scene.layout.AnchorPane;
import javafx.scene.layout.Pane;

public class SheetView implements GRoot
{
    private final Pane root;

    public SheetView()
    {
        root = new AnchorPane();
    }

    @Override
    public Pane getRoot()
    {
        return root;
    }
}
