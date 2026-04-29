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
    ui->setupUi(this);

    if (!centralWidget())
    {
        setCentralWidget(new QWidget(this));
    }

    setupWidgets();
    setupLayouts();
    setupStyles();
    setupConnections();

    animationTimer = new QTimer(this);
    connect(animationTimer, &QTimer::timeout, this, &MainWindow::onTimerTimeout);

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

    pseudoButton = new QPushButton(tr("PSEUDO CODE"), controlPanel);
    pseudoButton->setObjectName("pseudoButton");
    pseudoButton->setMinimumHeight(36);

    pseudoPopup = new PseudoPopup(this);
    pseudoPopup->hide();

    infoPanel = new QWidget(this);
    costLabel = new QLabel(tr("Cost: -"), infoPanel);
    timeLabel = new QLabel(tr("Time: -"), infoPanel);
    stepLabel = new QLabel(tr("Step: -"), infoPanel);
}

void MainWindow::setupLayouts()
{
    QWidget *central = centralWidget();
    if (!central)
    {
        central = new QWidget(this);
        setCentralWidget(central);
    }

    mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    contentLayout = new QHBoxLayout;
    contentLayout->setSpacing(12);

    canvasWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    contentLayout->addWidget(canvasWidget, 7);

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

    mainLayout->addWidget(pseudoPopup);
    pseudoPopup->hide();

    QHBoxLayout *infoLayout = new QHBoxLayout(infoPanel);
    infoLayout->setContentsMargins(14, 10, 14, 10);
    infoLayout->setSpacing(24);

    infoLayout->addWidget(costLabel);
    infoLayout->addWidget(timeLabel);
    infoLayout->addWidget(stepLabel);
    infoLayout->addStretch(1);
    infoLayout->addWidget(pseudoButton);

    infoPanel->setFixedHeight(80);
    mainLayout->addWidget(infoPanel, 0);
}

void MainWindow::setupConnections()
{
    connect(generateButton, &QPushButton::clicked, this, &MainWindow::onGenerateClicked);
    connect(runButton, &QPushButton::clicked, this, &MainWindow::onRunClicked);
    connect(stepButton, &QPushButton::clicked, this, &MainWindow::onStepClicked);
    connect(resetButton, &QPushButton::clicked, this, &MainWindow::onResetClicked);

    connect(algorithmCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onAlgorithmChanged);
    connect(speedSlider, &QSlider::valueChanged, this, &MainWindow::onSpeedChanged);

    connect(pseudoButton, &QPushButton::clicked, this, [this]()
    {
        if (pseudoPopup) pseudoPopup->toggle();
    });

    connect(animationTimer, &QTimer::timeout, this, &MainWindow::onTimerTimeout);
}

void MainWindow::onGenerateClicked()
{
    if (isPlaying)
    {
        pause();
    }

    int nodeCount;

    if (currentAlgorithm == TSPAlgorithm::BRANCH_AND_BOUND)
    {
        nodeCount = QRandomGenerator::global()->bounded(6, 9);
    }
    else
    {
        nodeCount = QRandomGenerator::global()->bounded(7, 11);
    }

    if (currentAlgorithm == TSPAlgorithm::BRANCH_AND_BOUND && nodeCount > 10)
    {
        QMessageBox::warning(this,
                             "Branch & Bound Limit",
                             "Branch & Bound has factorial complexity (O(n!)) and becomes very slow with many nodes.\n\n"
                             "Recommended maximum: 10 nodes.\n"
                             "The system will automatically reduce the number to 10 to prevent freezing.");

        nodeCount = 10;
    }

    canvasWidget->generateRandomPoints(nodeCount);

    points.clear();
    const QRect canvasRect = canvasWidget->contentsRect().adjusted(12, 12, -12, -12);

    for (int i = 0; i < nodeCount; ++i)
    {
        qreal x = canvasRect.left() + (i % 4) * 150;
        qreal y = canvasRect.top() + (i / 4) * 150;
        points.append(QPointF(x, y));
    }

    loadSteps();
    emit algorithmRunning(false);
}