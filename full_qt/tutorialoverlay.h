#pragma once

#include <QWidget>
#include <QVector>
#include <QPushButton>
#include <QLabel>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QVariantAnimation>

struct TutorialStep
{
    QWidget* target = nullptr;

    QRect customRect;

    bool useCustomRect = false;

    QString title;

    QString description;
};

class TutorialOverlay : public QWidget
{
    Q_OBJECT

public:

    explicit TutorialOverlay(QWidget *parent = nullptr);

    void startTutorial(
        const QVector<TutorialStep>& steps
        );

protected:

    void paintEvent(QPaintEvent *event) override;

private slots:

    void nextStep();

    void skipTutorial();

protected:

    bool eventFilter(
        QObject *obj,
        QEvent *event
        ) override;

private:
    void updateTooltipPosition(
        const QRect& targetRect
        );

    void updateStepUI();

    QRect targetRectGlobal() const;

    QVector<TutorialStep> tutorialSteps;

    int currentStep = 0;

    QWidget* currentTarget = nullptr;

    QWidget* tooltipCard;

    QLabel* titleLabel;

    QLabel* descLabel;

    QPushButton* nextButton;

    QPushButton* skipButton;

    QRect animatedTargetRect;

    QVariantAnimation* spotlightAnimation;

    QGraphicsOpacityEffect * opacityEffect;
    QPropertyAnimation * fadeAnimation;
};