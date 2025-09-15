package org.golarion.view.roots.sheet;

import javafx.scene.layout.AnchorPane;
import javafx.scene.layout.Pane;
import org.golarion.view.roots.GRoot;

public class SpellsView implements GRoot
{
    private final AnchorPane root;


    public SpellsView()
    {

        root = new AnchorPane();
        root.setPrefSize(800, 600);
    }

    public void update()
    {

    }


    @Override
    public Pane getRoot()
    {
        return root;
    }
}
