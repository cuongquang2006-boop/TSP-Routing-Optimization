#include "dppopup.h"

#include <cmath>

DPPopup::DPPopup(QWidget *parent)
    : QWidget(parent)
{

    setFixedSize(300, 220);

    setWindowFlags(Qt::Widget);

    setAttribute(Qt::WA_StyledBackground);

    effect =
        new QGraphicsOpacityEffect(this);

    setGraphicsEffect(effect);

    opacityAnim =
        new QPropertyAnimation(
            effect,
            "opacity",
            this
            );

    setStyleSheet(R"(

        QWidget {

            background:
                rgba(20,23,30,0.88);

            border-radius:14px;

            border:
                1px solid
                rgba(56,189,248,0.14);
        }

        QLabel {

            background: transparent;

            border:none;

            color:#e2e8f0;

            font-size:12px;
        }

    )");


    mainLayout =
        new QVBoxLayout(this);

    mainLayout->setContentsMargins(
        14,
        14,
        14,
        14
        );

    mainLayout->setSpacing(10);

    titleLabel =
        new QLabel(
            "HELD-KARP DP",
            this
            );

    titleLabel->setStyleSheet(R"(

        color:#38bdf8;

        font-size:13px;

        font-weight:700;

        padding-bottom:4px;
    )");

    mainLayout->addWidget(titleLabel);

    maskLabel =
        new QLabel(
            "Mask : 1011",
            this
            );

    setLabel =
        new QLabel(
            "Set : {0,1,3}",
            this
            );

    mainLayout->addWidget(maskLabel);

    mainLayout->addWidget(setLabel);

    transitionLabel =
        new QLabel(
            "1 → 2",
            this
            );

    transitionLabel->setStyleSheet(R"(

        color:#fbbf24;

        font-size:15px;

        font-weight:700;
    )");

    oldCostLabel =
        new QLabel(
            "old : INF",
            this
            );

    newCostLabel =
        new QLabel(
            "new : 1432",
            this
            );

    newCostLabel->setStyleSheet(R"(

        color:#22c55e;

        font-weight:700;
    )");

    mainLayout->addSpacing(4);

    mainLayout->addWidget(transitionLabel);

    mainLayout->addWidget(oldCostLabel);

    mainLayout->addWidget(newCostLabel);


    bitmaskLabel =
        new QLabel(
            "[1][0][1][1]",
            this
            );

    bitmaskLabel->setStyleSheet(R"(

        color:#f8fafc;

        font-size:13px;

        font-weight:700;

        letter-spacing:2px;
    )");

    citiesLabel =
        new QLabel(
            "●0   ○1   ●2   ●3",
            this
            );

    citiesLabel->setStyleSheet(R"(

        color:#94a3b8;

        font-size:11px;
    )");

    mainLayout->addSpacing(4);

    mainLayout->addWidget(bitmaskLabel);

    mainLayout->addWidget(citiesLabel);

    mainLayout->addStretch();

    hide();
}


void DPPopup::toggle()
{
    opacityAnim->stop();

    if (isVisible())
    {
        opacityAnim->setDuration(160);

        opacityAnim->setStartValue(1.0);

        opacityAnim->setEndValue(0.0);

        connect(
            opacityAnim,
            &QPropertyAnimation::finished,
            this,
            [this]()
            {
                if (effect->opacity() <= 0.01)
                {
                    hide();
                }
            },
            Qt::SingleShotConnection
            );

        opacityAnim->start();
    }
    else
    {
        show();

        raise();

        effect->setOpacity(0.0);

        opacityAnim->setDuration(180);

        opacityAnim->setStartValue(0.0);

        opacityAnim->setEndValue(1.0);

        opacityAnim->start();
    }
}

QString DPPopup::maskToBinary(int mask)
{
    QString result;

    for (int i = 0; i < 8; ++i)
    {
        int bit =
            (mask >> i) & 1;

        result +=
            QString("[%1]").arg(bit);
    }

    return result;
}

QString DPPopup::maskToSet(int mask)
{
    QString result = "{";

    bool first = true;

    for (int i = 0; i < 8; ++i)
    {
        if (mask & (1 << i))
        {
            if (!first)
            {
                result += ",";
            }

            result += QString::number(i);

            first = false;
        }
    }

    result += "}";

    return result;
}

void DPPopup::updateForStep(
    const Step &step
    )
{
    //equal 2 node
    if (step.action !=
        StepAction::UPDATE_BEST)
    {
        return;
    }

    if (step.dpMask < 0)
    {
        return;
    }

    maskLabel->setText(
        QString(
            "Mask : %1"
            )
            .arg(step.dpMask)
        );

    setLabel->setText(
        QString(
            "Set : %1"
            )
            .arg(
                maskToSet(
                    step.dpMask
                    )
                )
        );

    transitionLabel->setText(
        QString(
            "%1 → %2"
            )
            .arg(step.dpFrom)
            .arg(step.dpTo)
        );


    QString oldCostText;

    if (std::isinf(step.dpOldCost))
    {
        oldCostText = "INF";
    }
    else
    {
        oldCostText =
            QString::number(
                step.dpOldCost,
                'f',
                0
                );
    }

    oldCostLabel->setText(
        QString(
            "old : %1"
            )
            .arg(oldCostText)
        );

    newCostLabel->setText(
        QString(
            "new : %1"
            )
            .arg(
                QString::number(
                    step.dpNewCost,
                    'f',
                    0
                    )
                )
        );


    bitmaskLabel->setText(
        maskToBinary(
            step.dpMask
            )
        );
    

    QString cityText;

    for (int i = 0; i < 8; ++i)
    {
        bool visited =
            step.dpMask & (1 << i);

        cityText +=
            visited
                ? QString("●%1  ").arg(i)
                : QString("○%1  ").arg(i);
    }

    citiesLabel->setText(cityText);
}
