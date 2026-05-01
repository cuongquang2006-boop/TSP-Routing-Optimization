#ifndef TSP_STEPS_H
#define TSP_STEPS_H

#include <QVector>
#include <QPair>
#include <QString>
#include <QJsonObject>
#include <QPointF>
#include <limits>

enum class EdgeState
{
    CANDIDATE,
    SELECTED,
    REJECTED
};

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

    QVector<QPair<int,int>> candidate_edges;
    QVector<QPair<int,int>> selected_edges;
    QVector<QPair<int,int>> rejected_edges;

    QVector<int> tour;
    int currentNode = -1;
    QPair<int, int> edge = qMakePair(-1, -1);
    int bestCandidate = -1;

    double value = 0.0;
    double bound = std::numeric_limits<double>::quiet_NaN();
    double bestCost = std::numeric_limits<double>::quiet_NaN();

    QString debugInfo;

    int getAnimationDuration() const;
};

QString stepActionToString(StepAction action);
QJsonObject stepToJson(const Step &step);
QString stepToString(const Step &step);

QString buildStepDescription(const Step &step, const QVector<QPointF> &points = {});

#endif