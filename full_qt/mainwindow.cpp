#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "GraphCanvas.h"
#include "tsp_algorithms.h"
#include "pseudopopup.h"

#include <QMessageBox>
#include <QRandomGenerator>
#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QPainter>
#include <QFont>
#include <QTimer>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , currentStepIndex(0)
    , currentAlgorithm(TSPAlgorithm::RANDOM)
    , isPlaying(false)
    , animationTimer(nullptr)
    , mainLayout(nullptr)
    , contentLayout(nullptr)
{
    // Setup UI from designer (or create minimal central widget)
    ui->setupUi(this);

    resize(1200,600);

    // Ensure central widget exists
    if (!centralWidget()) {
        setCentralWidget(new QWidget(this));
    }

    // Initialize components
    setupWidgets();
    setupLayouts();
    setupStyles();
    setupConnections();

    // Create and connect animation timer
    animationTimer = new QTimer(this);
    connect(animationTimer, &QTimer::timeout, this, &MainWindow::onTimerTimeout);

    // Set initial values
    speedSlider->setValue(5);

    // 🔥 set default mode (nếu chưa có)
    canvasWidget->setInputMode(InputMode::CLICK);

    // 🔥 QUAN TRỌNG: sync UI theo mode
    updateInputModeUI();

    canvasWidget->setInputMode(InputMode::NONE);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupWidgets()
{

    tutorialOverlay =
        new TutorialOverlay(this);

    tutorialOverlay->hide();

    togglePanelButton =
        new QPushButton(
            "<",
            this
            );

    togglePanelButton->setFixedSize(
        38,
        88
        );

    toggleButtonAnim =
        new QPropertyAnimation(
            togglePanelButton,
            "pos",
            this
            );

    toggleButtonAnim->setDuration(260);

    toggleButtonAnim->setEasingCurve(
        QEasingCurve::InOutCubic
        );

    togglePanelButton->setCursor(
        Qt::PointingHandCursor
        );

    togglePanelButton->setObjectName(
        "togglePanelButton"
        );

    canvasWidget = new GraphCanvas(this);

    controlPanel = new QWidget(this);

    panelAnimation =
        new QPropertyAnimation(
            controlPanel,
            "maximumWidth",
            this
            );

    panelAnimation->setDuration(260);

    panelAnimation->setEasingCurve(
        QEasingCurve::InOutCubic
        );

    algorithmCombo = new QComboBox(controlPanel);
    algorithmCombo->addItems({
        "Random",
        "Greedy",
        "Nearest Insertion",
        "Branch & Bound",
        "Simulated Annealing",
        "Held-Karp"
    });
    generateButton = new QPushButton(tr("✦ Random Points"), controlPanel);
    runButton = new QPushButton(tr("▶ Run"), controlPanel);
    stepButton = new QPushButton(tr("Step ⮞"), controlPanel);
    backButton = new QPushButton(tr("⮜ Prev"),controlPanel);
    resetButton = new QPushButton(tr("↺ Reset"), controlPanel);
    speedSlider = new QSlider(Qt::Horizontal, controlPanel);
    speedSlider->setRange(1, 10);
    speedSlider->setValue(5);
    speedLabel = new QLabel(tr("Speed: 2.5x"), controlPanel);

    // bar explain
    legendBar = new QWidget(this);
    legendBar->setObjectName("legendBar");

    legendBar->setFixedHeight(40);

    // ===== INPUT MODE =====
    modeLabel = new QLabel("Input Mode", controlPanel);

    clickModeButton = new QPushButton("Click", controlPanel);
    inputModeButton = new QPushButton("Manual", controlPanel);

    // ===== INPUT FIELDS =====
    inputX = new QLineEdit(controlPanel);
    inputY = new QLineEdit(controlPanel);

    inputX->setPlaceholderText("X");
    inputY->setPlaceholderText("Y");

    addPointButton = new QPushButton("Add Point", controlPanel);

    // Pseudo code button
    pseudoButton = new QPushButton(tr("PSEUDO CODE"), controlPanel);
    pseudoButton->setObjectName("pseudoButton");
    pseudoButton->setMinimumHeight(36);

    dpButton =
        new QPushButton(
            tr("DP STATE"),
            controlPanel
            );

    dpButton->setObjectName(
        "dpButton"
        );

    dpButton->setMinimumHeight(36);

    // Create popup (hidden by default)
    pseudoPopup = new PseudoPopup(this);
    pseudoPopup->hide();

    dpPopup =
        new DPPopup(this);

    dpPopup->hide();

    dpButton->hide();

    aboutButton =
        new QPushButton(
            tr("ⓘ About"),
            controlPanel
            );

    aboutPopup =
        new AboutPopup(this);

    aboutPopup->hide();

    compareButton =
        new QPushButton(
            tr("📊 Compare"),
            controlPanel
            );

    comparePopup =
        new AlgorithmComparePopup(this);

    comparePopup->hide();

    infoPanel = new QWidget(this);
    costLabel = new QLabel(tr("Cost: -"), infoPanel);
    timeLabel = new QLabel(tr("Time: -"), infoPanel);
    stepLabel = new QLabel(tr("Step: -"), infoPanel);

    clickModeButton->setCheckable(true);
    inputModeButton->setCheckable(true);
}

void MainWindow::setupLayouts()
{
    // Create main layout on central widget
    QWidget *central = centralWidget();
    if (!central) {
        central = new QWidget(this);
        setCentralWidget(central);
    }

    mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    contentLayout = new QHBoxLayout;
    contentLayout->setSpacing(12);

    // Left canvas area
    canvasWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    contentLayout->addWidget(canvasWidget, 7);

    // Right control panel
    QVBoxLayout *controlLayout = new QVBoxLayout(controlPanel);
    controlLayout->setContentsMargins(12, 12, 12, 12);
    controlLayout->setSpacing(14);
    QLabel *algorithmLabel = new QLabel(tr("Select Algorithm"), controlPanel);
    algorithmLabel->setObjectName("sectionLabel");
    controlLayout->addWidget(algorithmLabel);
    controlLayout->addWidget(algorithmCombo);
    controlLayout->addSpacing(12);
    controlLayout->addWidget(generateButton);
    controlLayout->addWidget(runButton);

    QHBoxLayout *stepLayout =
        new QHBoxLayout;

    stepLayout->setSpacing(8);
    stepLayout->addWidget(backButton);
    stepLayout->addWidget(stepButton);
    controlLayout->addLayout(stepLayout);
    backButton->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Preferred
        );

    stepButton->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Preferred
        );

    controlLayout->addWidget(resetButton);
    controlLayout->addSpacing(8);
    controlLayout->addWidget(speedLabel);
    controlLayout->addWidget(speedSlider);

    // ===== INPUT MODE UI =====
    controlLayout->addSpacing(10);

    controlLayout->addWidget(modeLabel);

    QHBoxLayout *legendLayout = new QHBoxLayout(legendBar);
    legendLayout->setContentsMargins(20, 6, 20, 6);
    legendLayout->setSpacing(40);

    auto createLegendItem = [](QString text, QString color) {
        QWidget *item = new QWidget();

        QHBoxLayout *layout = new QHBoxLayout(item);
        layout->setContentsMargins(0,0,0,0);
        layout->setSpacing(6);

        QLabel *dot = new QLabel();
        dot->setFixedSize(10,10);
        dot->setStyleSheet(QString(
                               "background:%1; border-radius:5px;"
                               ).arg(color));

        QLabel *label = new QLabel(text);
        label->setStyleSheet(R"(
        color:#cbd5e1;
        font-size:12px;
    )");

        layout->addWidget(dot);
        layout->addWidget(label);

        return item;
    };

    legendLayout->addWidget(createLegendItem("Selected", "#22c55e"));
    legendLayout->addWidget(createLegendItem("Rejected", "#ef4444"));
    legendLayout->addWidget(createLegendItem("Candidate", "#eab308"));

    // mode buttons (Click / Manual)
    QHBoxLayout *modeLayout = new QHBoxLayout;
    modeLayout->addWidget(clickModeButton);
    modeLayout->addWidget(inputModeButton);
    controlLayout->addLayout(modeLayout);

    // input fields (X, Y)
    QHBoxLayout *inputLayout = new QHBoxLayout;
    inputLayout->addWidget(inputX);
    inputLayout->addWidget(inputY);
    controlLayout->addLayout(inputLayout);

    // add button
    controlLayout->addWidget(addPointButton);

    controlLayout->addStretch(1);

    controlPanel->setMinimumWidth(0);

    controlPanel->setMaximumWidth(360);

    controlPanel->setSizePolicy(
        QSizePolicy::Preferred,
        QSizePolicy::Expanding
        );

    contentLayout->addWidget(controlPanel, 3);

    mainLayout->addLayout(contentLayout, 1);

    pseudoPopup->setParent(this);
    pseudoPopup->raise();
    pseudoPopup->hide();

    dpPopup->setParent(this);

    dpPopup->raise();

    dpPopup->move(
        width() - 340,
        height() - 330
        );

    dpPopup->hide();

    pseudoPopup->hide();

    comparePopup->setParent(this);

    comparePopup->raise();

    comparePopup->move(
        width() / 2 - 270,
        height() / 2 - 300
        );

    comparePopup->hide();

    // Bottom info panel
    QHBoxLayout *infoLayout = new QHBoxLayout(infoPanel);
    infoLayout->setContentsMargins(14, 10, 14, 10);
    infoLayout->setSpacing(24);
    infoLayout->addWidget(costLabel);
    infoLayout->addWidget(timeLabel);
    infoLayout->addWidget(stepLabel);
    infoLayout->addStretch(1);
    infoLayout->addWidget(aboutButton);
    infoLayout->addWidget(compareButton);
    infoLayout->addWidget(pseudoButton);
    infoLayout->addWidget(dpButton);

    infoPanel->setFixedHeight(80);
    mainLayout->addWidget(infoPanel, 0);

    togglePanelButton->raise();

    togglePanelButton->show();

    togglePanelButton->move(
        width() - 410,
        height()/2 - 44
        );
}

