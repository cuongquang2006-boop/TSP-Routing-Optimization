#ifndef GRAPH_CANVAS_H
#define GRAPH_CANVAS_H

#include <QWidget>
#include <QColor>
#include <QPointF>
#include <QVector>
#include <QPair>
#include <QTimer>
#include "tsp_steps.h"
#include "tsp_algorithms.h"

struct GraphPoint
{
    QPointF normalizedPos;
    QColor color;
    bool isHighlighted = false;
};

struct Particle
{
    QPointF pos;
    qreal life;
    QColor color;
    qreal size;
};

struct AnimationFrame
{
    qreal progress;
    int currentNode;
    int bestNode;
    QPair<int, int> edge;
    QVector<Particle> particles;
};

enum class InputMode
{
    NONE,
    CLICK,
    MANUAL
};

class GraphCanvas : public QWidget
{
    Q_OBJECT

signals:
    void pointAdded(QPointF normalizedPos);

public:
    void setCurrentAlgorithm(TSPAlgorithm algo);

    QPointF getAnimatedNodePosition();

    bool isAnimationFinished() const;

    explicit GraphCanvas(QWidget *parent = nullptr);
    ~GraphCanvas();

    void setPoints(const QVector<GraphPoint> &points);

    void addPoint(const QPointF &pos);

    void generateRandomPoints(int count);

    void clearPoints();

    void setTour(
        const QVector<int> &tour,
        int currentNode = -1,
        int bestNode = -1,
        QPair<int,int> edge = {-1,-1}
        );

    void setStepAction(StepAction action);

    void setStep(const Step &step);

    void setAnimationProgress(qreal progress);

    void setAnimationParams(int durationMs, qreal speedMultiplier);

    void updateAnimation();

    void setInputMode(InputMode mode);

    void setMaxNodeLimit(int limit);

    InputMode getInputMode() const;

    void recomputeLayout();

    QVector<QPointF> getPoints() const
    {
        QVector<QPointF> result;

        for (const auto& p : points)
            result.append(p.normalizedPos);

        return result;
    }

protected:
    void paintEvent(QPaintEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

private:

    TSPAlgorithm currentAlgorithmMode = TSPAlgorithm::RANDOM;

    QPointF getTourNodePosition(
        int tourIndex,
        bool usePrevious
        ) const;

    QTimer glowTimer;

    int maxNodeLimit = 10;

    InputMode inputMode = InputMode::NONE;

    QSet<QPair<int,int>> committedEdges;

    QVector<int> previousTour;

    void renderEdges(QPainter &painter);

    void renderPoints(QPainter &painter);

    void drawEdgeWithGlow(
        QPainter &painter,
        const QPointF &from,
        const QPointF &to,
        const QColor &color,
        qreal progress
        );

    QColor getEdgeColor(
        bool isInTour,
        bool isHighlighted
        ) const;

    int getEdgeWidth(bool isHighlighted) const;

    void generateSweepParticles(
        const QPointF &from,
        const QPointF &to,
        qreal progress
        );

    void updateParticles(qreal deltaTime);

    qreal easeOut(qreal t) const;

    QRectF nodeRect(const QPointF &pos) const;

    QPointF toScreen(const QPointF& normalized) const;

    QVector<GraphPoint> points;

    QVector<QPair<int, int>> edges;

    QVector<int> currentTour;

    int highlightedNode = -1;

    int bestNode = -1;

    QPair<int, int> highlightedEdge = {-1, -1};

    QVector<QPair<int,int>> step_candidate_edges;

    QVector<QPair<int,int>> step_selected_edges;

    QVector<QPair<int,int>> step_rejected_edges;

    QVector<QPair<int,int>> previous_candidate_edges;

    QVector<QPair<int,int>> previous_selected_edges;

    QVector<QPair<int,int>> previous_rejected_edges;

    QVector<Particle> activeParticles;

    qreal currentProgress = 0.0;

    int animationDurationMs = 400;

    qreal speedMultiplier = 1.0;

    qreal elapsedTimeMs = 0.0;

    QTimer *particleUpdateTimer = nullptr;

    qreal lastFrameTime = 0.0;

    StepAction currentStepAction = StepAction::SELECT_NODE;

    double currentTemperature = -1.0;

    double currentBestCost = -1.0;

    bool showTemperatureHUD = false;
};

#endif