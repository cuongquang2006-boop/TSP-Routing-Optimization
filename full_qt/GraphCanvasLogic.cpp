#include "GraphCanvas.h"
#include <QPainter>
#include <QRandomGenerator>
#include <QTimer>
#include <cmath>
#include <QTime>
#include <algorithm>

static const double DIST_SCALE = 1000.0;

static QPair<int,int> normEdge(int u, int v)
{
    return (u < v) ? qMakePair(u, v) : qMakePair(v, u);
}

void GraphCanvas::setPoints(const QVector<GraphPoint> &newPoints)
{
    points = newPoints;
    edges.clear();
    for (int i = 1; i < points.size(); ++i)
    {
        edges.append(qMakePair(i - 1, i));
    }
    update();
}

void GraphCanvas::setMaxNodeLimit(int limit)
{
    maxNodeLimit = limit;
}

void GraphCanvas::recomputeLayout()
{
    int n = points.size();
    if (n == 0) return;

    QPointF center(0.5, 0.5);
    qreal radius = (n <= 8) ? 0.32 : 0.38;

    for (int i = 0; i < n; ++i)
    {
        qreal angle = (2.0 * M_PI * i) / n;

        qreal nx = center.x() + std::cos(angle) * radius;
        qreal ny = center.y() + std::sin(angle) * radius;

        points[i].normalizedPos = QPointF(nx, ny);
    }
}

void GraphCanvas::addPoint(const QPointF &pos)
{
    int maxNodes = maxNodeLimit;

    if (points.size() >= maxNodes)
    {
        return;
    }

    GraphPoint point;

    qreal nx = pos.x() / width();
    qreal ny = pos.y() / height();

    point.normalizedPos = QPointF(nx, ny);
    point.color = (points.size() % 2 == 0) ? QColor("#7dd3fc") : QColor("#ffffff");

    points.append(point);

    emit pointAdded(point.normalizedPos);

    if (points.size() > 1) {
        edges.append(qMakePair(points.size() - 2, points.size() - 1));
    }

    if (inputMode == InputMode::MANUAL) {
        recomputeLayout();
    }

    update();
}

void GraphCanvas::generateRandomPoints(int count)
{
    clearPoints();

    count = qMin(count, maxNodeLimit);

    QPointF center(0.5, 0.5);
    qreal radius = (count <= 8) ? 0.32 : 0.38;

    for (int i = 0; i < count; ++i)
    {
        qreal angle = (2.0 * M_PI * i) / count;

        qreal nx = center.x() + std::cos(angle) * radius;
        qreal ny = center.y() + std::sin(angle) * radius;

        GraphPoint point;
        point.normalizedPos = QPointF(nx, ny);
        point.color = (i % 2 == 0) ? QColor("#7dd3fc") : QColor("#ffffff");

        points.append(point);

        if (i > 0)
            edges.append(qMakePair(i - 1, i));
    }

    update();
}

void GraphCanvas::clearPoints()
{
    points.clear();
    edges.clear();
    currentTour.clear();
    committedEdges.clear();

    step_candidate_edges.clear();
    step_selected_edges.clear();
    step_rejected_edges.clear();

    highlightedNode = -1;
    bestNode = -1;
    highlightedEdge = qMakePair(-1, -1);

    activeParticles.clear();

    currentProgress = 0.0;
    elapsedTimeMs = 0.0;

    if (particleUpdateTimer->isActive()) {
        particleUpdateTimer->stop();
    }

    update();
}

void GraphCanvas::setTour(const QVector<int> &tour, int currentNode, int bestNode, QPair<int,int> edge)
{
    previousTour = currentTour;
    currentTour = tour;

    highlightedNode = currentNode;
    this->bestNode = bestNode;
    highlightedEdge = edge;

    step_candidate_edges.clear();
    step_selected_edges.clear();
    step_rejected_edges.clear();

    if (currentStepAction == StepAction::INSERT_NODE) {
        for (int i = 0; i < currentTour.size(); ++i) {
            int j = (i + 1) % currentTour.size();
            committedEdges.insert(normEdge(currentTour[i], currentTour[j]));
        }
    }

    currentProgress = 0.0;
    elapsedTimeMs = 0.0;

    if (!particleUpdateTimer->isActive()) {
        particleUpdateTimer->start(16);
    }
}