void MainWindow::setupConnections()
{
    // Button signals
    connect(generateButton, &QPushButton::clicked, this, &MainWindow::onGenerateClicked);
    connect(runButton, &QPushButton::clicked, this, &MainWindow::onRunClicked);
    connect(stepButton, &QPushButton::clicked, this, &MainWindow::onStepClicked);
    connect(resetButton, &QPushButton::clicked, this, &MainWindow::onResetClicked);

    connect(
        backButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            if (steps.isEmpty())
                return;

            if (currentStepIndex <= 0)
                return;

            pause();

            currentStepIndex--;

            applyStep(
                steps[currentStepIndex]
                );

            updateInfoPanel();
        }
        );

    // ComboBox & Slider signals
    connect(algorithmCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onAlgorithmChanged);
    connect(speedSlider, &QSlider::valueChanged, this, &MainWindow::onSpeedChanged);

    // Pseudo button
    connect(pseudoButton, &QPushButton::clicked, this, [this]() {
        if (pseudoPopup) pseudoPopup->toggle();
    });

    connect(
        dpButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            if (dpPopup)
            {
                dpPopup->toggle();
            }
        }
        );

    connect(
        aboutButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            if (aboutPopup)
            {
                aboutPopup->toggle();
            }
        }
        );

    // Animation timer signal
    connect(animationTimer, &QTimer::timeout, this, &MainWindow::onTimerTimeout);

    connect(clickModeButton, &QPushButton::clicked, this, [this]() {
        canvasWidget->setInputMode(InputMode::CLICK);
        clickModeButton->setChecked(true);
        inputModeButton->setChecked(false);

        updateInputModeUI(); // 🔥 thêm
    });

    connect(inputModeButton, &QPushButton::clicked, this, [this]() {
        canvasWidget->setInputMode(InputMode::MANUAL);
        clickModeButton->setChecked(false);
        inputModeButton->setChecked(true);

        updateInputModeUI(); // 🔥 thêm
    });

    connect(canvasWidget, &GraphCanvas::pointAdded, this, [this](QPointF normalizedPos) {

        // convert normalized → actual pixel giống thuật toán đang dùng
        const QRect canvasRect = canvasWidget->contentsRect().adjusted(12, 12, -12, -12);

        qreal x = canvasRect.left() + normalizedPos.x() * canvasRect.width();
        qreal y = canvasRect.top() + normalizedPos.y() * canvasRect.height();

        points.append(normalizedPos);


        currentStepIndex = 0;
        loadSteps();
    });

    connect(addPointButton, &QPushButton::clicked, this, [this]() {

        bool okX, okY;
        double x = inputX->text().toDouble(&okX);
        double y = inputY->text().toDouble(&okY);

        if (!okX || !okY) return;

        canvasWidget->addPoint(QPointF(x, y));
    });

    connect(
        togglePanelButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            panelCollapsed =
                !panelCollapsed;

            panelAnimation->stop();

            toggleButtonAnim->stop();

            int centerY =
                height() / 2
                - togglePanelButton->height() / 2;

            if (panelCollapsed)
            {
                // =====================================
                // PANEL COLLAPSE
                // =====================================

                panelAnimation->setStartValue(
                    controlPanel->width()
                    );

                panelAnimation->setEndValue(0);

                // =====================================
                // BUTTON SLIDE OUT
                // =====================================

                toggleButtonAnim->setStartValue(
                    togglePanelButton->pos()
                    );

                toggleButtonAnim->setEndValue(
                    QPoint(
                        width() - 52,
                        centerY
                        )
                    );

                togglePanelButton->setText("❯");
            }
            else
            {
                // =====================================
                // PANEL EXPAND
                // =====================================

                controlPanel->show();

                panelAnimation->setStartValue(0);

                panelAnimation->setEndValue(360);

                // =====================================
                // BUTTON RETURN
                // =====================================

                toggleButtonAnim->setStartValue(
                    togglePanelButton->pos()
                    );

                toggleButtonAnim->setEndValue(
                    QPoint(
                        width() - 400,
                        centerY
                        )
                    );

                togglePanelButton->setText("❮");
            }

            panelAnimation->start();

            toggleButtonAnim->start();
        }
        );

    connect(
        compareButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            if (comparePopup)
            {
                comparePopup->updateComparison(
                    canvasWidget->getPoints()
                    );

                comparePopup->toggle();
            }
        }
        );


    QTimer::singleShot(
        800,
        this,
        [this]()
        {
            QVector<TutorialStep> steps =
                {
                    {
                        algorithmCombo,
                        QRect(),
                        false,
                        "Algorithm Selection",
                        "Choose the TSP algorithm to visualize."
                    },

                    {
                        generateButton,
                        QRect(),
                        false,
                        "Random Points",
                        "Generate a new graph automatically."
                    },

                    {
                        runButton,
                        QRect(),
                        false,
                        "Run Visualization",
                        "Execute the algorithm automatically from start to finish."
                    },

                    {
                        stepButton,
                        QRect(),
                        false,
                        "Step Execution",
                        "Execute the algorithm one step at a time to observe decision making."
                    },

                    {
                        backButton,
                        QRect(),
                        false,
                        "Previous Step",
                        "Return to the previous visualization state."
                    },

                    {
                        resetButton,
                        QRect(),
                        false,
                        "Reset Visualization",
                        "Clear the current graph and reset all algorithm states."
                    },

                    {
                        speedSlider,
                        QRect(),
                        false,
                        "Visualization Speed",
                        "Adjust the playback speed of the algorithm visualization."
                    },

                    {
                        clickModeButton,
                        QRect(),
                        false,
                        "Click Mode",
                        "Click to input."
                    },

                    {
                        inputModeButton,
                        QRect(),
                        false,
                        "Input Mode",
                        "Switch between click input mode and manual coordinate input."
                    },

                    {
                        addPointButton,
                        QRect(),
                        false,
                        "Add Point",
                        "Insert a custom node into the graph manually."
                    },

                    {
                        pseudoButton,
                        QRect(),
                        false,
                        "Pseudo Code",
                        "Pseudo code synchronizes with the algorithm state."
                    },

                    {
                        compareButton,
                        QRect(),
                        false,
                        "Compare Mode",
                        "Run multiple TSP algorithms simultaneously and compare execution time, path cost, explored states, and solution quality."
                    },

                    {
                        aboutButton,
                        QRect(),
                        false,
                        "About Project",
                        "View information about the project and supported algorithms."
                    },

                    {
                        canvasWidget,
                        QRect(),
                        false,
                        "Graph Visualization",
                        "This area visualizes the graph, nodes, edges, and algorithm transitions."
                    },

                    {
                        legendBar,
                        QRect(),
                        false,
                        "Visualization Legend",
                        "Green = selected, yellow = candidate, red = rejected."
                    }
                };
                tutorialOverlay->startTutorial(
                steps
                );
        }
        );

}

