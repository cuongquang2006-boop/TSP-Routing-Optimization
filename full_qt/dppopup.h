#ifndef DPPOPUP_H
#define DPPOPUP_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

#include "tsp_steps.h"

class DPPopup : public QWidget
{
    Q_OBJECT

public:
    explicit DPPopup(QWidget *parent = nullptr);

    void updateForStep(const Step &step);

    void toggle();

private:

    QVBoxLayout *mainLayout;

    QLabel *titleLabel;

    QLabel *maskLabel;

    QLabel *setLabel;

    QLabel *transitionLabel;

    QLabel *oldCostLabel;

    QLabel *newCostLabel;

    QLabel *bitmaskLabel;

    QLabel *citiesLabel;

    QGraphicsOpacityEffect *effect;

    QPropertyAnimation *opacityAnim;

    QString maskToBinary(int mask);

    QString maskToSet(int mask);
};

#endif