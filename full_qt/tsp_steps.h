#ifndef TSP_STEPS_H
#define TSP_STEPS_H

#include <QVector>
#include <QPair>
#include <QString>
#include <QJsonObject>

enum class StepAction
{
    SELECT_NODE,
    COMPARE_EDGE,
    INSERT_NODE,
    UPDATE_BEST,
    VISIT_NODE,
    COMPLETE
};

struct Step
{
    StepAction action;
    QVector<int> tour;
    int currentNode = -1;
    QPair<int, int> edge = qMakePair(-1, -1);
    int bestCandidate = -1;
    double value = 0.0;
    QString debugInfo;
};

QString stepActionToString(StepAction action);
QJsonObject stepToJson(const Step &step);
QString stepToString(const Step &step);

#endif