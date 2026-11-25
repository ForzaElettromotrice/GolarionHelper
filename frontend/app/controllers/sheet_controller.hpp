#pragma once

#include <QObject>
#include <QFutureWatcher>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>
#include <memory>
#include <functional>
#include <utility>

namespace golarion { class CharacterSheet; class ClassDefinitionManager; struct CharacterSheetView; struct CharacterSheetSaveData; }

class SheetController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by the application")
    Q_PROPERTY(QVariantMap identity READ identity NOTIFY identityChanged FINAL)
    Q_PROPERTY(QVariantMap abilityScores READ abilityScores NOTIFY abilityScoresChanged FINAL)
    Q_PROPERTY(QVariantMap skillScores READ skillScores NOTIFY skillScoresChanged FINAL)
    Q_PROPERTY(QVariantMap savingThrows READ savingThrows NOTIFY savingThrowsChanged FINAL)
    Q_PROPERTY(QVariantMap armorClass READ armorClass NOTIFY armorClassChanged FINAL)
    Q_PROPERTY(QVariantMap hitPoints READ hitPoints NOTIFY hitPointsChanged FINAL)
    Q_PROPERTY(QVariantMap defenses READ defenses NOTIFY defensesChanged FINAL)
    Q_PROPERTY(QVariantMap movement READ movement NOTIFY movementChanged FINAL)
    Q_PROPERTY(QVariantMap activeConditions READ activeConditions NOTIFY activeConditionsChanged FINAL)
    Q_PROPERTY(QVariantMap initiative READ initiative NOTIFY initiativeChanged FINAL)
    Q_PROPERTY(QVariantMap attacks READ attacks NOTIFY attacksChanged FINAL)
    Q_PROPERTY(QVariantMap attackRoutines READ attackRoutines NOTIFY attackRoutinesChanged FINAL)
    Q_PROPERTY(QVariantMap actions READ actions NOTIFY actionsChanged FINAL)
    Q_PROPERTY(QVariantMap baseAttackBonus READ baseAttackBonus NOTIFY baseAttackBonusChanged FINAL)
    Q_PROPERTY(QVariantMap combatManeuvers READ combatManeuvers NOTIFY combatManeuversChanged FINAL)
    Q_PROPERTY(QVariantMap inventoryItems READ inventoryItems NOTIFY inventoryItemsChanged FINAL)
    Q_PROPERTY(QVariantMap inventoryContainers READ inventoryContainers NOTIFY inventoryContainersChanged FINAL)
    Q_PROPERTY(QVariantMap inventoryEquipment READ inventoryEquipment NOTIFY inventoryEquipmentChanged FINAL)
    Q_PROPERTY(QVariantMap inventoryMoney READ inventoryMoney NOTIFY inventoryMoneyChanged FINAL)
    Q_PROPERTY(QVariantMap encumbrance READ encumbrance NOTIFY encumbranceChanged FINAL)
    Q_PROPERTY(QVariantMap race READ race NOTIFY raceChanged FINAL)
    Q_PROPERTY(QVariantMap classes READ classes NOTIFY classesChanged FINAL)
    Q_PROPERTY(QVariantMap feats READ feats NOTIFY featsChanged FINAL)
    Q_PROPERTY(QVariantMap languages READ languages NOTIFY languagesChanged FINAL)
    Q_PROPERTY(QVariantMap reminders READ reminders NOTIFY remindersChanged FINAL)
    Q_PROPERTY(QVariantMap spellcastings READ spellcastings NOTIFY spellcastingsChanged FINAL)
    Q_PROPERTY(QVariantMap spellCatalogData READ spellCatalogData NOTIFY spellCatalogDataChanged FINAL)
    Q_PROPERTY(QVariantMap grantedSpells READ grantedSpells NOTIFY grantedSpellsChanged FINAL)
    Q_PROPERTY(QVariantMap lastCast READ lastCast NOTIFY lastCastChanged FINAL)
    Q_PROPERTY(QVariantMap enhancements READ enhancements NOTIFY enhancementsChanged FINAL)
    Q_PROPERTY(QVariantMap conditions READ conditions NOTIFY conditionsChanged FINAL)
    Q_PROPERTY(QString notes READ notes NOTIFY notesChanged FINAL)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged FINAL)
    Q_PROPERTY(bool opening READ opening NOTIFY openingChanged FINAL)
    Q_PROPERTY(QVariantList savedSheets READ savedSheets NOTIFY savedSheetsChanged FINAL)

