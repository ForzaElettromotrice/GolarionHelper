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
import org.golarion.model.character.ability.Ability;

import java.util.EnumSet;

public class CharacterAbilitiesView extends BorderPane
{
    private static final double ABILITY_COLUMN_WIDTH = 72;
    private static final double VALUE_COLUMN_WIDTH = 48;
    private static final double MODIFIER_COLUMN_WIDTH = 56;
    private static final double TABLE_WIDTH = ABILITY_COLUMN_WIDTH + VALUE_COLUMN_WIDTH + MODIFIER_COLUMN_WIDTH;

    private final CharacterSheet sheet;
    private final EnumSet<Ability> expandedAbilities;
    private final Runnable onAbilitiesChanged;

    public CharacterAbilitiesView(CharacterSheet sheet)
    {
        this(sheet, () -> {});
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
        this.expandedAbilities = EnumSet.noneOf(Ability.class);
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
        for (Ability ability : Ability.values())
        {
            AbilityData abilityData = getAbilityData(ability);
            boolean expanded = expandedAbilities.contains(ability);

            GridPane summaryRow = new GridPane();
            summaryRow.setAlignment(Pos.TOP_LEFT);
            summaryRow.setMaxWidth(Region.USE_PREF_SIZE);

            HBox abilityCell = buildAbilityCell(ability);
            TextField baseValueField = new TextField(Integer.toString(abilityData.totalValue()));
            Label modifierLabel = new Label(formatModifier(abilityData.modifier()));

            styleContainerCell(abilityCell, ABILITY_COLUMN_WIDTH, firstRow, 0, expanded);
            styleEditableCell(baseValueField, VALUE_COLUMN_WIDTH, firstRow, 1, expanded);
            styleCell(modifierLabel, MODIFIER_COLUMN_WIDTH, firstRow, 2, expanded);
            bindBaseValueField(ability, baseValueField, modifierLabel);

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

    private HBox buildAbilityCell(Ability ability)
    {
        Button gearButton = new Button("⚙");
        Label abilityLabel = new Label(abbreviate(ability));
        HBox abilityCell = new HBox(4, gearButton, abilityLabel);
        boolean expanded = expandedAbilities.contains(ability);

        gearButton.setVisible(expanded);
        gearButton.setManaged(gearButton.isVisible());
        gearButton.setFocusTraversable(false);
        gearButton.setPadding(new Insets(0));
        gearButton.setMinSize(16, 16);
        gearButton.setPrefSize(16, 16);
        gearButton.setStyle("-fx-background-color: transparent; -fx-cursor: hand;");
        gearButton.setOnAction(event -> toggleDetails(ability));

        abilityCell.setOnMouseEntered(event ->
        {
            gearButton.setVisible(true);
            gearButton.setManaged(true);
        });
        abilityCell.setOnMouseExited(event ->
        {
            if (!expandedAbilities.contains(ability))
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

    private AbilityData getAbilityData(Ability ability)
    {
        return sheet.getAbility(ability);
    }

    private String abbreviate(Ability ability)
    {
        String displayName = ability.getDisplayName().toUpperCase();
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

    private void bindBaseValueField(Ability ability, TextField baseValueField, Label modifierLabel)
    {
        baseValueField.hoverProperty().addListener((ignored, wasHover, isHover) ->
        {
            if (!baseValueField.isFocused() && wasHover != isHover)
            {
                refreshValueField(ability, baseValueField);
            }
        });
        baseValueField.setOnAction(event ->
        {
            applyBaseValue(ability, baseValueField, modifierLabel);
            requestFocus();
        });
        baseValueField.focusedProperty().addListener((ignored, hadFocus, hasFocus) ->
        {
            if (hasFocus)
            {
                showBaseValue(ability, baseValueField);
                baseValueField.selectAll();
            } else if (hadFocus)
            {
                applyBaseValue(ability, baseValueField, modifierLabel);
            }
        });
        baseValueField.setOnKeyPressed(event ->
        {
            if (event.getCode() == KeyCode.ESCAPE)
            {
                resetBaseValueField(ability, baseValueField);
                requestFocus();
            }
        });
    }

    private void applyBaseValue(Ability ability, TextField baseValueField, Label modifierLabel)
    {
        String rawValue = baseValueField.getText() == null ? "" : baseValueField.getText().trim();

        try
        {
            int baseValue = Integer.parseInt(rawValue);
            sheet.setAbilityBaseValue(ability, baseValue);
            refreshRow(ability, baseValueField, modifierLabel);
            onAbilitiesChanged.run();
        } catch (IllegalArgumentException exception)
        {
            refreshRow(ability, baseValueField, modifierLabel);
        }
    }

    private void resetBaseValueField(Ability ability, TextField baseValueField)
    {
        refreshValueField(ability, baseValueField);
    }

    private void refreshRow(Ability ability, TextField baseValueField, Label modifierLabel)
    {
        refreshValueField(ability, baseValueField);
        modifierLabel.setText(formatModifier(getAbilityData(ability).modifier()));
    }

    private void refreshValueField(Ability ability, TextField baseValueField)
    {
        if (baseValueField.isFocused() || baseValueField.isHover())
        {
            showBaseValue(ability, baseValueField);
            return;
        }

        showTotalValue(ability, baseValueField);
    }

    private void showBaseValue(Ability ability, TextField baseValueField)
    {
        baseValueField.setText(Integer.toString(getAbilityData(ability).baseValue()));
    }

    private void showTotalValue(Ability ability, TextField baseValueField)
    {
        baseValueField.setText(Integer.toString(getAbilityData(ability).totalValue()));
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

    private void toggleDetails(Ability ability)
    {
        if (expandedAbilities.contains(ability))
        {
            expandedAbilities.remove(ability);
        } else
        {
            expandedAbilities.add(ability);
        }

        refreshGrid();
    }

    private void refreshGrid()
    {
        setTop(buildAbilityList());
    }
}
