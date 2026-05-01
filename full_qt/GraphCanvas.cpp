#include "GraphCanvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QTimer>
#include <QTime>
#include <cmath>

GraphCanvas::GraphCanvas(QWidget *parent)
    : QWidget(parent)
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
    if (particleUpdateTimer) {
        particleUpdateTimer->stop();
    }
}

QPointF GraphCanvas::toScreen(const QPointF& normalized) const
{
    QRect area = contentsRect().adjusted(12, 12, -12, -12);

    qreal x = area.left() + normalized.x() * area.width();
    qreal y = area.top() + normalized.y() * area.height();

    return QPointF(x, y);
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

static QPair<int,int> normEdge(int u, int v)
{
    return (u < v) ? qMakePair(u, v) : qMakePair(v, u);
}

void GraphCanvas::renderEdges(QPainter &painter)
{
    QSet<QPair<int,int>> seenEdges;

    if (edges.isEmpty() && currentTour.isEmpty() &&
        step_candidate_edges.isEmpty() &&
        step_selected_edges.isEmpty() &&
        step_rejected_edges.isEmpty())
        return;

    QVector<std::tuple<QPointF, QPointF, double>> edgeWeights;

    for (const auto &edge : qAsConst(edges)) {
        if (edge.first < points.size() && edge.second < points.size()) {

            QPointF a = toScreen(points[edge.first].normalizedPos);
            QPointF b = toScreen(points[edge.second].normalizedPos);

            QPen edgePen(QColor("#2b3340"));
            edgePen.setWidth(1);
            painter.setPen(edgePen);
            painter.drawLine(a, b);

            QPair<int,int> key = normEdge(edge.first, edge.second);
            if (!seenEdges.contains(key)) {
                seenEdges.insert(key);

                int u = edge.first;
                int v = edge.second;

                const QPointF& p1 = points[u].normalizedPos;
                const QPointF& p2 = points[v].normalizedPos;

                double dist = std::hypot(p1.x() - p2.x(), p1.y() - p2.y());

                edgeWeights.push_back({a, b, dist});
            }
        }
    }

    for (const auto &e : qAsConst(step_rejected_edges)) {
        if (e.first < 0 || e.second < 0) continue;
        if (e.first >= points.size() || e.second >= points.size()) continue;

        QPen pen(QColor(239,68,68,90));
        pen.setWidth(2);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);

        painter.drawLine(
            toScreen(points[e.first].normalizedPos),
            toScreen(points[e.second].normalizedPos)
            );
    }

    for (const auto &e : qAsConst(step_candidate_edges)) {

        if (step_selected_edges.contains(e))
            continue;

        drawEdgeWithGlow(
            painter,
            toScreen(points[e.first].normalizedPos),
            toScreen(points[e.second].normalizedPos),
            QColor("#fbbf24"),
            currentProgress
            );
    }

    for (const auto &e : qAsConst(step_selected_edges)) {
        if (e.first < 0 || e.second < 0) continue;
        if (e.first >= points.size() || e.second >= points.size()) continue;

        QColor col = QColor("#22c55e");

        drawEdgeWithGlow(
            painter,
            toScreen(points[e.first].normalizedPos),
            toScreen(points[e.second].normalizedPos),
            col,
            currentProgress
            );
    }

    bool isComplete = (currentTour.size() == points.size());

    for (int i = 0; i < currentTour.size(); ++i) {

        int u = currentTour[i];
        int v;

        if (i == currentTour.size() - 1) {
            if (!isComplete) continue;
            v = currentTour[0];
        } else {
            v = currentTour[i + 1];
        }

        if (u >= points.size() || v >= points.size())
            continue;

        QPointF a = toScreen(points[u].normalizedPos);
        QPointF b = toScreen(points[v].normalizedPos);

        QColor color = QColor("#8d97ad");

        if (hasEdge(currentTour, u, v)) {
            color = QColor("#22c55e");
        }

        QPen pen(color);
        pen.setWidth(committedEdges.contains(normEdge(u, v)) ? 4 : 2);
        pen.setCapStyle(Qt::RoundCap);

        painter.setPen(pen);
        painter.drawLine(a, b);

        QPair<int,int> key = normEdge(u, v);
        if (!seenEdges.contains(key)) {
            seenEdges.insert(key);

            const QPointF& p1 = points[u].normalizedPos;
            const QPointF& p2 = points[v].normalizedPos;

            double dist = std::hypot(p1.x() - p2.x(), p1.y() - p2.y());
            edgeWeights.push_back({a, b, dist});
        }
    }

    for (const auto& [a, b, dist] : edgeWeights)
    {
        QPointF mid = (a + b) / 2.0;

        QPointF dir = b - a;
        double len = std::hypot(dir.x(), dir.y());
        if (len > 0) {
            QPointF normal(-dir.y()/len, dir.x()/len);
            mid += normal * 10;
        }

        QFont font = painter.font();
        font.setPointSize(9);
        font.setBold(true);
        painter.setFont(font);

        QRectF textRect(mid.x() - 18, mid.y() - 10, 36, 20);

        painter.setBrush(QColor(15, 20, 30, 220));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(textRect, 6, 6);

        painter.setPen(QColor(56,189,248,60));
        painter.drawRoundedRect(textRect, 6, 6);

        painter.setPen(QColor("#e2e8f0"));
        painter.drawText(textRect, Qt::AlignCenter,
            QString::number(dist * 1000, 'f', 2));
    }
}


void GraphCanvas::renderPoints(QPainter &painter)
{
    for (int i = 0; i < points.size(); ++i) {

        const GraphPoint &point = points[i];
        QColor color = point.color;

        if (i == highlightedNode) {
            color = QColor("#3b82f6");

            painter.setBrush(QColor(59, 130, 246, 80));
            painter.setPen(Qt::NoPen);

            QRectF glowRect = nodeRect(point.normalizedPos).adjusted(-8, -8, 8, 8);
            painter.drawEllipse(glowRect);
        }
        else if (i == bestNode) {
            color = QColor("#10b981");
        }

        painter.setBrush(color);
        painter.setPen(QPen(Qt::white, 2));
        painter.drawEllipse(nodeRect(point.normalizedPos));

        painter.setPen(Qt::black);

        QFont font = painter.font();
        font.setPointSize(8);
        font.setBold(true);
        painter.setFont(font);

        QString text = QString::number(i);

        QRectF rect = nodeRect(point.normalizedPos);
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

    if (easedProgress > 0.05) {
        painter.setBrush(QColor(color.red(), color.green(), color.blue(), 200));
        painter.setPen(Qt::NoPen);

        qreal tipSize = 5.0;
        painter.drawEllipse(QPointF(endPoint.x() - tipSize/2, endPoint.y() - tipSize/2),
                           tipSize/2, tipSize/2);
    }
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

QColor GraphCanvas::getEdgeColor(bool isInTour, bool isHighlighted) const
{
    if (isHighlighted) {
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

    if (isInTour) {
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