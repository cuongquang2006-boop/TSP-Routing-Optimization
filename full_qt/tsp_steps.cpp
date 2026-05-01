#include "tsp_steps.h"
#include <QJsonArray>

int Step::getAnimationDuration() const
{
    switch (action)
    {
    case StepAction::SELECT_NODE:
        return 300;
    case StepAction::COMPARE_EDGE:
        return 650;
    case StepAction::UPDATE_BEST:
        return 500;
    case StepAction::INSERT_NODE:
        return 750;
    case StepAction::VISIT_NODE:
        return 500;
    case StepAction::COMPLETE:
        return 800;
    }
    return 300;
}

QString stepActionToString(StepAction action)
{
    switch (action)
    {
    case StepAction::SELECT_NODE:
        return QStringLiteral("SELECT_NODE");
    case StepAction::COMPARE_EDGE:
        return QStringLiteral("COMPARE_EDGE");
    case StepAction::INSERT_NODE:
        return QStringLiteral("INSERT_NODE");
    case StepAction::UPDATE_BEST:
        return QStringLiteral("UPDATE_BEST");
    case StepAction::VISIT_NODE:
        return QStringLiteral("VISIT_NODE");
    case StepAction::COMPLETE:
        return QStringLiteral("COMPLETE");
    }
    return QStringLiteral("UNKNOWN_ACTION");
}

QJsonObject stepToJson(const Step &step)
{
    QJsonObject object;
    object["action"] = stepActionToString(step.action);

    QJsonArray tourArray;
    for (int index : step.tour)
    {
        tourArray.append(index);
    }
    object["tour"] = tourArray;
    object["currentNode"] = step.currentNode;
    object["edgeStart"] = step.edge.first;
    object["edgeEnd"] = step.edge.second;
    object["bestCandidate"] = step.bestCandidate;
    object["value"] = step.value;
    object["debugInfo"] = step.debugInfo;
    return object;
}

QString stepToString(const Step &step)
{
    QJsonObject object = stepToJson(step);
    QJsonDocument document(object);
    return QString::fromUtf8(document.toJson(QJsonDocument::Compact));
}

QString buildStepDescription(const Step &step, const QVector<QPointF> &points)
{
    switch (step.action)
    {
    case StepAction::SELECT_NODE:
        if (step.currentNode >= 0)
        {
            return QString("Select node %1").arg(step.currentNode);
        }
        return "Initialize tour";

    case StepAction::COMPARE_EDGE:
        if (step.edge.first >= 0 && step.edge.second >= 0)
        {
            qreal distance = step.value;
            return QString("Compare edge (%1 → %2), distance = %3")
                .arg(step.edge.first)
                .arg(step.edge.second)
                .arg(distance, 0, 'f', 1);
        }
        return "Compare edge";

    case StepAction::UPDATE_BEST:
        if (step.bestCandidate >= 0)
        {
            return QString("Update best candidate: node %1")
                .arg(step.bestCandidate);
        }
        return "Update best";

    case StepAction::INSERT_NODE:
        if (step.bestCandidate >= 0)
        {
            return QString("Insert node %1 into tour, cost = %2")
                .arg(step.bestCandidate)
                .arg(step.value, 0, 'f', 2);
        }
        return "Insert node";

    case StepAction::VISIT_NODE:
        if (step.currentNode >= 0)
        {
            return QString("Visit node %1, total cost = %2")
                .arg(step.currentNode)
                .arg(step.value, 0, 'f', 2);
        }
        return "Visit node";

    case StepAction::COMPLETE:
        return QString("Complete! Final tour cost = %1")
            .arg(step.value, 0, 'f', 2);

    default:
        return "Unknown action";
    }
}
