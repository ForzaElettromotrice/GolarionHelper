package org.golarion.view;

import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.TextField;
import javafx.scene.input.KeyCode;
import javafx.scene.layout.*;
import org.golarion.model.api.AbilityData;
import org.golarion.model.api.BonusData;
import org.golarion.model.api.PenaltyData;
import org.golarion.model.character.CharacterSheet;
import org.golarion.model.character.ability.AbilityType;

import java.util.EnumSet;

public class CharacterAbilitiesView extends BorderPane
{
    private static final double ABILITY_COLUMN_WIDTH = 72;
    private static final double VALUE_COLUMN_WIDTH = 48;
    private static final double MODIFIER_COLUMN_WIDTH = 56;
    private static final double TABLE_WIDTH = ABILITY_COLUMN_WIDTH + VALUE_COLUMN_WIDTH + MODIFIER_COLUMN_WIDTH;

    private final CharacterSheet sheet;
    private final EnumSet<AbilityType> expandedAbilities;
    private final Runnable onAbilitiesChanged;

    public CharacterAbilitiesView(CharacterSheet sheet)
    {
        this(sheet, () ->
        {
        });
    }

    public CharacterAbilitiesView(CharacterSheet sheet, Runnable onAbilitiesChanged)
    {
        if (sheet == null)
        {
            throw new IllegalArgumentException("sheet must not be null");
        }
        if (onAbilitiesChanged == null)
        {
            throw new IllegalArgumentException("onAbilitiesChanged must not be null");
        }

        this.sheet = sheet;
        this.expandedAbilities = EnumSet.noneOf(AbilityType.class);
        this.onAbilitiesChanged = onAbilitiesChanged;

        setPadding(new Insets(12));
        setFocusTraversable(true);
        refreshGrid();
    }

    private VBox buildAbilityList()
    {
        VBox list = new VBox();
        list.setAlignment(Pos.TOP_LEFT);
        list.setMaxWidth(Region.USE_PREF_SIZE);

        boolean firstRow = true;
        for (AbilityType abilityType : AbilityType.values())
        {
            AbilityData abilityData = getAbilityData(abilityType);
            boolean expanded = expandedAbilities.contains(abilityType);

            GridPane summaryRow = new GridPane();
            summaryRow.setAlignment(Pos.TOP_LEFT);
            summaryRow.setMaxWidth(Region.USE_PREF_SIZE);

            HBox abilityCell = buildAbilityCell(abilityType);
            TextField baseValueField = new TextField(Integer.toString(abilityData.totalValue()));
            Label modifierLabel = new Label(formatModifier(abilityData.modifier()));

            styleContainerCell(abilityCell, ABILITY_COLUMN_WIDTH, firstRow, 0, expanded);
            styleEditableCell(baseValueField, VALUE_COLUMN_WIDTH, firstRow, 1, expanded);
            styleCell(modifierLabel, MODIFIER_COLUMN_WIDTH, firstRow, 2, expanded);
            bindBaseValueField(abilityType, baseValueField, modifierLabel);

            summaryRow.add(abilityCell, 0, 0);
            summaryRow.add(baseValueField, 1, 0);
            summaryRow.add(modifierLabel, 2, 0);
            list.getChildren().add(summaryRow);

            if (expanded)
            {
                VBox detailsRow = buildDetailsRow(abilityData);
                styleDetailsRow(detailsRow);
                list.getChildren().add(detailsRow);
            }

            firstRow = false;
        }

        return list;
    }

    private HBox buildAbilityCell(AbilityType abilityType)
    {
        Button gearButton = new Button("⚙");
        Label abilityLabel = new Label(abbreviate(abilityType));
        HBox abilityCell = new HBox(4, gearButton, abilityLabel);
        boolean expanded = expandedAbilities.contains(abilityType);

        gearButton.setVisible(expanded);
        gearButton.setManaged(gearButton.isVisible());
        gearButton.setFocusTraversable(false);
        gearButton.setPadding(new Insets(0));
        gearButton.setMinSize(16, 16);
        gearButton.setPrefSize(16, 16);
        gearButton.setStyle("-fx-background-color: transparent; -fx-cursor: hand;");
        gearButton.setOnAction(event -> toggleDetails(abilityType));

        abilityCell.setOnMouseEntered(event ->
        {
            gearButton.setVisible(true);
            gearButton.setManaged(true);
        });
        abilityCell.setOnMouseExited(event ->
        {
            if (!expandedAbilities.contains(abilityType))
            {
                gearButton.setVisible(false);
                gearButton.setManaged(false);
            }
        });

        abilityCell.setAlignment(Pos.CENTER);
        return abilityCell;
    }

