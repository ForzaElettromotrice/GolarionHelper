package org.golarion.view;

import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.TextField;
import javafx.scene.input.KeyCode;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.GridPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Region;
import javafx.scene.layout.VBox;
import lombok.NonNull;
import org.golarion.model.api.BonusData;
import org.golarion.model.api.InitiativeData;
import org.golarion.model.api.PenaltyData;
import org.golarion.model.character.CharacterSheet;

public class CharacterInitiativeView extends BorderPane
{
    private static final double NAME_COLUMN_WIDTH = 120;
    private static final double VALUE_COLUMN_WIDTH = 48;
    private static final double TABLE_WIDTH = NAME_COLUMN_WIDTH + VALUE_COLUMN_WIDTH;

    private final CharacterSheet sheet;
    private boolean expanded;

    public CharacterInitiativeView(@NonNull CharacterSheet sheet)
    {
        this.sheet = sheet;

        setPadding(new Insets(12));
        setFocusTraversable(true);
        refreshGrid();
    }

    private VBox buildContent()
    {
        VBox content = new VBox(12);
        content.setAlignment(Pos.TOP_LEFT);
        content.setMaxWidth(Region.USE_PREF_SIZE);
        content.getChildren().add(buildSectionLabel("Iniziativa"));
        content.getChildren().add(buildInitiativeRow());
        return content;
    }

    private VBox buildInitiativeRow()
    {
        VBox container = new VBox();
        container.setAlignment(Pos.TOP_LEFT);
        container.setMaxWidth(Region.USE_PREF_SIZE);

        InitiativeData initiativeData = getInitiativeData();
        GridPane summaryRow = new GridPane();
        summaryRow.setAlignment(Pos.TOP_LEFT);
        summaryRow.setMaxWidth(Region.USE_PREF_SIZE);

        HBox initiativeCell = buildInitiativeCell();
        TextField valueField = new TextField(Integer.toString(initiativeData.totalValue()));

        styleContainerCell(initiativeCell, NAME_COLUMN_WIDTH, true, 0, expanded);
        styleEditableCell(valueField, VALUE_COLUMN_WIDTH, true, 1, expanded);
        bindValueField(valueField);

        summaryRow.add(initiativeCell, 0, 0);
        summaryRow.add(valueField, 1, 0);
        container.getChildren().add(summaryRow);

        if (expanded)
        {
            VBox detailsRow = buildDetailsRow(initiativeData);
            styleDetailsRow(detailsRow);
            container.getChildren().add(detailsRow);
        }

        return container;
    }

    private HBox buildInitiativeCell()
    {
        Button gearButton = new Button("⚙");
        Label initiativeLabel = new Label("Iniziativa");
        HBox initiativeCell = new HBox(4, gearButton, initiativeLabel);

        gearButton.setVisible(expanded);
        gearButton.setManaged(gearButton.isVisible());
        gearButton.setFocusTraversable(false);
        gearButton.setPadding(new Insets(0));
        gearButton.setMinSize(16, 16);
        gearButton.setPrefSize(16, 16);
        gearButton.setStyle("-fx-background-color: transparent; -fx-cursor: hand;");
        gearButton.setOnAction(event -> toggleDetails());

        initiativeCell.setOnMouseEntered(event ->
        {
            gearButton.setVisible(true);
            gearButton.setManaged(true);
        });
        initiativeCell.setOnMouseExited(event ->
        {
            if (!expanded)
            {
                gearButton.setVisible(false);
                gearButton.setManaged(false);
            }
        });

        initiativeCell.setAlignment(Pos.CENTER_LEFT);
        return initiativeCell;
    }

    private VBox buildDetailsRow(InitiativeData initiativeData)
    {
        VBox detailsRow = new VBox(4);
        detailsRow.setMaxWidth(TABLE_WIDTH);

        if (initiativeData.bonuses().isEmpty() && initiativeData.penalties().isEmpty())
        {
            detailsRow.getChildren().add(buildDetailsLabel("Nessun bonus o malus"));
            return detailsRow;
        }

        for (BonusData bonus : initiativeData.bonuses())
        {
            detailsRow.getChildren().add(buildDetailsLabel(formatBonus(bonus)));
        }

        for (PenaltyData penalty : initiativeData.penalties())
        {
            detailsRow.getChildren().add(buildDetailsLabel(formatPenalty(penalty)));
        }

        return detailsRow;
    }

    private InitiativeData getInitiativeData()
    {
        return sheet.getInitiative();
    }

