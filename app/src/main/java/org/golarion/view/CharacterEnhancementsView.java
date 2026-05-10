package org.golarion.view;

import javafx.application.Platform;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.*;
import javafx.scene.input.KeyCode;
import javafx.scene.layout.*;
import javafx.util.StringConverter;
import lombok.NonNull;
import org.golarion.model.api.EffectEntryData;
import org.golarion.model.api.EffectGroupData;
import org.golarion.model.character.CharacterSheet;
import org.golarion.model.character.modifier.BonusType;
import org.golarion.model.character.modifier.ModifierType;

import java.util.*;

public class CharacterEnhancementsView extends BorderPane
{
    private static final double EXPRESSION_WIDTH = 120;
    private static final double TARGET_WIDTH = 150;
    private static final double SOURCE_WIDTH = 120;
    private static final double DESCRIPTION_WIDTH = 180;
    private static final double TYPE_WIDTH = 90;
    private static final double BONUS_TYPE_WIDTH = 140;
    private static final double REMOVE_WIDTH = 32;

    private final CharacterSheet sheet;
    private final Map<UUID, GroupEditorState> groupStates;
    private final Runnable onEffectsChanged;
    private final Map<ControlKey, Control> controlsByKey;
    private PendingFocus pendingFocus;

    public CharacterEnhancementsView(@NonNull CharacterSheet sheet)
    {
        this(sheet, () ->
        {
        });
    }

    public CharacterEnhancementsView(@NonNull CharacterSheet sheet, @NonNull Runnable onEffectsChanged)
    {
        this.sheet = sheet;
        this.groupStates = new LinkedHashMap<>();
        this.onEffectsChanged = onEffectsChanged;
        this.controlsByKey = new HashMap<>();
        this.pendingFocus = null;

        setPadding(new Insets(12));
        refresh();
    }

    public void refresh()
    {
        syncGroupStates();
        controlsByKey.clear();
        setCenter(buildContent());
        restorePendingFocus();
    }

    private VBox buildContent()
    {
        VBox content = new VBox(12);
        content.setAlignment(Pos.TOP_LEFT);

        Button addGroupButton = new Button("+");
        addGroupButton.setFocusTraversable(false);
        addGroupButton.setOnAction(event -> createEffectGroup());

        HBox header = new HBox(8, new Label("Potenziamenti"), addGroupButton);
        header.setAlignment(Pos.CENTER_LEFT);

        content.getChildren().add(header);

        for (GroupEditorState groupState : groupStates.values())
        {
            content.getChildren().add(buildGroupCard(groupState));
        }

        return content;
    }

    private VBox buildGroupCard(GroupEditorState groupState)
    {
        VBox groupCard = new VBox(8);
        groupCard.setPadding(new Insets(10));
        groupCard.setStyle("-fx-border-color: #666666; -fx-border-width: 1; -fx-background-color: white;");

        TextField titleField = new TextField(groupState.name);
        titleField.setPrefWidth(220);
        titleField.setStyle("-fx-font-weight: bold;");
        titleField.setOnAction(event -> applyGroupName(groupState, titleField));
        titleField.focusedProperty().addListener((ignored, hadFocus, hasFocus) ->
        {
            if (!hasFocus && hadFocus)
            {
                applyGroupName(groupState, titleField);
            }
        });

        CheckBox enabledCheckBox = new CheckBox("Attivo");
        enabledCheckBox.setSelected(groupState.enabled);
        enabledCheckBox.setFocusTraversable(false);
        enabledCheckBox.setOnAction(event ->
        {
            sheet.setEffectGroupEnabled(groupState.groupId, enabledCheckBox.isSelected());
            groupState.enabled = enabledCheckBox.isSelected();
            onEffectsChanged.run();
            refresh();
        });

        Button removeGroupButton = new Button("x");
        removeGroupButton.setFocusTraversable(false);
        removeGroupButton.setOnAction(event ->
        {
            sheet.removeEffectGroup(groupState.groupId);
            onEffectsChanged.run();
            refresh();
        });

        HBox groupHeader = new HBox(12, titleField, enabledCheckBox, removeGroupButton);
        groupHeader.setAlignment(Pos.CENTER_LEFT);
        groupCard.getChildren().add(groupHeader);

        VBox rows = new VBox(6);
        for (EffectEditorState rowState : groupState.rows)
        {
            rows.getChildren().add(buildEffectRow(groupState, rowState));
        }

        groupCard.getChildren().add(rows);
        return groupCard;
    }

