package org.golarion.view.roots;

import javafx.scene.control.Label;
import javafx.scene.control.TextField;
import javafx.scene.layout.AnchorPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Pane;
import javafx.scene.layout.VBox;
import javafx.scene.paint.Color;
import org.golarion.model.Sheet;
import org.golarion.model.stats.Characteristic;

import java.util.Map;
import java.util.logging.Logger;

public class SheetView implements GRoot
{
    private static final Logger logger = Logger.getLogger(SheetView.class.getName());

    private final Sheet relatedSheet;
    private final Pane root;

    public SheetView(Sheet relatedSheet)
    {
        this.relatedSheet = relatedSheet;
        root = new AnchorPane();
        root.setPrefSize(800, 600);

        VBox vbox = new VBox();
        for (Characteristic c : Characteristic.values())
            vbox.getChildren().add(prepareCharacteristic(c, vbox));

        root.getChildren().add(vbox);
    }

    private HBox prepareCharacteristic(Characteristic c, VBox box)
    {
        HBox out = new HBox(10);

        Label label = new Label(c.getShortName().toUpperCase());
        label.setStyle("-fx-font-weight: bold;");
        label.setTextFill(Color.BLACK);
        label.setPrefWidth(30);
        label.setUserData(false);
        label.setOnMouseClicked(mouseEvent ->
        {
            if ((boolean) label.getUserData())
            {
                box.getChildren().remove(box.getChildren().indexOf(out) + 1);
                label.setUserData(false);

            } else
            {
                Label newLabel = new Label();
                updateInfoCharacteristicLabel(c, newLabel);
                newLabel.setPrefWidth(200);
                newLabel.setTextFill(Color.BLUE);
                newLabel.setStyle("-fx-font-weight: bold;");
                box.getChildren().add(box.getChildren().indexOf(out) + 1, newLabel);
                label.setUserData(true);
            }
        });

        TextField valueField = new TextField();
        valueField.setPromptText("10");
        valueField.setPrefWidth(30);
        valueField.setOnAction(actionEvent ->
        {
            String input = valueField.getText();
            if (!input.matches("\\d+"))
                return;

            relatedSheet.getStats().updateCharacteristic(c, "base", Integer.parseInt(input));
            if ((boolean) label.getUserData())
            {
                Label newLabel = (Label) box.getChildren().get(box.getChildren().indexOf(out) + 1);
                updateInfoCharacteristicLabel(c, newLabel);
            }
            root.requestFocus();
        });

        out.getChildren().addAll(label, valueField);
        out.setStyle("-fx-border-color: black; -fx-border-width: 1px; -fx-padding: 5px;");
        return out;
    }

    private void updateInfoCharacteristicLabel(Characteristic c, Label newLabel)
    {
        StringBuilder text = new StringBuilder();
        for (Map.Entry<String, Integer> stringIntegerEntry : relatedSheet.getStats().getCharacteristicMap(c))
        {
            text.append(stringIntegerEntry.getKey()).append(": ").append(stringIntegerEntry.getValue()).append(", ");
        }
        if (!text.isEmpty() && text.charAt(text.length() - 2) == ',')
            text = new StringBuilder(text.substring(0, text.length() - 2));
        newLabel.setText(text.toString());
    }

    @Override
    public Pane getRoot()
    {
        return root;
    }
}
