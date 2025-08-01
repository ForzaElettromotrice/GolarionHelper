package org.golarion.view.roots;

import javafx.geometry.Pos;
import javafx.scene.control.Button;
import javafx.scene.layout.AnchorPane;
import javafx.scene.layout.Background;
import javafx.scene.layout.Pane;
import javafx.scene.layout.VBox;

public class MainMenu implements GRoot
{
    private final Pane root;


    public MainMenu()
    {
        Button startButton = new Button("Start Game");
        startButton.setPrefWidth(100);
        startButton.setPrefHeight(50);
        Button quitButton = new Button("Quit Game");
        quitButton.setPrefWidth(100);
        quitButton.setPrefHeight(50);
        VBox buttonBox = new VBox(startButton, quitButton);
        buttonBox.setAlignment(Pos.CENTER);
        buttonBox.setPrefWidth(200);
        buttonBox.setPrefHeight(100);
        buttonBox.setBackground(Background.fill(javafx.scene.paint.Color.LIGHTGRAY));

        root = new AnchorPane(buttonBox);
        root.setPrefSize(800, 400);
        buttonBox.setLayoutX(root.getPrefWidth() / 2 - buttonBox.getPrefWidth() / 2);
        System.out.println(buttonBox.getPrefWidth() / 2);

    }

    @Override
    public Pane getRoot()
    {
        return root;
    }
}