    private HBox buildEffectRow(GroupEditorState groupState, EffectEditorState rowState)
    {
        HBox row = new HBox(6);
        row.setAlignment(Pos.CENTER_LEFT);

        TextField expressionField = new TextField(rowState.expression);
        expressionField.setPromptText("Espressione");
        expressionField.setPrefWidth(EXPRESSION_WIDTH);

        ComboBox<String> targetBox = new ComboBox<>();
        targetBox.getItems().addAll(sheet.getEffectTargets());
        targetBox.setPrefWidth(TARGET_WIDTH);
        targetBox.setValue(rowState.targetName);
        targetBox.setPromptText("Target");

        TextField sourceField = new TextField(rowState.source);
        sourceField.setPromptText("Source");
        sourceField.setPrefWidth(SOURCE_WIDTH);

        TextField descriptionField = new TextField(rowState.description);
        descriptionField.setPromptText("Descrizione");
        descriptionField.setPrefWidth(DESCRIPTION_WIDTH);

        ComboBox<ModifierType> typeBox = new ComboBox<>();
        typeBox.getItems().addAll(ModifierType.values());
        typeBox.setConverter(new StringConverter<>()
        {
            @Override
            public String toString(ModifierType value)
            {
                return value == null ? "" : value.getName();
            }

            @Override
            public ModifierType fromString(String string)
            {
                return Arrays.stream(ModifierType.values())
                        .filter(value -> value.getName().equals(string))
                        .findFirst()
                        .orElse(null);
            }
        });
        typeBox.setPrefWidth(TYPE_WIDTH);
        typeBox.setValue(rowState.modifierType);

        ComboBox<BonusType> bonusTypeBox = new ComboBox<>();
        bonusTypeBox.getItems().addAll(BonusType.values());
        bonusTypeBox.setConverter(new StringConverter<>()
        {
            @Override
            public String toString(BonusType value)
            {
                return value == null ? "" : value.getDisplayName();
            }

            @Override
            public BonusType fromString(String string)
            {
                return Arrays.stream(BonusType.values())
                        .filter(value -> value.getDisplayName().equals(string))
                        .findFirst()
                        .orElse(null);
            }
        });
        bonusTypeBox.setPrefWidth(BONUS_TYPE_WIDTH);
        bonusTypeBox.setValue(rowState.bonusType);
        updateBonusTypeVisibility(bonusTypeBox, rowState.modifierType);

        registerControl(rowState, FocusField.EXPRESSION, expressionField);
        registerControl(rowState, FocusField.TARGET, targetBox);
        registerControl(rowState, FocusField.SOURCE, sourceField);
        registerControl(rowState, FocusField.DESCRIPTION, descriptionField);
        registerControl(rowState, FocusField.TYPE, typeBox);
        registerControl(rowState, FocusField.BONUS_TYPE, bonusTypeBox);

        Button removeButton = new Button("x");
        removeButton.setFocusTraversable(false);
        removeButton.setPrefWidth(REMOVE_WIDTH);
        removeButton.setDisable(rowState.effectId == null && rowState.isBlank());
        removeButton.setOnAction(event -> removeRow(groupState, rowState));

        expressionField.textProperty().addListener((ignored, oldValue, newValue) ->
        {
            rowState.expression = newValue == null ? "" : newValue.trim();
            boolean refreshed = onRowInteracted(groupState, rowState, FocusField.EXPRESSION, expressionField);
            if (!refreshed)
            {
                commitRow(groupState, rowState);
            }
        });
        targetBox.valueProperty().addListener((ignored, oldValue, newValue) ->
        {
            rowState.targetName = newValue;
            boolean refreshed = onRowInteracted(groupState, rowState, FocusField.TARGET, targetBox);
            if (!refreshed)
            {
                commitRow(groupState, rowState);
            }
        });
        sourceField.textProperty().addListener((ignored, oldValue, newValue) ->
        {
            rowState.source = newValue == null ? "" : newValue.trim();
            boolean refreshed = onRowInteracted(groupState, rowState, FocusField.SOURCE, sourceField);
            if (!refreshed)
            {
                commitRow(groupState, rowState);
            }
        });
        descriptionField.textProperty().addListener((ignored, oldValue, newValue) ->
        {
            rowState.description = newValue == null ? "" : newValue.trim();
            boolean refreshed = onRowInteracted(groupState, rowState, FocusField.DESCRIPTION, descriptionField);
            if (!refreshed)
            {
                commitRow(groupState, rowState);
            }
        });
        typeBox.valueProperty().addListener((ignored, oldValue, newValue) ->
        {
            rowState.modifierType = newValue == null ? ModifierType.BONUS : newValue;
            if (rowState.modifierType == ModifierType.PENALTY)
            {
                rowState.bonusType = null;
            }
            else if (rowState.bonusType == null)
            {
                rowState.bonusType = BonusType.ALCHEMICAL;
            }
            updateBonusTypeVisibility(bonusTypeBox, rowState.modifierType);
            boolean refreshed = onRowInteracted(groupState, rowState, FocusField.TYPE, typeBox);
            if (!refreshed)
            {
                commitRow(groupState, rowState);
            }
        });
        bonusTypeBox.valueProperty().addListener((ignored, oldValue, newValue) ->
        {
            rowState.bonusType = newValue;
            boolean refreshed = onRowInteracted(groupState, rowState, FocusField.BONUS_TYPE, bonusTypeBox);
            if (!refreshed)
            {
                commitRow(groupState, rowState);
            }
        });

        bindCommit(expressionField, groupState, rowState);
        bindCommit(sourceField, groupState, rowState);
        bindCommit(descriptionField, groupState, rowState);
        targetBox.setOnAction(event -> commitRow(groupState, rowState));

        row.getChildren().addAll(expressionField, targetBox, sourceField, descriptionField, typeBox, bonusTypeBox, removeButton);
        return row;
    }

