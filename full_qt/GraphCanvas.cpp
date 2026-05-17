#include "GraphCanvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QTimer>
#include <cmath>
#include <QTime>
#include <algorithm>

static const double DIST_SCALE = 1000.0;

GraphCanvas::GraphCanvas(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
    setMinimumSize(600, 400);

    particleUpdateTimer = new QTimer(this);
    connect(particleUpdateTimer, &QTimer::timeout, this, &GraphCanvas::updateAnimation);

    lastFrameTime =
        QTime::currentTime().msecsSinceStartOfDay();

    connect(
        &glowTimer,
        &QTimer::timeout,
        this,
        [this]()
        {
            elapsedTimeMs += 16;

            update();
        }
        );

    glowTimer.start(16);
}

GraphCanvas::~GraphCanvas()
{
    if (particleUpdateTimer)
    {
        particleUpdateTimer->stop();
    }
}

static QPair<int,int> normEdge(int u, int v)
{
    return (u < v)
    ? qMakePair(u, v)
    : qMakePair(v, u);
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

QPointF GraphCanvas::toScreen(const QPointF& normalized) const
{
    QRect area =
        contentsRect().adjusted(12, 12, -12, -12);

    qreal x =
        area.left() +
        normalized.x() * area.width();

    qreal y =
        area.top() +
        normalized.y() * area.height();

    return QPointF(x, y);
}

void GraphCanvas::setMaxNodeLimit(int limit)
{
    maxNodeLimit = limit;
}

void GraphCanvas::recomputeLayout()
{
    int n = points.size();

    if (n == 0)
        return;

    QPointF center(0.5, 0.5);

    qreal radius =
        (n <= 8)
            ? 0.32
            : 0.38;

    for (int i = 0; i < n; ++i)
    {
        qreal angle =
            (2.0 * M_PI * i) / n;

        qreal nx =
            center.x() +
            std::cos(angle) * radius;

        qreal ny =
            center.y() +
            std::sin(angle) * radius;

        points[i].normalizedPos =
            QPointF(nx, ny);
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

    point.color =
        (points.size() % 2 == 0)
            ? QColor("#7dd3fc")
            : QColor("#ffffff");

    points.append(point);

    emit pointAdded(point.normalizedPos);

    if (points.size() > 1)
    {
        edges.append(
            qMakePair(
                points.size() - 2,
                points.size() - 1
                )
            );
    }

    if (inputMode == InputMode::MANUAL)
    {
        recomputeLayout();
    }

    update();
}

void GraphCanvas::generateRandomPoints(int count)
{
    clearPoints();

    count = qMin(count, maxNodeLimit);

    QPointF center(0.5, 0.5);

    qreal radius =
        (count <= 8)
            ? 0.32
            : 0.38;

    for (int i = 0; i < count; ++i)
    {
        qreal angle =
            (2.0 * M_PI * i) / count;

        qreal nx =
            center.x() +
            std::cos(angle) * radius;

        qreal ny =
            center.y() +
            std::sin(angle) * radius;

        GraphPoint point;

        point.normalizedPos =
            QPointF(nx, ny);

        point.color =
            (i % 2 == 0)
                ? QColor("#7dd3fc")
                : QColor("#ffffff");

        points.append(point);

        if (i > 0)
        {
            edges.append(qMakePair(i - 1, i));
        }
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

    previous_candidate_edges.clear();

    previous_selected_edges.clear();

    previous_rejected_edges.clear();

    highlightedNode = -1;

    bestNode = -1;

    highlightedEdge =
        qMakePair(-1, -1);

    currentTemperature = -1.0;

    currentBestCost = -1.0;

    showTemperatureHUD = false;

    activeParticles.clear();

    currentProgress = 0.0;

    elapsedTimeMs = 0.0;

    if (particleUpdateTimer->isActive())
    {
        particleUpdateTimer->stop();
    }

    update();
}

void GraphCanvas::setTour(
    const QVector<int> &tour,
    int currentNode,
    int bestNode,
    QPair<int,int> edge
    )
{
    previousTour = currentTour;

    currentTour = tour;

    highlightedNode = currentNode;

    this->bestNode = bestNode;

    highlightedEdge = edge;

    step_candidate_edges.clear();

    step_selected_edges.clear();

    step_rejected_edges.clear();

    if (currentStepAction == StepAction::INSERT_NODE)
    {
        for (int i = 0; i < currentTour.size(); ++i)
        {
            int j =
                (i + 1) % currentTour.size();

            committedEdges.insert(
                normEdge(
                    currentTour[i],
                    currentTour[j]
                    )
                );
        }
    }

    currentProgress = 0.0;

    elapsedTimeMs = 0.0;

    if (!particleUpdateTimer->isActive())
    {
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

    currentTemperature =
        step.temperature;

    showTemperatureHUD =
        (step.temperature > 0.0);

    if (!std::isnan(step.bestCost))
    {
        currentBestCost =
            step.bestCost;
    }

    highlightedNode =
        step.currentNode;

    bestNode =
        step.bestCandidate;

    highlightedEdge =
        step.edge;

    previous_candidate_edges =
        step_candidate_edges;

    previous_selected_edges =
        step_selected_edges;

    previous_rejected_edges =
        step_rejected_edges;

    step_candidate_edges =
        step.candidate_edges;

    step_selected_edges =
        step.selected_edges;

    step_rejected_edges =
        step.rejected_edges;

    if (step.action == StepAction::COMPLETE)
    {
        for (int i = 0; i < currentTour.size(); ++i)
        {
            int j =
                (i + 1) % currentTour.size();

            committedEdges.insert(
                normEdge(
                    currentTour[i],
                    currentTour[j]
                    )
                );
        }
    }

    currentStepAction =
        step.action;

    if (step.action == StepAction::COMPLETE)
    {
        currentProgress = 1.0;

        elapsedTimeMs = animationDurationMs;

        if (particleUpdateTimer->isActive())
        {
            particleUpdateTimer->stop();
        }

        update();

        return;
    }

    currentProgress = 0.0;

    elapsedTimeMs = 0.0;

    if (!particleUpdateTimer->isActive())
    {
        particleUpdateTimer->start(16);
    }

    update();
}

void GraphCanvas::setAnimationProgress(qreal progress)
{
    currentProgress =
        qBound(0.0, progress, 1.0);
}

void GraphCanvas::setAnimationParams(
    int durationMs,
    qreal speedMultiplier
    )
{
    animationDurationMs = durationMs;

    this->speedMultiplier =
        speedMultiplier;
}

void GraphCanvas::updateAnimation()
{
    qreal now =
        QTime::currentTime().msecsSinceStartOfDay();

    qreal deltaTime =
        now - lastFrameTime;

    lastFrameTime = now;

    deltaTime =
        qMin(deltaTime, 50.0);

    qreal t =
        qBound(0.5, speedMultiplier, 2.5);

    qreal adjustedDuration =
        animationDurationMs /
        (speedMultiplier * 0.35);

    adjustedDuration =
        qMax(adjustedDuration, 350.0);

    elapsedTimeMs += deltaTime;

    if (elapsedTimeMs >= adjustedDuration)
    {
        elapsedTimeMs = adjustedDuration;

        currentProgress = 1.0;

        if (currentStepAction == StepAction::INSERT_NODE)
        {
            for (int i = 0; i < currentTour.size(); ++i)
            {
                int j =
                    (i + 1) % currentTour.size();

                committedEdges.insert(
                    normEdge(
                        currentTour[i],
                        currentTour[j]
                        )
                    );
            }
        }

        particleUpdateTimer->stop();
    }
    else
    {
        qreal tnorm =
            elapsedTimeMs / adjustedDuration;

        currentProgress =
            easeOut(tnorm);
    }

    update();
}

bool hasEdge(
    const QVector<int>& tour,
    int u,
    int v
    )
{
    for (int i = 0; i < tour.size(); ++i)
    {
        int j =
            (i + 1) % tour.size();

        int a = tour[i];

        int b = tour[j];

        if (
            (a == u && b == v) ||
            (a == v && b == u)
            )
        {
            return true;
        }
    }

    return false;
}

void GraphCanvas::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);

    painter.setRenderHint(
        QPainter::Antialiasing
        );

    painter.fillRect(
        rect(),
        QColor("#171a20")
        );

    renderEdges(painter);

    renderPoints(painter);

    bool isSA =
        (
            currentAlgorithmMode ==
            TSPAlgorithm::SIMULATED_ANNEALING
            );

    bool shouldShowHUD =
        (currentBestCost > 0.0) ||
        (isSA && currentTemperature > 0.0);

    if (shouldShowHUD)
    {
        QRectF hudRect(
            18,
            18,
            210,
            isSA ? 110 : 58
            );

        qreal pulse =
            (
                std::sin(
                    elapsedTimeMs * 0.0028
                    ) + 1.0
                ) * 0.5;

        QColor borderGlow(
            80,
            255,
            140,
            80 + pulse * 120
            );

        painter.setPen(Qt::NoPen);

        painter.setBrush(
            QColor(15, 20, 30, 215)
            );

        painter.drawRoundedRect(
            hudRect,
            14,
            14
            );

        QPen borderPen(borderGlow);

        borderPen.setWidthF(1.8);

        painter.setPen(borderPen);

        painter.setBrush(Qt::NoBrush);

        painter.drawRoundedRect(
            hudRect,
            14,
            14
            );

        QFont small =
            painter.font();

        small.setPointSize(8);

        QFont big =
            painter.font();

        big.setPointSize(13);

        big.setBold(true);

        if (
            isSA &&
            currentTemperature > 0.0
            )
        {
            painter.setFont(small);

            painter.setPen(
                QColor("#9ca3af")
                );

            painter.drawText(
                QRectF(32, 24, 140, 16),
                "Temperature"
                );

            painter.setFont(big);

            painter.setPen(
                QColor("#50ff8c")
                );

            painter.drawText(
                QRectF(32, 40, 150, 20),
                QString::number(
                    currentTemperature,
                    'f',
                    1
                    )
                );

            painter.setFont(small);

            painter.setPen(
                QColor("#9ca3af")
                );

            painter.drawText(
                QRectF(32, 72, 140, 16),
                "Best Cost"
                );

            painter.setFont(big);

            painter.setPen(
                QColor(255, 215, 80)
                );

            painter.drawText(
                QRectF(32, 88, 160, 20),
                QString::number(
                    currentBestCost,
                    'f',
                    2
                    )
                );
        }
        else
        {
            painter.setFont(small);

            painter.setPen(
                QColor("#9ca3af")
                );

            painter.drawText(
                QRectF(32, 24, 140, 16),
                "Best Cost"
                );

            painter.setFont(big);

            painter.setPen(
                QColor(255, 215, 80)
                );

            painter.drawText(
                QRectF(32, 40, 160, 20),
                QString::number(
                    currentBestCost,
                    'f',
                    2
                    )
                );
        }
    }
}

void GraphCanvas::setCurrentAlgorithm(TSPAlgorithm algo)
{
    currentAlgorithmMode = algo;

    if (algo != TSPAlgorithm::SIMULATED_ANNEALING)
    {
        currentTemperature = -1.0;

        showTemperatureHUD = false;
    }

    update();
}

void GraphCanvas::mousePressEvent(QMouseEvent *event)
{
    if (particleUpdateTimer && particleUpdateTimer->isActive()) {
        return;
    }

    if (inputMode != InputMode::CLICK) {
        return;
    }

    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    const QPointF position = event->position();

    if (!contentsRect().contains(position.toPoint()))
        return;

    addPoint(position);
    event->accept();
}

static qreal transientFade(qreal t)
{
    if (t < 0.3)
    {
        return t / 0.3;
    }

    if (t < 0.7)
    {
        return 1.0;
    }

    return 1.0 - ((t - 0.7) / 0.3);
}

QPointF GraphCanvas::getTourNodePosition(
    int tourIndex,
    bool usePrevious
    ) const
{
    const QVector<int>& tour =
        usePrevious
            ? previousTour
            : currentTour;

    if (
        tourIndex < 0 ||
        tourIndex >= tour.size()
        )
    {
        return QPointF();
    }

    int nodeId =
        tour[tourIndex];

    if (
        nodeId < 0 ||
        nodeId >= points.size()
        )
    {
        return QPointF();
    }

    return points[nodeId]
        .normalizedPos;
}

void GraphCanvas::renderEdges(QPainter &painter)
{
    QSet<QPair<int,int>> seenEdges;

    if (
        edges.isEmpty() &&
        currentTour.isEmpty() &&
        step_candidate_edges.isEmpty() &&
        step_selected_edges.isEmpty() &&
        step_rejected_edges.isEmpty()
        )
    {
        return;
    }

    QVector<std::tuple<QPointF, QPointF, double>> edgeWeights;

    for (const auto &edge : qAsConst(edges))
    {
        if (
            edge.first < points.size() &&
            edge.second < points.size()
            )
        {
            QPointF a =
                toScreen(
                    points[edge.first].normalizedPos
                    );

            QPointF b =
                toScreen(
                    points[edge.second].normalizedPos
                    );

            QPen edgePen(
                QColor("#2b3340")
                );

            edgePen.setWidth(1);

            painter.setPen(edgePen);

            painter.drawLine(a, b);

            QPair<int,int> key =
                normEdge(
                    edge.first,
                    edge.second
                    );

            if (!seenEdges.contains(key))
            {
                seenEdges.insert(key);

                int u = edge.first;

                int v = edge.second;

                const QPointF& p1 =
                    points[u].normalizedPos;

                const QPointF& p2 =
                    points[v].normalizedPos;

                double dist =
                    std::hypot(
                        p1.x() - p2.x(),
                        p1.y() - p2.y()
                        );

                edgeWeights.push_back(
                    {a, b, dist}
                    );
            }
        }
    }

    for (const auto &e : qAsConst(previous_rejected_edges))
    {
        if (step_rejected_edges.contains(e))
            continue;

        if (e.first < 0 || e.second < 0)
            continue;

        if (
            e.first >= points.size() ||
            e.second >= points.size()
            )
        {
            continue;
        }

        qreal disappear =
            1.0 - easeOut(currentProgress);

        QColor rejectedColor(
            239,
            68,
            68,
            int(170 * disappear)
            );

        QPen pen(rejectedColor);

        pen.setWidthF(
            1.0 + disappear * 2.0
            );

        pen.setCapStyle(Qt::RoundCap);

        painter.setPen(pen);

        painter.drawLine(
            toScreen(points[e.first].normalizedPos),
            toScreen(points[e.second].normalizedPos)
            );
    }

    for (const auto &e : qAsConst(previous_candidate_edges))
    {
        if (step_candidate_edges.contains(e))
            continue;

        if (e.first < 0 || e.second < 0)
            continue;

        if (
            e.first >= points.size() ||
            e.second >= points.size()
            )
        {
            continue;
        }

        qreal disappear =
            1.0 - easeOut(currentProgress);

        QColor col(
            251,
            191,
            36,
            int(255 * disappear)
            );

        drawEdgeWithGlow(
            painter,
            toScreen(points[e.first].normalizedPos),
            toScreen(points[e.second].normalizedPos),
            col,
            disappear
            );
    }

    for (const auto &e : qAsConst(step_rejected_edges))
    {
        if (e.first < 0 || e.second < 0)
            continue;

        if (
            e.first >= points.size() ||
            e.second >= points.size()
            )
        {
            continue;
        }

        qreal appear =
            easeOut(currentProgress);

        QColor rejectedColor(
            239,
            68,
            68,
            int(170 * appear)
            );

        QPen pen(rejectedColor);

        pen.setWidthF(
            1.0 + appear * 2.0
            );

        pen.setCapStyle(Qt::RoundCap);

        painter.setPen(pen);

        painter.drawLine(
            toScreen(points[e.first].normalizedPos),
            toScreen(points[e.second].normalizedPos)
            );
    }

    for (const auto &e : qAsConst(step_candidate_edges))
    {
        if (step_selected_edges.contains(e))
            continue;

        if (e.first < 0 || e.second < 0)
            continue;

        if (
            e.first >= points.size() ||
            e.second >= points.size()
            )
        {
            continue;
        }

        qreal appear =
            easeOut(currentProgress);

        QColor animated(
            251,
            191,
            36,
            int(255 * appear)
            );

        drawEdgeWithGlow(
            painter,
            toScreen(points[e.first].normalizedPos),
            toScreen(points[e.second].normalizedPos),
            animated,
            appear
            );
    }

    for (const auto &e : qAsConst(step_selected_edges))
    {
        if (e.first < 0 || e.second < 0)
            continue;

        if (
            e.first >= points.size() ||
            e.second >= points.size()
            )
        {
            continue;
        }

        qreal appear =
            easeOut(currentProgress);

        QColor col(
            34,
            197,
            94,
            int(255 * appear)
            );

        drawEdgeWithGlow(
            painter,
            toScreen(points[e.first].normalizedPos),
            toScreen(points[e.second].normalizedPos),
            col,
            appear
            );
    }

    for (const auto &e : qAsConst(previous_selected_edges))
    {
        if (step_selected_edges.contains(e))
            continue;

        if (e.first < 0 || e.second < 0)
            continue;

        if (
            e.first >= points.size() ||
            e.second >= points.size()
            )
        {
            continue;
        }

        qreal disappear =
            1.0 - easeOut(currentProgress);

        QColor col(
            34,
            197,
            94,
            int(255 * disappear)
            );

        drawEdgeWithGlow(
            painter,
            toScreen(points[e.first].normalizedPos),
            toScreen(points[e.second].normalizedPos),
            col,
            disappear
            );
    }

    bool shouldCloseTour =
        (
            currentAlgorithmMode ==
            TSPAlgorithm::NEAREST_INSERTION
            )
        ||
        (
            currentTour.size() == points.size()
            );

    for (int i = 0; i < currentTour.size(); ++i)
    {
        int u = currentTour[i];

        int v;

        if (i == currentTour.size() - 1)
        {
            if (!shouldCloseTour)
                continue;

            v = currentTour[0];
        }
        else
        {
            v = currentTour[i + 1];
        }

        if (
            u >= points.size() ||
            v >= points.size()
            )
        {
            continue;
        }

        QPointF posA;

        QPointF posB;

        bool hasSwap =
            previousTour.size() == currentTour.size() &&
            previousTour != currentTour;

        if (hasSwap)
        {
            qreal t =
                easeOut(currentProgress);

            QPointF prevA =
                getTourNodePosition(i, true);

            QPointF newA =
                getTourNodePosition(i, false);

            QPointF prevB;

            int nextIndex =
                (i == currentTour.size() - 1)
                    ? 0
                    : i + 1;

            prevB =
                getTourNodePosition(nextIndex, true);

            QPointF newB =
                getTourNodePosition(nextIndex, false);

            posA =
                prevA + (newA - prevA) * t;

            posB =
                prevB + (newB - prevB) * t;
        }
        else
        {
            posA =
                points[u].normalizedPos;

            posB =
                points[v].normalizedPos;
        }

        QPointF a =
            toScreen(posA);

        QPointF b =
            toScreen(posB);

        QColor color =
            QColor("#8d97ad");

        if (hasEdge(currentTour, u, v))
        {
            color =
                QColor("#22c55e");
        }

        bool committed =
            committedEdges.contains(
                normEdge(u, v)
                );

        QColor animatedColor =
            color;

        bool completedTour =
            (
                currentStepAction ==
                StepAction::COMPLETE
                );

        if (committed)
        {
            qreal pulse =
                (
                    std::sin(
                        elapsedTimeMs * 0.0035
                        ) + 1.0
                    ) * 0.5;

            if (completedTour)
            {
                qreal time =
                    elapsedTimeMs * 0.0015;

                qreal edgeDelay =
                    i * 0.45;

                qreal local =
                    time - edgeDelay;

                local =
                    qBound(0.0, local, 1.0);

                qreal wave =
                    easeOut(local);

                int mainAlpha =
                    40 + wave * 215;

                animatedColor =
                    QColor(
                        80,
                        255,
                        140,
                        mainAlpha
                        );

                QColor glow =
                    animatedColor;

                int glowAlpha =
                    5 + wave * 80;

                glow.setAlpha(glowAlpha);

                QPen glowPen(glow);

                qreal glowWidth =
                    2.0 + wave * 6.0;

                glowPen.setWidthF(glowWidth);

                glowPen.setCapStyle(
                    Qt::RoundCap
                    );

                painter.setPen(glowPen);

                painter.drawLine(a, b);

                QColor midGlow =
                    animatedColor;

                midGlow.setAlpha(
                    15 + wave * 120
                    );

                QPen midPen(midGlow);

                qreal midWidth =
                    1.5 + wave * 2.5;

                midPen.setWidthF(midWidth);

                midPen.setCapStyle(
                    Qt::RoundCap
                    );

                painter.setPen(midPen);

                painter.drawLine(a, b);
            }
            else
            {
                int alpha =
                    140 + pulse * 95;

                animatedColor =
                    QColor(
                        80,
                        255,
                        140,
                        alpha
                        );

                QColor glow =
                    animatedColor;

                glow.setAlpha(35);

                QPen glowPen(glow);

                glowPen.setWidth(10);

                glowPen.setCapStyle(
                    Qt::RoundCap
                    );

                painter.setPen(glowPen);

                painter.drawLine(a, b);
            }
        }

        QPen pen(animatedColor);

        pen.setWidth(
            completedTour
                ? 2
                : committed
                      ? 3
                      : 2
            );

        pen.setCapStyle(Qt::RoundCap);

        painter.setPen(pen);

        painter.drawLine(a, b);

        QPair<int,int> key =
            normEdge(u, v);

        if (!seenEdges.contains(key))
        {
            seenEdges.insert(key);

            const QPointF& p1 =
                points[u].normalizedPos;

            const QPointF& p2 =
                points[v].normalizedPos;

            double dist =
                std::hypot(
                    p1.x() - p2.x(),
                    p1.y() - p2.y()
                    );

            edgeWeights.push_back(
                {a, b, dist}
                );
        }
    }

    for (const auto& [a, b, dist] : edgeWeights)
    {
        QPointF mid =
            (a + b) / 2.0;

        QPointF dir =
            b - a;

        double len =
            std::hypot(
                dir.x(),
                dir.y()
                );

        if (len > 0)
        {
            QPointF normal(
                -dir.y() / len,
                dir.x() / len
                );

            mid += normal * 10;
        }

        QFont font =
            painter.font();

        font.setPointSize(9);

        font.setBold(true);

        painter.setFont(font);

        QRectF textRect(
            mid.x() - 18,
            mid.y() - 10,
            36,
            20
            );

        painter.setBrush(
            QColor(15, 20, 30, 220)
            );

        painter.setPen(Qt::NoPen);

        painter.drawRoundedRect(
            textRect,
            6,
            6
            );

        painter.setPen(
            QColor(56,189,248,60)
            );

        painter.drawRoundedRect(
            textRect,
            6,
            6
            );

        painter.setPen(
            QColor("#e2e8f0")
            );

        painter.drawText(
            textRect,
            Qt::AlignCenter,
            QString::number(
                dist * 1000,
                'f',
                2
                )
            );
    }
}

void GraphCanvas::renderPoints(QPainter &painter)
{
    for (int i = 0; i < points.size(); ++i)
    {
        const GraphPoint &point = points[i];

        QColor color = point.color;

        bool isCurrent =
            (i == highlightedNode);

        bool isBest =
            (i == bestNode);

        QPointF center =
            toScreen(point.normalizedPos);

        QRectF rect =
            nodeRect(point.normalizedPos);

        if (isCurrent || isBest)
        {
            QColor glowColor =
                QColor(80, 255, 140);

            color =
                QColor(120, 255, 170);

            painter.setPen(Qt::NoPen);

            qreal pulse =
                qBound(0.0, currentProgress, 1.0);

            int maxRadius =
                28 * pulse;

            for (int r = maxRadius; r >= 10; r -= 2)
            {
                QColor layer = glowColor;

                int alpha =
                    22 - (maxRadius - r);

                alpha =
                    std::max(alpha, 0);

                layer.setAlpha(alpha);

                painter.setBrush(layer);

                painter.drawEllipse(
                    center,
                    r,
                    r
                    );
            }

            int midRadius =
                16 * pulse;

            for (int r = midRadius; r >= 8; --r)
            {
                QColor layer = glowColor;

                int alpha =
                    45 - (midRadius - r) * 4;

                alpha =
                    std::max(alpha, 0);

                layer.setAlpha(alpha);

                painter.setBrush(layer);

                painter.drawEllipse(
                    center,
                    r,
                    r
                    );
            }
        }

        painter.setBrush(color);

        QColor borderColor =
            (isCurrent || isBest)
                ? QColor(220,255,220)
                : QColor(220,220,220);

        painter.setPen(
            QPen(borderColor, 2)
            );

        painter.drawEllipse(rect);

        if (isCurrent || isBest)
        {
            QColor coreColor =
                isCurrent
                    ? QColor(220,240,255,180)
                    : QColor(220,255,240,180);

            painter.setPen(Qt::NoPen);

            painter.setBrush(coreColor);

            painter.drawEllipse(
                center,
                4,
                4
                );
        }

        painter.setPen(Qt::black);

        QFont font =
            painter.font();

        font.setPointSize(8);

        font.setBold(true);

        painter.setFont(font);

        QString text =
            QString::number(i);

        painter.drawText(
            rect,
            Qt::AlignCenter,
            text
            );
    }
}

void GraphCanvas::drawEdgeWithGlow(
    QPainter &painter,
    const QPointF &from,
    const QPointF &to,
    const QColor &color,
    qreal progress
    )
{
    qreal easedProgress =
        easeOut(progress);

    QPointF endPoint =
        from + (to - from) * easedProgress;

    QPen edgePen(color);

    edgePen.setWidth(3);

    edgePen.setCapStyle(Qt::RoundCap);

    edgePen.setJoinStyle(Qt::RoundJoin);

    painter.setPen(edgePen);

    painter.drawLine(from, endPoint);

    if (easedProgress > 0.05)
    {
        painter.setBrush(
            QColor(
                color.red(),
                color.green(),
                color.blue(),
                200
                )
            );

        painter.setPen(Qt::NoPen);

        qreal tipSize = 5.0;

        painter.drawEllipse(
            QPointF(
                endPoint.x() - tipSize / 2,
                endPoint.y() - tipSize / 2
                ),
            tipSize / 2,
            tipSize / 2
            );
    }
}

void GraphCanvas::generateSweepParticles(
    const QPointF &from,
    const QPointF &to,
    qreal progress
    )
{
    qreal easedProgress =
        easeOut(progress);

    QPointF sweepPos =
        from + (to - from) * easedProgress;

    QPointF direction =
        to - from;

    qreal length =
        std::sqrt(
            direction.x() * direction.x() +
            direction.y() * direction.y()
            );

    if (length > 0.0001)
    {
        direction.setX(
            direction.x() / length
            );

        direction.setY(
            direction.y() / length
            );
    }

    QPointF perpendicular(
        -direction.y(),
        direction.x()
        );

    int particleCount =
        2 + (
            QRandomGenerator::global()
                ->bounded(2)
            );

    for (int i = 0; i < particleCount; ++i)
    {
        Particle p;

        qreal offsetDist =
            (
                QRandomGenerator::global()
                    ->generateDouble() - 0.5
                ) * 12.0;

        qreal alongDist =
            (
                QRandomGenerator::global()
                    ->generateDouble() - 0.5
                ) * 6.0;

        p.pos =
            sweepPos +
            perpendicular * offsetDist +
            direction * alongDist;

        p.life = 1.0;

        p.color =
            QColor("#fbbf24");

        p.size =
            4.0 +
            QRandomGenerator::global()
                    ->generateDouble() * 3.0;

        activeParticles.append(p);
    }
}

void GraphCanvas::updateParticles(qreal deltaTime)
{
    const qreal particleDecayRate = 3.0;

    for (auto &particle : activeParticles)
    {
        particle.life -=
            deltaTime * particleDecayRate;
    }

    activeParticles.erase(
        std::remove_if(
            activeParticles.begin(),
            activeParticles.end(),
            [](const Particle &p)
            {
                return p.life <= 0.0;
            }
            ),
        activeParticles.end()
        );
}

qreal GraphCanvas::easeOut(qreal t) const
{
    if (t < 0.5)
    {
        return 4 * t * t * t;
    }
    else
    {
        qreal f =
            (2 * t - 2);

        return 0.5 * f * f * f + 1;
    }
}

QRectF GraphCanvas::nodeRect(const QPointF &normalized) const
{
    QPointF pos = toScreen(normalized);

    constexpr qreal radius = 12.5;

    return QRectF(
        pos.x() - radius,
        pos.y() - radius,
        radius * 2,
        radius * 2
        );
}

QColor GraphCanvas::getEdgeColor(
    bool isInTour,
    bool isHighlighted
    ) const
{
    if (isHighlighted)
    {
        switch (currentStepAction)
        {
        case StepAction::COMPARE_EDGE:
            return QColor("#fbbf24");

        case StepAction::UPDATE_BEST:

        case StepAction::INSERT_NODE:
            return QColor("#10b981");

        default:
            return QColor("#fbbf24");
        }
    }

    if (isInTour)
    {
        return QColor("#8d97ad");
    }

    return QColor("#5a626f");
}

int GraphCanvas::getEdgeWidth(bool isHighlighted) const
{
    return isHighlighted ? 3 : 2;
}

bool GraphCanvas::isAnimationFinished() const
{
    return currentProgress >= 0.999;
}

void GraphCanvas::setInputMode(InputMode mode)
{
    inputMode = mode;
}

InputMode GraphCanvas::getInputMode() const
{
    return inputMode;
}