public:
    enum SpellCommand { Prepare, PrepareFlexible, ConsumePrepared, ConsumeSpontaneous, ConsumeFlexible,
        ClearSlot, ClearPreparation, ResetSlots, AddToRepertoire, SelectRepertoire, ConsumeGranted };
    Q_ENUM(SpellCommand)
    explicit SheetController(QObject *parent = nullptr, QString sheetDirectory = {});
    ~SheetController() override;

    QVariantMap identity() const;
    QVariantMap home() const;
    QVariantMap abilityScores() const { return abilityScores_; }
    QVariantMap skillScores() const { return skillScores_; }
    QVariantMap savingThrows() const { return savingThrows_; }
    QVariantMap armorClass() const { return armorClass_; }
    QVariantMap hitPoints() const { return hitPoints_; }
    QVariantMap defenses() const { return defenses_; }
    QVariantMap movement() const { return movement_; }
    QVariantMap activeConditions() const { return activeConditions_; }
    QVariantMap initiative() const { return initiative_; }
    QVariantMap combat() const;
    QVariantMap attacks() const { return attacks_; }
    QVariantMap attackRoutines() const { return attackRoutines_; }
    QVariantMap actions() const { return actions_; }
    QVariantMap baseAttackBonus() const { return baseAttackBonus_; }
    QVariantMap combatManeuvers() const { return combatManeuvers_; }
    QVariantMap inventory() const;
    QVariantMap inventoryItems() const { return inventoryItems_; }
    QVariantMap inventoryContainers() const { return inventoryContainers_; }
    QVariantMap inventoryEquipment() const { return inventoryEquipment_; }
    QVariantMap inventoryMoney() const { return inventoryMoney_; }
    QVariantMap encumbrance() const { return encumbrance_; }
    QVariantMap race() const { return race_; }
    QVariantMap classes() const { return classes_; }
    QVariantMap talents() const;
    QVariantMap feats() const { return feats_; }
    QVariantMap languages() const { return languages_; }
    QVariantMap reminders() const { return reminders_; }
    QVariantMap spells() const;
    QVariantMap spellcastings() const { return spellcastings_; }
    QVariantMap spellCatalogData() const { return spellCatalogData_; }
    QVariantMap grantedSpells() const { return grantedSpells_; }
    QVariantMap lastCast() const { return lastCast_; }
    QVariantMap enhancements() const { return enhancements_; }
    QVariantMap conditions() const { return conditions_; }
    QString notes() const;
    QString errorMessage() const;
    bool opening() const;
    void preloadCatalogs();
    Q_INVOKABLE void openLocalSheet();
    QVariantList savedSheets() const;
    Q_INVOKABLE void openSavedSheet(const QUrl &file);
    Q_INVOKABLE void createSavedSheet(const QString &name);
    Q_INVOKABLE void importSavedSheet(const QUrl &file);
    Q_INVOKABLE void exportSavedSheet(const QUrl &file, const QUrl &destination);
    Q_INVOKABLE void deleteSavedSheet(const QUrl &file);
    Q_INVOKABLE void refreshSavedSheets();
    Q_INVOKABLE void setNotes(const QString &notes);
    Q_INVOKABLE void addTemporaryHitPoints(const QString &source, int amount);
    Q_INVOKABLE void removeTemporaryHitPoints(const QString &source);
    Q_INVOKABLE void damage(int amount, bool nonLethal);
    Q_INVOKABLE void heal(int amount);
    Q_INVOKABLE void setIdentityField(const QString &field, const QVariant &value);
    Q_INVOKABLE void setAbilityBaseValue(int abilityType, int value);
    Q_INVOKABLE void setSkillRanks(const QString &resourceName, int ranks);
    Q_INVOKABLE void addSkillSpecialization(int skillType, const QString &name);
    Q_INVOKABLE void removeSkillSpecialization(int skillType, const QString &id);
    Q_INVOKABLE void setCustomClassSkill(int skillType, bool enabled);
    Q_INVOKABLE void addAttackRoutine(const QString &routineId);
    Q_INVOKABLE void removeAttackRoutine(const QString &attackId);
    Q_INVOKABLE void renameAttackRoutine(const QString &attackId, const QString &name);
    Q_INVOKABLE void reorderAttackRoutine(const QString &attackId, const QVariant &beforeAttackId);
    Q_INVOKABLE void assignAttackStrike(const QString &attackId, const QString &slotId, const QString &strikeGrantId);
    Q_INVOKABLE QVariantList itemCatalog();
    Q_INVOKABLE void addCatalogItem(const QString &definitionId);
    Q_INVOKABLE void addCustomItem(const QVariantMap &definition);
    Q_INVOKABLE void addContainer(const QVariantMap &definition);
    Q_INVOKABLE void moveItem(const QString &itemId, const QString &containerId);
    Q_INVOKABLE void setItemQuantity(const QString &itemId, int quantity);
    Q_INVOKABLE void mergeCoinStacks(const QString &sourceId, const QString &targetId);
    Q_INVOKABLE void splitCoinStack(const QString &itemId, int quantity);
    Q_INVOKABLE void restoreItemUsagePool(const QString &itemId, const QString &poolId, int amount);
    Q_INVOKABLE void refillItemUsagePool(const QString &itemId, const QString &poolId);
    Q_INVOKABLE void reorderItem(const QString &itemId, const QVariant &beforeId);
    Q_INVOKABLE void moveAndReorderItem(const QString &itemId, const QString &containerId, const QVariant &beforeId);
    Q_INVOKABLE void unequipItem(const QString &itemId, const QString &containerId);
    Q_INVOKABLE void equipItem(const QString &itemId, const QString &replacedId);
    Q_INVOKABLE void changeMoney(int denomination, int quantity, const QString &containerId);
    Q_INVOKABLE void convertMoney(int from, int to, int fromQuantity, int toQuantity, const QString &sourceId, const QString &destinationId);
    Q_INVOKABLE QVariantList raceCatalog();
    Q_INVOKABLE void setRace(const QString &raceId);
    Q_INVOKABLE void selectAlternateRacialTrait(const QString &traitId);
    Q_INVOKABLE void removeAlternateRacialTrait(const QString &traitId);
    Q_INVOKABLE void setRacialChoice(const QString &elementId, const QString &choiceId, const QStringList &optionIds);
    Q_INVOKABLE QVariantList classCatalog() const;
    Q_INVOKABLE QVariantList archetypeCatalog(int classType);
    Q_INVOKABLE QString archetypeDescription(const QString &id);
    Q_INVOKABLE void addClassLevel(int type);
    Q_INVOKABLE void removeClassLevel(int type);
    Q_INVOKABLE void setFavoredClass(int type, bool favored);
    Q_INVOKABLE void setClassHitDieResult(int type, int classLevel, int result);
    Q_INVOKABLE void selectFavoredClassBonus(int type, int classLevel, const QString &optionId);
    Q_INVOKABLE void setClassArchetype(int type, const QString &archetypeId, bool enabled);
    Q_INVOKABLE void setClassOption(int type, const QString &acquisitionId, const QString &choiceId, const QString &optionId, bool selected);
    Q_INVOKABLE QVariantList featCatalog(const QString &slotId);
    Q_INVOKABLE QVariantMap featDetail(const QString &featId);
    Q_INVOKABLE void selectFeat(const QString &slotId, const QString &featId, const QVariantMap &choices);
    Q_INVOKABLE void removeFeat(const QString &slotId, bool extra);
    Q_INVOKABLE void addLanguage(const QString &name, int origin);
    Q_INVOKABLE void removeLanguage(const QString &id);
    Q_INVOKABLE void spellCommand(SpellCommand command, const QString &castingId, const QVariantMap &input);
    Q_INVOKABLE int spellPreparationCount(const QString &castingId, const QVariantMap &slot, const QString &spellId, const QVariantList &metamagic);
    Q_INVOKABLE bool canConsumeSpell(SpellCommand command, const QString &castingId, const QVariantMap &slot, const QVariantMap &input);
    Q_INVOKABLE QVariantList spellMetamagicOptions(SpellCommand command, const QVariantMap &casting, const QVariantMap &slot, const QVariantMap &input);
    Q_INVOKABLE QVariantList spellConversionOptions(const QString &castingId, const QVariantMap &slot);
    Q_INVOKABLE void addEnhancementGroup(const QVariantMap &definition);
    Q_INVOKABLE QVariantList enhancementBonusTypes() const;
    Q_INVOKABLE void addEnhancement(const QString &groupId, const QVariantMap &definition);
    Q_INVOKABLE void removeEnhancement(const QString &groupId, const QString &effectId);
    Q_INVOKABLE void removeEnhancementGroup(const QString &groupId);
    Q_INVOKABLE void setEnhancementGroupActive(const QString &groupId, bool active);
    Q_INVOKABLE void addCondition(const QVariantMap &definition);
    Q_INVOKABLE void removeConditions(const QStringList &ids);