    private void bindCommit(TextField field, GroupEditorState groupState, EffectEditorState rowState)
    {
        field.setOnAction(event -> commitRow(groupState, rowState));
        field.focusedProperty().addListener((ignored, hadFocus, hasFocus) ->
        {
            if (!hasFocus && hadFocus)
            {
                commitRow(groupState, rowState);
            }
        });
        field.setOnKeyPressed(event ->
        {
            if (event.getCode() == KeyCode.ESCAPE)
            {
                refresh();
            }
        });
    }

    private void commitRow(GroupEditorState groupState, EffectEditorState rowState)
    {
        if (!rowState.isComplete())
        {
            return;
        }

        try
        {
            if (rowState.effectId != null)
            {
                sheet.removeEffect(groupState.groupId, rowState.effectId);
                rowState.effectId = null;
            }

            UUID newEffectId = sheet.addEffect(
                    groupState.groupId,
                    rowState.modifierType,
                    rowState.modifierType == ModifierType.BONUS ? rowState.bonusType : null,
                    rowState.expression,
                    rowState.targetName,
                    rowState.source,
                    rowState.description
            );

            rowState.effectId = newEffectId;
            onEffectsChanged.run();
        }
        catch (IllegalArgumentException ignored)
        {
        }
    }

    private void removeRow(GroupEditorState groupState, EffectEditorState rowState)
    {
        if (rowState.effectId != null)
        {
            sheet.removeEffect(groupState.groupId, rowState.effectId);
            onEffectsChanged.run();
        }

        groupState.rows.remove(rowState);
        ensureTrailingBlankRow(groupState);
        refresh();
    }

    private void updateBonusTypeVisibility(ComboBox<BonusType> bonusTypeBox, ModifierType modifierType)
    {
        boolean visible = modifierType == ModifierType.BONUS;
        bonusTypeBox.setVisible(visible);
        bonusTypeBox.setManaged(visible);
    }

    private boolean onRowInteracted(GroupEditorState groupState, EffectEditorState rowState, FocusField focusField, Control control)
    {
        if (!rowState.interacted)
        {
            rowState.interacted = true;
            ensureTrailingBlankRow(groupState);
            pendingFocus = PendingFocus.from(rowState, focusField, control);
            refresh();
            return true;
        }

        return false;
    }

    private void createEffectGroup()
    {
        int nextIndex = groupStates.size() + 1;
        UUID groupId = sheet.createEffectGroup("Potenziamento " + nextIndex);
        GroupEditorState groupState = new GroupEditorState(groupId, "Potenziamento " + nextIndex);
        groupState.rows.add(new EffectEditorState());
        groupStates.put(groupId, groupState);
        refresh();
    }

    private void applyGroupName(GroupEditorState groupState, TextField titleField)
    {
        String newName = titleField.getText() == null ? "" : titleField.getText().trim();
        if (newName.isEmpty() || newName.equals(groupState.name))
        {
            titleField.setText(groupState.name);
            return;
        }

        try
        {
            sheet.setEffectGroupName(groupState.groupId, newName);
            groupState.name = newName;
        }
        catch (IllegalArgumentException ignored)
        {
            titleField.setText(groupState.name);
        }
    }

    private void registerControl(EffectEditorState rowState, FocusField focusField, Control control)
    {
        controlsByKey.put(new ControlKey(rowState, focusField), control);
    }

    private void restorePendingFocus()
    {
        if (pendingFocus == null)
        {
            return;
        }

        PendingFocus focusToRestore = pendingFocus;
        pendingFocus = null;

        Control control = controlsByKey.get(new ControlKey(focusToRestore.rowState, focusToRestore.focusField));
        if (control == null)
        {
            return;
        }

        Platform.runLater(() ->
        {
            control.requestFocus();
            if (control instanceof TextInputControl textInputControl)
            {
                textInputControl.positionCaret(Math.min(focusToRestore.caretPosition, textInputControl.getText().length()));
            }
        });
    }

