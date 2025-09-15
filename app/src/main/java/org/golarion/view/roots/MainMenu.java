package org.golarion.view.roots;

import javafx.geometry.Pos;
import javafx.scene.control.Button;
import javafx.scene.layout.AnchorPane;
import javafx.scene.layout.Background;
import javafx.scene.layout.Pane;
import javafx.scene.layout.VBox;
import javafx.scene.paint.Color;
import org.golarion.model.Sheet;
import org.golarion.view.DisplayMan;
import org.golarion.view.SheetMan;
import org.golarion.view.roots.sheet.SheetView;

import java.util.UUID;

public class MainMenu implements GRoot
{
    private final Pane root;


    public MainMenu()
    {
        Button newSheet = new Button();
        Button loadSheet = new Button();
        Button quit = new Button();
        newSheet.setPrefSize(120, 60);
        loadSheet.setPrefSize(120, 60);
        quit.setPrefSize(120, 60);
        newSheet.setText("Nuova Scheda");
        loadSheet.setText("Carica Scheda");
        quit.setText("Esci");

        newSheet.setOnAction(actionEvent -> SheetMan.getInstance().open("Nuova Scheda", UUID.randomUUID(), new SheetView(new Sheet())));
        quit.setOnAction(actionEvent -> DisplayMan.getInstance().close());

        VBox box = new VBox(newSheet, loadSheet, quit);
        box.setSpacing(25);
        box.setPrefSize(200, 300);
        box.setAlignment(Pos.CENTER);
        box.setBackground(Background.fill(Color.LIGHTGRAY));

        root = new AnchorPane(box);
        root.setPrefSize(300, 400);


        box.setLayoutX(root.getPrefWidth() / 2 - box.getPrefWidth() / 2);
        box.setLayoutY(root.getPrefHeight() / 2 - box.getPrefHeight() / 2);
    }

    @Override
    public Pane getRoot()
    {
        return root;
    }
}
