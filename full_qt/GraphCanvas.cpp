#include "GraphCanvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QRandomGenerator>

GraphCanvas::GraphCanvas(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
    setMinimumSize(600, 400);
}

void GraphCanvas::setPoints(const QVector<GraphPoint> &newPoints)
{
    points = newPoints;
    edges.clear();
    for (int i = 1; i < points.size(); ++i) {
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

    for (int i = 0; i < count; ++i) {
        qreal x = area.left() + QRandomGenerator::global()->bounded(area.width());
        qreal y = area.top() + QRandomGenerator::global()->bounded(area.height());
        GraphPoint point;
        point.pos = QPointF(x, y);
        point.color = (i % 2 == 0) ? QColor("#7dd3fc") : QColor("#ffffff");
        points.append(point);
        if (i > 0) {
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
    update();
}

void GraphCanvas::setTour(const QVector<int> &tour, int currentNode, int bestNode, QPair<int,int> edge)
{
    currentTour = tour;
    highlightedNode = currentNode;
    this->bestNode = bestNode;
    highlightedEdge = edge;
    update();
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

void GraphCanvas::renderEdges(QPainter &painter)
{
    if (edges.isEmpty() && currentTour.isEmpty())
        return;

    QPen defaultPen(QColor("#8d97ad"));
    defaultPen.setWidth(2);
    QPen highlightPen(QColor("#fbbf24"));
    highlightPen.setWidth(3);
    QPen bestPen(QColor("#ef4444"));
    bestPen.setWidth(3);

    painter.setPen(defaultPen);
    for (const auto &edge : qAsConst(edges)) {
        const GraphPoint &a = points[edge.first];
        const GraphPoint &b = points[edge.second];
        painter.drawLine(a.pos, b.pos);
    }

    // Draw tour edges with highlights
    for (int i = 0; i < currentTour.size(); ++i) {
        int j = (i + 1) % currentTour.size();
        const GraphPoint &a = points[currentTour[i]];
        const GraphPoint &b = points[currentTour[j]];

        QPair<int,int> tourEdge = {currentTour[i], currentTour[j]};
        if (tourEdge == highlightedEdge) {
            painter.setPen(highlightPen);
        } else {
            painter.setPen(defaultPen);
        }
        painter.drawLine(a.pos, b.pos);
    }
}

void GraphCanvas::renderPoints(QPainter &painter)
{
    constexpr int radius = 7;
    for (int i = 0; i < points.size(); ++i) {
        const GraphPoint &point = points[i];
        QColor color = point.color;

        // Highlight logic
        if (i == highlightedNode) {
            color = QColor("#3b82f6");
        } else if (i == bestNode) {
            color = QColor("#10b981");
        }

        painter.setBrush(color);
        painter.setPen(QPen(Qt::white, 2));
        painter.drawEllipse(nodeRect(point.pos));
    }
}

QRectF GraphCanvas::nodeRect(const QPointF &pos) const
{
    constexpr qreal radius = 7.0;
    return QRectF(pos.x() - radius, pos.y() - radius, radius * 2, radius * 2);
}