    private void syncGroupStates()
    {
        Map<UUID, EffectGroupData> groupsById = new LinkedHashMap<>();
        for (EffectGroupData effectGroupData : sheet.getEffectGroups())
        {
            groupsById.put(effectGroupData.id(), effectGroupData);
        }

        groupStates.keySet().removeIf(groupId -> !groupsById.containsKey(groupId));

        for (EffectGroupData effectGroupData : groupsById.values())
        {
            GroupEditorState groupState = groupStates.computeIfAbsent(
                    effectGroupData.id(),
                    groupId -> new GroupEditorState(groupId, effectGroupData.name())
            );
            groupState.name = effectGroupData.name();
            groupState.enabled = effectGroupData.entries().stream().allMatch(EffectEntryData::enabled);

            Map<UUID, EffectEditorState> existingRowsById = new LinkedHashMap<>();
            List<EffectEditorState> draftRows = new ArrayList<>();
            for (EffectEditorState rowState : groupState.rows)
            {
                if (rowState.effectId == null)
                {
                    draftRows.add(rowState);
                }
                else
                {
                    existingRowsById.put(rowState.effectId, rowState);
                }
            }

            List<EffectEditorState> mergedRows = new ArrayList<>();
            for (EffectEntryData effectEntryData : effectGroupData.entries())
            {
                EffectEditorState rowState = existingRowsById.get(effectEntryData.id());
                if (rowState == null)
                {
                    rowState = new EffectEditorState();
                }
                rowState.loadFrom(effectEntryData);
                mergedRows.add(rowState);
            }

            for (EffectEditorState draftRow : draftRows)
            {
                if (!draftRow.isBlank())
                {
                    mergedRows.add(draftRow);
                }
            }

            groupState.rows.clear();
            groupState.rows.addAll(mergedRows);
            ensureTrailingBlankRow(groupState);
        }
    }

    private void ensureTrailingBlankRow(GroupEditorState groupState)
    {
        if (groupState.rows.isEmpty() || !groupState.rows.get(groupState.rows.size() - 1).isBlank())
        {
            groupState.rows.add(new EffectEditorState());
        }
    }

    private static class GroupEditorState
    {
        private final UUID groupId;
        private final List<EffectEditorState> rows;
        private String name;
        private boolean enabled;

        private GroupEditorState(UUID groupId, String name)
        {
            this.groupId = groupId;
            this.name = name;
            this.enabled = true;
            this.rows = new ArrayList<>();
        }
    }

    private static class EffectEditorState
    {
        private UUID effectId;
        private String expression;
        private String targetName;
        private String source;
        private String description;
        private ModifierType modifierType;
        private BonusType bonusType;
        private boolean interacted;

        private EffectEditorState()
        {
            this.effectId = null;
            this.expression = "";
            this.targetName = null;
            this.source = "";
            this.description = "";
            this.modifierType = ModifierType.BONUS;
            this.bonusType = BonusType.ALCHEMICAL;
            this.interacted = false;
        }

        private void loadFrom(EffectEntryData effectEntryData)
        {
            this.effectId = effectEntryData.id();
            this.expression = effectEntryData.expression();
            this.targetName = effectEntryData.targetName();
            this.source = effectEntryData.source();
            this.description = effectEntryData.description();
            this.modifierType = effectEntryData.modifierType() == null ? ModifierType.BONUS : effectEntryData.modifierType();
            this.bonusType = effectEntryData.bonusType() == null ? BonusType.ALCHEMICAL : effectEntryData.bonusType();
            this.interacted = true;
        }

        private boolean isBlank()
        {
            return expression.isBlank()
                    && (targetName == null || targetName.isBlank())
                    && source.isBlank()
                    && description.isBlank()
                    && effectId == null;
        }

        private boolean isComplete()
        {
            return !expression.isBlank()
                    && targetName != null
                    && !targetName.isBlank()
                    && !source.isBlank()
                    && (modifierType == ModifierType.PENALTY || bonusType != null);
        }
    }

    private enum FocusField
    {
        EXPRESSION,
        TARGET,
        SOURCE,
        DESCRIPTION,
        TYPE,
        BONUS_TYPE
    }

    private record ControlKey(EffectEditorState rowState, FocusField focusField)
    {
    }

    private record PendingFocus(EffectEditorState rowState, FocusField focusField, int caretPosition)
    {
        private static PendingFocus from(EffectEditorState rowState, FocusField focusField, Control control)
        {
            int caretPosition = control instanceof TextInputControl textInputControl
                    ? textInputControl.getText().length()
                    : 0;
            return new PendingFocus(rowState, focusField, caretPosition);
        }
    }
}
