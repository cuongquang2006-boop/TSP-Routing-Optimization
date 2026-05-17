#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

#include "tsp_algorithms.h"

class AboutPopup : public QWidget
{
    Q_OBJECT

public:
    explicit AboutPopup(QWidget *parent = nullptr);

    void setAlgorithm(TSPAlgorithm alg);

    void toggle();

private:

    void rebuildContent();

    QLabel *titleLabel;

    QLabel *contentLabel;

    QVBoxLayout *mainLayout;

    TSPAlgorithm currentAlg =
        TSPAlgorithm::RANDOM;

    QGraphicsOpacityEffect *effect;

    QPropertyAnimation *opacityAnim;

};