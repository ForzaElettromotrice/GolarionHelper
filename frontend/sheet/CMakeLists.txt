qt_add_qml_module(golarion_sheet_ui STATIC
    URI Golarion.Sheet
    VERSION 1.0
    RESOURCES open-book.svg die-d6.svg die-d8.svg die-d10.svg die-d12.svg character-silhouette.svg
    CLASS_NAME GolarionSheetPlugin
    RESOURCE_PREFIX /qt/qml
    QML_FILES InventoryPanel.qml InventoryCard.qml
    QML_FILES AttackDetails.qml DamageText.js CombatManeuversPanel.qml LanguagesPanel.qml FeatsPanel.qml FeatCatalogPanel.qml RacePanel.qml ClassesPanel.qml HitDieButton.qml FavoredBonusIcons.qml
    QML_FILES IdentityPanel.qml AbilityScoresPanel.qml SkillsPanel.qml SkillDetails.qml InfoButton.qml InfoDetails.qml InfoFrame.qml SavingThrowsPanel.qml ArmorClassPanel.qml InitiativePanel.qml MovementPanel.qml AttacksPanel.qml ActionCostIcon.qml HitPointsPanel.qml DefensesPanel.qml DefenseShield.qml ModifierText.js
)
target_link_libraries(golarion_sheet_ui PUBLIC Qt6::Quick Qt6::QuickControls2)
