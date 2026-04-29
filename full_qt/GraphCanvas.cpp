#include "GraphCanvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QTimer>
#include <cmath>
#include <QTime>
#include <algorithm>

GraphCanvas::GraphCanvas(QWidget *parent)  : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
    setMinimumSize(600, 400);

    particleUpdateTimer = new QTimer(this);
    connect(particleUpdateTimer, &QTimer::timeout, this, &GraphCanvas::updateAnimation);
    lastFrameTime = QTime::currentTime().msecsSinceStartOfDay();
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

void GraphCanvas::addPoint(const QPointF &pos)
{
    if (points.size() >= 30)
        return;

    GraphPoint point;
    point.pos = pos;
    point.color = (points.size() % 2 == 0) ? QColor("#7dd3fc") : QColor("#ffffff");
    points.append(point);

    if (points.size() > 1) {
        edges.append(qMakePair(points.size() - 2, points.size() - 1));
    }

    update();
}

void GraphCanvas::generateRandomPoints(int count)
{
    points.clear();
    edges.clear();
    count = qMin(count, 30);

    const QRect area = contentsRect().adjusted(12, 12, -12, -12);
    if (area.width() <= 0 || area.height() <= 0)
        return;

    for (int i = 0; i < count; ++i) 
    {
        qreal x = area.left() + QRandomGenerator::global()->bounded(area.width());
        qreal y = area.top() + QRandomGenerator::global()->bounded(area.height());
        GraphPoint point;
        point.pos = QPointF(x, y);
        point.color = (i % 2 == 0) ? QColor("#7dd3fc") : QColor("#ffffff");
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
    highlightedNode = -1;
    bestNode = -1;
    highlightedEdge = qMakePair(-1, -1);
    activeParticles.clear();
    currentProgress = 0.0;
    elapsedTimeMs = 0.0;
    if (particleUpdateTimer->isActive()) 
    {
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

    if (currentStepAction == StepAction::INSERT_NODE) 
    {
        for (int i = 0; i < currentTour.size(); ++i) 
        {
            int j = (i + 1) % currentTour.size();
            committedEdges.insert(normEdge(currentTour[i], currentTour[j]));
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

    highlightedNode = step.currentNode;
    bestNode = step.bestCandidate;
    highlightedEdge = step.edge;

    step_candidate_edges = step.candidate_edges;
    step_selected_edges = step.selected_edges;
    step_rejected_edges = step.rejected_edges;

    if (step.action == StepAction::INSERT_NODE || step.action == StepAction::COMPLETE) 
    {
        for (int i = 0; i < currentTour.size(); ++i) 
        {
            int j = (i + 1) % currentTour.size();
            committedEdges.insert(normEdge(currentTour[i], currentTour[j]));
        }
    }

    currentStepAction = step.action;
    currentProgress = 0.0;
    elapsedTimeMs = 0.0;

    if (!particleUpdateTimer->isActive()) 
    {
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

    if (elapsedTimeMs >= adjustedDuration) {
        elapsedTimeMs = adjustedDuration;
        currentProgress = 1.0;
        particleUpdateTimer->stop();
    } else {
        currentProgress = elapsedTimeMs / adjustedDuration;
    }

    update();
}

bool hasEdge(const QVector<int>& tour, int u, int v)
{
    for (int i = 0; i < tour.size(); ++i) {
        int j = (i + 1) % tour.size();
        int a = tour[i];
        int b = tour[j];

        if ((a == u && b == v) || (a == v && b == u))
            return true;
    }
    return false;
}

void GraphCanvas::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor("#171a20"));

    renderEdges(painter);
    renderPoints(painter);
}

void GraphCanvas::mousePressEvent(QMouseEvent *event)
{
    if (particleUpdateTimer && particleUpdateTimer->isActive()) 
    {
        return;
    }

    if (event->button() != Qt::LeftButton) 
    {
        QWidget::mousePressEvent(event);
        return;
    }

    const QPointF position = event->position();
    if (!contentsRect().contains(position.toPoint()))
        return;

    addPoint(position);
    event->accept();
}

void GraphCanvas::renderEdges(QPainter &painter)
{
    if (edges.isEmpty() && currentTour.isEmpty() && step_candidate_edges.isEmpty() && step_selected_edges.isEmpty() && step_rejected_edges.isEmpty())
        return;

    for (const auto &edge : qAsConst(edges)) {
        if (edge.first < points.size() && edge.second < points.size()) 
        {
            const GraphPoint &a = points[edge.first];
            const GraphPoint &b = points[edge.second];

            QPen edgePen(QColor("#2b3340"));
            edgePen.setWidth(1);
            painter.setPen(edgePen);
            painter.drawLine(a.pos, b.pos);
        }
    }

    for (const auto &e : qAsConst(step_rejected_edges)) 
    {
        if (e.first < 0 || e.second < 0) continue;
        if (e.first >= points.size() || e.second >= points.size()) continue;
        const QPointF &a = points[e.first].pos;
        const QPointF &b = points[e.second].pos;

        QPen pen(QColor(239,68,68,90)); 
        pen.setWidth(2);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        painter.drawLine(a, b);
    }

    for (const auto &e : qAsConst(step_candidate_edges)) 
    {
        if (e.first < 0 || e.second < 0) continue;
        if (e.first >= points.size() || e.second >= points.size()) continue;
        const QPointF &a = points[e.first].pos;
        const QPointF &b = points[e.second].pos;

        QColor col = QColor("#fbbf24");
        drawEdgeWithGlow(painter, a, b, col, currentProgress);
    }

    for (const auto &e : qAsConst(step_selected_edges)) 
    {
        if (e.first < 0 || e.second < 0) continue;
        if (e.first >= points.size() || e.second >= points.size()) continue;
        const QPointF &a = points[e.first].pos;
        const QPointF &b = points[e.second].pos;

        QPen pen(QColor("#22c55e")); 
        pen.setWidth(4);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        painter.drawLine(a, b);
    }

    for (int i = 0; i < currentTour.size(); ++i) 
    {
        int j = (i + 1) % currentTour.size();

        int u = currentTour[i];
        int v = currentTour[j];

        if (u >= points.size() || v >= points.size())
            continue;

        QPair<int,int> norm = normEdge(u,v);
        bool drawnAsSelected = false;
        for (const auto &se : qAsConst(step_selected_edges)) 
        {
            if (normEdge(se.first, se.second) == norm) 
            {
                drawnAsSelected = true; break;
            }
        }
        if (drawnAsSelected) continue;

        const GraphPoint &a = points[u];
        const GraphPoint &b = points[v];

        QPair<int,int> tourEdge = {u, v};
        QPair<int,int> e = norm;

        bool existedBefore = hasEdge(previousTour, u, v);
        bool isCommitted = committedEdges.contains(e);
        bool isHighlighted = (tourEdge == highlightedEdge);
        bool isCompareEdge =
            (currentStepAction == StepAction::COMPARE_EDGE &&
             (tourEdge == highlightedEdge ||
              QPair<int,int>(tourEdge.second, tourEdge.first) == highlightedEdge));

        if (!existedBefore) 
        {

            QColor edgeColor = getEdgeColor(true, true);
            drawEdgeWithGlow(painter, a.pos, b.pos, edgeColor, currentProgress);

            if (currentProgress >= 0.99 && isCommitted) 
            {
                QPen pen(QColor("#22c55e"));
                pen.setWidth(4);
                pen.setCapStyle(Qt::RoundCap);

                painter.setPen(pen);
                painter.drawLine(a.pos, b.pos);
            }
        }

        else if (isCommitted) 
        {
            QPen pen(QColor("#22c55e"));
            pen.setWidth(4);
            pen.setCapStyle(Qt::RoundCap);

            painter.setPen(pen);
            painter.drawLine(a.pos, b.pos);
        }

        else
        {
            if (isCompareEdge) 
            {
                QColor edgeColor = QColor("#fbbf24"); 

                QPen glow(QColor(251,191,36,80));
                glow.setWidth(9);
                glow.setCapStyle(Qt::RoundCap);
                painter.setPen(glow);
                painter.drawLine(a.pos, b.pos);

                QPen pen(edgeColor);
                pen.setWidth(5);
                pen.setCapStyle(Qt::RoundCap);
                painter.setPen(pen);
                painter.drawLine(a.pos, b.pos);

                return; 
            }

            QColor edgeColor = getEdgeColor(true, isHighlighted);
            int edgeWidth = getEdgeWidth(isHighlighted);

            QPen edgePen(edgeColor);
            edgePen.setWidth(edgeWidth);
            edgePen.setCapStyle(Qt::RoundCap);

            painter.setPen(edgePen);
            painter.drawLine(a.pos, b.pos);
        }
    }
}

void GraphCanvas::renderPoints(QPainter &painter)
{
    constexpr int radius = 7;

    for (int i = 0; i < points.size(); ++i) 
    {
        const GraphPoint &point = points[i];
        QColor color = point.color;

        if (i == highlightedNode) 
        {
            color = QColor("#3b82f6");

            painter.setBrush(QColor(59, 130, 246, 80)); 
            painter.setPen(Qt::NoPen);

            QRectF glowRect = nodeRect(point.pos).adjusted(-8, -8, 8, 8);
            painter.drawEllipse(glowRect);
        }
        else if (i == bestNode) {
            color = QColor("#10b981");
        }

        painter.setBrush(color);
        painter.setPen(QPen(Qt::white, 2));
        painter.drawEllipse(nodeRect(point.pos));

        painter.setPen(Qt::black);

        QFont font = painter.font();
        font.setPointSize(8);
        font.setBold(true);
        painter.setFont(font);

        QString text = QString::number(i);

        QRectF rect = nodeRect(point.pos);
        painter.drawText(rect, Qt::AlignCenter, text);
    }
}

void GraphCanvas::drawEdgeWithGlow(QPainter &painter, const QPointF &from, const QPointF &to,
                                  const QColor &color, qreal progress)
{
    qreal easedProgress = easeOut(progress);

    QPointF endPoint = from + (to - from) * easedProgress;

    QPen edgePen(color);
    edgePen.setWidth(3);
    edgePen.setCapStyle(Qt::RoundCap);
    edgePen.setJoinStyle(Qt::RoundJoin);

    painter.setPen(edgePen);
    painter.drawLine(from, endPoint);

    if (easedProgress > 0.05) 
    {
        painter.setBrush(QColor(color.red(), color.green(), color.blue(), 200));
        painter.setPen(Qt::NoPen);

        qreal tipSize = 5.0;
        painter.drawEllipse(QPointF(endPoint.x() - tipSize/2, endPoint.y() - tipSize/2),
                           tipSize/2, tipSize/2);
    }
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
        p.color = QColor("#fbbf24"); // Yellow glow color
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

qreal GraphCanvas::easeOut(qreal t) const
{
    if (t < 0.5) {
        return 4 * t * t * t;
    } else {
        qreal f = (2 * t - 2);
        return 0.5 * f * f * f + 1;
    }
}

QRectF GraphCanvas::nodeRect(const QPointF &pos) const
{
    constexpr qreal radius = 7.0;
    return QRectF(pos.x() - radius, pos.y() - radius, radius * 2, radius * 2);
}

QColor GraphCanvas::getEdgeColor(bool isInTour, bool isHighlighted) const
{
    if (isHighlighted) 
    {
        switch (currentStepAction) {
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
