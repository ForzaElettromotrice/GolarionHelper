qt_add_qml_module(golarion_sheet_ui STATIC
    URI Golarion.Sheet
    VERSION 1.0
    RESOURCES open-book.svg die-d6.svg die-d8.svg die-d10.svg die-d12.svg character-silhouette.svg
    RESOURCES
        condition-icons/droplet.svg
        condition-icons/eye-off.svg
        condition-icons/shuffle.svg
        condition-icons/battery-low.svg
        condition-icons/knocked-out-stars.svg
        condition-icons/sun.svg
        condition-icons/skull.svg
        condition-icons/ear-off.svg
        condition-icons/circle-minus.svg
        condition-icons/heart-pulse.svg
        condition-icons/fishing-net.svg
        condition-icons/sparkles.svg
        condition-icons/ghost.svg
        condition-icons/footprints.svg
        condition-icons/link-2.svg
        condition-icons/shield-off.svg
        condition-icons/wind.svg
        condition-icons/person-standing.svg
        condition-icons/face-vomiting.svg
        condition-icons/trending-down.svg
        condition-icons/lock-keyhole.svg
        condition-icons/mountain.svg
        condition-icons/punch.svg
        condition-icons/pill.svg
        condition-icons/heart-plus.svg
        condition-icons/route.svg
        condition-icons/zap.svg
        condition-icons/sleep.svg
        condition-icons/despair.svg
        condition-icons/LICENSE-Lucide.txt
        condition-icons/LICENSE-Apache-2.0.txt
        condition-icons/CREDITS.txt
    CLASS_NAME GolarionSheetPlugin
    RESOURCE_PREFIX /qt/qml
    QML_FILES InventoryPanel.qml InventoryCard.qml MoneyPanel.qml SpellcastingsPanel.qml GrantedSpellsPanel.qml EnhancementsPanel.qml NotesPanel.qml ConditionsPanel.qml ConditionIcons.js RemindersPanel.qml PendingChoicesPanel.qml SheetView.qml
    QML_FILES AttackDetails.qml DamageText.js CombatManeuversPanel.qml ActionsPanel.qml LanguagesPanel.qml FeatsPanel.qml FeatCatalogPanel.qml RacePanel.qml ClassesPanel.qml HitDieButton.qml FavoredBonusIcons.qml TalentsTab.qml
    QML_FILES IdentityPanel.qml AbilityScoresPanel.qml SkillsPanel.qml SkillDetails.qml InfoButton.qml InfoDetails.qml InfoFrame.qml SavingThrowsPanel.qml SituationalBonusCards.qml ArmorClassPanel.qml InitiativePanel.qml MovementPanel.qml AttacksPanel.qml ActionCostIcon.qml HitPointsPanel.qml DefensesPanel.qml DefenseShield.qml ShieldShape.js ModifierText.js
)
target_link_libraries(golarion_sheet_ui PUBLIC Qt6::Quick Qt6::QuickControls2)