void MainWindow::onGenerateClicked()
{
    if (isPlaying) {
        pause();
    }

    // =====================================
    // RESET VISUAL/UI STATE
    // =====================================

    currentStepIndex = 0;

    steps.clear();

    costLabel->setText("Cost: -");
    timeLabel->setText("Time: -");
    stepLabel->setText("Step: -");

    // reset pseudo highlight
    if (pseudoPopup)
    {
        pseudoPopup->updateForStep(Step());
    }

    emit algorithmRunning(false);

    // =====================================
    // RANDOM NODE
    // =====================================

    int nodeCount;

    if (currentAlgorithm == TSPAlgorithm::BRANCH_AND_BOUND ||
        currentAlgorithm == TSPAlgorithm::HELD_KARP)
    {
        nodeCount = QRandomGenerator::global()->bounded(6, 9);
    }
    else
    {
        nodeCount = QRandomGenerator::global()->bounded(7, 11);
    }

    // ===== B&B LIMIT =====

    if (currentAlgorithm == TSPAlgorithm::BRANCH_AND_BOUND &&
        nodeCount > 10)
    {
        QMessageBox::warning(
            this,
            "Branch & Bound Limit",
            "Branch & Bound has factorial complexity (O(n!)) and becomes very slow with many nodes.\n\n"
            "Recommended maximum: 10 nodes.\n"
            "The system will automatically reduce the number to 10 to prevent freezing."
            );

        nodeCount = 10;
    }

    // =====================================
    // RESET GRAPH
    // =====================================

    canvasWidget->clearPoints();

    // Generate points
    canvasWidget->generateRandomPoints(nodeCount);

    // rebuild points data
    points.clear();

    auto canvasPoints =
        canvasWidget->getPoints();

    for (const auto& p : canvasPoints)
    {
        points.append(p);
    }

    loadSteps();
}

