#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QScrollArea>

#include "tsp_algorithms.h"

class AlgorithmComparePopup : public QWidget
{
    Q_OBJECT

public:
    explicit AlgorithmComparePopup(QWidget *parent = nullptr);

    void updateComparison(
        const QVector<QPointF>& points
        );

    void toggle();

private:

    QString formatOperationCount(double ops);

    QWidget* createAlgorithmCard(
        const QString& name,
        const QString& complexity,
        const QString& memory,
        const QString& estimatedOps,
        const QString& quality
        );

    QVBoxLayout *mainLayout;

    QLabel *titleLabel;

    QLabel *summaryLabel;

    QGraphicsOpacityEffect *effect;

    QPropertyAnimation *opacityAnim;

    QScrollArea *scrollArea;

    QWidget *contentWidget;

    QVBoxLayout *contentLayout;
};