void GraphCanvas::setStepAction(StepAction action)
{
    currentStepAction = action;
}

void GraphCanvas::setStep(const Step &step)
{
    previousTour = currentTour;
    currentTour = step.tour;

    highlightedNode = step.currentNode;
    bestNode = step.bestCandidate;
    highlightedEdge = step.edge;

    step_candidate_edges = step.candidate_edges;
    step_selected_edges = step.selected_edges;
    step_rejected_edges = step.rejected_edges;

    if (step.action == StepAction::COMPLETE)
    {
        for (int i = 0; i < currentTour.size(); ++i)
        {
            int j = (i + 1) % currentTour.size();
            committedEdges.insert(normEdge(currentTour[i], currentTour[j]));
        }
    }

    currentStepAction = step.action;
    if (step.action == StepAction::COMPLETE)
    {
        currentProgress = 1.0;
        elapsedTimeMs = animationDurationMs;

        if (particleUpdateTimer->isActive())
            particleUpdateTimer->stop();

        update();
        return;
    }

    currentProgress = 0.0;
    elapsedTimeMs = 0.0;

    if (!particleUpdateTimer->isActive()) {
        particleUpdateTimer->start(16);
    }
}

void GraphCanvas::setAnimationProgress(qreal progress)
{
    currentProgress = qBound(0.0, progress, 1.0);
}

void GraphCanvas::setAnimationParams(int durationMs, qreal speedMultiplier)
{
    animationDurationMs = durationMs;
    this->speedMultiplier = speedMultiplier;
}

void GraphCanvas::updateAnimation()
{
    qreal now = QTime::currentTime().msecsSinceStartOfDay();
    qreal deltaTime = now - lastFrameTime;
    lastFrameTime = now;

    deltaTime = qMin(deltaTime, 50.0);

    qreal adjustedDuration = animationDurationMs / (speedMultiplier * 0.7);
    adjustedDuration = qMax(adjustedDuration, 180.0);

    elapsedTimeMs += deltaTime;

    if (elapsedTimeMs >= adjustedDuration)
    {
        elapsedTimeMs = adjustedDuration;
        currentProgress = 1.0;

        if (currentStepAction == StepAction::INSERT_NODE)
        {
            for (int i = 0; i < currentTour.size(); ++i)
            {
                int j = (i + 1) % currentTour.size();
                committedEdges.insert(normEdge(currentTour[i], currentTour[j]));
            }
        }

        particleUpdateTimer->stop();
    }
    else
    {
        qreal tnorm = elapsedTimeMs / adjustedDuration;
        currentProgress = easeOut(tnorm);
    }

    update();
}

void GraphCanvas::generateSweepParticles(const QPointF &from, const QPointF &to, qreal progress)
{
    qreal easedProgress = easeOut(progress);

    QPointF sweepPos = from + (to - from) * easedProgress;

    QPointF direction = to - from;
    qreal length = std::sqrt(direction.x() * direction.x() + direction.y() * direction.y());
    if (length > 0.0001) {
        direction.setX(direction.x() / length);
        direction.setY(direction.y() / length);
    }

    QPointF perpendicular(-direction.y(), direction.x());

    int particleCount = 2 + (QRandomGenerator::global()->bounded(2));

    for (int i = 0; i < particleCount; ++i) {
        Particle p;

        qreal offsetDist = (QRandomGenerator::global()->generateDouble() - 0.5) * 12.0;
        qreal alongDist = (QRandomGenerator::global()->generateDouble() - 0.5) * 6.0;

        p.pos = sweepPos + perpendicular * offsetDist + direction * alongDist;
        p.life = 1.0;
        p.color = QColor("#fbbf24");
        p.size = 4.0 + QRandomGenerator::global()->generateDouble() * 3.0;

        activeParticles.append(p);
    }
}

void GraphCanvas::updateParticles(qreal deltaTime)
{
    const qreal particleDecayRate = 3.0;

    for (auto &particle : activeParticles)
    {
        particle.life -= deltaTime * particleDecayRate;
    }

    activeParticles.erase(
        std::remove_if(activeParticles.begin(), activeParticles.end(),
                      [](const Particle &p) { return p.life <= 0.0; }),
        activeParticles.end()
    );
}

void GraphCanvas::setInputMode(InputMode mode)
{
    inputMode = mode;
}

InputMode GraphCanvas::getInputMode() const
{
    return inputMode;
}
