package org.golarion.view;

import javafx.geometry.Insets;
import javafx.scene.control.Label;
import javafx.scene.control.Tab;
import javafx.scene.control.TabPane;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.VBox;
import org.golarion.model.character.CharacterSheet;

public class CharacterSheetView extends BorderPane
{
    public CharacterSheetView(CharacterSheet sheet)
    {
        Label characterNameLabel = new Label(sheet.getCharacterName());
        characterNameLabel.setStyle("-fx-font-size: 20px; -fx-font-weight: bold;");

        VBox header = new VBox(characterNameLabel);
        header.setPadding(new Insets(16, 16, 12, 16));

        Tab mainTab = new Tab("Principale", new CharacterAbilitiesView(sheet));
        mainTab.setClosable(false);

        TabPane tabPane = new TabPane(mainTab);
        tabPane.setTabClosingPolicy(TabPane.TabClosingPolicy.UNAVAILABLE);

        setTop(header);
        setCenter(tabPane);
    }
}
