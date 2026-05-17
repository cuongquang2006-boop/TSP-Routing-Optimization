#include "algorithmcomparepopup.h"
#include <QScrollArea>
#include <QFrame>
#include <cmath>


AlgorithmComparePopup::AlgorithmComparePopup(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(540, 620);

    setWindowFlags(Qt::Widget);

    setAttribute(Qt::WA_StyledBackground);

    setStyleSheet(R"(

        QWidget {

            background:
                rgba(15,18,24,0.96);

            border-radius:14px;

            border:
                1px solid
                rgba(56,189,248,0.25);
        }

        QLabel {

            background:transparent;

            color:#e2e8f0;
        }

    )");


    effect =
        new QGraphicsOpacityEffect(this);

    setGraphicsEffect(effect);

    opacityAnim =
        new QPropertyAnimation(
            effect,
            "opacity",
            this
            );

    opacityAnim->setDuration(180);


    mainLayout =
        new QVBoxLayout(this);

    mainLayout->setContentsMargins(
        16,
        16,
        16,
        16
        );

    mainLayout->setSpacing(12);

    titleLabel =
        new QLabel(
            "COMPARE ALL ALGORITHMS",
            this
            );

    titleLabel->setStyleSheet(R"(

        color:#38bdf8;

        font-size:18px;

        font-weight:700;

        letter-spacing:1px;

        padding:8px 12px;

        background:
            rgba(56,189,248,0.08);

        border-radius:10px;

    )");

    mainLayout->addWidget(titleLabel);


    summaryLabel =
        new QLabel(
            "Realtime complexity comparison based on current node count.",
            this
            );

    summaryLabel->setWordWrap(true);

    summaryLabel->setStyleSheet(R"(

        color:#94a3b8;

        font-size:12px;

        padding-left:4px;
    )");

    mainLayout->addWidget(summaryLabel);


    scrollArea =
        new QScrollArea(this);

    scrollArea->setWidgetResizable(true);

    scrollArea->setFrameShape(QFrame::NoFrame);

    scrollArea->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff
        );

    scrollArea->setStyleSheet(R"(

        QScrollArea {

            border:none;

            background:transparent;
        }

        QScrollBar:vertical {

            width:8px;

            background:transparent;
        }

        QScrollBar::handle:vertical {

            background:
                rgba(56,189,248,0.35);

            border-radius:4px;
        }

    )");

    contentWidget =
        new QWidget;

    contentLayout =
        new QVBoxLayout(contentWidget);

    contentLayout->setSpacing(12);

    contentLayout->setContentsMargins(
        4,
        4,
        4,
        4
        );


    contentLayout->addStretch();

    scrollArea->setWidget(contentWidget);

    mainLayout->addWidget(scrollArea);

    hide();
}

QWidget* AlgorithmComparePopup::createAlgorithmCard(
    const QString& name,
    const QString& complexity,
    const QString& memory,
    const QString& estimatedOps,
    const QString& quality
    )
{
    QWidget *card =
        new QWidget;

    card->setStyleSheet(R"(

        QWidget {

            background:
                rgba(30,41,59,0.55);

            border-radius:12px;

            border:
                1px solid
                rgba(148,163,184,0.12);
        }

    )");

    QVBoxLayout *layout =
        new QVBoxLayout(card);

    layout->setContentsMargins(
        14,
        12,
        14,
        12
        );

    layout->setSpacing(8);

    QLabel *nameLabel =
        new QLabel(name);

    nameLabel->setStyleSheet(R"(

        color:#f8fafc;

        font-size:15px;

        font-weight:700;
    )");

    layout->addWidget(nameLabel);


    QLabel *infoLabel =
        new QLabel(
            QString(
                "Time Complexity : %1<br>"
                "Memory Usage : %2<br>"
                "Estimated Operations : %3<br>"
                "Solution Quality : %4"
                )
                .arg(complexity)
                .arg(memory)
                .arg(estimatedOps)
                .arg(quality)
            );

    infoLabel->setTextFormat(Qt::RichText);

    infoLabel->setStyleSheet(R"(

        color:#cbd5e1;

        font-size:12px;

        line-height:1.5;
    )");

    infoLabel->setWordWrap(true);

    layout->addWidget(infoLabel);

    return card;
}

void AlgorithmComparePopup::toggle()
{
    if (isVisible())
    {
        hide();
    }
    else
    {
        show();

        raise();
    }
}

QString AlgorithmComparePopup::formatOperationCount(double ops)
{
    if (ops >= 1000000000.0)
    {
        return QString::number(
                   ops / 1000000000.0,
                   'f',
                   1
                   ) + "B";
    }

    if (ops >= 1000000.0)
    {
        return QString::number(
                   ops / 1000000.0,
                   'f',
                   1
                   ) + "M";
    }

    if (ops >= 1000.0)
    {
        return QString::number(
                   ops / 1000.0,
                   'f',
                   1
                   ) + "K";
    }

    return QString::number(
        static_cast<int>(ops)
        );
}

void AlgorithmComparePopup::updateComparison(
    const QVector<QPointF>& points
    )
{

    QLayoutItem *item;

    while (
        (item = contentLayout->takeAt(0))
        != nullptr
        )
    {
        if (item->widget())
        {
            item->widget()->deleteLater();
        }

        delete item;
    }

    int n = points.size();

    summaryLabel->setText(
        QString(
            "Realtime comparison based on %1 nodes."
            ).arg(n)
        );

    double randomOps =
        n;

    contentLayout->addWidget(
        createAlgorithmCard(
            "Random Walk",
            "O(n)",
            "O(1)",
            formatOperationCount(randomOps),
            "Poor"
            )
        );

    double greedyOps =
        n * n;

    contentLayout->addWidget(
        createAlgorithmCard(
            "Greedy",
            "O(n²)",
            "O(n)",
            formatOperationCount(greedyOps),
            "Good"
            )
        );

    double insertionOps =
        n * n;

    contentLayout->addWidget(
        createAlgorithmCard(
            "Nearest Insertion",
            "O(n²)",
            "O(n)",
            formatOperationCount(insertionOps),
            "Very Good"
            )
        );

    double factorial = 1.0;

    for (int i = 2; i <= n; ++i)
    {
        factorial *= i;
    }

    contentLayout->addWidget(
        createAlgorithmCard(
            "Branch & Bound",
            "O(n!)",
            "High",
            formatOperationCount(factorial),
            "Optimal"
            )
        );


    double saOps =
        n * 200.0;

    contentLayout->addWidget(
        createAlgorithmCard(
            "Simulated Annealing",
            "O(k·n)",
            "O(n)",
            formatOperationCount(saOps),
            "Near Optimal"
            )
        );

    double heldKarpOps =
        n * n * std::pow(2.0, n);

    contentLayout->addWidget(
        createAlgorithmCard(
            "Held-Karp",
            "O(n²2ⁿ)",
            "O(n2ⁿ)",
            formatOperationCount(heldKarpOps),
            "Optimal"
            )
        );

    contentLayout->addStretch();
}
