set(sheet_qml_files
    SheetView.qml
    components/ActionCostIcon.qml
    components/AttackDetails.qml
    components/DefenseShield.qml
    components/FavoredBonusIcons.qml
    components/HitDieButton.qml
    components/InfoButton.qml
    components/InfoDetails.qml
    components/InfoFrame.qml
    components/InventoryCard.qml
    components/SituationalBonusCards.qml
    components/SkillDetails.qml
    helpers/ConditionIcons.js
    helpers/DamageText.js
    helpers/ModifierText.js
    helpers/ShieldShape.js
    panels/AbilityScoresPanel.qml
    panels/ActionsPanel.qml
    panels/ArmorClassPanel.qml
    panels/AttacksPanel.qml
    panels/ClassesPanel.qml
    panels/CombatManeuversPanel.qml
    panels/ConditionsPanel.qml
    panels/DefensesPanel.qml
    panels/EnhancementsPanel.qml
    panels/FeatCatalogPanel.qml
    panels/FeatsPanel.qml
    panels/GrantedSpellsPanel.qml
    panels/HitPointsPanel.qml
    panels/IdentityPanel.qml
    panels/InitiativePanel.qml
    panels/InventoryPanel.qml
    panels/LanguagesPanel.qml
    panels/MoneyPanel.qml
    panels/MovementPanel.qml
    panels/NotesPanel.qml
    panels/PendingChoicesPanel.qml
    panels/RacePanel.qml
    panels/RemindersPanel.qml
    panels/SavingThrowsPanel.qml
    panels/SkillsPanel.qml
    panels/SpellcastingsPanel.qml
    panels/TalentsTab.qml
)

set(sheet_assets
    assets/open-book.svg
    assets/die-d6.svg
    assets/die-d8.svg
    assets/die-d10.svg
    assets/die-d12.svg
    assets/character-silhouette.svg
    assets/condition-icons/droplet.svg
    assets/condition-icons/eye-off.svg
    assets/condition-icons/shuffle.svg
    assets/condition-icons/battery-low.svg
    assets/condition-icons/knocked-out-stars.svg
    assets/condition-icons/sun.svg
    assets/condition-icons/skull.svg
    assets/condition-icons/ear-off.svg
    assets/condition-icons/circle-minus.svg
    assets/condition-icons/heart-pulse.svg
    assets/condition-icons/fishing-net.svg
    assets/condition-icons/sparkles.svg
    assets/condition-icons/ghost.svg
    assets/condition-icons/footprints.svg
    assets/condition-icons/link-2.svg
    assets/condition-icons/shield-off.svg
    assets/condition-icons/wind.svg
    assets/condition-icons/person-standing.svg
    assets/condition-icons/face-vomiting.svg
    assets/condition-icons/trending-down.svg
    assets/condition-icons/lock-keyhole.svg
    assets/condition-icons/mountain.svg
    assets/condition-icons/punch.svg
    assets/condition-icons/pill.svg
    assets/condition-icons/heart-plus.svg
    assets/condition-icons/route.svg
    assets/condition-icons/zap.svg
    assets/condition-icons/sleep.svg
    assets/condition-icons/despair.svg
    assets/condition-icons/LICENSE-Lucide.txt
    assets/condition-icons/LICENSE-Apache-2.0.txt
    assets/condition-icons/CREDITS.txt
)

# Keep the module's existing type and resource URLs independent of source layout.
foreach(file IN LISTS sheet_qml_files)
    get_filename_component(resource_name "${file}" NAME)
    set_source_files_properties("${file}" PROPERTIES QT_RESOURCE_ALIAS "${resource_name}")
endforeach()
foreach(file IN LISTS sheet_assets)
    string(REGEX REPLACE "^assets/" "" resource_name "${file}")
    set_source_files_properties("${file}" PROPERTIES QT_RESOURCE_ALIAS "${resource_name}")
endforeach()

qt_add_qml_module(golarion_sheet_ui STATIC
    URI Golarion.Sheet
    VERSION 1.0
    CLASS_NAME GolarionSheetPlugin
    RESOURCE_PREFIX /qt/qml
    QML_FILES ${sheet_qml_files}
    RESOURCES ${sheet_assets}
)
target_link_libraries(golarion_sheet_ui PUBLIC Qt6::Quick Qt6::QuickControls2)
