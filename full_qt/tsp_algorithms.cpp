#include "tsp_algorithms.h"
#include <QRandomGenerator>
#include <QSet>
#include <algorithm>
#include <cmath>
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

    // ===== PATH COST =====
    for (int i = 0; i < tour.size() - 1; ++i) {
        cost += calculateDistance(points[tour[i]], points[tour[i + 1]]);
    }

    // ===== ONLY CLOSE WHEN COMPLETE =====
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

    // INIT
    Step initStep;
    initStep.action = StepAction::SELECT_NODE;
    initStep.tour = path;
    initStep.debugInfo = "init random";
    steps.push_back(initStep);

    // Start from a random node
    if (unpicked.isEmpty()) return steps;
    int startIdx = QRandomGenerator::global()->bounded(unpicked.size());
    int current = unpicked.takeAt(startIdx);
    path.append(current);

    while (!unpicked.isEmpty()) {
        // Candidate step: all valid next edges from current to unpicked
        Step candidateStep;
        candidateStep.action = StepAction::COMPARE_EDGE;
        candidateStep.tour = path;
        candidateStep.currentNode = current;
        candidateStep.selected_edges = {}; // none selected yet
        candidateStep.rejected_edges = {};
        for (int v : unpicked) {
            candidateStep.candidate_edges.append(qMakePair(current, v));
        }
        candidateStep.debugInfo = "highlight possible moves";
        steps.push_back(candidateStep);

        // Randomly pick one
        int pickIdx = QRandomGenerator::global()->bounded(unpicked.size());
        int chosen = unpicked.takeAt(pickIdx);

        // Selected step: chosen edge becomes selected
        Step selStep;
        selStep.action = StepAction::UPDATE_BEST;
        selStep.tour = path;
        selStep.currentNode = current;
        selStep.selected_edges.append(qMakePair(current, chosen));
        selStep.debugInfo = "randomly selecting next node";
        steps.push_back(selStep);

        // Rejected step: all other candidate edges are rejected
        Step rejStep;
        rejStep.action = StepAction::VISIT_NODE;
        rejStep.tour = path;
        rejStep.currentNode = chosen;
        for (int v : unpicked) {
            rejStep.rejected_edges.append(qMakePair(current, v));
        }
        rejStep.debugInfo = "others rejected";
        steps.push_back(rejStep);

        // commit move
        path.append(chosen);
        current = chosen;

        // Visit/insert step: update tour
        Step visitStep;
        visitStep.action = StepAction::INSERT_NODE;
        visitStep.tour = path;
        visitStep.currentNode = current;
        visitStep.value = calculateTourCost(points, path);
        visitStep.debugInfo = "visited node";
        steps.push_back(visitStep);
    }

    // COMPLETE
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
    startStep.debugInfo = "start node";
    steps.push_back(startStep);

    while (!unvisited.isEmpty()) {
        int cur = path.last();

        // 1) Candidate step: highlight all edges from cur -> unvisited
        Step candidateStep;
        candidateStep.action = StepAction::COMPARE_EDGE;
        candidateStep.tour = path;
        candidateStep.currentNode = cur;
        for (int v : unvisited) {
            candidateStep.candidate_edges.append(qMakePair(cur, v));
        }
        candidateStep.debugInfo = "Evaluating neighbors of current node";
        steps.push_back(candidateStep);

        // Find nearest
        double minDist = std::numeric_limits<double>::max();
        int next = -1;
        for (int v : unvisited) {
            double dist = calculateDistance(points[cur], points[v]);
            if (dist < minDist) {
                minDist = dist;
                next = v;
            }
        }

        // 2) Select step: chosen edge moves to selected_edges
        Step selectStep;
        selectStep.action = StepAction::UPDATE_BEST;
        selectStep.tour = path;
        selectStep.currentNode = cur;
        selectStep.selected_edges.append(qMakePair(cur, next));
        selectStep.value = minDist;
        selectStep.debugInfo = "Selecting shortest edge";
        steps.push_back(selectStep);

        // 3) Reject step: others marked rejected
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

        // Commit move
        path.append(next);
        unvisited.remove(next);

        // Visit node step
        Step visitStep;
        visitStep.action = StepAction::INSERT_NODE;
        visitStep.tour = path;
        visitStep.currentNode = next;
        visitStep.value = calculateTourCost(points, path);
        visitStep.debugInfo = "visited node";
        steps.push_back(visitStep);
    }

    // COMPLETE
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
        // trivial
        Step s;
        s.action = StepAction::COMPLETE;
        s.tour = tour;
        steps.push_back(s);
        return steps;
    }

    // ===== INIT with first two nodes =====
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

        // PHASE A: choose node closest to tour
        double minDist = std::numeric_limits<double>::max();
        int bestNode = -1;

        // Build candidate edges from every unvisited node to the tour (for UI highlight)
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

        // PHASE B: evaluate insertion positions
        double bestCost = std::numeric_limits<double>::max();
        int bestPos = -1;

        for (int i = 0; i < tour.size(); ++i) {
            int j = (i + 1) % tour.size();

            double cost =
                calculateDistance(points[tour[i]], points[bestNode]) +
                calculateDistance(points[bestNode], points[tour[j]]) -
                calculateDistance(points[tour[i]], points[tour[j]]);

            // For UI, candidate insertion positions are represented as candidate edges (i->j)
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

        // Perform insertion
        tour.insert(bestPos + 1, bestNode);
        unvisited.remove(bestNode);

        Step insertStep;
        insertStep.action = StepAction::INSERT_NODE;
        insertStep.tour = tour;
        insertStep.currentNode = bestNode;
        insertStep.value = calculateTourCost(points, tour);
        insertStep.debugInfo = "Insert node into tour";
        // mark new selected edges (entire tour as selected for clarity)
        for (int i = 0; i < tour.size(); ++i) {
            int j = (i + 1) % tour.size();
            insertStep.selected_edges.append(qMakePair(tour[i], tour[j]));
        }
        steps.push_back(insertStep);
    }

    // COMPLETE
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

    // ===== START =====
    path.append(0);
    visited.insert(0);

    Step start;
    start.action = StepAction::SELECT_NODE;
    start.tour = path;
    start.currentNode = 0;
    start.debugInfo = "Start Branch & Bound";
    steps.push_back(start);

    // ===== DFS =====
    std::function<void()> dfs = [&]() {

        // ===== COMPLETE TOUR =====
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

        // ===== TRY EXPAND =====

        // 🔥 STEP 3: build + sort candidates
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
            // ===== CURRENT COST (STEP 2 FIX) =====
            double currentCost = calculatePathCost(points, path);

            // ===== BOUND =====
            double bound = currentCost + estimateRemaining(points, visited);

            // ===== STEP: EXPLORE =====
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

            // ===== PRUNE =====
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

            // ===== CHOOSE =====
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

            // ===== BACKTRACK =====
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

    // ===== FINAL =====
    Step finalStep;
    finalStep.action = StepAction::COMPLETE;
    finalStep.tour = bestTour;
    finalStep.value = bestCost;
    finalStep.bestCost = bestCost;
    finalStep.debugInfo = "Final optimal tour";

    // 🔥 FIX CHUẨN: build edges rõ ràng cho UI
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

static QVector<int> randomTour(int n)
{
    QVector<int> tour;

    for (int i = 0; i < n; ++i)
        tour.append(i);

    std::shuffle(
        tour.begin(),
        tour.end(),
        *QRandomGenerator::global()
        );

    return tour;
}

QVector<Step> generateSimulatedAnnealingSteps(
    const QVector<QPointF> &points)
{
    QVector<Step> steps;

    const int n = points.size();

    if (n < 2)
        return steps;

    // =========================================
    // INITIAL RANDOM TOUR
    // =========================================

    QVector<int> currentTour =
        randomTour(n);

    double currentCost =
        calculateTourCost(points, currentTour);

    QVector<int> bestTour =
        currentTour;

    double bestCost =
        currentCost;

    // =========================================
    // SA PARAMETERS
    // =========================================

    double temperature = 1000.0;

    const double coolingRate = 0.95;

    const int iterations = 140;

    // =========================================
    // EARLY CONVERGENCE
    // =========================================

    int stagnantSteps = 0;

    // =========================================
    // INIT STEP
    // =========================================

    Step init;

    init.action =
        StepAction::SELECT_NODE;

    init.tour =
        currentTour;

    init.temperature =
        temperature;

    init.value =
        currentCost;

    init.bestCost =
        bestCost;

    init.debugInfo =
        QString("Initial random solution | cost=%1")
            .arg(currentCost, 0, 'f', 1);

    steps.push_back(init);

    // =========================================
    // MAIN LOOP
    // =========================================

    for (int iter = 0; iter < iterations; ++iter)
    {
        // -------------------------------------
        // PICK 2 RANDOM POSITIONS
        // -------------------------------------

        int i =
            QRandomGenerator::global()->bounded(n);

        int j =
            QRandomGenerator::global()->bounded(n);

        while (i == j)
        {
            j =
                QRandomGenerator::global()->bounded(n);
        }

        // city ids
        int cityA = currentTour[i];
        int cityB = currentTour[j];

        // -------------------------------------
        // BUILD NEIGHBOR
        // -------------------------------------

        QVector<int> neighbor =
            currentTour;

        std::swap(
            neighbor[i],
            neighbor[j]
            );

        double neighborCost =
            calculateTourCost(points, neighbor);

        double delta =
            neighborCost - currentCost;

        // =====================================
        // STEP 1 : VISUALIZE SWAP ATTEMPT
        // =====================================

        Step compareStep;

        compareStep.action =
            StepAction::COMPARE_EDGE;

        compareStep.tour =
            currentTour;

        compareStep.swapIndexA = i;
        compareStep.swapIndexB = j;

        compareStep.currentNode =
            cityA;

        compareStep.bestCandidate =
            cityB;

        compareStep.temperature =
            temperature;

        compareStep.value =
            neighborCost;

        compareStep.bestCost =
            bestCost;

        compareStep.debugInfo =
            QString(
                "Trying swap %1 ↔ %2 | T=%3"
                )
                .arg(cityA)
                .arg(cityB)
                .arg(temperature, 0, 'f', 1);

        steps.push_back(compareStep);

        // =====================================
        // ACCEPTANCE TEST
        // =====================================

        bool acceptMove = false;

        if (delta < 0)
        {
            acceptMove = true;
        }
        else
        {
            double probability =
                std::exp(-delta / temperature);

            double randomValue =
                QRandomGenerator::global()
                    ->generateDouble();

            if (randomValue < probability)
            {
                acceptMove = true;
            }
        }

        // =====================================
        // ACCEPT MOVE
        // =====================================

        if (acceptMove)
        {
            currentTour =
                neighbor;

            currentCost =
                neighborCost;

            if (currentCost < bestCost)
            {
                bestTour =
                    currentTour;

                bestCost =
                    currentCost;

                stagnantSteps = 0;
            }

            Step acceptStep;

            acceptStep.action =
                StepAction::UPDATE_BEST;

            acceptStep.tour =
                currentTour;

            acceptStep.swapIndexA = i;
            acceptStep.swapIndexB = j;

            acceptStep.currentNode =
                cityA;

            acceptStep.bestCandidate =
                cityB;

            acceptStep.temperature =
                temperature;

            acceptStep.acceptedMove =
                true;

            acceptStep.value =
                currentCost;

            acceptStep.bestCost =
                bestCost;

            acceptStep.debugInfo =
                QString(
                    "Accepted swap | cost=%1"
                    )
                    .arg(currentCost, 0, 'f', 1);

            steps.push_back(acceptStep);
        }

        // =====================================
        // REJECT MOVE
        // =====================================

        else
        {
            Step rejectStep;

            rejectStep.action =
                StepAction::VISIT_NODE;

            rejectStep.tour =
                currentTour;

            rejectStep.swapIndexA = i;
            rejectStep.swapIndexB = j;

            rejectStep.currentNode =
                cityA;

            rejectStep.bestCandidate =
                cityB;

            rejectStep.temperature =
                temperature;

            rejectStep.acceptedMove =
                false;

            rejectStep.value =
                currentCost;

            rejectStep.bestCost =
                bestCost;

            rejectStep.debugInfo =
                QString(
                    "Rejected swap"
                    );

            steps.push_back(rejectStep);
        }

        // =====================================
        // COOL DOWN
        // =====================================
        stagnantSteps++;

        temperature *= coolingRate;

        // =====================================
        // STOP CONDITIONS
        // =====================================

        // sufficiently cooled
        if (temperature <= 0.001)
        {
            break;
        }

        // converged for too long
        // =====================================
        // EARLY CONVERGENCE
        // =====================================

        // chỉ stop stagnation
        // khi nhiệt đã khá thấp

        if (temperature < 5.0 &&
            stagnantSteps >= 40)
        {
            break;
        }
    }

    // =========================================
    // FINAL BEST TOUR
    // =========================================

    Step complete;

    complete.action =
        StepAction::COMPLETE;

    complete.tour =
        bestTour;

    complete.temperature = temperature;

    complete.value =
        bestCost;

    complete.bestCost =
        bestCost;

    complete.debugInfo =
        QString(
            "SA complete | best=%1"
            )
            .arg(bestCost, 0, 'f', 1);

    steps.push_back(complete);

    return steps;
}

QVector<int> reconstructHeldKarpPath(
    int mask,
    int cur,
    const QVector<QVector<int>>& parent
    )
{
    QVector<int> path;

    while (cur != -1)
    {
        path.push_front(cur);

        int prev =
            parent[mask][cur];

        mask ^= (1 << cur);

        cur = prev;
    }

    return path;
}

QVector<Step> generateHeldKarpSteps(const QVector<QPointF>& points)
{
    QVector<Step> steps;

    int n = points.size();

    if (n < 2)
        return steps;

    const double INF = 1e18;

    int FULL =
        1 << n;

    QVector<QVector<double>> dp(
        FULL,
        QVector<double>(n, INF)
        );

    QVector<QVector<int>> parent(
        FULL,
        QVector<int>(n, -1)
        );

    // =====================================
    // INIT
    // =====================================

    dp[1][0] = 0.0;

    Step init;

    init.action =
        StepAction::SELECT_NODE;

    init.currentNode = 0;

    init.tour = {0};

    init.debugInfo =
        "Initialize DP with start node";

    steps.push_back(init);

    // =====================================
    // DP
    // =====================================

    for (int mask = 1;
         mask < FULL;
         ++mask)
    {
        for (int u = 0;
             u < n;
             ++u)
        {
            if (!(mask & (1 << u)))
                continue;

            if (dp[mask][u] >= INF)
                continue;

            for (int v = 0;
                 v < n;
                 ++v)
            {
                // already visited
                if (mask & (1 << v))
                    continue;

                int nextMask =
                    mask | (1 << v);

                double edgeCost =
                    calculateDistance(
                        points[u],
                        points[v]
                        );

                double candidate =
                    dp[mask][u] +
                    edgeCost;

                // =========================
                // COMPARE STEP
                // =========================

                Step compare;

                compare.action =
                    StepAction::COMPARE_EDGE;

                compare.currentNode =
                    u;

                compare.edge =
                    qMakePair(u, v);

                compare.candidate_edges.append(
                    qMakePair(u, v)
                    );

                compare.value =
                    candidate;

                compare.bestCost =
                    candidate;

                compare.debugInfo =
                    QString(
                        "DP[%1][%2] -> %3"
                        )
                        .arg(mask, 0, 2)
                        .arg(u)
                        .arg(v);

                steps.push_back(compare);

                // =========================
                // RELAX
                // =========================

                if (candidate <
                    dp[nextMask][v])
                {
                    double oldValue =
                        dp[nextMask][v];

                    dp[nextMask][v] =
                        candidate;

                    parent[nextMask][v] =
                        u;

                    Step relax;

                    relax.action =
                        StepAction::UPDATE_BEST;

                    relax.currentNode =
                        v;

                    relax.edge =
                        qMakePair(u, v);

                    // =====================================
                    // DP METADATA
                    // =====================================

                    relax.dpMask =
                        nextMask;

                    relax.dpPrevMask =
                        mask;

                    relax.dpFrom =
                        u;

                    relax.dpTo =
                        v;

                    relax.dpOldCost =
                        oldValue;

                    relax.dpNewCost =
                        candidate;

                    // =====================================
                    // BUILD CURRENT DP PATH
                    // =====================================

                    QVector<int> partialPath =
                        reconstructHeldKarpPath(
                            mask,
                            u,
                            parent
                            );

                    // =====================================
                    // VISUALIZE CURRENT SUBPATH
                    // =====================================

                    for (int i = 0;
                         i + 1 < partialPath.size();
                         ++i)
                    {
                        relax.selected_edges.append(
                            qMakePair(
                                partialPath[i],
                                partialPath[i + 1]
                                )
                            );
                    }

                    // =====================================
                    // SHOW EXPANSION EDGE
                    // =====================================

                    relax.candidate_edges.append(
                        qMakePair(u, v)
                        );

                    relax.tour =
                        partialPath;

                    relax.value =
                        candidate;

                    relax.bestCost =
                        candidate;

                    relax.debugInfo =
                        QString(
                            "Relax DP[%1][%2]"
                            )
                            .arg(nextMask, 0, 2)
                            .arg(v);

                    steps.push_back(relax);
                }
                else
                {
                    // =====================
                    // REJECT STEP
                    // =====================

                    Step reject;

                    reject.action =
                        StepAction::VISIT_NODE;

                    reject.currentNode =
                        v;

                    reject.edge =
                        qMakePair(u, v);

                    reject.rejected_edges.append(
                        qMakePair(u, v)
                        );

                    reject.value =
                        candidate;

                    reject.bestCost =
                        dp[nextMask][v];

                    reject.debugInfo =
                        "Transition rejected";

                    steps.push_back(reject);
                }
            }
        }
    }

    // =====================================
    // FIND BEST END
    // =====================================

    int finalMask =
        FULL - 1;

    double best =
        INF;

    int last =
        -1;

    for (int u = 1;
         u < n;
         ++u)
    {
        double finalCost =
            dp[finalMask][u] +
            calculateDistance(
                points[u],
                points[0]
                );

        Step compareFinal;

        compareFinal.action =
            StepAction::COMPARE_EDGE;

        compareFinal.currentNode =
            u;

        compareFinal.edge =
            qMakePair(u, 0);

        compareFinal.candidate_edges.append(
            qMakePair(u, 0)
            );

        compareFinal.value =
            finalCost;

        compareFinal.bestCost =
            best;

        compareFinal.debugInfo =
            "Evaluating final return edge";

        steps.push_back(compareFinal);

        if (finalCost < best)
        {
            best = finalCost;
            last = u;
        }
    }

    // =====================================
    // RECONSTRUCT PATH
    // =====================================

    QVector<int> path;

    int mask =
        finalMask;

    int cur =
        last;

    while (cur != -1)
    {
        path.push_front(cur);

        int prev =
            parent[mask][cur];

        mask ^= (1 << cur);

        cur = prev;
    }

    // =====================================
    // COMPLETE TOUR
    // =====================================

    Step complete;

    complete.action =
        StepAction::COMPLETE;

    complete.tour =
        path;

    complete.value =
        best;

    complete.bestCost =
        best;

    complete.debugInfo =
        "Held-Karp optimal tour";

    for (int i = 0;
         i < path.size();
         ++i)
    {
        int j =
            (i + 1) % path.size();

        complete.selected_edges.append(
            qMakePair(
                path[i],
                path[j]
                )
            );
    }

    steps.push_back(complete);

    return steps;
}