    private String formatBonus(BonusData bonus)
    {
        String status = bonus.enabled() ? "" : " [disattivo]";
        return "+ " + bonus.value() + " " + bonus.bonusType().getDisplayName() + " - " + bonus.source() + status;
    }

    private String formatPenalty(PenaltyData penalty)
    {
        String status = penalty.enabled() ? "" : " [disattivo]";
        return "- " + penalty.value() + " - " + penalty.source() + status;
    }

    private Label buildDetailsLabel(String text)
    {
        Label label = new Label(text);
        label.setWrapText(true);
        label.setMaxWidth(TABLE_WIDTH - 16);
        return label;
    }

    private Label buildSectionLabel(String text)
    {
        Label label = new Label(text);
        label.setStyle("-fx-font-size: 14px; -fx-font-weight: bold;");
        return label;
    }

    private void bindValueField(TextField valueField)
    {
        valueField.hoverProperty().addListener((ignored, wasHover, isHover) ->
        {
            if (!valueField.isFocused() && wasHover != isHover)
            {
                refreshValueField(valueField);
            }
        });
        valueField.setOnAction(event ->
        {
            applyBaseValue(valueField);
            requestFocus();
        });
        valueField.focusedProperty().addListener((ignored, hadFocus, hasFocus) ->
        {
            if (hasFocus)
            {
                showBaseValue(valueField);
                valueField.selectAll();
            }
            else if (hadFocus)
            {
                applyBaseValue(valueField);
            }
        });
        valueField.setOnKeyPressed(event ->
        {
            if (event.getCode() == KeyCode.ESCAPE)
            {
                resetValueField(valueField);
                requestFocus();
            }
        });
    }

    private void applyBaseValue(TextField valueField)
    {
        String rawValue = valueField.getText() == null ? "" : valueField.getText().trim();

        try
        {
            int baseValue = Integer.parseInt(rawValue);
            sheet.setInitiativeBaseValue(baseValue);
            refreshRow(valueField);
        }
        catch (IllegalArgumentException exception)
        {
            refreshRow(valueField);
        }
    }

    private void resetValueField(TextField valueField)
    {
        refreshValueField(valueField);
    }

    private void refreshRow(TextField valueField)
    {
        refreshValueField(valueField);
    }

    private void refreshValueField(TextField valueField)
    {
        if (valueField.isFocused() || valueField.isHover())
        {
            showBaseValue(valueField);
            return;
        }

        showTotalValue(valueField);
    }

    private void showBaseValue(TextField valueField)
    {
        valueField.setText(Integer.toString(getInitiativeData().baseValue()));
    }

    private void showTotalValue(TextField valueField)
    {
        valueField.setText(Integer.toString(getInitiativeData().totalValue()));
    }

    private void styleEditableCell(TextField textField, double minWidth, boolean firstRow, int column, boolean expanded)
    {
        String borderWidth = buildBorderWidth(firstRow, column, expanded);

        textField.setMinWidth(minWidth);
        textField.setMaxWidth(minWidth);
        textField.setAlignment(Pos.CENTER);
        textField.setPadding(new Insets(6, 8, 6, 8));
        textField.setStyle("-fx-border-color: #666666; -fx-border-width: " + borderWidth + "; -fx-background-insets: 0; -fx-background-radius: 0;");
    }

    private void styleContainerCell(Region region, double minWidth, boolean firstRow, int column, boolean expanded)
    {
        String borderWidth = buildBorderWidth(firstRow, column, expanded);

        region.setMinWidth(minWidth);
        region.setMaxWidth(minWidth);
        region.setPadding(new Insets(6, 8, 6, 8));
        region.setStyle("-fx-border-color: #666666; -fx-border-width: " + borderWidth + ";");
    }

    private void styleDetailsRow(VBox detailsRow)
    {
        detailsRow.setPadding(new Insets(8));
        detailsRow.setStyle("-fx-border-color: #666666; -fx-border-width: 1 1 1 1;");
    }

    private String buildBorderWidth(boolean firstRow, int column, boolean expanded)
    {
        int top = firstRow ? 1 : 0;
        int right = 1;
        int bottom = expanded ? 0 : 1;
        int left = column == 0 ? 1 : 0;

        return top + " " + right + " " + bottom + " " + left;
    }

    private void toggleDetails()
    {
        expanded = !expanded;
        refreshGrid();
    }

    private void refreshGrid()
    {
        setTop(buildContent());
    }

    public void refresh()
    {
        refreshGrid();
    }
}
