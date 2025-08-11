package org.golarion.view.roots.sheet;

import javafx.scene.layout.AnchorPane;
import javafx.scene.layout.Pane;
import org.golarion.model.Sheet;
import org.golarion.view.roots.GRoot;

import java.util.logging.Logger;

public class SheetView implements GRoot
{
    private static final Logger logger = Logger.getLogger(SheetView.class.getName());

    private final Sheet relatedSheet;

    private final Pane root;
    private final CharacteristicView cView;

    public SheetView(Sheet relatedSheet)
    {
        this.relatedSheet = relatedSheet;

        root = new AnchorPane();
        root.setPrefSize(800, 600);

        cView = new CharacteristicView(relatedSheet.getStats());


        root.getChildren().addAll(cView.getRoot());
    }


    @Override
    public Pane getRoot()
    {
        return root;
    }
}
