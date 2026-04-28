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
    for (int i = 0; i < tour.size() - 1; ++i) {
        cost += calculateDistance(points[tour[i]], points[tour[i + 1]]);
    }
    if (!tour.isEmpty()) {
        cost += calculateDistance(points[tour.last()], points[tour.first()]);
    }
    return cost;
}

QVector<Step> generateRandomSteps(const QVector<QPointF> &points)
{
    QVector<Step> steps;
    QVector<int> tour;
    for (int i = 0; i < points.size(); ++i) {
        tour.append(i);
    }

    // INIT step
    Step initStep;
    initStep.action = StepAction::SELECT_NODE;
    initStep.tour = {};
    initStep.currentNode = -1;
    initStep.bestCandidate = -1;
    initStep.edge = qMakePair(-1, -1);
    initStep.value = 0.0;
    initStep.debugInfo = "init random";
    steps.push_back(initStep);

    // Shuffle (optional: visualize swaps)
    std::shuffle(tour.begin(), tour.end(), *QRandomGenerator::global());

    // COMPLETE step
    double cost = calculateTourCost(points, tour);
    Step completeStep;
    completeStep.action = StepAction::INSERT_NODE;
    completeStep.tour = tour;
    completeStep.currentNode = -1;
    completeStep.bestCandidate = -1;
    completeStep.edge = qMakePair(-1, -1);
    completeStep.value = cost;
    completeStep.debugInfo = "random complete";
    steps.push_back(completeStep);

    return steps;
}

QVector<Step> generateGreedySteps(const QVector<QPointF> &points)
{
    QVector<Step> steps;
    QVector<int> path;
    QSet<int> unvisited;
    for (int i = 0; i < points.size(); ++i) {
        unvisited.insert(i);
    }

    int start = 0;
    path.append(start);
    unvisited.remove(start);

    // SELECT_NODE for start
    Step startStep;
    startStep.action = StepAction::SELECT_NODE;
    startStep.tour = path;
    startStep.currentNode = start;
    startStep.bestCandidate = -1;
    startStep.edge = qMakePair(-1, -1);
    startStep.value = 0.0;
    startStep.debugInfo = "start node";
    steps.push_back(startStep);

    while (!unvisited.isEmpty()) {
        int cur = path.last();
        double minDist = std::numeric_limits<double>::max();
        int next = -1;

        for (int j : unvisited) {
            double dist = calculateDistance(points[cur], points[j]);
            // COMPARE_EDGE
            Step compareStep;
            compareStep.action = StepAction::COMPARE_EDGE;
            compareStep.tour = path;
            compareStep.currentNode = cur;
            compareStep.bestCandidate = next;
            compareStep.edge = qMakePair(cur, j);
            compareStep.value = dist;
            compareStep.debugInfo = "comparing neighbor";
            steps.push_back(compareStep);
            if (dist < minDist) {
                next = j;
                minDist = dist;
                // UPDATE_BEST
                Step updateStep;
                updateStep.action = StepAction::UPDATE_BEST;
                updateStep.tour = path;
                updateStep.currentNode = cur;
                updateStep.bestCandidate = next;
                updateStep.edge = qMakePair(cur, next);
                updateStep.value = minDist;
                updateStep.debugInfo = "best neighbor";
                steps.push_back(updateStep);
            }
        }

        path.append(next);
        unvisited.remove(next);
        // VISIT_NODE
        double totalCost = calculateTourCost(points, path);
        Step visitStep;
        visitStep.action = StepAction::INSERT_NODE;
        visitStep.tour = path;
        visitStep.currentNode = next;
        visitStep.bestCandidate = -1;
        visitStep.edge = qMakePair(-1, -1);
        visitStep.value = totalCost;
        visitStep.debugInfo = "visited node";
        steps.push_back(visitStep);
    }

    return steps;
}

