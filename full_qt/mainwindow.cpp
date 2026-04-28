#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "GraphCanvas.h"
#include "tsp_algorithms.h"

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
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupWidgets()
{
    canvasWidget = new GraphCanvas(this);

    controlPanel = new QWidget(this);
    algorithmCombo = new QComboBox(controlPanel);
    algorithmCombo->addItems({"Random", "Greedy", "Nearest Insertion", "Branch & Bound"});
    generateButton = new QPushButton(tr("Generate Random Points"), controlPanel);
    runButton = new QPushButton(tr("Run"), controlPanel);
    stepButton = new QPushButton(tr("Step"), controlPanel);
    resetButton = new QPushButton(tr("Reset"), controlPanel);
    speedSlider = new QSlider(Qt::Horizontal, controlPanel);
    speedSlider->setRange(1, 10);
    speedSlider->setValue(5);
    speedLabel = new QLabel(tr("Speed: 2.5x"), controlPanel);

    infoPanel = new QWidget(this);
    costLabel = new QLabel(tr("Cost: -"), infoPanel);
    timeLabel = new QLabel(tr("Time: -"), infoPanel);
    stepLabel = new QLabel(tr("Step: -"), infoPanel);
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
    controlLayout->addWidget(stepButton);
    controlLayout->addWidget(resetButton);
    controlLayout->addSpacing(8);
    controlLayout->addWidget(speedLabel);
    controlLayout->addWidget(speedSlider);
    controlLayout->addStretch(1);

    controlPanel->setMinimumWidth(260);
    contentLayout->addWidget(controlPanel, 3);

    mainLayout->addLayout(contentLayout, 1);

    // Bottom info panel
    QHBoxLayout *infoLayout = new QHBoxLayout(infoPanel);
    infoLayout->setContentsMargins(14, 10, 14, 10);
    infoLayout->setSpacing(24);
    infoLayout->addWidget(costLabel);
    infoLayout->addWidget(timeLabel);
    infoLayout->addWidget(stepLabel);
    infoLayout->addStretch(1);

    infoPanel->setFixedHeight(80);
    mainLayout->addWidget(infoPanel, 0);
}

void MainWindow::setupConnections()
{
    // Button signals
    connect(generateButton, &QPushButton::clicked, this, &MainWindow::onGenerateClicked);
    connect(runButton, &QPushButton::clicked, this, &MainWindow::onRunClicked);
    connect(stepButton, &QPushButton::clicked, this, &MainWindow::onStepClicked);
    connect(resetButton, &QPushButton::clicked, this, &MainWindow::onResetClicked);

    // ComboBox & Slider signals
    connect(algorithmCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onAlgorithmChanged);
    connect(speedSlider, &QSlider::valueChanged, this, &MainWindow::onSpeedChanged);

    // Animation timer signal
    connect(animationTimer, &QTimer::timeout, this, &MainWindow::onTimerTimeout);
}

void MainWindow::onGenerateClicked()
{
    // Pause animation if running
    if (isPlaying) {
        pause();
    }

    // Generate random points on canvas
    canvasWidget->generateRandomPoints(12);

    // Create point positions for algorithm
    points.clear();
    const QRect canvasRect = canvasWidget->contentsRect().adjusted(12, 12, -12, -12);
    for (int i = 0; i < 12; ++i) {
        qreal x = canvasRect.left() + (i % 4) * 150;
        qreal y = canvasRect.top() + (i / 4) * 150;
        points.append(QPointF(x, y));
    }

    // Load algorithm steps
    loadSteps();
    emit algorithmRunning(false);
}

void MainWindow::onRunClicked()
{
    if (isPlaying) {
        pause();
    } else {
        play();
    }
}

void MainWindow::onStepClicked()
{
    nextStep();
}

void MainWindow::onResetClicked()
{
    reset();
}

void MainWindow::onAlgorithmChanged(int index)
{
    if (isPlaying) {
        pause();
    }
    currentAlgorithm = static_cast<TSPAlgorithm>(index);
    reset();
    if (!steps.isEmpty()) {
        loadSteps();
    }
}

void MainWindow::onSpeedChanged(int value)
{
    double speed = 0.5 + 0.5 * (value - 1);
    speedLabel->setText(tr("Speed: %1x").arg(speed, 0, 'f', 1));

    // Update timer interval if playing
    if (isPlaying && animationTimer->isActive()) {
        int interval = std::max(50, 1000 / (value + 1));
        animationTimer->setInterval(interval);
    }
}

