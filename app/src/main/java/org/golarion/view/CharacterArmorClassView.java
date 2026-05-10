package org.golarion.view;

import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Region;
import javafx.scene.layout.VBox;
import lombok.NonNull;
import org.golarion.model.api.ArmorClassData;
import org.golarion.model.api.ModifierData;
import org.golarion.model.character.CharacterSheet;

public class CharacterArmorClassView extends BorderPane
{
    private static final double LABEL_COLUMN_WIDTH = 112;
    private static final double VALUE_COLUMN_WIDTH = 48;
    private static final double DETAILS_WIDTH = (LABEL_COLUMN_WIDTH + VALUE_COLUMN_WIDTH) * 3;

    private final CharacterSheet sheet;
    private boolean expanded;

    public CharacterArmorClassView(@NonNull CharacterSheet sheet)
    {
        this.sheet = sheet;

        setPadding(new Insets(12));
        refreshGrid();
    }

    private VBox buildContent()
    {
        VBox content = new VBox(12);
        content.setAlignment(Pos.TOP_LEFT);
        content.setMaxWidth(Region.USE_PREF_SIZE);
        content.getChildren().add(buildSectionLabel("Classe Armatura"));
        content.getChildren().add(buildArmorClassBlock());
        return content;
    }

    private VBox buildArmorClassBlock()
    {
        VBox container = new VBox();
        container.setAlignment(Pos.TOP_LEFT);
        container.setMaxWidth(Region.USE_PREF_SIZE);

        container.getChildren().add(buildArmorClassRow());
        if (expanded)
        {
            VBox detailsRow = buildDetailsRow();
            styleDetailsRow(detailsRow);
            container.getChildren().add(detailsRow);
        }

        return container;
    }

    private HBox buildArmorClassRow()
    {
        ArmorClassData armorClassData = sheet.getArmorClass();

        HBox row = new HBox();
        row.setAlignment(Pos.TOP_LEFT);
        row.setMaxWidth(Region.USE_PREF_SIZE);

        row.getChildren().add(buildValueGroup("CA", armorClassData.totalValue(), true, 0, true));
        row.getChildren().add(buildValueGroup("Contatto", armorClassData.touchValue(), false, 1));
        row.getChildren().add(buildValueGroup("Impreparato", armorClassData.flatFootedValue(), false, 2));

        return row;
    }

    private HBox buildValueGroup(String labelText, int value, boolean firstGroup, int columnIndex)
    {
        return buildValueGroup(labelText, value, firstGroup, columnIndex, false);
    }

    private HBox buildValueGroup(String labelText, int value, boolean firstGroup, int columnIndex, boolean withGear)
    {
        HBox group = new HBox();
        group.setAlignment(Pos.TOP_LEFT);

        Region labelRegion = withGear ? buildLabelCellWithGear(labelText) : new Label(labelText);
        Label valueLabel = new Label(Integer.toString(value));

        styleCell(labelRegion, LABEL_COLUMN_WIDTH, firstGroup, columnIndex * 2, expanded);
        styleCell(valueLabel, VALUE_COLUMN_WIDTH, firstGroup, columnIndex * 2 + 1, expanded);

        group.getChildren().addAll(labelRegion, valueLabel);
        return group;
    }

    private Region buildLabelCellWithGear(String labelText)
    {
        Button gearButton = new Button("⚙");
        Label label = new Label(labelText);
        HBox labelCell = new HBox(4, gearButton, label);

        gearButton.setVisible(expanded);
        gearButton.setManaged(gearButton.isVisible());
        gearButton.setFocusTraversable(false);
        gearButton.setPadding(new Insets(0));
        gearButton.setMinSize(16, 16);
        gearButton.setPrefSize(16, 16);
        gearButton.setStyle("-fx-background-color: transparent; -fx-cursor: hand;");
        gearButton.setOnAction(event -> toggleDetails());

        labelCell.setOnMouseEntered(event ->
        {
            gearButton.setVisible(true);
            gearButton.setManaged(true);
        });
        labelCell.setOnMouseExited(event ->
        {
            if (!expanded)
            {
                gearButton.setVisible(false);
                gearButton.setManaged(false);
            }
        });

        labelCell.setAlignment(Pos.CENTER_LEFT);
        return labelCell;
    }

    private VBox buildDetailsRow()
    {
        ArmorClassData armorClassData = sheet.getArmorClass();
        VBox detailsRow = new VBox(4);
        detailsRow.setMaxWidth(DETAILS_WIDTH);

        if (armorClassData.modifiers().isEmpty())
        {
            detailsRow.getChildren().add(buildDetailsLabel("Nessun bonus o malus"));
            return detailsRow;
        }

        for (ModifierData modifier : armorClassData.modifiers())
        {
            detailsRow.getChildren().add(buildDetailsLabel(formatModifier(modifier)));
        }

        return detailsRow;
    }

    private Label buildSectionLabel(String text)
    {
        Label label = new Label(text);
        label.setStyle("-fx-font-size: 14px; -fx-font-weight: bold;");
        return label;
    }

    private Label buildDetailsLabel(String text)
    {
        Label label = new Label(text);
        label.setWrapText(true);
        label.setMaxWidth(DETAILS_WIDTH - 16);
        return label;
    }

    private String formatModifier(ModifierData modifier)
    {
        return ModifierDisplayFormatter.format(modifier);
    }

    private void styleCell(Region region, double width, boolean firstGroup, int column, boolean expanded)
    {
        String borderWidth = buildBorderWidth(firstGroup, column, expanded);

        region.setMinWidth(width);
        region.setPrefWidth(width);
        region.setMaxWidth(width);
        region.setPadding(new Insets(6, 8, 6, 8));
        region.setStyle("-fx-border-color: #666666; -fx-border-width: " + borderWidth + ";");
        if (region instanceof Label label)
        {
            label.setAlignment(Pos.CENTER);
        }
    }

    private String buildBorderWidth(boolean firstGroup, int column, boolean expanded)
    {
        int top = 1;
        int right = 1;
        int bottom = expanded ? 0 : 1;
        int left = firstGroup && column == 0 ? 1 : 0;

        return top + " " + right + " " + bottom + " " + left;
    }

    private void styleDetailsRow(VBox detailsRow)
    {
        detailsRow.setPadding(new Insets(8));
        detailsRow.setStyle("-fx-border-color: #666666; -fx-border-width: 1 1 1 1;");
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
