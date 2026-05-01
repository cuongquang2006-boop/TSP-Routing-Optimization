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

    resize(1000,900);

    if (!centralWidget()) {
        setCentralWidget(new QWidget(this));
    }

    setupWidgets();
    setupLayouts();
    setupStyles();
    setupConnections();

    animationTimer = new QTimer(this);
    connect(animationTimer, &QTimer::timeout, this, &MainWindow::onTimerTimeout);

    speedSlider->setValue(5);

    canvasWidget->setInputMode(InputMode::CLICK);

    updateInputModeUI();

    canvasWidget->setInputMode(InputMode::NONE);
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

    legendBar = new QWidget(this);
    legendBar->setObjectName("legendBar");

    legendBar->setFixedHeight(40);

    modeLabel = new QLabel("Input Mode", controlPanel);

    clickModeButton = new QPushButton("Click", controlPanel);
    inputModeButton = new QPushButton("Manual", controlPanel);

    inputX = new QLineEdit(controlPanel);
    inputY = new QLineEdit(controlPanel);

    inputX->setPlaceholderText("X");
    inputY->setPlaceholderText("Y");

    addPointButton = new QPushButton("Add Point", controlPanel);

    pseudoButton = new QPushButton(tr("PSEUDO CODE"), controlPanel);
    pseudoButton->setObjectName("pseudoButton");
    pseudoButton->setMinimumHeight(36);

    pseudoPopup = new PseudoPopup(this);
    pseudoPopup->hide();

    infoPanel = new QWidget(this);
    costLabel = new QLabel(tr("Cost: -"), infoPanel);
    timeLabel = new QLabel(tr("Time: -"), infoPanel);
    stepLabel = new QLabel(tr("Step: -"), infoPanel);

    clickModeButton->setCheckable(true);
    inputModeButton->setCheckable(true);
}

void MainWindow::setupLayouts()
{
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

    QHBoxLayout *modeLayout = new QHBoxLayout;
    modeLayout->addWidget(clickModeButton);
    modeLayout->addWidget(inputModeButton);
    controlLayout->addLayout(modeLayout);

    QHBoxLayout *inputLayout = new QHBoxLayout;
    inputLayout->addWidget(inputX);
    inputLayout->addWidget(inputY);
    controlLayout->addLayout(inputLayout);

    controlLayout->addWidget(addPointButton);

    controlLayout->addStretch(1);

    controlPanel->setMinimumWidth(260);
    contentLayout->addWidget(controlPanel, 3);

    mainLayout->addLayout(contentLayout, 1);

    pseudoPopup->setParent(this);
    pseudoPopup->raise();
    pseudoPopup->hide();

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

    connect(pseudoButton, &QPushButton::clicked, this, [this]() {
        if (pseudoPopup) pseudoPopup->toggle();
    });

    connect(animationTimer, &QTimer::timeout, this, &MainWindow::onTimerTimeout);

    connect(clickModeButton, &QPushButton::clicked, this, [this]() {
        canvasWidget->setInputMode(InputMode::CLICK);
        clickModeButton->setChecked(true);
        inputModeButton->setChecked(false);

        updateInputModeUI();
    });

    connect(inputModeButton, &QPushButton::clicked, this, [this]() {
        canvasWidget->setInputMode(InputMode::MANUAL);
        clickModeButton->setChecked(false);
        inputModeButton->setChecked(true);

        updateInputModeUI();
    });

    connect(canvasWidget, &GraphCanvas::pointAdded, this, [this](QPointF normalizedPos) {

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
}

void MainWindow::setupStyles()
{
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
            border-radius: 4px;
            padding: 2px 4px;
        }

        QLabel#sectionLabel {
            font-weight: 600;
            font-size: 12px;
            background: #1f242f;
            padding: 4px 8px;
        }

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

        QGraphicsView {
            border: 1px solid #2d3340;
            border-radius: 12px;
            background: #171a20;
        }

        QSlider::sub-page:horizontal {
            background: #38bdf8;
            border-radius: 3px;
        }

        QSlider::add-page:horizontal {
            background: #1f2937;
            border-radius: 3px;
        }

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
    )";

    setStyleSheet(globalStyle);

    controlPanel->setObjectName("controlPanel");
    infoPanel->setObjectName("infoPanel");

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

    connect(speedSlider, &QSlider::valueChanged, this, [this](int value)
    {
        double speed = 0.5 + 0.5 * (value - 1);
        speedLabel->setText(tr("Speed: %1x").arg(speed, 0, 'f', 1));
    });

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

    auto anim = new QPropertyAnimation(glow, "blurRadius", btn);
    anim->setDuration(220);
    anim->setStartValue(8);
    anim->setKeyValueAt(0.5, 28);
    anim->setEndValue(8);
    anim->setEasingCurve(QEasingCurve::OutCubic);

    connect(btn, &QPushButton::pressed, [anim]()
    {
        anim->stop();
        anim->start();
    });
}


void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    const int margin = 20;

    if (pseudoPopup)
    {
        int w = pseudoPopup->width();
        int h = pseudoPopup->height();

        int x = width() - w - margin;
        int y = height() - h - 100;

        pseudoPopup->move(x, y);
    }
    if (legendBar)
    {
        int w = 420;
        int h = 40;

        int x = (width() - w) / 2;
        int y = margin;

        legendBar->setGeometry(x, y, w, h);
        legendBar->raise();
    }
}