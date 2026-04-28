#ifndef GRAPH_CANVAS_H
#define GRAPH_CANVAS_H

#include <QWidget>
#include <QColor>
#include <QPointF>
#include <QVector>
#include <QPair>

struct GraphPoint
{
    QPointF pos;
    QColor color;
    bool isHighlighted = false;
};

class GraphCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit GraphCanvas(QWidget *parent = nullptr);
    void setPoints(const QVector<GraphPoint> &points);
    void addPoint(const QPointF &pos);
    void generateRandomPoints(int count);
    void clearPoints();
    void setTour(const QVector<int> &tour, int currentNode = -1, int bestNode = -1, QPair<int,int> edge = {-1,-1});

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void renderEdges(QPainter &painter);
    void renderPoints(QPainter &painter);
    QRectF nodeRect(const QPointF &pos) const;

    QVector<GraphPoint> points;
    QVector<QPair<int, int>> edges;
    QVector<int> currentTour;
    int highlightedNode = -1;
    int bestNode = -1;
    QPair<int, int> highlightedEdge = {-1, -1};
};

#endif
