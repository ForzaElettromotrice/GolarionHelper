package org.golarion.view;

import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.Button;
import javafx.scene.control.CheckBox;
import javafx.scene.control.Label;
import javafx.scene.control.TextField;
import javafx.scene.input.KeyCode;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.GridPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Region;
import javafx.scene.layout.StackPane;
import javafx.scene.layout.VBox;
import org.golarion.model.api.ModifierData;
import org.golarion.model.api.SkillData;
import org.golarion.model.character.CharacterSheet;
import org.golarion.model.character.skill.SkillType;

import java.util.HashSet;
import java.util.Set;

public class CharacterSkillsView extends BorderPane
{
    private static final double CLASS_COLUMN_WIDTH = 32;
    private static final double NAME_COLUMN_WIDTH = 260;
    private static final double VALUE_COLUMN_WIDTH = 48;
    private static final double RANKS_COLUMN_WIDTH = 48;
    private static final double ROW_HEIGHT = 34;
    private static final double TABLE_WIDTH = CLASS_COLUMN_WIDTH + NAME_COLUMN_WIDTH + VALUE_COLUMN_WIDTH + RANKS_COLUMN_WIDTH;

    private final CharacterSheet sheet;
    private final Set<String> expandedRows;

    public CharacterSkillsView(CharacterSheet sheet)
    {
        if (sheet == null)
        {
            throw new IllegalArgumentException("sheet must not be null");
        }

        this.sheet = sheet;
        this.expandedRows = new HashSet<>();

        setPadding(new Insets(12));
        setFocusTraversable(true);
        refreshGrid();
    }

    private VBox buildSkillList()
    {
        VBox list = new VBox();
        list.setAlignment(Pos.TOP_LEFT);
        list.setMaxWidth(Region.USE_PREF_SIZE);

        boolean firstVisibleRow = true;
        for (SkillType skillType : SkillType.values())
        {
            SkillData skillData = getSkillData(skillType);
            String summaryRowKey = buildSummaryRowKey(skillType);

            list.getChildren().add(buildSummaryRow(skillType, skillData, summaryRowKey, firstVisibleRow));
            if (isExpanded(summaryRowKey))
            {
                list.getChildren().add(buildDetailsRow(skillData));
            }

            firstVisibleRow = false;

            if (!skillType.isRequiresSpecialization())
            {
                continue;
            }

            for (String specialization : sheet.getSkillSpecializations(skillType))
            {
                SkillData specializationData = getSkillData(skillType, specialization);
                String specializationRowKey = buildSpecializationRowKey(skillType, specialization);

                list.getChildren().add(buildSpecializationRow(skillType, specialization, specializationData, specializationRowKey));
                if (isExpanded(specializationRowKey))
                {
                    list.getChildren().add(buildDetailsRow(specializationData));
                }
            }
        }

        return list;
    }

    private GridPane buildSummaryRow(SkillType skillType, SkillData skillData, String rowKey, boolean firstRow)
    {
        GridPane row = new GridPane();
        row.setAlignment(Pos.TOP_LEFT);
        row.setMaxWidth(Region.USE_PREF_SIZE);

        boolean expanded = isExpanded(rowKey);
        Region classSkillCell = buildClassSkillCell(skillType);
        Region nameCell = buildSkillNameCell(skillType, rowKey);
        Label valueLabel = new Label(formatModifier(skillData.totalValue()));
        TextField ranksField = buildRanksField(skillType);

        styleContainerCell(classSkillCell, CLASS_COLUMN_WIDTH, firstRow, 0, expanded);
        styleContainerCell(nameCell, NAME_COLUMN_WIDTH, firstRow, 1, expanded);
        styleCell(valueLabel, VALUE_COLUMN_WIDTH, firstRow, 2, expanded);
        styleEditableCell(ranksField, RANKS_COLUMN_WIDTH, firstRow, 3, expanded);

        row.add(classSkillCell, 0, 0);
        row.add(nameCell, 1, 0);
        row.add(valueLabel, 2, 0);
        row.add(ranksField, 3, 0);

        return row;
    }

