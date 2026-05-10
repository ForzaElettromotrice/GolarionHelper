package org.golarion.view;

import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.TextField;
import javafx.scene.input.KeyCode;
import javafx.scene.layout.*;
import lombok.NonNull;
import org.golarion.model.api.ModifierData;
import org.golarion.model.api.SavingThrowData;
import org.golarion.model.character.CharacterSheet;
import org.golarion.model.character.savingthrow.SavingThrowType;

import java.util.EnumSet;

public class CharacterSavingThrowsView extends BorderPane
{
    private static final double NAME_COLUMN_WIDTH = 120;
    private static final double VALUE_COLUMN_WIDTH = 48;
    private static final double TABLE_WIDTH = NAME_COLUMN_WIDTH + VALUE_COLUMN_WIDTH;

    private final CharacterSheet sheet;
    private final EnumSet<SavingThrowType> expandedSavingThrows;

    public CharacterSavingThrowsView(@NonNull CharacterSheet sheet)
    {
        this.sheet = sheet;
        this.expandedSavingThrows = EnumSet.noneOf(SavingThrowType.class);

        setPadding(new Insets(12));
        setFocusTraversable(true);
        refreshGrid();
    }

    public void refresh()
    {
        refreshGrid();
    }

    private VBox buildContent()
    {
        VBox content = new VBox(12);
        content.setAlignment(Pos.TOP_LEFT);
        content.setMaxWidth(Region.USE_PREF_SIZE);
        content.getChildren().add(buildSectionLabel("Tiri salvezza"));
        content.getChildren().add(buildSavingThrowList());
        return content;
    }

    private VBox buildSavingThrowList()
    {
        VBox list = new VBox();
        list.setAlignment(Pos.TOP_LEFT);
        list.setMaxWidth(Region.USE_PREF_SIZE);

        boolean firstRow = true;
        for (SavingThrowType savingThrowType : SavingThrowType.values())
        {
            SavingThrowData savingThrowData = getSavingThrowData(savingThrowType);
            boolean expanded = expandedSavingThrows.contains(savingThrowType);

            GridPane summaryRow = new GridPane();
            summaryRow.setAlignment(Pos.TOP_LEFT);
            summaryRow.setMaxWidth(Region.USE_PREF_SIZE);

            HBox savingThrowCell = buildSavingThrowCell(savingThrowType);
            TextField valueField = new TextField(Integer.toString(savingThrowData.totalValue()));

            styleContainerCell(savingThrowCell, NAME_COLUMN_WIDTH, firstRow, 0, expanded);
            styleEditableCell(valueField, VALUE_COLUMN_WIDTH, firstRow, 1, expanded);
            bindValueField(savingThrowType, valueField);

            summaryRow.add(savingThrowCell, 0, 0);
            summaryRow.add(valueField, 1, 0);
            list.getChildren().add(summaryRow);

            if (expanded)
            {
                VBox detailsRow = buildDetailsRow(savingThrowData);
                styleDetailsRow(detailsRow);
                list.getChildren().add(detailsRow);
            }

            firstRow = false;
        }

        return list;
    }

    private HBox buildSavingThrowCell(SavingThrowType savingThrowType)
    {
        Button gearButton = new Button("⚙");
        Label savingThrowLabel = new Label(savingThrowType.getDisplayName());
        HBox savingThrowCell = new HBox(4, gearButton, savingThrowLabel);

        gearButton.setVisible(expandedSavingThrows.contains(savingThrowType));
        gearButton.setManaged(gearButton.isVisible());
        gearButton.setFocusTraversable(false);
        gearButton.setPadding(new Insets(0));
        gearButton.setMinSize(16, 16);
        gearButton.setPrefSize(16, 16);
        gearButton.setStyle("-fx-background-color: transparent; -fx-cursor: hand;");
        gearButton.setOnAction(event -> toggleDetails(savingThrowType));

        savingThrowCell.setOnMouseEntered(event ->
        {
            gearButton.setVisible(true);
            gearButton.setManaged(true);
        });
        savingThrowCell.setOnMouseExited(event ->
        {
            if (!expandedSavingThrows.contains(savingThrowType))
            {
                gearButton.setVisible(false);
                gearButton.setManaged(false);
            }
        });

        savingThrowCell.setAlignment(Pos.CENTER_LEFT);
        return savingThrowCell;
    }

