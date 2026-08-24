#include "golarion/character/attack.hpp"

#include "golarion/character/attack_routine.hpp"
#include "golarion/character/strike.hpp"
#include "golarion/util/string_utils.hpp"
#include "golarion/view/attack_routines_view.hpp"
#include "golarion/view/attacks_view.hpp"
#include "golarion/view/strikes_view.hpp"

#include <algorithm>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <utility>

namespace
{
    bool channelsOverlap(const std::vector<std::string> &left, const std::vector<std::string> &right)
    {
        return std::ranges::any_of(left, [&right](const std::string &channel)
        {
            return std::ranges::find(right, channel) != right.end();
        });
    }

    const golarion::AttackRoutineView *findRoutine(const golarion::AttackRoutinesView &view, std::string_view routineId)
    {
        const auto routine = std::ranges::find(view.routines, routineId, &golarion::AttackRoutineView::id);
        return routine == view.routines.end() ? nullptr : &*routine;
    }

    const golarion::RoutineSlotView *findSlot(const golarion::AttackRoutineView &routine, std::string_view slotId)
    {
        const auto slot = std::ranges::find(routine.slots, slotId, &golarion::RoutineSlotView::id);
        return slot == routine.slots.end() ? nullptr : &*slot;
    }

    const golarion::RoutineStrikeCandidateView *findCandidate(const golarion::RoutineSlotView &slot, std::string_view grantId)
    {
        const auto candidate = std::ranges::find(slot.candidates, grantId, &golarion::RoutineStrikeCandidateView::grantId);
        return candidate == slot.candidates.end() ? nullptr : &*candidate;
    }

    const golarion::StrikeView *findStrike(const golarion::StrikesView &view, std::string_view grantId)
    {
        const auto strike = std::ranges::find(view.strikes, grantId, &golarion::StrikeView::grantId);
        return strike == view.strikes.end() ? nullptr : &*strike;
    }

    int checkedAttackAdjustment(long long value)
    {
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("attack routine adjustment is out of range");
        }
        return static_cast<int>(value);
    }

    std::optional<golarion::WeaponWeight> weaponWeightForPurpose(const golarion::StrikeView &strike, golarion::WeaponWeightPurpose purpose)
    {
        return purpose == golarion::WeaponWeightPurpose::General ? strike.effectiveWeaponWeight : strike.twoWeaponFightingWeaponWeight;
    }
}

namespace golarion
{
    Attack::Attack(std::string id, std::string name, std::string routineId)
        : id_(normalize(id)), name_(normalize(name)), routineId_(normalize(routineId))
    {
    }

    Attacks::Attacks(AttackRoutines &attackRoutines, Strikes &strikes) : attackRoutines_(attackRoutines), strikes_(strikes)
    {
    }

    void Attacks::create(std::string_view id, std::string_view name, std::string_view routineId)
    {
        Attack newAttack{std::string(id), std::string(name), std::string(routineId)};
        if (std::ranges::find(attacks_, newAttack.id_, &Attack::id_) != attacks_.end())
        {
            throw std::invalid_argument("attack is already registered: " + newAttack.id_);
        }
        if (findRoutine(attackRoutines_.toView(), newAttack.routineId_) == nullptr)
        {
            throw std::invalid_argument("attack routine is not registered: " + newAttack.routineId_);
        }
        attacks_.push_back(std::move(newAttack));
    }

    void Attacks::remove(std::string_view attackId)
    {
        const std::string id = normalize(attackId);
        const auto existingAttack = std::ranges::find(attacks_, id, &Attack::id_);
        if (existingAttack == attacks_.end())
        {
            throw std::invalid_argument("attack is not registered: " + id);
        }
        attacks_.erase(existingAttack);
    }