    private GridPane buildSpecializationRow(SkillType skillType, String specialization, SkillData skillData, String rowKey)
    {
        GridPane row = new GridPane();
        row.setAlignment(Pos.TOP_LEFT);
        row.setMaxWidth(Region.USE_PREF_SIZE);

        boolean expanded = isExpanded(rowKey);
        Region deleteCell = buildDeleteCell(skillType, specialization);
        Region nameCell = buildSpecializationNameCell(specialization, rowKey);
        Label valueLabel = new Label(formatModifier(skillData.totalValue()));
        TextField ranksField = new TextField(Integer.toString(skillData.ranks()));

        bindSpecializationRanksField(skillType, specialization, ranksField);

        styleContainerCell(deleteCell, CLASS_COLUMN_WIDTH, false, 0, expanded);
        styleContainerCell(nameCell, NAME_COLUMN_WIDTH, false, 1, expanded);
        styleCell(valueLabel, VALUE_COLUMN_WIDTH, false, 2, expanded);
        styleEditableCell(ranksField, RANKS_COLUMN_WIDTH, false, 3, expanded);

        row.add(deleteCell, 0, 0);
        row.add(nameCell, 1, 0);
        row.add(valueLabel, 2, 0);
        row.add(ranksField, 3, 0);

        return row;
    }

    private Region buildClassSkillCell(SkillType skillType)
    {
        CheckBox checkBox = new CheckBox();
        checkBox.setSelected(getSkillData(skillType).classSkill());
        checkBox.setFocusTraversable(false);
        checkBox.setOnAction(event ->
        {
            sheet.setSkillClassSkill(skillType, checkBox.isSelected());
            refreshGrid();
        });

        StackPane cell = new StackPane(checkBox);
        cell.setAlignment(Pos.CENTER);
        return cell;
    }

    private Region buildSkillNameCell(SkillType skillType, String rowKey)
    {
        Button gearButton = buildGearButton(rowKey);

        if (!skillType.isRequiresSpecialization())
        {
            HBox nameCell = new HBox(4, gearButton, new Label(skillType.getDisplayName()));
            nameCell.setAlignment(Pos.CENTER_LEFT);
            bindGearVisibility(nameCell, gearButton, rowKey);
            return nameCell;
        }

        Label nameLabel = new Label(skillType.getDisplayName());
        TextField addField = new TextField();
        addField.setPromptText("Nuova");
        addField.setMinWidth(112);
        addField.setPrefWidth(112);
        addField.setMaxWidth(112);
        addField.setMinHeight(22);
        addField.setPrefHeight(22);
        addField.setMaxHeight(22);
        addField.setStyle(
                "-fx-background-color: transparent; " +
                        "-fx-control-inner-background: transparent; " +
                        "-fx-background-insets: 0; " +
                        "-fx-background-radius: 0; " +
                        "-fx-focus-color: transparent; " +
                        "-fx-faint-focus-color: transparent; " +
                        "-fx-text-fill: #333333; " +
                        "-fx-prompt-text-fill: #888888; " +
                        "-fx-border-color: transparent transparent #999999 transparent; " +
                        "-fx-border-width: 0 0 1 0; " +
                        "-fx-border-radius: 0; " +
                        "-fx-padding: 0 4 0 4;"
        );

        addField.setOnAction(event -> addSpecialization(skillType, addField));
        addField.focusedProperty().addListener((ignored, hadFocus, hasFocus) ->
        {
            if (!hasFocus && hadFocus)
            {
                addField.clear();
            }
        });
        addField.setOnKeyPressed(event ->
        {
            if (event.getCode() == KeyCode.ESCAPE)
            {
                addField.clear();
                requestFocus();
            }
        });

        HBox nameCell = new HBox(4, gearButton, nameLabel, addField);
        nameCell.setAlignment(Pos.CENTER_LEFT);
        bindGearVisibility(nameCell, gearButton, rowKey);
        return nameCell;
    }

    private Region buildSpecializationNameCell(String specialization, String rowKey)
    {
        Button gearButton = buildGearButton(rowKey);
        Label nameLabel = new Label(specialization);
        HBox nameCell = new HBox(4, gearButton, nameLabel);
        nameCell.setAlignment(Pos.CENTER_LEFT);
        nameCell.setPadding(new Insets(0, 0, 0, 16));
        bindGearVisibility(nameCell, gearButton, rowKey);
        return nameCell;
    }

    private Region buildDeleteCell(SkillType skillType, String specialization)
    {
        Button deleteButton = new Button("x");
        deleteButton.setFocusTraversable(false);
        deleteButton.setPadding(new Insets(0));
        deleteButton.setMinSize(18, 18);
        deleteButton.setPrefSize(18, 18);
        deleteButton.setStyle("-fx-background-color: transparent; -fx-cursor: hand;");
        deleteButton.setOnAction(event ->
        {
            sheet.removeSkillSpecialization(skillType, specialization);
            refreshGrid();
        });

        StackPane cell = new StackPane(deleteButton);
        cell.setAlignment(Pos.CENTER);
        return cell;
    }

