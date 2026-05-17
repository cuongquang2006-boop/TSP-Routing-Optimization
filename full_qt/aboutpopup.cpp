#include "aboutpopup.h"

AboutPopup::AboutPopup(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(620, 540);

    setWindowFlags(Qt::Widget);

    setStyleSheet(R"(

        QWidget {

            background:
                rgba(15,18,24,0.985)

            border-radius:14px;

            border:
                1px solid
                rgba(56,189,248,0.35)
        }

        QLabel {

            color:#cbd5e1;

            background:transparent;
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
        16,16,16,16
        );

    mainLayout->setSpacing(12);

    titleLabel =
        new QLabel(this);

    titleLabel->setStyleSheet(R"(

        color:#38bdf8;

        font-size:20px;

        letter-spacing:1px;

        font-weight:700;

        padding:6px 10px;

        background:
            rgba(56,189,248,0.08);

        border-radius:10px;

    )");

    contentLabel =
        new QLabel(this);

    contentLabel->setWordWrap(true);

    contentLabel->setAlignment(
        Qt::AlignTop |
        Qt::AlignLeft
        );

    contentLabel->setStyleSheet(R"(

        font-size:14px;

        color:#dbe4ee;

        line-height:1.5;

        padding:8px;

    )");

    mainLayout->addWidget(titleLabel);

    mainLayout->addWidget(contentLabel);

    rebuildContent();

    hide();
}

void AboutPopup::toggle()
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

void AboutPopup::setAlgorithm(
    TSPAlgorithm alg
    )
{
    currentAlg = alg;

    rebuildContent();
}

void AboutPopup::rebuildContent()
{
    QString title;

    QString text;

    switch(currentAlg)
    {

    case TSPAlgorithm::RANDOM:

        title = "Random Walk";

        text = R"(

<b>Idea</b><br>
Randomly selects the next node
without optimization.<br><br>

<b>Flow</b><br>
• Start from random node<br>
• Randomly choose next edge<br>
• Continue until all visited<br><br>

<b>Complexity</b><br>
Time: O(n)<br>
Memory: O(1)<br><br>

<b>Strength</b><br>
Very fast and simple.<br><br>

<b>Weakness</b><br>
Usually produces poor tours.<br><br>

<b>Visualization Hint</b><br>
Yellow = candidate edges<br>
Green = selected move

)";
        break;

    case TSPAlgorithm::GREEDY:

        title = "Greedy Nearest Neighbor";

        text = R"(

<b>Idea</b><br>
Always chooses the nearest
unvisited node.<br><br>

<b>Flow</b><br>
• Start from initial node<br>
• Evaluate neighbors<br>
• Pick shortest edge<br>
• Repeat greedily<br><br>

<b>Complexity</b><br>
Time: O(n²)<br>
Memory: O(n)<br><br>

<b>Strength</b><br>
Fast and intuitive.<br><br>

<b>Weakness</b><br>
Can get trapped in local optimum.<br><br>

<b>Visualization Hint</b><br>
Yellow = evaluated neighbors<br>
Green = chosen shortest edge

)";
        break;

    case TSPAlgorithm::NEAREST_INSERTION:

        title = "Nearest Insertion";

        text = R"(

<b>Idea</b><br>
Gradually builds a tour by
inserting closest nodes into
the best position.<br><br>

<b>Flow</b><br>
• Initialize small tour<br>
• Find closest external node<br>
• Evaluate insertion positions<br>
• Insert with minimum increase<br><br>

<b>Complexity</b><br>
Time: O(n²)<br>
Memory: O(n)<br><br>

<b>Strength</b><br>
Usually better than greedy.<br><br>

<b>Weakness</b><br>
Still heuristic-based.<br><br>

<b>Visualization Hint</b><br>
Yellow = insertion candidates<br>
Green = committed insertion

)";
        break;

    case TSPAlgorithm::BRANCH_AND_BOUND:

        title = "Branch and Bound";

        text = R"(

<b>Idea</b><br>
Searches solution tree while
pruning branches that cannot
beat the current best solution.<br><br>

<b>Flow</b><br>
• Expand DFS branch<br>
• Estimate lower bound<br>
• Prune expensive states<br>
• Update best solution<br><br>

<b>Complexity</b><br>
Worst-case: O(n!)<br><br>

<b>Strength</b><br>
Guarantees optimal solution.<br><br>

<b>Weakness</b><br>
Explodes rapidly for large n.<br><br>

<b>Visualization Hint</b><br>
Red = pruned edges<br>
Green = best branch

)";
        break;

    case TSPAlgorithm::SIMULATED_ANNEALING:

        title = "Simulated Annealing";

        text = R"(

<b>Idea</b><br>
Uses probabilistic search inspired
by thermodynamic cooling.<br><br>

<b>Flow</b><br>
• Generate random tour<br>
• Swap nodes<br>
• Accept better moves<br>
• Sometimes accept worse moves<br>
• Gradually cool temperature<br><br>

<b>Complexity</b><br>
Approx: O(iterations × n)<br><br>

<b>Strength</b><br>
Escapes local optimum.<br><br>

<b>Weakness</b><br>
No guaranteed optimal solution.<br><br>

<b>Visualization Hint</b><br>
Yellow = candidate mutation<br>
Green = accepted transition

)";
        break;

    case TSPAlgorithm::HELD_KARP:

        title = "Held-Karp Dynamic Programming";

        text = R"(

<b>Idea</b><br>
Dynamic Programming over subsets.
Each DP state stores the minimum
cost to reach a node after visiting
a subset of cities.<br><br>

<b>Flow</b><br>
• Initialize DP table<br>
• Enumerate bitmask states<br>
• Relax transitions u → v<br>
• Update optimal subproblems<br>
• Reconstruct optimal path<br><br>

<b>Complexity</b><br>
Time: O(n² · 2ⁿ)<br>
Memory: O(n · 2ⁿ)<br><br>

<b>Strength</b><br>
Guarantees optimal solution.<br><br>

<b>Weakness</b><br>
Very memory intensive.<br><br>

<b>Visualization Hint</b><br>
Yellow = current transition<br>
Green = committed subpath<br>
DP Popup = current DP state

)";
        break;
    }

    titleLabel->setText(title);

    contentLabel->setText(text);
}