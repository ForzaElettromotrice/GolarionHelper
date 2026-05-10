package org.golarion.view;

import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.Label;
import javafx.scene.control.ScrollPane;
import javafx.scene.control.Tab;
import javafx.scene.control.TabPane;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.HBox;
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

        CharacterSkillsView skillsView = new CharacterSkillsView(sheet);
        CharacterSavingThrowsView savingThrowsView = new CharacterSavingThrowsView(sheet);
        CharacterInitiativeView initiativeView = new CharacterInitiativeView(sheet);
        CharacterArmorClassView armorClassView = new CharacterArmorClassView(sheet);
        CharacterHitPointsView hitPointsView = new CharacterHitPointsView(sheet);
        CharacterAbilitiesView abilitiesView = new CharacterAbilitiesView(sheet, () ->
        {
            skillsView.refresh();
            savingThrowsView.refresh();
            initiativeView.refresh();
            armorClassView.refresh();
        });
        VBox leftColumn = new VBox(12, abilitiesView, savingThrowsView, hitPointsView, initiativeView, armorClassView);
        leftColumn.setAlignment(Pos.TOP_LEFT);
        HBox mainContent = new HBox(24, leftColumn, skillsView);
        mainContent.setAlignment(Pos.TOP_LEFT);

        ScrollPane mainScrollPane = new ScrollPane(mainContent);
        mainScrollPane.setFitToHeight(false);
        mainScrollPane.setFitToWidth(true);
        mainScrollPane.setPannable(true);

        Tab mainTab = new Tab("Principale", mainScrollPane);
        mainTab.setClosable(false);

        CharacterEnhancementsView enhancementsView = new CharacterEnhancementsView(sheet, () ->
        {
            abilitiesView.refresh();
            skillsView.refresh();
            savingThrowsView.refresh();
            hitPointsView.refresh();
            initiativeView.refresh();
            armorClassView.refresh();
        });
        ScrollPane enhancementsScrollPane = new ScrollPane(enhancementsView);
        enhancementsScrollPane.setFitToHeight(false);
        enhancementsScrollPane.setFitToWidth(true);
        enhancementsScrollPane.setPannable(true);

        Tab enhancementsTab = new Tab("Potenziamenti", enhancementsScrollPane);
        enhancementsTab.setClosable(false);

        TabPane tabPane = new TabPane(mainTab, enhancementsTab);
        tabPane.setTabClosingPolicy(TabPane.TabClosingPolicy.UNAVAILABLE);

        setTop(header);
        setCenter(tabPane);
    }
}