    private TextField buildRanksField(SkillType skillType)
    {
        TextField ranksField = new TextField();

        if (skillType.isRequiresSpecialization())
        {
            ranksField.setText("-");
            ranksField.setEditable(false);
            ranksField.setFocusTraversable(false);
            return ranksField;
        }

        ranksField.setText(Integer.toString(getSkillData(skillType).ranks()));
        bindRanksField(skillType, ranksField);
        return ranksField;
    }

    private void addSpecialization(SkillType skillType, TextField addField)
    {
        String specialization = addField.getText() == null ? "" : addField.getText().trim();
        if (specialization.isEmpty())
        {
            return;
        }

        try
        {
            sheet.addSkillSpecialization(skillType, specialization);
        }
        catch (IllegalArgumentException ignored)
        {
            return;
        }

        refreshGrid();
    }

    private void bindRanksField(SkillType skillType, TextField ranksField)
    {
        ranksField.setOnAction(event ->
        {
            applyRanks(skillType, ranksField);
            requestFocus();
        });
        ranksField.focusedProperty().addListener((ignored, hadFocus, hasFocus) ->
        {
            if (!hasFocus && hadFocus)
            {
                applyRanks(skillType, ranksField);
            }
        });
        ranksField.setOnKeyPressed(event ->
        {
            if (event.getCode() == KeyCode.ESCAPE)
            {
                ranksField.setText(Integer.toString(getSkillData(skillType).ranks()));
                requestFocus();
            }
        });
    }

    private void bindSpecializationRanksField(SkillType skillType, String specialization, TextField ranksField)
    {
        ranksField.setOnAction(event ->
        {
            applySpecializationRanks(skillType, specialization, ranksField);
            requestFocus();
        });
        ranksField.focusedProperty().addListener((ignored, hadFocus, hasFocus) ->
        {
            if (!hasFocus && hadFocus)
            {
                applySpecializationRanks(skillType, specialization, ranksField);
            }
        });
        ranksField.setOnKeyPressed(event ->
        {
            if (event.getCode() == KeyCode.ESCAPE)
            {
                ranksField.setText(Integer.toString(getSkillData(skillType, specialization).ranks()));
                requestFocus();
            }
        });
    }

    private void applyRanks(SkillType skillType, TextField ranksField)
    {
        String rawValue = ranksField.getText() == null ? "" : ranksField.getText().trim();

        try
        {
            sheet.setSkillRanks(skillType, Integer.parseInt(rawValue));
        }
        catch (IllegalArgumentException ignored)
        {
        }

        refreshGrid();
    }

    private void applySpecializationRanks(SkillType skillType, String specialization, TextField ranksField)
    {
        String rawValue = ranksField.getText() == null ? "" : ranksField.getText().trim();

        try
        {
            sheet.setSkillRanks(skillType, specialization, Integer.parseInt(rawValue));
        }
        catch (IllegalArgumentException ignored)
        {
        }

        refreshGrid();
    }

    private VBox buildDetailsRow(SkillData skillData)
    {
        VBox detailsRow = new VBox(4);
        detailsRow.setMaxWidth(TABLE_WIDTH);
        detailsRow.setPadding(new Insets(8));
        detailsRow.setStyle("-fx-border-color: #666666; -fx-border-width: 1 1 1 1;");

        if (skillData.modifiers().isEmpty())
        {
            detailsRow.getChildren().add(buildDetailsLabel("Nessun bonus o malus"));
            return detailsRow;
        }

        for (ModifierData modifier : skillData.modifiers())
        {
            detailsRow.getChildren().add(buildDetailsLabel(formatModifier(modifier)));
        }

        return detailsRow;
    }

    private String formatModifier(int modifier)
    {
        return modifier >= 0 ? "+" + modifier : Integer.toString(modifier);
    }

    private String formatModifier(ModifierData modifier)
    {
        return ModifierDisplayFormatter.format(modifier);
    }

    private SkillData getSkillData(SkillType skillType)
    {
        return sheet.getSkill(skillType);
    }

    private SkillData getSkillData(SkillType skillType, String specialization)
    {
        return sheet.getSkill(skillType, specialization);
    }

