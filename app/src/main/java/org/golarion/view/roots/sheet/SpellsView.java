package org.golarion.view.roots.sheet;

import javafx.scene.Parent;
import javafx.scene.control.Button;
import javafx.scene.layout.AnchorPane;
import javafx.scene.layout.Pane;
import org.golarion.model.Sheet;
import org.golarion.view.roots.GRoot;

public class SpellsView implements GRoot
{
    private final Sheet relatedSheet;

    private final AnchorPane root;
    private final Button returnButton;


    public SpellsView(Sheet relatedSheet)
    {
        this.relatedSheet = relatedSheet;

        returnButton = new Button("Return");


        root = new AnchorPane(returnButton);
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

    public void update(int level, Parent oldRoot)
    {
        returnButton.setOnAction(e -> root.getScene().setRoot(oldRoot));
    }
}