    void Attacks::assignStrike(std::string_view attackId, std::string_view slotId, std::string_view strikeGrantId)
    {
        Attack &targetAttack = attack(attackId);
        const std::string normalizedSlotId = normalize(slotId);
        const std::string normalizedGrantId = normalize(strikeGrantId);
        const AttackRoutinesView routinesView = attackRoutines_.toView();
        const AttackRoutineView *routine = findRoutine(routinesView, targetAttack.routineId_);
        if (routine == nullptr)
        {
            throw std::invalid_argument("attack routine is not registered: " + targetAttack.routineId_);
        }
        const RoutineSlotView *slot = findSlot(*routine, normalizedSlotId);
        if (slot == nullptr)
        {
            throw std::invalid_argument("attack routine slot is not registered: " + normalizedSlotId);
        }
        if (slot->selectionMode != RoutineSlotSelectionMode::ChooseOne)
        {
            throw std::invalid_argument("strikes can only be assigned to ChooseOne routine slots: " + normalizedSlotId);
        }
        const RoutineStrikeCandidateView *candidate = findCandidate(*slot, normalizedGrantId);
        if (candidate == nullptr)
        {
            throw std::invalid_argument("strike grant is not registered: " + normalizedGrantId);
        }
        if (!candidate->accepted)
        {
            throw std::invalid_argument("strike is not accepted by the routine slot: " + normalizedGrantId);
        }

        const StrikesView strikesView = strikes_.toView();
        const StrikeView *selectedStrike = findStrike(strikesView, normalizedGrantId);
        for (const auto &[assignedSlotId, assignedGrantId] : targetAttack.assignments_)
        {
            if (assignedSlotId == normalizedSlotId)
            {
                continue;
            }
            const StrikeView *assignedStrike = findStrike(strikesView, assignedGrantId);
            if (assignedStrike != nullptr && channelsOverlap(selectedStrike->usageChannels, assignedStrike->usageChannels))
            {
                throw std::invalid_argument("strike usage channels conflict with slot: " + assignedSlotId);
            }
        }
        targetAttack.assignments_.insert_or_assign(normalizedSlotId, normalizedGrantId);
    }

    void Attacks::unassignStrike(std::string_view attackId, std::string_view slotId)
    {
        Attack &targetAttack = attack(attackId);
        const std::string normalizedSlotId = normalize(slotId);
        if (targetAttack.assignments_.erase(normalizedSlotId) == 0)
        {
            throw std::invalid_argument("attack slot has no assigned strike: " + normalizedSlotId);
        }
    }

    AttacksView Attacks::toView()
    {
        return toView(StrikeCalculationContext{});
    }

