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
import org.golarion.model.character.CharacterSheet;
import org.golarion.model.character.skill.SkillEntry;
import org.golarion.model.character.skill.SkillType;

import java.util.Map;

public class CharacterSkillsView extends BorderPane
{
    private static final double CLASS_COLUMN_WIDTH = 32;
    private static final double NAME_COLUMN_WIDTH = 260;
    private static final double VALUE_COLUMN_WIDTH = 48;
    private static final double RANKS_COLUMN_WIDTH = 48;
    private static final double ROW_HEIGHT = 34;

    private final CharacterSheet sheet;

    public CharacterSkillsView(CharacterSheet sheet)
    {
        if (sheet == null)
        {
            throw new IllegalArgumentException("sheet must not be null");
        }

        this.sheet = sheet;

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
            list.getChildren().add(buildSummaryRow(skillType, firstVisibleRow));
            firstVisibleRow = false;

            if (!skillType.isRequiresSpecialization())
            {
                continue;
            }

            for (Map.Entry<String, SkillEntry> specializationEntry : sheet.getSkills().getSpecializations(skillType).entrySet())
            {
                list.getChildren().add(buildSpecializationRow(skillType, specializationEntry.getKey(), specializationEntry.getValue()));
            }
        }

        return list;
    }

    private GridPane buildSummaryRow(SkillType skillType, boolean firstRow)
    {
        GridPane row = new GridPane();
        row.setAlignment(Pos.TOP_LEFT);
        row.setMaxWidth(Region.USE_PREF_SIZE);

        Region classSkillCell = buildClassSkillCell(skillType);
        Region nameCell = buildSkillNameCell(skillType);
        Label valueLabel = new Label(formatModifier(sheet.getSkillModifier(skillType)));
        TextField ranksField = buildRanksField(skillType);

        styleContainerCell(classSkillCell, CLASS_COLUMN_WIDTH, firstRow, 0);
        styleContainerCell(nameCell, NAME_COLUMN_WIDTH, firstRow, 1);
        styleCell(valueLabel, VALUE_COLUMN_WIDTH, firstRow, 2);
        styleEditableCell(ranksField, RANKS_COLUMN_WIDTH, firstRow, 3);

        row.add(classSkillCell, 0, 0);
        row.add(nameCell, 1, 0);
        row.add(valueLabel, 2, 0);
        row.add(ranksField, 3, 0);

        return row;
    }

    private GridPane buildSpecializationRow(SkillType skillType, String specialization, SkillEntry entry)
    {
        GridPane row = new GridPane();
        row.setAlignment(Pos.TOP_LEFT);
        row.setMaxWidth(Region.USE_PREF_SIZE);

        Region deleteCell = buildDeleteCell(skillType, specialization);
        Label nameLabel = new Label(specialization);
        Label valueLabel = new Label(formatModifier(sheet.getSkillModifier(skillType, specialization)));
        TextField ranksField = new TextField(Integer.toString(entry.getRanks()));

        bindSpecializationRanksField(skillType, specialization, ranksField);

        styleContainerCell(deleteCell, CLASS_COLUMN_WIDTH, false, 0);
        styleIndentedNameCell(nameLabel, false, 1);
        styleCell(valueLabel, VALUE_COLUMN_WIDTH, false, 2);
        styleEditableCell(ranksField, RANKS_COLUMN_WIDTH, false, 3);

        row.add(deleteCell, 0, 0);
        row.add(nameLabel, 1, 0);
        row.add(valueLabel, 2, 0);
        row.add(ranksField, 3, 0);

        return row;
    }

    private Region buildClassSkillCell(SkillType skillType)
    {
        CheckBox checkBox = new CheckBox();
        checkBox.setSelected(sheet.getSkills().isClassSkill(skillType));
        checkBox.setFocusTraversable(false);
        checkBox.setOnAction(event ->
        {
            sheet.getSkills().setClassSkill(skillType, checkBox.isSelected());
            refreshGrid();
        });

        StackPane cell = new StackPane(checkBox);
        cell.setAlignment(Pos.CENTER);
        return cell;
    }

    private Region buildSkillNameCell(SkillType skillType)
    {
        if (!skillType.isRequiresSpecialization())
        {
            HBox nameCell = new HBox();
            nameCell.setAlignment(Pos.CENTER_LEFT);
            nameCell.getChildren().add(new Label(skillType.getDisplayName()));
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

        HBox nameCell = new HBox(8, nameLabel, addField);
        nameCell.setAlignment(Pos.CENTER_LEFT);
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
            sheet.getSkills().removeSpecialization(skillType, specialization);
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

        SkillEntry entry = sheet.getSkills().get(skillType);
        ranksField.setText(Integer.toString(entry.getRanks()));
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
            sheet.getSkills().getOrCreateSpecialization(skillType, specialization);
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
                ranksField.setText(Integer.toString(sheet.getSkills().get(skillType).getRanks()));
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
                ranksField.setText(Integer.toString(sheet.getSkills().getSpecialization(skillType, specialization).getRanks()));
                requestFocus();
            }
        });
    }

    private void applyRanks(SkillType skillType, TextField ranksField)
    {
        String rawValue = ranksField.getText() == null ? "" : ranksField.getText().trim();

        try
        {
            sheet.getSkills().setRanks(skillType, Integer.parseInt(rawValue));
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
            sheet.getSkills().setRanks(skillType, specialization, Integer.parseInt(rawValue));
        }
        catch (IllegalArgumentException ignored)
        {
        }

        refreshGrid();
    }

    private String formatModifier(int modifier)
    {
        return modifier >= 0 ? "+" + modifier : Integer.toString(modifier);
    }

    private void styleCell(Label label, double minWidth, boolean firstRow, int column)
    {
        String borderWidth = buildBorderWidth(firstRow, column);

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

    private void styleEditableCell(TextField textField, double minWidth, boolean firstRow, int column)
    {
        String borderWidth = buildBorderWidth(firstRow, column);

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

    private void styleContainerCell(Region region, double minWidth, boolean firstRow, int column)
    {
        String borderWidth = buildBorderWidth(firstRow, column);

        region.setMinWidth(minWidth);
        region.setPrefWidth(minWidth);
        region.setMaxWidth(minWidth);
        region.setMinHeight(ROW_HEIGHT);
        region.setPrefHeight(ROW_HEIGHT);
        region.setMaxHeight(ROW_HEIGHT);
        region.setPadding(new Insets(6, 8, 6, 8));
        region.setStyle("-fx-border-color: #666666; -fx-border-width: " + borderWidth + ";");
    }

    private void styleIndentedNameCell(Label label, boolean firstRow, int column)
    {
        String borderWidth = buildBorderWidth(firstRow, column);

        label.setMinWidth(NAME_COLUMN_WIDTH);
        label.setPrefWidth(NAME_COLUMN_WIDTH);
        label.setMaxWidth(NAME_COLUMN_WIDTH);
        label.setAlignment(Pos.CENTER_LEFT);
        label.setPadding(new Insets(6, 8, 6, 24));
        label.setStyle("-fx-border-color: #666666; -fx-border-width: " + borderWidth + ";");
    }

    private String buildBorderWidth(boolean firstRow, int column)
    {
        int top = firstRow ? 1 : 0;
        int right = 1;
        int bottom = 1;
        int left = column == 0 ? 1 : 0;

        return top + " " + right + " " + bottom + " " + left;
    }

    private void refreshGrid()
    {
        setTop(buildSkillList());
    }
}