void MainWindow::onRunClicked()
{
    if (points.size() < 2) {
        QMessageBox::warning(this,
                             "Not enough nodes",
                             "You need at least 2 nodes to run TSP algorithm.");
        return;
    }

    if (isPlaying) {
        pause();
    } else {
        play();
    }
}

void MainWindow::onStepClicked()
{
    if (points.size() < 2) return;

    setInputUIEnabled(false);   // 🔥 khóa input

    nextStep();
}

void MainWindow::onResetClicked()
{
    reset();
}

void MainWindow::onAlgorithmChanged(int index)
{
    TSPAlgorithm newAlgo = static_cast<TSPAlgorithm>(index);

    if (newAlgo == TSPAlgorithm::BRANCH_AND_BOUND && points.size() > 10) {
        QMessageBox::information(this,
                                 "Branch & Bound Limit",
                                 "The current number of nodes is too large for Branch & Bound.\n\n"
                                 "Recommended limit: ≤ 10 nodes.\n"
                                 "Please generate fewer nodes or choose another algorithm.");

        return;
    }

    // 🧠 Update state trước
    currentAlgorithm = newAlgo;

    // 🎯 Set limit theo algo mới
    if (currentAlgorithm ==
            TSPAlgorithm::BRANCH_AND_BOUND ||

            currentAlgorithm ==
            TSPAlgorithm::HELD_KARP)
    {
        canvasWidget->setMaxNodeLimit(8);
    }
    else
    {
        canvasWidget->setMaxNodeLimit(10);
    }

    canvasWidget->setCurrentAlgorithm(
        currentAlgorithm
        );

    // ⏸️ Pause nếu đang chạy
    if (isPlaying) {
        pause();
    }

    // 🔁 Reset step state (KHÔNG reset graph)
    currentStepIndex = 0;
    steps.clear();

    // 🔄 regenerate steps với graph hiện tại
    if (!points.isEmpty()) {
        loadSteps();
    }

    // 📜 update pseudo code
    if (pseudoPopup)
        pseudoPopup->setAlgorithm(currentAlgorithm);

    if (currentAlgorithm ==
        TSPAlgorithm::HELD_KARP)
    {
        dpButton->show();
    }
    else
    {
        dpButton->hide();

        if (dpPopup)
        {
            dpPopup->hide();
        }
    }

    if (aboutPopup)
    {
        aboutPopup->setAlgorithm(
            currentAlgorithm
            );
    }
}

