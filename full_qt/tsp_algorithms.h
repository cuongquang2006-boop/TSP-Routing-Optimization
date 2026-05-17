#ifndef TSP_ALGORITHMS_H
#define TSP_ALGORITHMS_H

#include <QVector>
#include <QPair>
#include <QString>
#include <QPointF>
#include "tsp_steps.h"

enum class TSPAlgorithm
{
    RANDOM,
    GREEDY,
    NEAREST_INSERTION,
    BRANCH_AND_BOUND,
    SIMULATED_ANNEALING,
    HELD_KARP
};

QVector<Step> generateRandomSteps(const QVector<QPointF> &points);
QVector<Step> generateGreedySteps(const QVector<QPointF> &points);
QVector<Step> generateNearestInsertionSteps(const QVector<QPointF> &points);
QVector<Step> generateBranchAndBoundSteps(const QVector<QPointF> &points);
QVector<Step> generateSimulatedAnnealingSteps(const QVector<QPointF> &points);
QVector<Step>generateHeldKarpSteps(const QVector<QPointF>& points);

double calculateDistance(const QPointF &a, const QPointF &b);
double calculateTourCost(const QVector<QPointF> &points, const QVector<int> &tour);

#endif