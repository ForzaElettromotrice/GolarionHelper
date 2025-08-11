package org.golarion.view.roots;

import javafx.scene.layout.AnchorPane;
import javafx.scene.layout.Pane;

public class SpellView implements GRoot
{

    private final AnchorPane root;


    public SpellView()
    {
        root = new AnchorPane();
        root.setPrefSize(800, 600);
    }


    @Override
    public Pane getRoot()
    {
        return root;
    }
}