void MainWindow::onSpeedChanged(int value)
{
    // ===== 1. Map slider → speed multiplier (hiển thị) =====
    // scale lại cho trực quan hơn
    double speed = 0.5 + (value - 1) * 0.2; // 0.5x → 2.3x
    speedLabel->setText(tr("Speed: %1x").arg(speed, 0, 'f', 1));

    // ===== 2. Điều chỉnh FPS (mượt + không quá nhanh) =====
    // cinematic range: 24 FPS → 60 FPS
    qreal adaptiveFPS = 24.0 + (value - 1) * 4.0;

    int interval = static_cast<int>(1000.0 / adaptiveFPS);

    // clamp để tránh quá nhanh
    interval = std::clamp(interval, 16, 50);
    // ~60 FPS max, ~20 FPS min

    // ===== 3. LUÔN apply (không phụ thuộc isPlaying) =====
    animationTimer->setInterval(interval);
}

void MainWindow::onTimerTimeout()
{
    // 👉 cho phép step đầu tiên chạy luôn
    if (currentStepIndex == 0) {
        nextStep();
        return;
    }

    // 👉 các step sau phải chờ animation xong
    if (!canvasWidget->isAnimationFinished()) {
        return;
    }

    nextStep();
}

void MainWindow::loadSteps()
{
    if (points.isEmpty()) {
        return;
    }

    // Clear previous steps
    steps.clear();
    currentStepIndex = 0;

    // Generate steps based on selected algorithm
    switch (currentAlgorithm) {
    case TSPAlgorithm::RANDOM:
        steps = generateRandomSteps(points);
        break;
    case TSPAlgorithm::GREEDY:
        steps = generateGreedySteps(points);
        break;
    case TSPAlgorithm::NEAREST_INSERTION:
        steps = generateNearestInsertionSteps(points);
        break;
    case TSPAlgorithm::BRANCH_AND_BOUND:
        steps = generateBranchAndBoundSteps(points);
        break;
    case TSPAlgorithm::SIMULATED_ANNEALING:
        steps = generateSimulatedAnnealingSteps(points);
        break;
    case TSPAlgorithm::HELD_KARP:
        steps = generateHeldKarpSteps(points);
        break;
    }

    updateInfoPanel();
    emit algorithmRunning(false);
}

void MainWindow::play()
{
    if (steps.isEmpty()) {
        return;
    }

    if (currentStepIndex >= steps.size()) {
        // Reset to start if at end
        reset();
    }

    isPlaying = true;

    setInputUIEnabled(false);

    runButton->setText(tr("❚❚ Pause"));
    emit playStateChanged(true);

    // Start timer with adaptive FPS based on speed slider
    int speedValue = speedSlider->value();
    qreal adaptiveFPS = 30.0 + (speedValue - 1) * 7.5;
    int interval = static_cast<int>(1000.0 / adaptiveFPS);
    interval = std::max(11, interval); // Minimum ~90 FPS
    animationTimer->start(interval);
}