void MainWindow::onTimerTimeout()
{
    // Auto-advance to next step
    nextStep();

    // Stop timer when reached end
    if (currentStepIndex >= steps.size()) {
        pause();
    }
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
    runButton->setText(tr("Pause"));
    emit playStateChanged(true);

    // Start timer with speed-controlled interval
    int speedValue = speedSlider->value();
    int interval = std::max(50, 1000 / (speedValue + 1));
    animationTimer->start(interval);
}

void MainWindow::pause()
{
    if (animationTimer->isActive()) {
        animationTimer->stop();
    }

    isPlaying = false;
    runButton->setText(tr("Run"));
    emit playStateChanged(false);
}

void MainWindow::nextStep()
{
    if (currentStepIndex < steps.size()) {
        // Apply current step to canvas
        applyStep(steps[currentStepIndex]);
        currentStepIndex++;

        // Update UI
        updateInfoPanel();
        emit stepChanged(currentStepIndex, steps.size());
    }
}

void MainWindow::reset()
{
    // Stop animation
    if (isPlaying) {
        pause();
    }

    // Reset state
    currentStepIndex = 0;
    canvasWidget->clearPoints();
    updateInfoPanel();
    emit stepChanged(0, steps.size());
    emit algorithmRunning(false);
}

void MainWindow::applyStep(const Step &step)
{
    // Update canvas visualization with step data:
    // - tour: current TSP tour nodes
    // - currentNode: node being processed (highlighted blue)
    // - bestCandidate: best node found so far (highlighted green)
    // - edge: edge being compared (highlighted yellow/red)
    canvasWidget->setTour(step.tour, step.currentNode, step.bestCandidate, step.edge);
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
    // Setup Tahoma bold font for buttons and labels
    QFont buttonFont("Tahoma", 10);
    buttonFont.setBold(true);
    
    QFont labelFont("Tahoma", 11);
    QFont titleFont("Tahoma", 12);
    titleFont.setBold(true);

    setWindowTitle(tr("TSP Visualizer"));
    resize(1280, 860);

    QString globalStyle = R"(
        QMainWindow {
            background: #121417;
        }
        QWidget#centralwidget, QWidget {
            background: #121417;
            color: #e5e7eb;
        }
        QGraphicsView {
            border: 1px solid #2d3340;
            border-radius: 12px;
            background: #171a20;
        }
        QWidget {
            border-radius: 12px;
        }
        QWidget#controlPanel, QWidget#infoPanel {
            background: #191c24;
            border: 1px solid #2f3747;
        }
        QLabel {
            color: #e5e7eb;
            font-size: 13px;
            font-family: Tahoma;
        }
        QLabel#sectionLabel {
            font-weight: 600;
            margin-bottom: 4px;
            font-size: 12px;
        }
        QComboBox, QSlider {
            background: #1f242f;
            border: 1px solid #323a48;
            color: #f8fafc;
            min-height: 36px;
            border-radius: 10px;
            font-family: Tahoma;
            font-size: 11px;
        }
        QPushButton {
            background: #1f242f;
            border: 1px solid #323a48;
            color: #f8fafc;
            min-height: 36px;
            border-radius: 10px;
            font-family: Tahoma;
            font-weight: bold;
            font-size: 10px;
            padding: 8px 12px;
        }
        QPushButton:hover {
            background: #253046;
            border: 1px solid #3d4a5c;
            color: #ffffff;
        }
        QPushButton:pressed {
            background: #1a1f2e;
            border: 1px solid #2d3340;
            padding: 10px 10px 6px 14px;
        }
        QComboBox {
            padding-right: 46px;
            border-radius: 14px;
        }

        QComboBox {
            border-radius: 12px;
            padding-right: 46px;
        }

        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 44px;

            background: #253046;

            border-top-right-radius: 12px;
            border-bottom-right-radius: 12px;

            border-left: none;
            margin: 0px;
        }

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
    )";

    setStyleSheet(globalStyle);
    controlPanel->setObjectName("controlPanel");
    infoPanel->setObjectName("infoPanel");

    // Apply Tahoma font to buttons
    generateButton->setFont(buttonFont);
    runButton->setFont(buttonFont);
    stepButton->setFont(buttonFont);
    resetButton->setFont(buttonFont);
    algorithmCombo->setFont(labelFont);

    // Apply font to labels
    costLabel->setFont(labelFont);
    timeLabel->setFont(labelFont);
    stepLabel->setFont(labelFont);
    speedLabel->setFont(titleFont);

    connect(speedSlider, &QSlider::valueChanged, this, [this](int value) {
        double speed = 0.5 + 0.5 * (value - 1);
        speedLabel->setText(tr("Speed: %1x").arg(speed, 0, 'f', 1));
    });
}
