#include "pseudopopup.h"
#include <QScrollBar>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QTimer>

PseudoPopup::PseudoPopup(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Widget);

    effect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(effect);

    opacityAnim = new QPropertyAnimation(effect, "opacity", this);

    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    setStyleSheet(R"(
        QWidget {
            background: rgba(23,26,32,0.88);
            border-radius: 10px;
            border: 1px solid #2f3747;
        }
    )");

    titleLabel = new QLabel("Pseudo Code", this);
    titleLabel->setStyleSheet(R"(
        color:#7dd3fc;
        font-weight:600;
        font-size:12px;
    )");
    mainLayout->addWidget(titleLabel);

    scrollArea = new QScrollArea(this);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);

    scrollContent = new QWidget();
    codeLayout = new QVBoxLayout(scrollContent);
    codeLayout->setSpacing(4);

    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea);

    compareLabel = new QLabel(this);
    compareLabel->setStyleSheet(R"(
        color:#cbd5e1;
        background: rgba(15,23,42,0.5);
        padding:6px;
        border-radius:6px;
    )");
    mainLayout->addWidget(compareLabel);

    buildPseudoForAlg();
    hide();
}

void PseudoPopup::clearLines()
{
    for (auto &line : codeLines)
    {
        delete line.label;
    }

    codeLines.clear();
}

void PseudoPopup::buildPseudoForAlg()
{
    clearLines();

    QVector<QString> lines;

    switch (currentAlg)
    {
    case TSPAlgorithm::RANDOM:
        lines =
        {
            "Start at random node",
            "Get all possible next edges",
            "Randomly choose edge",
            "Reject remaining edges",
            "Append node to tour"
        };
        break;

    case TSPAlgorithm::GREEDY:
        lines =
        {
            "Start at first node",
            "Evaluate all neighbors",
            "Pick shortest edge",
            "Reject others",
            "Append to tour"
        };
        break;

    case TSPAlgorithm::NEAREST_INSERTION:
        lines =
        {
            "Initialize 2-node tour",
            "Find closest node",
            "Evaluate insert positions",
            "Choose best insertion",
            "Insert node"
        };
        break;

    case TSPAlgorithm::BRANCH_AND_BOUND:
        lines =
        {
            "Start DFS",
            "Expand edge",
            "Compute bound",
            "Prune branch",
            "Update best"
        };
        break;
    }

    for (const QString &text : lines)
    {
        QLabel *lbl = new QLabel(text, this);

        lbl->setStyleSheet(R"(
            color:#6b7280;
            font-size:11px;
            padding:2px 4px;
        )");

        codeLayout->addWidget(lbl);
        codeLines.push_back({lbl});
    }

    codeLayout->addStretch();
}

void PseudoPopup::setAlgorithm(TSPAlgorithm alg)
{
    currentAlg = alg;
    buildPseudoForAlg();
}

void PseudoPopup::highlightLine(int index)
{
    for (int i = 0; i < codeLines.size(); i++)
    {
        auto &line = codeLines[i];

        if (i == index)
        {
            line.label->setStyleSheet(R"(
                color:#e2e8f0;
                font-weight:600;
                border-left: 3px solid #38bdf8;
                padding-left:6px;
            )");
        }
        else
        {
            line.label->setStyleSheet(R"(
                color:#6b7280;
                padding-left:6px;
            )");
        }
    }
}

void PseudoPopup::updateForStep(const Step &step)
{
    int idx = 0;

    switch (step.action)
    {
    case StepAction::SELECT_NODE:
        idx = 0;
        break;
    case StepAction::COMPARE_EDGE:
        idx = 1;
        break;
    case StepAction::UPDATE_BEST:
        idx = 2;
        break;
    case StepAction::VISIT_NODE:
        idx = 3;
        break;
    case StepAction::INSERT_NODE:
        idx = 4;
        break;
    case StepAction::COMPLETE:
        idx = 4;
        break;
    }

    highlightLine(idx);

    QString text;

    if (!step.candidate_edges.isEmpty())
    {
        text = "Comparing: ";
        for (auto e : step.candidate_edges)
        {
            text += QString("(%1→%2) ").arg(e.first).arg(e.second);
        }
    }
    else if (!step.selected_edges.isEmpty())
    {
        text = "Selected: ";
        for (auto e : step.selected_edges)
        {
            text += QString("(%1→%2) ").arg(e.first).arg(e.second);
        }
    }
    else if (!step.rejected_edges.isEmpty())
    {
        text = "Rejected: ";
        for (auto e : step.rejected_edges)
        {
            text += QString("(%1→%2) ").arg(e.first).arg(e.second);
        }
    }

    compareLabel->setText(text);
}

void PseudoPopup::toggle()
{
    if (isVisible())
    {
        opacityAnim->setDuration(180);
        opacityAnim->setStartValue(1.0);
        opacityAnim->setEndValue(0.0);
        opacityAnim->start();

        QTimer::singleShot(180, this, &QWidget::hide);
    }
    else
    {
        show();

        opacityAnim->setDuration(180);
        opacityAnim->setStartValue(0.0);
        opacityAnim->setEndValue(1.0);
        opacityAnim->start();
    }
}