#include "tutorialoverlay.h"

#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <QEvent>

TutorialOverlay::TutorialOverlay(QWidget *parent)
    : QWidget(parent)
{

    if (parent)
    {
        parent->installEventFilter(this);
    }


    setAttribute(
        Qt::WA_NoSystemBackground
        );

    setAttribute(
        Qt::WA_TranslucentBackground
        );

    setAttribute(Qt::WA_TransparentForMouseEvents, false);

    setWindowFlags(Qt::FramelessWindowHint);

    hide();

    // =========================
    // TOOLTIP CARD
    // =========================

    tooltipCard = new QWidget(this);

    tooltipCard->setFixedWidth(320);

    tooltipCard->setStyleSheet(R"(

        background:
            rgba(20,23,30,0.82);

        border-radius:14px;

        border:
            1px solid
            rgba(56,189,248,0.35);

    )");

    QVBoxLayout* layout =
        new QVBoxLayout(tooltipCard);

    layout->setContentsMargins(
        16,16,16,16
        );

    layout->setSpacing(12);

    titleLabel =
        new QLabel(tooltipCard);

    titleLabel->setStyleSheet(R"(

        color:#38bdf8;

        font-size:16px;

        font-weight:700;

    )");

    descLabel =
        new QLabel(tooltipCard);

    descLabel->setWordWrap(true);

    descLabel->setStyleSheet(R"(

        color:#dbe4ee;

        font-size:13px;

        line-height:1.5;

    )");

    layout->addWidget(titleLabel);

    layout->addWidget(descLabel);

    // =========================
    // BUTTONS
    // =========================

    QHBoxLayout* btnLayout =
        new QHBoxLayout;

    skipButton =
        new QPushButton("Skip");

    nextButton =
        new QPushButton("Next");

    skipButton->setCursor(
        Qt::PointingHandCursor
        );

    nextButton->setCursor(
        Qt::PointingHandCursor
        );

    skipButton->setStyleSheet(R"(

        QPushButton {

            background:
                rgba(51,65,85,0.7);

            color:#e2e8f0;

            border:none;

            border-radius:10px;

            padding:8px 14px;
        }

    )");

    nextButton->setStyleSheet(R"(

        QPushButton {

            background:
                rgba(56,189,248,0.18);

            color:#38bdf8;

            border:
                1px solid
                rgba(56,189,248,0.35);

            border-radius:10px;

            padding:8px 16px;

            font-weight:700;
        }

    )");

    btnLayout->addWidget(skipButton);

    btnLayout->addStretch();

    btnLayout->addWidget(nextButton);

    layout->addLayout(btnLayout);

    connect(
        nextButton,
        &QPushButton::clicked,
        this,
        &TutorialOverlay::nextStep
        );

    connect(
        skipButton,
        &QPushButton::clicked,
        this,
        &TutorialOverlay::skipTutorial
        );

    opacityEffect =
        new QGraphicsOpacityEffect(this);

    setGraphicsEffect(
        opacityEffect
        );


    fadeAnimation =
        new QPropertyAnimation(
            opacityEffect,
            "opacity",
            this
            );

    fadeAnimation->setDuration(250);

    spotlightAnimation =
        new QVariantAnimation(this);

    spotlightAnimation->setDuration(220);

    connect(
        spotlightAnimation,
        &QVariantAnimation::valueChanged,
        this,
        [this](const QVariant& value)
        {
            animatedTargetRect =
                value.toRect();

            update();
        }
        );
}

void TutorialOverlay::startTutorial(
    const QVector<TutorialStep>& steps
    )
{
    tutorialSteps = steps;

    currentStep = 0;

    resize(parentWidget()->size());

    show();

    raise();

    opacityEffect->setOpacity(0.0);

    fadeAnimation->stop();

    fadeAnimation->setStartValue(0.0);

    fadeAnimation->setEndValue(1.0);

    fadeAnimation->start();

    updateStepUI();
}

void TutorialOverlay::nextStep()
{
    currentStep++;

    if (currentStep >= tutorialSteps.size())
    {
        hide();

        return;
    }

    updateStepUI();

    repaint();
}

void TutorialOverlay::skipTutorial()
{
    hide();
}

