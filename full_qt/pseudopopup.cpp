#include "pseudopopup.h"
#include <QScrollBar>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QTimer>

PseudoPopup::PseudoPopup(QWidget *parent)
    : QWidget(parent)
{

    setFixedSize(420, 300);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setWindowFlags(Qt::Widget);

    effect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(effect);

    opacityAnim = new QPropertyAnimation(effect, "opacity", this);

    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    setStyleSheet(R"(
        QWidget {
            background: rgba(20,23,30,0.94);
            border-radius: 14px;
            border: 1px solid rgba(90,159,191,0.25);
        }
    )");

    titleLabel = new QLabel("Pseudo Code", this);
    titleLabel->setStyleSheet(R"(
    color:#38bdf8;
    font-weight:700;
    font-size:13px;
    letter-spacing:1px;

    background: rgba(56,189,248,0.10);
    border-radius:10px;
    padding:4px 10px;
    )");
    mainLayout->addWidget(titleLabel);

    QWidget *codeContainer = new QWidget(this);

    codeContainer->setStyleSheet(R"(
    background: qlineargradient(
        x1:0, y1:0, x2:1, y2:1,
        stop:0 rgba(20,25,35,0.95),
        stop:1 rgba(15,20,30,0.95)
    );

    border-radius:14px;

    border: 1px solid rgba(56,189,248,0.12);
    )");

    codeLayout = new QVBoxLayout(codeContainer);
    codeLayout->setContentsMargins(6, 6, 6, 6);
    codeLayout->setSpacing(6);

    mainLayout->addWidget(codeContainer);

    compareLabel = new QLabel(this);
    compareLabel->setStyleSheet(R"(
        color:#cbd5e1;
        background: rgba(30,41,59,0.6);
        padding:8px 10px;
        border-radius:8px;
        border:1px solid rgba(148,163,184,0.15);
        font-size:12px;
    )");
    mainLayout->addWidget(compareLabel);

    buildPseudoForAlg();
    hide();
}

void PseudoPopup::clearLines()
{
    for (auto &line : codeLines)
        delete line.label;

    codeLines.clear();
}

void PseudoPopup::buildPseudoForAlg()
{
    clearLines();

    QVector<QString> lines;

    switch (currentAlg) {
    case TSPAlgorithm::RANDOM:
        lines = {
            "Start at random node",
            "Get all possible next edges",
            "Randomly choose edge",
            "Reject remaining edges",
            "Append node to tour"
        }; break;

    case TSPAlgorithm::GREEDY:
        lines = {
            "Start at first node",
            "Evaluate all neighbors",
            "Pick shortest edge",
            "Reject others",
            "Append to tour"
        }; break;

    case TSPAlgorithm::NEAREST_INSERTION:
        lines = {
            "Initialize 2-node tour",
            "Find closest node",
            "Evaluate insert positions",
            "Choose best insertion",
            "Insert node"
        }; break;

    case TSPAlgorithm::BRANCH_AND_BOUND:
        lines = {
            "Start DFS",
            "Expand edge",
            "Compute bound",
            "Prune branch",
            "Update best"
        }; break;
    }

    for (const QString &text : lines)
    {
        QLabel *lbl = new QLabel(text);

        lbl->setStyleSheet(R"(
        color:#64748b;
        font-size:13px;
        padding:6px 10px;
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
    for (int i = 0; i < codeLines.size(); i++) {

        auto &line = codeLines[i];

        if (i == index) {

            line.label->setStyleSheet(R"(
                color:#f1f5f9;
                font-weight:600;
                font-size:13px;

                background: rgba(56,189,248,0.08);
                border-radius:6px;
                padding:6px 10px;
            )");

        } else {

            line.label->setStyleSheet(R"(
                color:#64748b;
                font-size:13px;
                padding:6px 10px;
            )");
        }
    }
}

void PseudoPopup::updateForStep(const Step &step)
{
    int idx = 0;

    switch (step.action) {
    case StepAction::SELECT_NODE: idx = 0; break;
    case StepAction::COMPARE_EDGE: idx = 1; break;
    case StepAction::UPDATE_BEST: idx = 2; break;
    case StepAction::VISIT_NODE: idx = 3; break;
    case StepAction::INSERT_NODE: idx = 4; break;
    case StepAction::COMPLETE: idx = 4; break;
    }

    highlightLine(idx);

    QString text;

    if (!step.candidate_edges.isEmpty()) {
        text = "Comparing: ";
        for (auto e : step.candidate_edges)
            text += QString("(%1→%2) ").arg(e.first).arg(e.second);
    }
    else if (!step.selected_edges.isEmpty()) {
        text = "Selected: ";
        for (auto e : step.selected_edges)
            text += QString("(%1→%2) ").arg(e.first).arg(e.second);
    }
    else if (!step.rejected_edges.isEmpty()) {
        text = "Rejected: ";
        for (auto e : step.rejected_edges)
            text += QString("(%1→%2) ").arg(e.first).arg(e.second);
    }

    compareLabel->setText(text);
}

void PseudoPopup::toggle()
{
    if (isVisible()) {
        opacityAnim->setDuration(180);
        opacityAnim->setStartValue(1.0);
        opacityAnim->setEndValue(0.0);
        opacityAnim->start();

        QTimer::singleShot(180, this, &QWidget::hide);
    }
    else {
        show();

        opacityAnim->setDuration(180);
        opacityAnim->setStartValue(0.0);
        opacityAnim->setEndValue(1.0);
        opacityAnim->start();
    }
}