void MainWindow::pause()
{
    if (animationTimer->isActive())
    {
        animationTimer->stop();
    }

    isPlaying = false;

    runButton->setText(tr("▶ Run"));
    emit playStateChanged(false);
}

void MainWindow::nextStep()
{
    if (currentStepIndex < steps.size())
    {
        applyStep(steps[currentStepIndex]);
        currentStepIndex++;

        updateInfoPanel();
        emit stepChanged(currentStepIndex, steps.size());
    }

    // 🔥 CHECK COMPLETE (QUAN TRỌNG)
    if (currentStepIndex >= steps.size())
    {
        if (animationTimer->isActive())
            animationTimer->stop();   // 🔥 đảm bảo dừng hẳn

        isPlaying = false;

        setInputUIEnabled(true);      // mở UI chung
        updateInputModeUI();          // 🔥 sync lại manual/click

        runButton->setText(tr("▶ Run"));
    }
}

void MainWindow::reset()
{
    // 🔥 Không có gì thì khỏi hỏi
    if (points.isEmpty() && steps.isEmpty() && currentStepIndex == 0)
        return;

    QMessageBox::StandardButton reply;

    reply = QMessageBox::question(
        this,
        "Confirm Reset",
        "Are you sure you want to reset?\nAll points and progress will be lost.",
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply != QMessageBox::Yes)
        return;

    // ===== RESET LOGIC =====
    if (isPlaying) {
        pause();
    }

    currentStepIndex = 0;

    points.clear();
    steps.clear();

    canvasWidget->clearPoints();

    updateInfoPanel();
    emit stepChanged(0, steps.size());
    emit algorithmRunning(false);

    setInputUIEnabled(true);
    updateInputModeUI(); // sync lại theo mode
}


void MainWindow::applyStep(const Step &step)
{
    // Update canvas visualization with step data:
    // - tour: current TSP tour nodes
    // - currentNode: node being processed (highlighted blue)
    // - bestCandidate: best node found so far (highlighted green)
    // - edge: edge being compared (highlighted yellow/red)

    // Get animation duration based on step action type
    int animationDuration = step.getAnimationDuration();

    // Calculate speed multiplier from slider (1-10)
    // Speed 1 (slow): 0.5x, Speed 5 (medium): 1.0x, Speed 10 (fast): 2.0x
    qreal speedValue = speedSlider->value();

    // scale lại cho dễ nhìn hơn (chậm hơn nhiều)
    qreal speedMultiplier = 0.3 + speedValue * 0.15;
    // range: 0.45 → 1.8

    // Set animation parameters in canvas
    canvasWidget->setAnimationParams(animationDuration, speedMultiplier);

    // Apply the entire Step object to canvas (preferred: keeps rendering logic in UI)
    canvasWidget->setStep(step);

    // Update pseudo-code popup with context
    if (pseudoPopup) {
        pseudoPopup->setAlgorithm(currentAlgorithm);
        pseudoPopup->updateForStep(step);
    }

    if (dpPopup)
    {
        dpPopup->updateForStep(step);
    }
}

void MainWindow::updateInfoPanel()
{
    if (currentStepIndex > 0 && currentStepIndex <= steps.size()) {
        const Step &step = steps[currentStepIndex - 1];

        // Display cost value
        costLabel->setText(tr("Cost: %1").arg(step.value, 0, 'f', 2));

        // Display current step position
        stepLabel->setText(tr("Step: %1/%2").arg(currentStepIndex).arg(steps.size()));

        // Display action type
        QString actionStr = stepActionToString(step.action);
        timeLabel->setText(tr("Action: %1").arg(actionStr));
    } else {
        // Reset display when no steps
        costLabel->setText(tr("Cost: -"));
        stepLabel->setText(tr("Step: -"));
        timeLabel->setText(tr("Action: -"));
    }
}


