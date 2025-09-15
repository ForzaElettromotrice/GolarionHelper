package org.golarion.view.roots.sheet;

import javafx.geometry.Pos;
import javafx.scene.Parent;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.TextField;
import javafx.scene.layout.*;
import org.golarion.exceptions.MaxSpellSlotsException;
import org.golarion.model.Sheet;
import org.golarion.model.spell.Spell;
import org.golarion.view.roots.GRoot;

import java.util.List;


public class SpellListView implements GRoot
{
    private final Sheet relateSheet;

    private final SpellsView sView;

    private final Pane root;
    private final VBox[] spellListBoxes;

    public SpellListView(Sheet relateSheet)
    {
        this.relateSheet = relateSheet;

        sView = new SpellsView(relateSheet);

        root = new AnchorPane();
        root.setPrefSize(800, 600);

        VBox spellBox = new VBox();

        // DEBUG
        List<String> colors = List.of("red", "blue", "green", "yellow", "purple", "orange", "pink", "brown", "gray", "cyan");

        spellListBoxes = new VBox[10];
        spellBox.setSpacing(20);


        for (int i = 0; i < 5; i++)
        {
            HBox row = new HBox();
            row.setSpacing(20);
            for (int j = 0; j < 2; j++)
            {
                Label level = new Label("Level " + (i * 2 + j) + " - 0/" + relateSheet.getSpellList().getMaxSpellSlots(i * 2 + j));
                level.setStyle("-fx-font-weight: bold; -fx-font-size: 16px;");
                level.setPrefSize(200, 50);
                level.setAlignment(Pos.CENTER);

                VBox box = new VBox(level);
                box.setAlignment(Pos.TOP_CENTER);
                box.setStyle("-fx-background-color: " + colors.get(i * 2 + j) + ";");
                box.setPrefWidth(200);
                row.getChildren().add(box);
                spellListBoxes[i * 2 + j] = box;
            }
            spellBox.getChildren().add(row);
        }
        root.getChildren().add(spellBox);
        this.update();
    }

    public void update()
    {
        for (int i = 0; i < 10; i++)
        {
            int level = i;
            int maxSlots = relateSheet.getSpellList().getMaxSpellSlots(i);
            if (maxSlots == 0)
            {
                spellListBoxes[i].setVisible(false);
                continue;
            }
            spellListBoxes[i].getChildren().clear();
            for (Spell s : relateSheet.getSpellList().getSpellsByLevel(i))
            {
                Label spellLabel = new Label(s.name());
                spellLabel.setPrefHeight(25);
                spellLabel.setStyle("-fx-font-size: 15px;");

                HBox cBox = new HBox();
                cBox.setSpacing(5);
                cBox.setAlignment(Pos.CENTER);

                Button[] buttons = {new Button("-"), new Button("+")};
                for (Button b : buttons)
                {
                    b.setPrefSize(20, 20);
                    b.setStyle("-fx-font-size: 8px;");
                }

                TextField counter = new TextField("0");
                counter.setPrefSize(30, 25);
                counter.setAlignment(Pos.CENTER);


                cBox.getChildren().addAll(buttons[0], counter, buttons[1]);

                Region spacer = new Region();
                HBox.setHgrow(spacer, Priority.ALWAYS);


                HBox hbox = new HBox(spellLabel, spacer, cBox);
                hbox.setStyle("-fx-border-color: BLACK; -fx-border-width: 1px;");
                hbox.setAlignment(Pos.CENTER_LEFT);


                buttons[0].setOnAction(e -> minusClicked(counter, level));
                buttons[1].setOnAction(e -> plusClicked(counter, level));
                spellListBoxes[i].getChildren().add(hbox);
            }
            Button add = new Button("+ Aggiungi Incantesimo");
            add.setOnAction(e -> addClicked(level));

            spellListBoxes[i].getChildren().add(add);
            spellListBoxes[i].setVisible(true);
        }
    }

    public void addClicked(int level)
    {
        Parent oldRoot = root.getScene().getRoot();
        sView.update(level, oldRoot);
        root.getScene().setRoot(sView.getRoot());
    }

    public void minusClicked(TextField counter, int level)
    {
        int val = Integer.parseInt(counter.getText());
        if (val > 0)
        {
            relateSheet.getSpellList().modifyCurrentCounter(level, -1);
            counter.setText(String.valueOf(val - 1));
            ((Label) spellListBoxes[level].getChildren().getFirst()).setText("Level " + level + " - " + (relateSheet.getSpellList().getCurrentCounter(level)) + "/" + relateSheet.getSpellList().getMaxSpellSlots(level));

        }
    }

    public void plusClicked(TextField counter, int level)
    {
        int val = Integer.parseInt(counter.getText());
        try
        {
            relateSheet.getSpellList().modifyCurrentCounter(level, 1);
        } catch (MaxSpellSlotsException err)
        {
            return;
        }
        counter.setText(String.valueOf(val + 1));
        ((Label) spellListBoxes[level].getChildren().getFirst()).setText("Level " + level + " - " + (relateSheet.getSpellList().getCurrentCounter(level)) + "/" + relateSheet.getSpellList().getMaxSpellSlots(level));


    }

    @Override
    public Pane getRoot()
    {
        return root;
    }

}