    private VBox buildDetailsRow(AbilityData abilityData)
    {
        VBox detailsRow = new VBox(4);
        detailsRow.setMaxWidth(TABLE_WIDTH);

        if (abilityData.bonuses().isEmpty() && abilityData.penalties().isEmpty())
        {
            detailsRow.getChildren().add(buildDetailsLabel("Nessun bonus o malus"));
            return detailsRow;
        }

        for (BonusData bonus : abilityData.bonuses())
        {
            detailsRow.getChildren().add(buildDetailsLabel(formatBonus(bonus)));
        }

        for (PenaltyData penalty : abilityData.penalties())
        {
            detailsRow.getChildren().add(buildDetailsLabel(formatPenalty(penalty)));
        }

        return detailsRow;
    }

    private AbilityData getAbilityData(AbilityType abilityType)
    {
        return sheet.getAbility(abilityType);
    }

    private String abbreviate(AbilityType abilityType)
    {
        String displayName = abilityType.getDisplayName().toUpperCase();
        return displayName.substring(0, Math.min(3, displayName.length()));
    }

    private String formatModifier(int modifier)
    {
        return modifier >= 0 ? "+" + modifier : Integer.toString(modifier);
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

    private void bindBaseValueField(AbilityType abilityType, TextField baseValueField, Label modifierLabel)
    {
        baseValueField.hoverProperty().addListener((ignored, wasHover, isHover) ->
        {
            if (!baseValueField.isFocused() && wasHover != isHover)
            {
                refreshValueField(abilityType, baseValueField);
            }
        });
        baseValueField.setOnAction(event ->
        {
            applyBaseValue(abilityType, baseValueField, modifierLabel);
            requestFocus();
        });
        baseValueField.focusedProperty().addListener((ignored, hadFocus, hasFocus) ->
        {
            if (hasFocus)
            {
                showBaseValue(abilityType, baseValueField);
                baseValueField.selectAll();
            } else if (hadFocus)
            {
                applyBaseValue(abilityType, baseValueField, modifierLabel);
            }
        });
        baseValueField.setOnKeyPressed(event ->
        {
            if (event.getCode() == KeyCode.ESCAPE)
            {
                resetBaseValueField(abilityType, baseValueField);
                requestFocus();
            }
        });
    }

    private void applyBaseValue(AbilityType abilityType, TextField baseValueField, Label modifierLabel)
    {
        String rawValue = baseValueField.getText() == null ? "" : baseValueField.getText().trim();

        try
        {
            int baseValue = Integer.parseInt(rawValue);
            sheet.setAbilityBaseValue(abilityType, baseValue);
            refreshRow(abilityType, baseValueField, modifierLabel);
            onAbilitiesChanged.run();
        } catch (IllegalArgumentException exception)
        {
            refreshRow(abilityType, baseValueField, modifierLabel);
        }
    }

    private void resetBaseValueField(AbilityType abilityType, TextField baseValueField)
    {
        refreshValueField(abilityType, baseValueField);
    }

    private void refreshRow(AbilityType abilityType, TextField baseValueField, Label modifierLabel)
    {
        refreshValueField(abilityType, baseValueField);
        modifierLabel.setText(formatModifier(getAbilityData(abilityType).modifier()));
    }

    private void refreshValueField(AbilityType abilityType, TextField baseValueField)
    {
        if (baseValueField.isFocused() || baseValueField.isHover())
        {
            showBaseValue(abilityType, baseValueField);
            return;
        }

        showTotalValue(abilityType, baseValueField);
    }

    private void showBaseValue(AbilityType abilityType, TextField baseValueField)
    {
        baseValueField.setText(Integer.toString(getAbilityData(abilityType).baseValue()));
    }

    private void showTotalValue(AbilityType abilityType, TextField baseValueField)
    {
        baseValueField.setText(Integer.toString(getAbilityData(abilityType).totalValue()));
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

    private void toggleDetails(AbilityType abilityType)
    {
        if (expandedAbilities.contains(abilityType))
        {
            expandedAbilities.remove(abilityType);
        } else
        {
            expandedAbilities.add(abilityType);
        }

        refreshGrid();
    }

    private void refreshGrid()
    {
        setTop(buildAbilityList());
    }
}
