#include "golarion/character/attack_distance.hpp"

#include "golarion/character/attack.hpp"
#include "golarion/util/string_utils.hpp"

#include <stdexcept>
#include <utility>

namespace
{
    std::string_view resourceRoot(golarion::AttackDistanceProperty property)
    {
        switch (property)
        {
            case golarion::AttackDistanceProperty::MinimumReach:
                return "attackReach.minimum";
            case golarion::AttackDistanceProperty::MaximumReach:
                return "attackReach.maximum";
            case golarion::AttackDistanceProperty::RangeIncrement:
                return "attackRange.increment";
            case golarion::AttackDistanceProperty::MaximumRangeIncrements:
                return "attackRange.maximumIncrements";
            case golarion::AttackDistanceProperty::RangePenaltyPerAdditionalIncrement:
                return "attackRange.penaltyPerAdditionalIncrement";
        }

        throw std::invalid_argument("unknown attack distance property");
    }
}

namespace golarion
{
    std::string_view displayName(AttackDistanceProperty property)
    {
        switch (property)
        {
            case AttackDistanceProperty::MinimumReach:
                return "Portata minima";
            case AttackDistanceProperty::MaximumReach:
                return "Portata massima";
            case AttackDistanceProperty::RangeIncrement:
                return "Incremento di gittata";
            case AttackDistanceProperty::MaximumRangeIncrements:
                return "Incrementi di gittata massimi";
            case AttackDistanceProperty::RangePenaltyPerAdditionalIncrement:
                return "Penalità per incremento aggiuntivo";
        }

        throw std::invalid_argument("unknown attack distance property");
    }

    std::string_view displayName(AttackDistanceAdjustmentType type)
    {
        switch (type)
        {
            case AttackDistanceAdjustmentType::Multiplier:
                return "Moltiplicatore";
            case AttackDistanceAdjustmentType::Minimum:
                return "Valore minimo";
            case AttackDistanceAdjustmentType::Maximum:
                return "Valore massimo";
        }

        throw std::invalid_argument("unknown attack distance adjustment type");
    }

    std::string attackDistanceResourceName(AttackDistanceProperty property)
    {
        return std::string(resourceRoot(property)) + ".all";
    }

    std::string attackDistanceResourceName(AttackDistanceProperty property, AttackMode mode)
    {
        return std::string(resourceRoot(property)) + "." + attackResourceName(mode).substr(std::string("attack.").size());
    }

    std::string attackDistanceResourceName(AttackDistanceProperty property, AttackTag tag)
    {
        return std::string(resourceRoot(property)) + "." + attackResourceName(tag).substr(std::string("attack.").size());
    }

    std::string attackDistanceResourceName(AttackDistanceProperty property, std::string_view grantId)
    {
        return std::string(resourceRoot(property)) + ".grant." + normalize(grantId);
    }

    AttackDistanceAdjustment::AttackDistanceAdjustment(AttackDistanceAdjustmentDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          targetResourceName_(normalize(definition.targetResourceName)),
          type_(definition.type),
          expression_(normalize(definition.expression)),
          condition_(std::move(definition.condition))
    {
        if (condition_.has_value())
        {
            condition_ = normalize(*condition_);
        }
    }
}