Q_SIGNALS:
    void identityChanged();
    void abilityScoresChanged();
    void skillScoresChanged();
    void savingThrowsChanged();
    void armorClassChanged();
    void hitPointsChanged();
    void defensesChanged();
    void movementChanged();
    void activeConditionsChanged();
    void initiativeChanged();
    void attacksChanged();
    void attackRoutinesChanged();
    void actionsChanged();
    void baseAttackBonusChanged();
    void combatManeuversChanged();
    void inventoryItemsChanged();
    void inventoryContainersChanged();
    void inventoryEquipmentChanged();
    void inventoryMoneyChanged();
    void encumbranceChanged();
    void raceChanged();
    void classesChanged();
    void featsChanged();
    void languagesChanged();
    void remindersChanged();
    void spellcastingsChanged();
    void spellCatalogDataChanged();
    void grantedSpellsChanged();
    void lastCastChanged();
    void enhancementsChanged();
    void conditionsChanged();
    void notesChanged();
    void errorMessageChanged();
    void openingChanged();
    void sheetOpened();
    void savedSheetsChanged();

private:
    template<typename Change>
    void applySheetChange(Change change, const QString &failureMessage);
    template<typename Change>
    void applySheetTransaction(Change change, const QString &failureMessage);
    void finishCatalogLoad();
    void finishBrowsingCatalogLoad();
    void finishOpeningWhenCatalogsReady();
    static void prepareStandardSpellCatalog();
    void createLocalSheet();
    void requestSheet(const QString &path, bool create);
    void setErrorMessage(QString message);
    void queueCurrentSheetSave();
    void startPendingSave();
    bool finishSave();
    bool flushSaves();
    QFutureWatcher<bool> saveWatcher_;
    std::shared_ptr<const golarion::CharacterSheetSaveData> activeSave_, pendingSave_;
    void publish(bool save = true, QString error = {});
    struct PublishedInputs;
    std::unique_ptr<PublishedInputs> previousInputs_;
    static QVariantMap spellcastingSnapshot(const golarion::CharacterSheetView &view);
    static QVariantMap spellCatalogSnapshot(const golarion::CharacterSheetView &view);
    static QVariantMap grantedSpellSnapshot(const golarion::CharacterSheetView &view);
    using ClassCatalogResult = std::pair<const golarion::ClassDefinitionManager *, QString>;
    QFutureWatcher<ClassCatalogResult> catalogWatcher_;
    QFutureWatcher<QString> browsingCatalogWatcher_;
    const golarion::ClassDefinitionManager *classCatalog_ = nullptr;
    bool catalogLoading_ = false;
    bool browsingCatalogLoading_ = false;
    bool browsingCatalogReady_ = false;
    QString classCatalogError_;
    QString browsingCatalogError_;
    bool opening_ = false;
    bool createPendingFile_ = false;
    QString sheetDirectory_;
    QString importSource_;
    QString pendingPath_;
    QString currentPath_;
    std::unique_ptr<golarion::CharacterSheet> sheet_;
    QVariantMap identity_;
    QVariantMap abilityScores_;
    QVariantMap skillScores_;
    QVariantMap savingThrows_;
    QVariantMap armorClass_;
    QVariantMap hitPoints_;
    QVariantMap defenses_;
    QVariantMap movement_;
    QVariantMap activeConditions_;
    QVariantMap initiative_;
    QVariantMap attacks_;
    QVariantMap attackRoutines_;
    QVariantMap actions_;
    QVariantMap baseAttackBonus_;
    QVariantMap combatManeuvers_;
    QVariantMap inventoryItems_;
    QVariantMap inventoryContainers_;
    QVariantMap inventoryEquipment_;
    QVariantMap inventoryMoney_;
    QVariantMap encumbrance_;
    QVariantMap race_;
    QVariantMap classes_;
    QVariantMap feats_;
    QVariantMap languages_;
    QVariantMap reminders_;
    QVariantList customFeatCatalog_;
    QVariantMap spellcastings_;
    QVariantMap spellCatalogData_;
    QVariantMap grantedSpells_;
    QVariantMap lastCast_;
    QVariantMap lastSpellCast_;
    QVariantMap enhancements_;
    QVariantMap conditions_;
    QString notes_;
    QString errorMessage_;
};
