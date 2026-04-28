#include "tsp_steps.h"
#include <QJsonArray>

QString stepActionToString(StepAction action)
{
    switch (action) {
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
    for (int index : step.tour) {
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