    private Label buildDetailsLabel(String text)
    {
        Label label = new Label(text);
        label.setWrapText(true);
        label.setMaxWidth(TABLE_WIDTH - 16);
        return label;
    }

    private Button buildGearButton(String rowKey)
    {
        Button gearButton = new Button("⚙");
        gearButton.setVisible(isExpanded(rowKey));
        gearButton.setManaged(gearButton.isVisible());
        gearButton.setFocusTraversable(false);
        gearButton.setPadding(new Insets(0));
        gearButton.setMinSize(16, 16);
        gearButton.setPrefSize(16, 16);
        gearButton.setStyle("-fx-background-color: transparent; -fx-cursor: hand;");
        gearButton.setOnAction(event -> toggleDetails(rowKey));
        return gearButton;
    }

    private void bindGearVisibility(Region rowRegion, Button gearButton, String rowKey)
    {
        rowRegion.setOnMouseEntered(event ->
        {
            gearButton.setVisible(true);
            gearButton.setManaged(true);
        });
        rowRegion.setOnMouseExited(event ->
        {
            if (!isExpanded(rowKey))
            {
                gearButton.setVisible(false);
                gearButton.setManaged(false);
            }
        });
    }

    private void styleCell(Label label, double minWidth, boolean firstRow, int column, boolean expanded)
    {
        String borderWidth = buildBorderWidth(firstRow, column, expanded);

        label.setMinWidth(minWidth);
        label.setPrefWidth(minWidth);
        label.setMaxWidth(minWidth);
        label.setMinHeight(ROW_HEIGHT);
        label.setPrefHeight(ROW_HEIGHT);
        label.setMaxHeight(ROW_HEIGHT);
        label.setAlignment(Pos.CENTER);
        label.setPadding(new Insets(6, 8, 6, 8));
        label.setStyle("-fx-border-color: #666666; -fx-border-width: " + borderWidth + ";");
    }

    private void styleEditableCell(TextField textField, double minWidth, boolean firstRow, int column, boolean expanded)
    {
        String borderWidth = buildBorderWidth(firstRow, column, expanded);

        textField.setMinWidth(minWidth);
        textField.setPrefWidth(minWidth);
        textField.setMaxWidth(minWidth);
        textField.setMinHeight(ROW_HEIGHT);
        textField.setPrefHeight(ROW_HEIGHT);
        textField.setMaxHeight(ROW_HEIGHT);
        textField.setAlignment(Pos.CENTER);
        textField.setPadding(new Insets(6, 8, 6, 8));
        textField.setStyle("-fx-border-color: #666666; -fx-border-width: " + borderWidth + "; -fx-background-insets: 0; -fx-background-radius: 0;");
    }

    private void styleContainerCell(Region region, double minWidth, boolean firstRow, int column, boolean expanded)
    {
        String borderWidth = buildBorderWidth(firstRow, column, expanded);

        region.setMinWidth(minWidth);
        region.setPrefWidth(minWidth);
        region.setMaxWidth(minWidth);
        region.setMinHeight(ROW_HEIGHT);
        region.setPrefHeight(ROW_HEIGHT);
        region.setMaxHeight(ROW_HEIGHT);
        region.setPadding(new Insets(6, 8, 6, 8));
        region.setStyle("-fx-border-color: #666666; -fx-border-width: " + borderWidth + ";");
    }

    private String buildBorderWidth(boolean firstRow, int column, boolean expanded)
    {
        int top = firstRow ? 1 : 0;
        int right = 1;
        int bottom = expanded ? 0 : 1;
        int left = column == 0 ? 1 : 0;

        return top + " " + right + " " + bottom + " " + left;
    }

    private boolean isExpanded(String rowKey)
    {
        return expandedRows.contains(rowKey);
    }

    private String buildSummaryRowKey(SkillType skillType)
    {
        return skillType.name();
    }

    private String buildSpecializationRowKey(SkillType skillType, String specialization)
    {
        return skillType.name() + "::" + specialization;
    }

    private void toggleDetails(String rowKey)
    {
        if (isExpanded(rowKey))
        {
            expandedRows.remove(rowKey);
        }
        else
        {
            expandedRows.add(rowKey);
        }

        refreshGrid();
    }

    private void refreshGrid()
    {
        setTop(buildSkillList());
    }

    public void refresh()
    {
        refreshGrid();
    }
}