void MainWindow::setupStyles()
{
    // ===== FONT SETUP =====
    QFont buttonFont("Tahoma", 10);
    buttonFont.setBold(true);

    QFont labelFont("Tahoma", 11);

    QFont titleFont("Tahoma", 12);
    titleFont.setBold(true);

    setWindowTitle(tr("TSP Visualizer"));

    QString globalStyle = R"(

        /* ================= ROOT ================= */

        QMainWindow {
            background: #121417;
        }

        QWidget#centralwidget, QWidget {
            background: #121417;
            color: #e5e7eb;
        }

        QWidget {
            border-radius: 12px;
        }

        QWidget#controlPanel, QWidget#infoPanel {
            background: #191c24;
            border: 1px solid #2f3747;
        }

        /* ================= LABEL ================= */

        QLabel {
            color: #e5e7eb;
            font-size: 13px;
            font-family: Tahoma;
            border-radius: 4px;
            padding: 2px 4px;
        }

        QLabel#sectionLabel {
            font-weight: 600;
            font-size: 12px;
            background: #1f242f;
            padding: 4px 8px;
        }

        /* 🔥 FIX LEGEND (QUAN TRỌNG NHẤT) */
        #legendBar {
            background: rgba(15,20,30,0.9);
            border-radius: 18px;
            border: 1px solid rgba(148,163,184,0.15);
            padding-left: 12px;
            padding-right: 12px;
        }

        #legendBar QLabel {
            background: transparent;
            border: none;
            padding: 0px;
            border-radius: 0px;
        }

        /* ================= INPUT ================= */

        QLineEdit {
            background: #1f242f;
            border: 1px solid #323a48;
            color: #f8fafc;

            min-height: 32px;
            border-radius: 4px;

            padding: 4px 8px;
            font-family: Tahoma;
            font-size: 11px;
        }

        QLineEdit:focus {
            border: 1px solid #38bdf8;
            background: #262d3a;
        }

        /* ================= COMBO + SLIDER ================= */

        QComboBox, QSlider {
            background: #1f242f;
            border: 1px solid #323a48;
            color: #f8fafc;
            min-height: 36px;
            border-radius: 6px;
            font-family: Tahoma;
            font-size: 11px;
        }

        QComboBox {
            padding-right: 42px;
        }

        QComboBox:hover {
            border: 1px solid #4a8ab3;
            background: #262d3a;
        }

        QComboBox:focus {
            border: 2px solid #5a9fbf;
            background: #262d3a;
        }

        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: center right;
            width: 36px;
            background: transparent;
            border: none;
            margin: 0px 4px 0px 0px;
        }

        /* ================= BUTTON ================= */

        QPushButton {
            background: #3a3f4b;
            border: 1px solid #505862;
            color: #e5e7eb;

            min-height: 36px;
            border-radius: 8px;

            font-family: Tahoma;
            font-weight: bold;
            font-size: 10px;
            padding: 8px 16px;
        }

        QPushButton:hover {
            background: #4a5261;
            border: 1px solid #6b7280;
            color: #ffffff;
        }

        QPushButton:pressed {
            background: #343a46;
            border: 1px solid #5a9fbf;
            color: #ffffff;
        }

        QPushButton:focus {
            border: 2px solid #5a9fbf;
        }

        QPushButton:checked {
            background: #5a9fbf;
            color: #0f172a;
        }

        /* ================= SLIDER ================= */

        QSlider::groove:horizontal {
            background: #2a3141;
            height: 8px;
            border-radius: 4px;
            margin: 2px 0;
        }

        QSlider::handle:horizontal {
            background: #4d7d84;
            width: 18px;
            margin: -5px 0;
            border-radius: 9px;
            border: 1px solid #5a9199;
        }

        QSlider::handle:horizontal:hover {
            background: #5a9199;
        }

        /* ================= GRAPH ================= */

        QGraphicsView {
            border: 1px solid #2d3340;
            border-radius: 12px;
            background: #171a20;
        }

        QSlider::groove:horizontal {
            height: 6px;
            background: #1f2937;
            border-radius: 3px;
        }

        QSlider::sub-page:horizontal {
            background: #38bdf8;   /* 🔥 phần sáng */
            border-radius: 3px;
        }

        QSlider::add-page:horizontal {
            background: #1f2937;
            border-radius: 3px;
        }

        QSlider::handle:horizontal {
            background: #7dd3fc;
            border: 2px solid #38bdf8;
            width: 14px;
            height: 14px;
            margin: -5px 0;   /* canh giữa */
            border-radius: 7px;
        }

        /* ===== DISABLED STATE ===== */

        QPushButton:disabled {
            background: #1a1f2a;
            color: #6b7280;
            border: 1px solid #2a3140;
        }

        QLineEdit:disabled {
            background: #161b24;
            color: #6b7280;
            border: 1px solid #2a3140;
        }

        QComboBox:disabled {
            background: #161b24;
            color: #6b7280;
            border: 1px solid #2a3140;
        }

        QSlider:disabled {
            background: #161b24;
        }

        #togglePanelButton {

        background:
            rgba(20,23,30,0.92);

        border:
            1px solid
            rgba(56,189,248,0.18);

        border-left:none;

        border-top-right-radius:0px;
        border-bottom-right-radius:0px;

        border-top-left-radius:14px;
        border-bottom-left-radius:14px;

        color:#e2e8f0;

        font-size:20px;

        font-weight:700;
        }

        #togglePanelButton:hover {

        background:
            rgba(30,35,45,0.96);

        border:
            1px solid
            rgba(56,189,248,0.45);

        border-left:none;
        }

    )";

    setStyleSheet(globalStyle);

    // ===== OBJECT NAME =====
    controlPanel->setObjectName("controlPanel");
    infoPanel->setObjectName("infoPanel");

    // ===== FONT APPLY =====
    generateButton->setFont(buttonFont);
    runButton->setFont(buttonFont);
    stepButton->setFont(buttonFont);
    resetButton->setFont(buttonFont);

    clickModeButton->setFont(buttonFont);
    inputModeButton->setFont(buttonFont);
    addPointButton->setFont(buttonFont);

    algorithmCombo->setFont(labelFont);

    costLabel->setFont(labelFont);
    timeLabel->setFont(labelFont);
    stepLabel->setFont(labelFont);
    speedLabel->setFont(titleFont);

    // ===== SPEED LABEL =====
    connect(speedSlider, &QSlider::valueChanged, this, [this](int value) {
        double speed = 0.5 + 0.5 * (value - 1);
        speedLabel->setText(tr("Speed: %1x").arg(speed, 0, 'f', 1));
    });

    // ===== GLOW EFFECT =====
    applyGlowEffect(generateButton);
    applyGlowEffect(runButton);
    applyGlowEffect(stepButton);
    applyGlowEffect(resetButton);

    applyGlowEffect(clickModeButton);
    applyGlowEffect(inputModeButton);
    applyGlowEffect(addPointButton);
}