void TutorialOverlay::updateStepUI()
{
    if (tutorialSteps.isEmpty())
        return;

    const auto& step =
        tutorialSteps[currentStep];

    currentTarget =
        step.target;

    if (!currentTarget)
    {
        tooltipCard->hide();

        update();

        return;
    }

    tooltipCard->show();

    titleLabel->setText(
        step.title
        );

    descLabel->setText(
        step.description
        );

    QRect r =
        targetRectGlobal();

    spotlightAnimation->stop();

    spotlightAnimation->setStartValue(
        animatedTargetRect
        );

    spotlightAnimation->setEndValue(r);

    spotlightAnimation->start();

    if (!r.isValid())
    {
        tooltipCard->hide();

        update();

        return;
    }

    tooltipCard->adjustSize();

    // =========================
    // DEFAULT POSITION
    // =========================

    int tooltipX =
        r.right() + 20;

    int tooltipY =
        r.center().y()
        - tooltipCard->height() / 2;

    // =========================
    // IF OVERFLOW RIGHT
    // -> MOVE LEFT SIDE
    // =========================

    if (
        tooltipX + tooltipCard->width()
        > width() - 20
        )
    {
        tooltipX =
            r.left()
            - tooltipCard->width()
            - 20;
    }

    // =========================
    // PREVENT BOTTOM OVERFLOW
    // =========================

    if (
        tooltipY + tooltipCard->height()
        > height() - 20
        )
    {
        tooltipY =
            height()
            - tooltipCard->height()
            - 20;
    }

    // =========================
    // PREVENT TOP OVERFLOW
    // =========================

    if (tooltipY < 20)
    {
        tooltipY = 20;
    }

    // =========================
    // APPLY POSITION
    // =========================

    tooltipCard->move(
        tooltipX,
        tooltipY
        );
}

QRect TutorialOverlay::targetRectGlobal() const
{
    if (
        tutorialSteps.isEmpty()
        )
    {
        return QRect();
    }

    const auto& step =
        tutorialSteps[currentStep];

    // =========================
    // CUSTOM RECT
    // =========================

    if (step.useCustomRect)
    {
        return step.customRect;
    }

    // =========================
    // NORMAL WIDGET TARGET
    // =========================

    if (!currentTarget)
        return QRect();

    QPoint globalTopLeft =
        currentTarget->mapToGlobal(
            QPoint(0,0)
            );

    QPoint localTopLeft =
        mapFromGlobal(
            globalTopLeft
            );

    return QRect(
        localTopLeft,
        currentTarget->size()
        );
}

void TutorialOverlay::paintEvent(
    QPaintEvent *event
    )
{
    Q_UNUSED(event)

    QPainter painter(this);

    painter.setRenderHint(
        QPainter::Antialiasing
        );

    // =========================
    // SOFT OVERLAY
    // =========================

    QColor overlayColor(
        0,
        0,
        0,
        35
        );

    painter.setBrush(
        overlayColor
        );

    painter.setPen(
        Qt::NoPen
        );

    painter.drawRect(rect());

    // =========================
    // TARGET
    // =========================

    QRect target =
        animatedTargetRect;

    if (!target.isValid())
        return;

    // =========================
    // GLOW
    // =========================

    QPen glowPen(
        QColor(56,189,248,220)
        );

    glowPen.setWidth(2);

    painter.setPen(glowPen);

    painter.setBrush(Qt::NoBrush);

    painter.drawRoundedRect(
        target.adjusted(
            -6,
            -6,
            6,
            6
            ),
        14,
        14
        );
}


void TutorialOverlay::updateTooltipPosition(
    const QRect& r
    )
{
    tooltipCard->adjustSize();

    int tooltipX =
        r.right() + 20;

    int tooltipY =
        r.center().y()
        - tooltipCard->height() / 2;

    // =========================
    // RIGHT OVERFLOW
    // =========================

    if (
        tooltipX + tooltipCard->width()
        > width() - 20
        )
    {
        tooltipX =
            r.left()
            - tooltipCard->width()
            - 20;
    }

    // =========================
    // BOTTOM OVERFLOW
    // =========================

    if (
        tooltipY + tooltipCard->height()
        > height() - 20
        )
    {
        tooltipY =
            height()
            - tooltipCard->height()
            - 20;
    }

    // =========================
    // TOP OVERFLOW
    // =========================

    if (tooltipY < 20)
    {
        tooltipY = 20;
    }

    tooltipCard->move(
        tooltipX,
        tooltipY
        );
}

bool TutorialOverlay::eventFilter(
    QObject *obj,
    QEvent *event
    )
{
    // =========================
    // ONLY TRACK PARENT WINDOW
    // =========================

    if (obj != parentWidget())
    {
        return QWidget::eventFilter(
            obj,
            event
            );
    }

    // =========================
    // HANDLE RESIZE
    // =========================

    if (
        event->type()
        != QEvent::Resize
        )
    {
        return QWidget::eventFilter(
            obj,
            event
            );
    }

    // =========================
    // UPDATE OVERLAY SIZE
    // =========================

    resize(
        parentWidget()->size()
        );

    // =========================
    // NO ACTIVE STEP
    // =========================

    if (
        tutorialSteps.isEmpty()
        ||
        currentStep >= tutorialSteps.size()
        )
    {
        return QWidget::eventFilter(
            obj,
            event
            );
    }

    // =========================
    // UPDATE TARGET RECT
    // =========================

    QRect targetRect =
        targetRectGlobal();

    animatedTargetRect =
        targetRect;

    // =========================
    // UPDATE TOOLTIP POSITION
    // =========================

    updateTooltipPosition(
        targetRect
        );

    update();

    return QWidget::eventFilter(
        obj,
        event
        );
}
