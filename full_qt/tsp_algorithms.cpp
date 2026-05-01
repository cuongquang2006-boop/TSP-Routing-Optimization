#include "tsp_algorithms.h"
#include <QRandomGenerator>
#include <QSet>
#include <algorithm>
#include <limits>

double calculateDistance(const QPointF &a, const QPointF &b)
{
    return std::hypot(a.x() - b.x(), a.y() - b.y()) * 1000;
}

double calculateTourCost(const QVector<QPointF> &points, const QVector<int> &tour)
{
    double cost = 0.0;

    if (tour.size() < 2)
        return cost;

    for (int i = 0; i < tour.size() - 1; ++i) {
        cost += calculateDistance(points[tour[i]], points[tour[i + 1]]);
    }

    if (tour.size() == points.size()) {
        cost += calculateDistance(points[tour.last()], points[tour.first()]);
    }

    return cost;
}

QVector<Step> generateRandomSteps(const QVector<QPointF> &points)
{
    QVector<Step> steps;
    QVector<int> unpicked;
    for (int i = 0; i < points.size(); ++i) {
        unpicked.append(i);
    }

    QVector<int> path;

    Step initStep;
    initStep.action = StepAction::SELECT_NODE;
    initStep.tour = path;
    initStep.debugInfo = "init random";
    steps.push_back(initStep);

    if (unpicked.isEmpty()) return steps;
    int startIdx = QRandomGenerator::global()->bounded(unpicked.size());
    int current = unpicked.takeAt(startIdx);
    path.append(current);

    while (!unpicked.isEmpty()) {
        Step candidateStep;
        candidateStep.action = StepAction::COMPARE_EDGE;
        candidateStep.tour = path;
        candidateStep.currentNode = current;
        candidateStep.selected_edges = {};
        candidateStep.rejected_edges = {};
        for (int v : unpicked) {
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
        for (int v : unpicked) {
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

    while (!unvisited.isEmpty()) {
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
        for (int v : unvisited) {
            double dist = calculateDistance(points[cur], points[v]);
            if (dist < minDist) {
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
        for (int v : unvisited) {
            if (v == next) continue;
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


QVector<Step> generateNearestInsertionSteps(const QVector<QPointF> &points)
{
    QVector<Step> steps;
    QVector<int> tour;
    QSet<int> unvisited;

    for (int i = 0; i < points.size(); ++i)
        unvisited.insert(i);

    if (points.size() < 2) {
        Step s;
        s.action = StepAction::COMPLETE;
        s.tour = tour;
        steps.push_back(s);
        return steps;
    }

    int a = 0, b = 1;
    tour = {a, b};
    unvisited.remove(a);
    unvisited.remove(b);

    Step initSel;
    initSel.action = StepAction::SELECT_NODE;
    initSel.tour = tour;
    initSel.currentNode = a;
    initSel.debugInfo = "init tour";
    steps.push_back(initSel);

    Step initInsert;
    initInsert.action = StepAction::INSERT_NODE;
    initInsert.tour = tour;
    initInsert.value = calculateTourCost(points, tour);
    initInsert.debugInfo = "init tour";
    steps.push_back(initInsert);

    while (!unvisited.isEmpty()) {

        double minDist = std::numeric_limits<double>::max();
        int bestNode = -1;

        Step candidateSelect;
        candidateSelect.action = StepAction::SELECT_NODE;
        candidateSelect.tour = tour;

        for (int node : unvisited) {
            for (int t : tour) {
                double d = calculateDistance(points[node], points[t]);
                candidateSelect.candidate_edges.append(qMakePair(node, t));
                if (d < minDist) {
                    minDist = d;
                    bestNode = node;
                }
            }
        }
        candidateSelect.currentNode = bestNode;
        candidateSelect.value = minDist;
        candidateSelect.debugInfo = "Selecting closest node to tour";
        steps.push_back(candidateSelect);

        double bestCost = std::numeric_limits<double>::max();
        int bestPos = -1;

        for (int i = 0; i < tour.size(); ++i) {
            int j = (i + 1) % tour.size();

            double cost =
                calculateDistance(points[tour[i]], points[bestNode]) +
                calculateDistance(points[bestNode], points[tour[j]]) -
                calculateDistance(points[tour[i]], points[tour[j]]);

            Step insertionCompare;
            insertionCompare.action = StepAction::COMPARE_EDGE;
            insertionCompare.tour = tour;
            insertionCompare.currentNode = bestNode;
            insertionCompare.candidate_edges.append(qMakePair(tour[i], tour[j]));
            insertionCompare.value = cost;
            insertionCompare.debugInfo = "Evaluating insertion positions";
            steps.push_back(insertionCompare);

            if (cost < bestCost) {
                bestCost = cost;
                bestPos = i;

                Step updateBest;
                updateBest.action = StepAction::UPDATE_BEST;
                updateBest.tour = tour;
                updateBest.currentNode = bestNode;
                updateBest.selected_edges.append(qMakePair(tour[i], tour[j]));
                updateBest.value = cost;
                updateBest.debugInfo = "Choosing minimal cost increase";
                steps.push_back(updateBest);
            }
        }

        tour.insert(bestPos + 1, bestNode);
        unvisited.remove(bestNode);

        Step insertStep;
        insertStep.action = StepAction::INSERT_NODE;
        insertStep.tour = tour;
        insertStep.currentNode = bestNode;
        insertStep.value = calculateTourCost(points, tour);
        insertStep.debugInfo = "Insert node into tour";
        for (int i = 0; i < tour.size(); ++i) {
            int j = (i + 1) % tour.size();
            insertStep.selected_edges.append(qMakePair(tour[i], tour[j]));
        }
        steps.push_back(insertStep);
    }

    Step completeStep;
    completeStep.action = StepAction::COMPLETE;
    completeStep.tour = tour;
    completeStep.value = calculateTourCost(points, tour);
    completeStep.debugInfo = "complete tour";
    steps.push_back(completeStep);

    return steps;
}


double estimateRemaining(const QVector<QPointF>& points, const QSet<int>& visited)
{
    double estimate = 0.0;
    int n = points.size();

    for (int i = 0; i < n; ++i)
    {
        if (visited.contains(i)) continue;

        double first = std::numeric_limits<double>::max();
        double second = std::numeric_limits<double>::max();

        for (int j = 0; j < n; ++j)
        {
            if (i == j) continue;

            double d = calculateDistance(points[i], points[j]);

            if (d < first) {
                second = first;
                first = d;
            } else if (d < second) {
                second = d;
            }
        }

        estimate += (first + second) / 2.0;
    }

    return estimate;
}


double calculatePathCost(const QVector<QPointF>& points, const QVector<int>& path)
{
    double cost = 0.0;
    for (int i = 0; i + 1 < path.size(); ++i)
        cost += calculateDistance(points[path[i]], points[path[i+1]]);
    return cost;
}

QVector<Step> generateBranchAndBoundSteps(const QVector<QPointF> &points)
{
    QVector<Step> steps;

    int n = points.size();
    QVector<int> bestTour;
    double bestCost = std::numeric_limits<double>::max();

    QVector<int> path;
    QSet<int> visited;

    if (n == 0) return steps;

    path.append(0);
    visited.insert(0);

    Step start;
    start.action = StepAction::SELECT_NODE;
    start.tour = path;
    start.currentNode = 0;
    start.debugInfo = "Start Branch & Bound";
    steps.push_back(start);

    std::function<void()> dfs = [&]() {

        if (path.size() == n)
        {
            double cost = calculateTourCost(points, path);

            Step completeCandidate;
            completeCandidate.action = StepAction::VISIT_NODE;
            completeCandidate.tour = path;
            completeCandidate.value = cost;
            completeCandidate.debugInfo = "Complete tour candidate";
            steps.push_back(completeCandidate);

            if (cost < bestCost)
            {
                bestCost = cost;
                bestTour = path;

                Step update;
                update.action = StepAction::UPDATE_BEST;
                update.tour = path;
                update.value = cost;
                update.bestCost = bestCost;
                update.debugInfo = "New best solution";
                steps.push_back(update);
            }
            return;
        }

        QVector<int> candidates;
        for (int i = 0; i < n; ++i)
        {
            if (!visited.contains(i))
                candidates.append(i);
        }

        std::sort(candidates.begin(), candidates.end(),
                  [&](int a, int b)
                  {
                      return calculateDistance(points[path.last()], points[a]) <
                             calculateDistance(points[path.last()], points[b]);
                  });

        for (int i : candidates)
        {
            double currentCost = calculatePathCost(points, path);

            double bound = currentCost + estimateRemaining(points, visited);

            Step explore;
            explore.action = StepAction::COMPARE_EDGE;
            explore.tour = path;
            explore.currentNode = path.last();
            explore.candidate_edges.append(qMakePair(path.last(), i));
            explore.value = currentCost;
            explore.bound = bound;
            explore.bestCost = bestCost;
            explore.debugInfo = "Compute bound = cost + estimate";
            steps.push_back(explore);

            if (bound >= bestCost)
            {
                Step prune;
                prune.action = StepAction::VISIT_NODE;
                prune.tour = path;
                prune.rejected_edges.append(qMakePair(path.last(), i));
                prune.value = currentCost;
                prune.bound = bound;
                prune.bestCost = bestCost;
                prune.debugInfo = "Prune branch (bound >= best)";
                steps.push_back(prune);
                continue;
            }

            path.append(i);
            visited.insert(i);

            Step visit;
            visit.action = StepAction::VISIT_NODE;
            visit.tour = path;
            visit.currentNode = i;

            for (int k = 0; k + 1 < path.size(); ++k)
            {
                visit.selected_edges.append(qMakePair(path[k], path[k+1]));
            }

            visit.value = currentCost;
            visit.debugInfo = "DFS expand";
            steps.push_back(visit);

            dfs();

            path.removeLast();
            visited.remove(i);

            Step back;
            back.action = StepAction::SELECT_NODE;
            back.tour = path;
            back.debugInfo = "Backtrack";
            steps.push_back(back);
        }
    };

    dfs();

    Step finalStep;
    finalStep.action = StepAction::COMPLETE;
    finalStep.tour = bestTour;
    finalStep.value = bestCost;
    finalStep.bestCost = bestCost;
    finalStep.debugInfo = "Final optimal tour";

    if (bestTour.size() > 1)
    {
        for (int i = 0; i < bestTour.size(); ++i)
        {
            int u = bestTour[i];
            int v = bestTour[(i + 1) % bestTour.size()];

            finalStep.selected_edges.append(qMakePair(u, v));
        }
    }

    steps.push_back(finalStep);

    return steps;
}