    private VBox buildDetailsRow(SavingThrowData savingThrowData)
    {
        VBox detailsRow = new VBox(4);
        detailsRow.setMaxWidth(TABLE_WIDTH);

        if (savingThrowData.modifiers().isEmpty())
        {
            detailsRow.getChildren().add(buildDetailsLabel("Nessun bonus o malus"));
            return detailsRow;
        }

        for (ModifierData modifier : savingThrowData.modifiers())
        {
            detailsRow.getChildren().add(buildDetailsLabel(formatModifier(modifier)));
        }

        return detailsRow;
    }

    private SavingThrowData getSavingThrowData(@NonNull SavingThrowType savingThrowType)
    {
        return sheet.getSavingThrow(savingThrowType);
    }

    private String formatModifier(ModifierData modifier)
    {
        return ModifierDisplayFormatter.format(modifier);
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

    private void bindValueField(SavingThrowType savingThrowType, TextField valueField)
    {
        valueField.hoverProperty().addListener((ignored, wasHover, isHover) ->
        {
            if (!valueField.isFocused() && wasHover != isHover)
            {
                refreshValueField(savingThrowType, valueField);
            }
        });
        valueField.setOnAction(event ->
        {
            applyBaseValue(savingThrowType, valueField);
            requestFocus();
        });
        valueField.focusedProperty().addListener((ignored, hadFocus, hasFocus) ->
        {
            if (hasFocus)
            {
                showBaseValue(savingThrowType, valueField);
                valueField.selectAll();
            } else if (hadFocus)
            {
                applyBaseValue(savingThrowType, valueField);
            }
        });
        valueField.setOnKeyPressed(event ->
        {
            if (event.getCode() == KeyCode.ESCAPE)
            {
                resetValueField(savingThrowType, valueField);
                requestFocus();
            }
        });
    }

    private void applyBaseValue(SavingThrowType savingThrowType, TextField valueField)
    {
        String rawValue = valueField.getText() == null ? "" : valueField.getText().trim();

        try
        {
            int baseValue = Integer.parseInt(rawValue);
            sheet.setSavingThrowBaseValue(savingThrowType, baseValue);
            refreshRow(savingThrowType, valueField);
        } catch (IllegalArgumentException exception)
        {
            refreshRow(savingThrowType, valueField);
        }
    }

    private void resetValueField(SavingThrowType savingThrowType, TextField valueField)
    {
        refreshValueField(savingThrowType, valueField);
    }

    private void refreshRow(SavingThrowType savingThrowType, TextField valueField)
    {
        refreshValueField(savingThrowType, valueField);
    }

    private void refreshValueField(SavingThrowType savingThrowType, TextField valueField)
    {
        if (valueField.isFocused() || valueField.isHover())
        {
            showBaseValue(savingThrowType, valueField);
            return;
        }

        showTotalValue(savingThrowType, valueField);
    }

    private void showBaseValue(SavingThrowType savingThrowType, TextField valueField)
    {
        valueField.setText(Integer.toString(getSavingThrowData(savingThrowType).baseValue()));
    }

    private void showTotalValue(SavingThrowType savingThrowType, TextField valueField)
    {
        valueField.setText(Integer.toString(getSavingThrowData(savingThrowType).totalValue()));
    }

    private void styleCell(Label label, double minWidth, boolean firstRow, int column, boolean expanded)
    {
        String borderWidth = buildBorderWidth(firstRow, column, expanded);

        label.setMinWidth(minWidth);
        label.setMaxWidth(minWidth);
        label.setAlignment(Pos.CENTER);
        label.setPadding(new Insets(6, 8, 6, 8));
        label.setStyle("-fx-border-color: #666666; -fx-border-width: " + borderWidth + ";");
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

    private void toggleDetails(SavingThrowType savingThrowType)
    {
        if (expandedSavingThrows.contains(savingThrowType))
        {
            expandedSavingThrows.remove(savingThrowType);
        } else
        {
            expandedSavingThrows.add(savingThrowType);
        }

        refreshGrid();
    }

    private void refreshGrid()
    {
        setTop(buildContent());
    }
}