    AttacksView Attacks::toView(const StrikeCalculationContext &context)
    {
        const AttackRoutinesView routinesView = attackRoutines_.toView();
        const StrikesView strikesView = strikes_.toView(context);
        std::vector<AttackView> attackViews;
        attackViews.reserve(attacks_.size());

        for (const Attack &configuredAttack : attacks_)
        {
            const AttackRoutineView *routine = findRoutine(routinesView, configuredAttack.routineId_);
            if (routine == nullptr)
            {
                attackViews.push_back(AttackView{
                    .id = configuredAttack.id_,
                    .name = configuredAttack.name_,
                    .routineId = configuredAttack.routineId_,
                    .routineName = std::nullopt,
                    .complete = false,
                    .usable = false,
                    .failureReasons = {"La routine selezionata non è disponibile"},
                    .slots = {}
                });
                continue;
            }

            std::map<std::string, std::vector<std::string>> explicitChannels;
            std::vector<std::string> attackFailureReasons;
            for (const auto &[slotId, grantId] : configuredAttack.assignments_)
            {
                const RoutineSlotView *slot = findSlot(*routine, slotId);
                if (slot == nullptr)
                {
                    attackFailureReasons.push_back("Lo slot assegnato non esiste più: " + slotId);
                    continue;
                }
                const StrikeView *strike = findStrike(strikesView, grantId);
                if (strike == nullptr)
                {
                    attackFailureReasons.push_back("Lo strike assegnato non è disponibile: " + grantId);
                    continue;
                }
                explicitChannels.emplace(slotId, strike->usageChannels);
            }

            for (auto left = explicitChannels.begin(); left != explicitChannels.end(); ++left)
            {
                for (auto right = std::next(left); right != explicitChannels.end(); ++right)
                {
                    if (channelsOverlap(left->second, right->second))
                    {
                        attackFailureReasons.push_back("Gli slot assegnati usano lo stesso canale: " + left->first + " e " + right->first);
                    }
                }
            }

            std::vector<std::string> automaticallyOccupiedChannels;
            std::vector<AttackSlotView> slotViews;
            slotViews.reserve(routine->slots.size());
            bool complete = attackFailureReasons.empty();
            bool usable = attackFailureReasons.empty();
            for (const RoutineSlotView &routineSlot : routine->slots)
            {
                const auto assignment = configuredAttack.assignments_.find(routineSlot.id);
                const std::optional<std::string> assignedGrantId = assignment == configuredAttack.assignments_.end() ? std::nullopt : std::optional<std::string>(assignment->second);
                std::vector<RoutineStrikeCandidateView> candidates = routineSlot.candidates;
                std::vector<std::string> effectiveGrantIds;
                std::vector<std::string> slotFailureReasons;

                for (RoutineStrikeCandidateView &candidate : candidates)
                {
                    bool channelConflict = false;
                    for (const auto &[assignedSlotId, channels] : explicitChannels)
                    {
                        if (routineSlot.selectionMode == RoutineSlotSelectionMode::ChooseOne && assignedSlotId == routineSlot.id)
                        {
                            continue;
                        }
                        if (channelsOverlap(candidate.usageChannels, channels))
                        {
                            channelConflict = true;
                            break;
                        }
                    }
                    if (!channelConflict && routineSlot.selectionMode == RoutineSlotSelectionMode::AllMatching && channelsOverlap(candidate.usageChannels, automaticallyOccupiedChannels))
                    {
                        channelConflict = true;
                    }
                    if (channelConflict)
                    {
                        candidate.accepted = false;
                        candidate.rejectionReasons.push_back("Un canale d'uso dello strike è già occupato");
                    }

                    if (routineSlot.selectionMode == RoutineSlotSelectionMode::AllMatching && candidate.accepted)
                    {
                        effectiveGrantIds.push_back(candidate.grantId);
                        automaticallyOccupiedChannels.insert(automaticallyOccupiedChannels.end(), candidate.usageChannels.begin(), candidate.usageChannels.end());
                        const StrikeView *strike = findStrike(strikesView, candidate.grantId);
                        if (strike != nullptr && !strike->usable)
                        {
                            usable = false;
                            slotFailureReasons.push_back("Lo strike non è utilizzabile: " + candidate.name);
                        }
                    }
                }

                bool slotComplete = true;
                bool slotUsable = true;
                if (routineSlot.selectionMode == RoutineSlotSelectionMode::ChooseOne)
                {
                    if (!assignedGrantId.has_value())
                    {
                        slotComplete = false;
                        slotUsable = false;
                        slotFailureReasons.push_back("Nessuno strike è assegnato allo slot");
                    }
                    else
                    {
                        const auto assignedCandidate = std::ranges::find(candidates, *assignedGrantId, &RoutineStrikeCandidateView::grantId);
                        if (assignedCandidate == candidates.end())
                        {
                            slotComplete = false;
                            slotUsable = false;
                            slotFailureReasons.push_back("Lo strike assegnato non è disponibile");
                        }
                        else if (!assignedCandidate->accepted)
                        {
                            slotComplete = false;
                            slotUsable = false;
                            slotFailureReasons.push_back("Lo strike assegnato non è più compatibile con lo slot");
                        }
                        else
                        {
                            effectiveGrantIds.push_back(*assignedGrantId);
                            const StrikeView *strike = findStrike(strikesView, *assignedGrantId);
                            if (strike != nullptr && !strike->usable)
                            {
                                slotUsable = false;
                                slotFailureReasons.push_back("Lo strike assegnato non è utilizzabile");
                            }
                        }
                    }
                }
                if (!slotFailureReasons.empty())
                {
                    slotUsable = false;
                }
                complete = complete && slotComplete;
                usable = usable && slotUsable;
                slotViews.push_back(AttackSlotView{
                    .id = routineSlot.id,
                    .name = routineSlot.name,
                    .selector = routineSlot.selector,
                    .selectionMode = routineSlot.selectionMode,
                    .strikeUsage = routineSlot.strikeUsage,
                    .baseAttackBonusAdjustmentExpression = routineSlot.baseAttackBonusAdjustmentExpression,
                    .baseAttackBonusAdjustment = routineSlot.baseAttackBonusAdjustment,
                    .effectiveAttackBonusAdjustment = routineSlot.baseAttackBonusAdjustment,
                    .progressions = routineSlot.progressions,
                    .appliedAttackBonusAdjustments = {},
                    .damageAbilityRuleOverride = routineSlot.damageAbilityRuleOverride,
                    .assignedGrantId = assignedGrantId,
                    .effectiveGrantIds = std::move(effectiveGrantIds),
                    .candidates = std::move(candidates),
                    .complete = slotComplete,
                    .usable = slotUsable,
                    .failureReasons = std::move(slotFailureReasons)
                });
            }
            for (AttackSlotView &slot : slotViews)
            {
                const RoutineSlotView *routineSlot = findSlot(*routine, slot.id);
                long long effectiveAdjustment = slot.baseAttackBonusAdjustment;
                for (const RoutineAttackBonusAdjustmentView &adjustment : routineSlot->attackBonusAdjustments)
                {
                    const bool requirementsSatisfied = std::ranges::all_of(adjustment.requirements, [&slotViews, &strikesView](const RoutineAssignmentRequirementView &requirement)
                    {
                        const auto requiredSlot = std::ranges::find(slotViews, requirement.slotId, &AttackSlotView::id);
                        if (requiredSlot == slotViews.end())
                        {
                            return false;
                        }
                        return std::ranges::any_of(requiredSlot->effectiveGrantIds, [&requirement, &strikesView](const std::string &grantId)
                        {
                            const StrikeView *strike = findStrike(strikesView, grantId);
                            if (strike == nullptr)
                            {
                                return false;
                            }
                            if (requirement.strikeGrantId.has_value() && strike->grantId != *requirement.strikeGrantId)
                            {
                                return false;
                            }
                            if (requirement.requiredTag.has_value() && std::ranges::find(strike->tags, *requirement.requiredTag) == strike->tags.end())
                            {
                                return false;
                            }
                            if (requirement.weaponWeightPurpose.has_value() && weaponWeightForPurpose(*strike, *requirement.weaponWeightPurpose) != requirement.weaponWeight)
                            {
                                return false;
                            }
                            return true;
                        });
                    });
                    if (requirementsSatisfied)
                    {
                        effectiveAdjustment += adjustment.resolvedValue;
                        slot.appliedAttackBonusAdjustments.push_back(adjustment);
                    }
                }
                slot.effectiveAttackBonusAdjustment = checkedAttackAdjustment(effectiveAdjustment);
            }
            for (const AttackSlotView &slot : slotViews)
            {
                for (const std::string &reason : slot.failureReasons)
                {
                    attackFailureReasons.push_back(slot.name + ": " + reason);
                }
            }
            attackViews.push_back(AttackView{
                .id = configuredAttack.id_,
                .name = configuredAttack.name_,
                .routineId = configuredAttack.routineId_,
                .routineName = routine->name,
                .complete = complete,
                .usable = usable,
                .failureReasons = std::move(attackFailureReasons),
                .slots = std::move(slotViews)
            });
        }

        return AttacksView{.attacks = std::move(attackViews)};
    }

