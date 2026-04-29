#include "tsp_algorithms.h"
#include <QRandomGenerator>
#include <QSet>
#include <algorithm>
#include <limits>

double calculateDistance(const QPointF &a, const QPointF &b)
{
    return std::hypot(a.x() - b.x(), a.y() - b.y());
}

double calculateTourCost(const QVector<QPointF> &points, const QVector<int> &tour)
{
    double cost = 0.0;

    for (int i = 0; i < tour.size() - 1; ++i)
    {
        cost += calculateDistance(points[tour[i]], points[tour[i + 1]]);
    }

    if (!tour.isEmpty())
    {
        cost += calculateDistance(points[tour.last()], points[tour.first()]);
    }

    return cost;
}

QVector<Step> generateRandomSteps(const QVector<QPointF> &points)
{
    QVector<Step> steps;
    QVector<int> unpicked;

    for (int i = 0; i < points.size(); ++i)
    {
        unpicked.append(i);
    }

    QVector<int> path;

    Step initStep;
    initStep.action = StepAction::SELECT_NODE;
    initStep.tour = path;
    initStep.debugInfo = "init random";
    steps.push_back(initStep);

    if (unpicked.isEmpty())
    {
        return steps;
    }

    int startIdx = QRandomGenerator::global()->bounded(unpicked.size());
    int current = unpicked.takeAt(startIdx);
    path.append(current);

    while (!unpicked.isEmpty())
    {
        Step candidateStep;
        candidateStep.action = StepAction::COMPARE_EDGE;
        candidateStep.tour = path;
        candidateStep.currentNode = current;

        for (int v : unpicked)
        {
            candidateStep.candidate_edges.append(qMakePair(current, v));
        }

        candidateStep.debugInfo = "highlight possible moves";
        steps.push_back(candidateStep);

        int pickIdx = QRandomGenerator::global()->bounded(unpicked.size());
        int chosen = unpicked.takeAt(pickIdx);

        Step selStep;
        selStep.action = StepAction::UPDATE_BEST;
        selStep.tour = path;
        selStep.currentNode = current;
        selStep.selected_edges.append(qMakePair(current, chosen));
        selStep.debugInfo = "randomly selecting next node";
        steps.push_back(selStep);

        Step rejStep;
        rejStep.action = StepAction::VISIT_NODE;
        rejStep.tour = path;
        rejStep.currentNode = chosen;

        for (int v : unpicked)
        {
            rejStep.rejected_edges.append(qMakePair(current, v));
        }

        rejStep.debugInfo = "others rejected";
        steps.push_back(rejStep);

        path.append(chosen);
        current = chosen;

        Step visitStep;
        visitStep.action = StepAction::INSERT_NODE;
        visitStep.tour = path;
        visitStep.currentNode = current;
        visitStep.value = calculateTourCost(points, path);
        visitStep.debugInfo = "visited node";
        steps.push_back(visitStep);
    }

    Step completeStep;
    completeStep.action = StepAction::COMPLETE;
    completeStep.tour = path;
    completeStep.value = calculateTourCost(points, path);
    completeStep.debugInfo = "random complete";
    steps.push_back(completeStep);

    return steps;
}

QVector<Step> generateGreedySteps(const QVector<QPointF> &points)
{
    QVector<Step> steps;
    QVector<int> path;
    QSet<int> unvisited;

    for (int i = 0; i < points.size(); ++i)
    {
        unvisited.insert(i);
    }

    int start = 0;
    path.append(start);
    unvisited.remove(start);

    Step startStep;
    startStep.action = StepAction::SELECT_NODE;
    startStep.tour = path;
    startStep.currentNode = start;
    startStep.debugInfo = "start node";
    steps.push_back(startStep);

    while (!unvisited.isEmpty())
    {
        int cur = path.last();

        Step candidateStep;
        candidateStep.action = StepAction::COMPARE_EDGE;
        candidateStep.tour = path;
        candidateStep.currentNode = cur;

        for (int v : unvisited)
        {
            candidateStep.candidate_edges.append(qMakePair(cur, v));
        }

        candidateStep.debugInfo = "Evaluating neighbors of current node";
        steps.push_back(candidateStep);

        double minDist = std::numeric_limits<double>::max();
        int next = -1;

        for (int v : unvisited)
        {
            double dist = calculateDistance(points[cur], points[v]);

            if (dist < minDist)
            {
                minDist = dist;
                next = v;
            }
        }

        Step selectStep;
        selectStep.action = StepAction::UPDATE_BEST;
        selectStep.tour = path;
        selectStep.currentNode = cur;
        selectStep.selected_edges.append(qMakePair(cur, next));
        selectStep.value = minDist;
        selectStep.debugInfo = "Selecting shortest edge";
        steps.push_back(selectStep);

        Step rejectStep;
        rejectStep.action = StepAction::VISIT_NODE;
        rejectStep.tour = path;
        rejectStep.currentNode = next;

        for (int v : unvisited)
        {
            if (v == next)
            {
                continue;
            }

            rejectStep.rejected_edges.append(qMakePair(cur, v));
        }

        rejectStep.debugInfo = "Non-optimal choices rejected";
        steps.push_back(rejectStep);

        path.append(next);
        unvisited.remove(next);

        Step visitStep;
        visitStep.action = StepAction::INSERT_NODE;
        visitStep.tour = path;
        visitStep.currentNode = next;
        visitStep.value = calculateTourCost(points, path);
        visitStep.debugInfo = "visited node";
        steps.push_back(visitStep);
    }

    Step completeStep;
    completeStep.action = StepAction::COMPLETE;
    completeStep.tour = path;
    completeStep.value = calculateTourCost(points, path);
    completeStep.debugInfo = "greedy complete";
    steps.push_back(completeStep);

    return steps;
}