QVector<Step> generateNearestInsertionSteps(const QVector<QPointF> &points)
{
    QVector<Step> steps;
    QVector<int> tour;
    QSet<int> unvisited;
    for (int i = 0; i < points.size(); ++i) {
        unvisited.insert(i);
    }

    // Init with first two points
    int a = 0, b = 1;
    tour.append(a);
    tour.append(b);
    unvisited.remove(a);
    unvisited.remove(b);

    Step initStep;
    initStep.action = StepAction::SELECT_NODE;
    initStep.tour = tour;
    initStep.currentNode = a;
    initStep.bestCandidate = -1;
    initStep.edge = qMakePair(-1, -1);
    initStep.value = 0.0;
    initStep.debugInfo = "init tour";
    steps.push_back(initStep);

    Step addSecondStep;
    addSecondStep.action = StepAction::INSERT_NODE;
    addSecondStep.tour = tour;
    addSecondStep.currentNode = b;
    addSecondStep.bestCandidate = -1;
    addSecondStep.edge = qMakePair(-1, -1);
    addSecondStep.value = calculateTourCost(points, tour);
    addSecondStep.debugInfo = "added second";
    steps.push_back(addSecondStep);

    while (!unvisited.isEmpty()) {
        double bestCost = std::numeric_limits<double>::max();
        int bestNode = -1;
        int bestPos = -1;

        // SELECT_NODE: find best node to insert
        for (int node : unvisited) {
            for (int i = 0; i < tour.size(); ++i) {
                int j = (i + 1) % tour.size();
                double cost = calculateDistance(points[tour[i]], points[node]) +
                             calculateDistance(points[node], points[tour[j]]) -
                             calculateDistance(points[tour[i]], points[tour[j]]);
                if (cost < bestCost) {
                    bestCost = cost;
                    bestNode = node;
                    bestPos = i;
                }
            }
        }

        Step selectStep;
        selectStep.action = StepAction::SELECT_NODE;
        selectStep.tour = tour;
        selectStep.currentNode = bestNode;
        selectStep.bestCandidate = -1;
        selectStep.edge = qMakePair(-1, -1);
        selectStep.value = bestCost;
        selectStep.debugInfo = "selected node to insert";
        steps.push_back(selectStep);

        // Compare edges
        for (int i = 0; i < tour.size(); ++i) {
            int j = (i + 1) % tour.size();
            double cost = calculateDistance(points[tour[i]], points[bestNode]) +
                         calculateDistance(points[bestNode], points[tour[j]]) -
                         calculateDistance(points[tour[i]], points[tour[j]]);
            Step compareStep;
            compareStep.action = StepAction::COMPARE_EDGE;
            compareStep.tour = tour;
            compareStep.currentNode = bestNode;
            compareStep.bestCandidate = -1;
            compareStep.edge = qMakePair(tour[i], tour[j]);
            compareStep.value = cost;
            compareStep.debugInfo = "comparing edge";
            steps.push_back(compareStep);
            if (i == bestPos) {
                Step updateStep;
                updateStep.action = StepAction::UPDATE_BEST;
                updateStep.tour = tour;
                updateStep.currentNode = bestNode;
                updateStep.bestCandidate = -1;
                updateStep.edge = qMakePair(tour[i], tour[j]);
                updateStep.value = cost;
                updateStep.debugInfo = "best position";
                steps.push_back(updateStep);
            }
        }

        // INSERT_NODE
        tour.insert(bestPos + 1, bestNode);
        unvisited.remove(bestNode);
        double totalCost = calculateTourCost(points, tour);
        Step insertStep;
        insertStep.action = StepAction::INSERT_NODE;
        insertStep.tour = tour;
        insertStep.currentNode = bestNode;
        insertStep.bestCandidate = -1;
        insertStep.edge = qMakePair(-1, -1);
        insertStep.value = totalCost;
        insertStep.debugInfo = "inserted node";
        steps.push_back(insertStep);
    }

    return steps;
}

QVector<Step> generateBranchAndBoundSteps(const QVector<QPointF> &points)
{
    QVector<Step> steps;
    QVector<int> bestTour;
    double bestCost = std::numeric_limits<double>::max();

    // Simplified DFS with pruning
    QVector<int> path;
    QSet<int> visited;
    path.append(0);
    visited.insert(0);

    Step startStep;
    startStep.action = StepAction::SELECT_NODE;
    startStep.tour = path;
    startStep.currentNode = 0;
    startStep.bestCandidate = -1;
    startStep.edge = qMakePair(-1, -1);
    startStep.value = 0.0;
    startStep.debugInfo = "start DFS";
    steps.push_back(startStep);

    // Note: Full B&B is complex, this is simplified for visualization
    // In practice, implement proper DFS with bound calculation

    // For demo, just return a simple path
    for (int i = 1; i < points.size(); ++i) {
        path.append(i);
        double cost = calculateTourCost(points, path);
        Step visitStep;
        visitStep.action = StepAction::VISIT_NODE;
        visitStep.tour = path;
        visitStep.currentNode = i;
        visitStep.bestCandidate = -1;
        visitStep.edge = qMakePair(-1, -1);
        visitStep.value = cost;
        visitStep.debugInfo = "visit node";
        steps.push_back(visitStep);
        if (cost < bestCost) {
            bestCost = cost;
            bestTour = path;
            Step updateStep;
            updateStep.action = StepAction::UPDATE_BEST;
            updateStep.tour = path;
            updateStep.currentNode = -1;
            updateStep.bestCandidate = -1;
            updateStep.edge = qMakePair(-1, -1);
            updateStep.value = bestCost;
            updateStep.debugInfo = "new best";
            steps.push_back(updateStep);
        }
    }

    Step completeStep;
    completeStep.action = StepAction::INSERT_NODE;
    completeStep.tour = bestTour;
    completeStep.currentNode = -1;
    completeStep.bestCandidate = -1;
    completeStep.edge = qMakePair(-1, -1);
    completeStep.value = bestCost;
    completeStep.debugInfo = "B&B complete";
    steps.push_back(completeStep);

    return steps;
}