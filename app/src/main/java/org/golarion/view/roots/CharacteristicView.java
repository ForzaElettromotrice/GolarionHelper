package org.golarion.view.roots;

import javafx.scene.control.Label;
import javafx.scene.control.TextField;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Pane;
import javafx.scene.layout.VBox;
import org.golarion.model.stats.Characteristic;
import org.golarion.model.stats.Stats;

import java.util.Map;

public class CharacteristicView implements GRoot
{
    private final Stats relatedStats;

    private final VBox root;

    private final Label[] characteristicLabels;
    private final TextField[] characteristicFields;
    private final HBox[] characteristicsBoxes;
    private final Label[] characteristicInfos;


    public CharacteristicView(Stats relatedStats)
    {
        this.relatedStats = relatedStats;

        root = new VBox();

        characteristicLabels = new Label[Characteristic.values().length];
        characteristicFields = new TextField[Characteristic.values().length];
        characteristicsBoxes = new HBox[Characteristic.values().length];
        characteristicInfos = new Label[Characteristic.values().length];

        for (Characteristic c : Characteristic.values())
        {
            Label label = new Label(c.getShortName().toUpperCase());
            label.setStyle("-fx-font-weight: bold;");
            label.setPrefWidth(30);
            label.setUserData(false);


            TextField valueField = new TextField();
            valueField.setText(relatedStats.getCharacteristic(c) + "");
            valueField.setPrefWidth(30);

            HBox characteristicsBox = new HBox(label, valueField);
            characteristicsBox.setStyle("-fx-border-color: BLACK; -fx-border-width: 1px; -fx-padding: 5px;");

            StringBuilder builder = new StringBuilder();
            for (Map.Entry<String, Integer> entry : relatedStats.getCharacteristicMap(c))
                builder.append(entry.getKey()).append(": ").append(entry.getValue()).append("\n");
            builder.setLength(builder.length() - 1);

            Label infoLabel = new Label(builder.toString());

            label.setOnMouseClicked(mouseEvent -> clickCharacteristic(label, characteristicsBox, c));
            valueField.setOnAction(event -> enterStats(valueField, c));
            valueField.setOnMouseEntered(mouseEvent -> overEnteredStats(valueField, c));
            valueField.setOnMouseExited(mouseEvent -> overExitedStats(valueField, c));


            characteristicLabels[c.ordinal()] = label;
            characteristicFields[c.ordinal()] = valueField;
            characteristicsBoxes[c.ordinal()] = characteristicsBox;
            characteristicInfos[c.ordinal()] = infoLabel;

            root.getChildren().addAll(characteristicsBox);
        }
    }

    private void enterStats(TextField field, Characteristic c)
    {
        String input = field.getText();
        if (!input.matches("\\d+"))
            return;

        relatedStats.updateCharacteristic(c, "base", Integer.parseInt(input));

        characteristicInfos[c.ordinal()].setText(
                relatedStats.getCharacteristicMap(c).stream()
                        .map(entry -> entry.getKey() + ": " + entry.getValue())
                        .reduce((a, b) -> a + "\n" + b)
                        .orElse("")
        );
        field.setText(relatedStats.getCharacteristic(c) + "");
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
            root.getChildren().remove(characteristicInfos[c.ordinal()]);
            label.setUserData(false);
        } else
        {
            root.getChildren().add(root.getChildren().indexOf(box) + 1, characteristicInfos[c.ordinal()]);
            label.setUserData(true);
        }

    }

    @Override
    public Pane getRoot()
    {
        return root;
    }
}
