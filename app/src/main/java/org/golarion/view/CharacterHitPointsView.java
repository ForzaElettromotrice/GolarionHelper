package org.golarion.view;

import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.Label;
import javafx.scene.control.TextField;
import javafx.scene.input.KeyCode;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Region;
import javafx.scene.layout.VBox;
import lombok.NonNull;
import org.golarion.model.api.HitPointsData;
import org.golarion.model.character.CharacterSheet;
import org.golarion.model.character.hitpoints.HitPointField;

public class CharacterHitPointsView extends BorderPane
{
    private static final double LABEL_COLUMN_WIDTH = 84;
    private static final double VALUE_COLUMN_WIDTH = 56;

    private final CharacterSheet sheet;

    public CharacterHitPointsView(@NonNull CharacterSheet sheet)
    {
        this.sheet = sheet;

        setPadding(new Insets(12));
        setFocusTraversable(true);
        refresh();
    }

    private VBox buildContent()
    {
        VBox content = new VBox(12);
        content.setAlignment(Pos.TOP_LEFT);
        content.setMaxWidth(Region.USE_PREF_SIZE);
        content.getChildren().add(buildSectionLabel("Hit Points"));
        content.getChildren().add(buildHitPointsRow());
        return content;
    }

    private HBox buildHitPointsRow()
    {
        HitPointsData hitPointsData = sheet.getHitPoints();
        HBox row = new HBox();
        row.setAlignment(Pos.TOP_LEFT);
        row.setMaxWidth(Region.USE_PREF_SIZE);

        row.getChildren().add(buildValueGroup("Max", hitPointsData.maxHp(), HitPointField.MAX));
        row.getChildren().add(buildValueGroup("Current", hitPointsData.currentHp(), HitPointField.CURRENT));
        row.getChildren().add(buildValueGroup("Temp", hitPointsData.temporaryHp(), HitPointField.TEMPORARY));
        row.getChildren().add(buildValueGroup("Non lethal", hitPointsData.nonlethalDamage(), HitPointField.NONLETHAL));

        return row;
    }

    private HBox buildValueGroup(String labelText, int value, HitPointField field)
    {
        HBox group = new HBox();
        group.setAlignment(Pos.TOP_LEFT);

        Label label = new Label(labelText);
        TextField valueField = new TextField(Integer.toString(value));

        styleLabelCell(label, field == HitPointField.MAX);
        styleValueCell(valueField);
        bindValueField(valueField, field);

        group.getChildren().addAll(label, valueField);
        return group;
    }

    private void bindValueField(TextField valueField, HitPointField field)
    {
        valueField.setOnAction(event ->
        {
            requestFocus();
        });
        valueField.focusedProperty().addListener((ignored, hadFocus, hasFocus) ->
        {
            if (hasFocus)
            {
                valueField.selectAll();
            }
            else if (hadFocus)
            {
                applyValue(valueField, field);
            }
        });
        valueField.setOnKeyPressed(event ->
        {
            if (event.getCode() == KeyCode.ESCAPE)
            {
                refresh();
                requestFocus();
            }
        });
    }

    private void applyValue(TextField valueField, HitPointField field)
    {
        String rawValue = valueField.getText() == null ? "" : valueField.getText().trim();

        try
        {
            if (rawValue.startsWith("+") || rawValue.startsWith("-"))
            {
                sheet.changeHitPoints(field, Integer.parseInt(rawValue));
            }
            else
            {
                sheet.setHitPoints(field, Integer.parseInt(rawValue));
            }
        }
        catch (IllegalArgumentException ignored)
        {
        }

        refresh();
    }

    private Label buildSectionLabel(String text)
    {
        Label label = new Label(text);
        label.setStyle("-fx-font-size: 14px; -fx-font-weight: bold;");
        return label;
    }

    private void styleLabelCell(Label label, boolean firstGroup)
    {
        label.setMinWidth(LABEL_COLUMN_WIDTH);
        label.setPrefWidth(LABEL_COLUMN_WIDTH);
        label.setMaxWidth(LABEL_COLUMN_WIDTH);
        label.setAlignment(Pos.CENTER);
        label.setPadding(new Insets(6, 8, 6, 8));
        label.setStyle("-fx-border-color: #666666; -fx-border-width: 1 1 1 " + (firstGroup ? 1 : 0) + ";");
    }

    private void styleValueCell(TextField textField)
    {
        textField.setMinWidth(VALUE_COLUMN_WIDTH);
        textField.setPrefWidth(VALUE_COLUMN_WIDTH);
        textField.setMaxWidth(VALUE_COLUMN_WIDTH);
        textField.setAlignment(Pos.CENTER);
        textField.setPadding(new Insets(6, 8, 6, 8));
        textField.setStyle("-fx-border-color: #666666; -fx-border-width: 1 1 1 0; -fx-background-insets: 0; -fx-background-radius: 0;");
    }

    public void refresh()
    {
        setTop(buildContent());
    }
}
