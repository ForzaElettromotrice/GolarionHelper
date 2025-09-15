package org.golarion.view.roots.sheet;

import javafx.geometry.Pos;
import javafx.scene.control.Label;
import javafx.scene.control.TextField;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Pane;
import javafx.scene.layout.VBox;
import org.golarion.model.stats.Characteristic;
import org.golarion.model.stats.Stats;
import org.golarion.view.roots.GRoot;

public class CharacteristicView implements GRoot
{
    private final Stats relatedStats;

    private final VBox root;

    private final Label[] infos;
    private final Label[] modifiers;


    public CharacteristicView(Stats relatedStats)
    {
        this.relatedStats = relatedStats;

        root = new VBox();

        infos = new Label[Characteristic.values().length];
        modifiers = new Label[Characteristic.values().length];

        for (Characteristic c : Characteristic.values())
        {
            Label name = new Label(c.getShortName().toUpperCase());
            name.setStyle("-fx-font-weight: bold;");
            name.setPrefWidth(30);
            name.setUserData(false);


            TextField valueField = new TextField();
            valueField.setText(relatedStats.getCharacteristic(c) + "");
            valueField.setPrefSize(30, 25);


            Label modifier = new Label(relatedStats.getCharacteristicModifier(c) + "");
            modifier.setStyle("-fx-border-color: BLACK; -fx-border-width: 1px; -fx-border-radius: 2px;");
            modifier.setAlignment(Pos.CENTER);
            modifier.setPrefSize(30, 25);

            HBox box = new HBox(name, valueField, modifier);
            box.setSpacing(5);
            box.setStyle("-fx-border-color: BLACK; -fx-border-width: 1px; -fx-padding: 5px;");

            Label info = new Label(relatedStats.getCharacteristicMap(c).stream()
                    .map(entry -> entry.getKey() + ": " + entry.getValue())
                    .reduce((a, b) -> a + "\n" + b)
                    .orElse(""));

            name.setOnMouseClicked(mouseEvent -> clickCharacteristic(name, box, c));
            valueField.setOnAction(event -> enterStats(valueField, c));
            valueField.setOnMouseEntered(mouseEvent -> overEnteredStats(valueField, c));
            valueField.setOnMouseExited(mouseEvent -> overExitedStats(valueField, c));


            infos[c.ordinal()] = info;
            modifiers[c.ordinal()] = modifier;

            root.getChildren().addAll(box);
        }
    }

    private void enterStats(TextField field, Characteristic c)
    {
        String input = field.getText();
        if (!input.matches("\\d+"))
            return;

        relatedStats.updateCharacteristic(c, "base", Integer.parseInt(input));

        infos[c.ordinal()].setText(
                relatedStats.getCharacteristicMap(c).stream()
                        .map(entry -> entry.getKey() + ": " + entry.getValue())
                        .reduce((a, b) -> a + "\n" + b)
                        .orElse("")
        );
        field.setText(relatedStats.getCharacteristic(c) + "");
        modifiers[c.ordinal()].setText(relatedStats.getCharacteristicModifier(c) + "");

        root.requestFocus();
    }

    private void overEnteredStats(TextField field, Characteristic c)
    {
        if (field.isFocused())
            return;
        field.setText(relatedStats.getBaseCharacteristic(c) + "");
    }

    private void overExitedStats(TextField field, Characteristic c)
    {
        if (field.isFocused())
            return;
        field.setText(relatedStats.getCharacteristic(c) + "");
    }

    private void clickCharacteristic(Label label, HBox box, Characteristic c)
    {
        if ((boolean) label.getUserData())
        {
            root.getChildren().remove(infos[c.ordinal()]);
            label.setUserData(false);
        } else
        {
            root.getChildren().add(root.getChildren().indexOf(box) + 1, infos[c.ordinal()]);
            label.setUserData(true);
        }

    }

    public void update()
    {
        //TODO
    }

    @Override
    public Pane getRoot()
    {
        return root;
    }
}
