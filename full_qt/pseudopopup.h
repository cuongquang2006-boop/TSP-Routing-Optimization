#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QVector>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QScrollArea>
#include "tsp_algorithms.h"
#include "tsp_steps.h"

struct PseudoLine
{
    QLabel *label;
};

class PseudoPopup : public QWidget
{
    Q_OBJECT
public:
    explicit PseudoPopup(QWidget *parent = nullptr);

    void setAlgorithm(TSPAlgorithm alg);
    void updateForStep(const Step &step);
    void toggle();

private:
    void buildPseudoForAlg();
    void clearLines();
    void highlightLine(int index);

    QVBoxLayout *mainLayout;
    QLabel *titleLabel;
    QLabel *compareLabel;

    QScrollArea *scrollArea;
    QWidget *scrollContent;
    QVBoxLayout *codeLayout;

    QVector<PseudoLine> codeLines;

    TSPAlgorithm currentAlg = TSPAlgorithm::RANDOM;

    QPropertyAnimation *opacityAnim;
    QGraphicsOpacityEffect *effect;
};