package org.golarion.view.roots.sheet;

import javafx.scene.layout.AnchorPane;
import javafx.scene.layout.Pane;
import javafx.scene.layout.VBox;
import javafx.scene.layout.HBox;
import javafx.scene.control.Label;
import java.util.List;

import org.golarion.model.spell.SpellList;
import org.golarion.view.roots.GRoot;
import org.golarion.model.spell.Spell;


public class SpellListView implements GRoot
{
    private final SpellList relatedSpellList;

    private final Pane root;
    private final VBox[] spellListBoxes;
    private final VBox spellBox;

    public SpellListView(SpellList relatedSpellList)
    {
        this.relatedSpellList = relatedSpellList;

        root = new AnchorPane();
        root.setPrefSize(800, 600);

        spellBox = new VBox();

        // DEBUG
        List<String> colors = List.of("red", "blue", "green", "yellow", "purple", "orange", "pink", "brown", "gray", "cyan");

        spellListBoxes = new VBox[10];
        for (int i = 0; i < 10; i++)
        {
            VBox box = new VBox(); 
            // box.setPrefSize(200, 50);
            spellListBoxes[i] = box;
            Label levelLabel = new Label("Level " + i);
            levelLabel.setStyle("-fx-font-weight: bold; -fx-font-size: 16px;");
            box.getChildren().add(levelLabel);
            box.setStyle("-fx-background-color: " + colors.get(i % colors.size()) + ";");
            spellBox.getChildren().add(box);
        }
        root.getChildren().add(spellBox);
        this.update();
    }

    private void update()
    {
        for (int i = 0; i < 10; i++)
        {
            int maxSlots = relatedSpellList.getMaxSpellSlots(i);
            // int currentSlots = relatedSpellList.getCurrentSpellSlots(i);
            if (maxSlots == 0) {
                spellListBoxes[i].setVisible(false);
                continue;
            }

            for (Spell s : relatedSpellList.getSpellsByLevel(i))
            {
                Label spellLabel = new Label(s.name());
                HBox hbox = new HBox(spellLabel);
                spellListBoxes[i].getChildren().add(hbox);
            }
            
            spellListBoxes[i].setVisible(true);
        }

    }
    
    @Override
    public Pane getRoot()
    {
        return root;
    }

}