    AttacksData Attacks::toData() const
    {
        std::vector<AttackData> attackData;
        attackData.reserve(attacks_.size());
        for (const Attack &configuredAttack : attacks_)
        {
            std::vector<AttackAssignmentData> assignments;
            assignments.reserve(configuredAttack.assignments_.size());
            for (const auto &[slotId, grantId] : configuredAttack.assignments_)
            {
                assignments.push_back(AttackAssignmentData{.slotId = slotId, .strikeGrantId = grantId});
            }
            attackData.push_back(AttackData{
                .id = configuredAttack.id_,
                .name = configuredAttack.name_,
                .routineId = configuredAttack.routineId_,
                .assignments = std::move(assignments)
            });
        }
        return AttacksData{.attacks = std::move(attackData)};
    }

    void Attacks::load(const AttacksData &data)
    {
        if (!attacks_.empty())
        {
            throw std::invalid_argument("attacks can only be loaded into an empty collection");
        }
        std::vector<Attack> loadedAttacks;
        loadedAttacks.reserve(data.attacks.size());
        for (const AttackData &dataAttack : data.attacks)
        {
            Attack loadedAttack(dataAttack.id, dataAttack.name, dataAttack.routineId);
            if (std::ranges::find(loadedAttacks, loadedAttack.id_, &Attack::id_) != loadedAttacks.end())
            {
                throw std::invalid_argument("attack is duplicated in save data: " + loadedAttack.id_);
            }
            for (const AttackAssignmentData &assignment : dataAttack.assignments)
            {
                const std::string slotId = normalize(assignment.slotId);
                const std::string grantId = normalize(assignment.strikeGrantId);
                if (!loadedAttack.assignments_.emplace(slotId, grantId).second)
                {
                    throw std::invalid_argument("attack assignment is duplicated in save data: " + slotId);
                }
            }
            loadedAttacks.push_back(std::move(loadedAttack));
        }
        attacks_ = std::move(loadedAttacks);
    }

    Attack &Attacks::attack(std::string_view attackId)
    {
        const std::string id = normalize(attackId);
        const auto existingAttack = std::ranges::find(attacks_, id, &Attack::id_);
        if (existingAttack == attacks_.end())
        {
            throw std::invalid_argument("attack is not registered: " + id);
        }
        return *existingAttack;
    }
}