void MainWindow::applyGlowEffect(QPushButton* btn)
{
    auto glow = new QGraphicsDropShadowEffect(btn);
    glow->setBlurRadius(8);
    glow->setColor(QColor(90, 159, 191, 120));
    glow->setOffset(0);

    btn->setGraphicsEffect(glow);

    // Animation: pulse glow
    auto anim = new QPropertyAnimation(glow, "blurRadius", btn);
    anim->setDuration(220);
    anim->setStartValue(8);
    anim->setKeyValueAt(0.5, 28);  // 👈 peak glow
    anim->setEndValue(8);
    anim->setEasingCurve(QEasingCurve::OutCubic);

    connect(btn, &QPushButton::pressed, [anim]() {
        anim->stop();
        anim->start();
    });



}


void MainWindow::resizeEvent(QResizeEvent *event)
{

    if (comparePopup)
    {
        comparePopup->move(
            width() / 2 - comparePopup->width() / 2,
            height() / 2 - comparePopup->height() / 2
            );
    }

    QMainWindow::resizeEvent(event);

    const int margin = 20;

    // ===== PSEUDO POPUP (bottom-right) =====
    if (pseudoPopup)
    {
        int w = pseudoPopup->width();
        int h = pseudoPopup->height();

        int x = width() - w - margin;
        int y = height() - h - 100;

        pseudoPopup->move(x, y);
    }

    // ===== DP POPUP (above pseudo popup) =====
    if (dpPopup)
    {
        int w = dpPopup->width();
        int h = dpPopup->height();

        int x = width() - w - margin;

        int y =
            height()
            - h
            - pseudoPopup->height()
            - 120;

        dpPopup->move(x, y);
    }

    // ===== LEGEND BAR (top-center) =====
    if (legendBar)
    {
        int w = 420;
        int h = 40;

        int x = (width() - w) / 2;
        int y = margin;

        legendBar->setGeometry(x, y, w, h);
        legendBar->raise();
    }

    if (togglePanelButton)
    {
        int centerY =
            height()/2
            - togglePanelButton->height()/2;

        if (panelCollapsed)
        {
            togglePanelButton->move(
                width() - 52,
                centerY
                );
        }
        else
        {
            togglePanelButton->move(
                width() - 400,
                centerY
                );
        }
    }

    if (aboutPopup)
    {
        int x =
            (width()
             - aboutPopup->width()) / 2;

        int y =
            (height()
             - aboutPopup->height()) / 2;

        aboutPopup->move(x, y);
    }

}

void MainWindow::setInputUIEnabled(bool enabled)
{
    generateButton->setEnabled(enabled);

    clickModeButton->setEnabled(enabled);
    inputModeButton->setEnabled(enabled);

    inputX->setEnabled(enabled);
    inputY->setEnabled(enabled);
    addPointButton->setEnabled(enabled);

    algorithmCombo->setEnabled(enabled); // 🔥 QUAN TRỌNG
}


void MainWindow::updateInputModeUI()
{
    bool isClick = clickModeButton->isChecked();
    bool isManual = inputModeButton->isChecked();

    // manual form
    inputX->setEnabled(isManual);
    inputY->setEnabled(isManual);
    addPointButton->setEnabled(isManual);

    // canvas click
    canvasWidget->setEnabled(isClick); // 🔥 chỉ click mode mới click được
}