#include "mainwindow.h"
#include "GraphCanvas.h"
#include "tsp_algorithms.h"
#include "pseudopopup.h"

#include <QMessageBox>
#include <QComboBox>
#include <QRandomGenerator>
#include <QSlider>
#include <QPushButton>
#include <QTimer>
#include <algorithm>

void MainWindow::onGenerateClicked()
{
    if (isPlaying) {
        pause();
    }

    int nodeCount;

    if (currentAlgorithm == TSPAlgorithm::BRANCH_AND_BOUND) {
        nodeCount = QRandomGenerator::global()->bounded(6, 9);
    }
    else
    {
        nodeCount = QRandomGenerator::global()->bounded(7, 11);
    }

    if (currentAlgorithm == TSPAlgorithm::BRANCH_AND_BOUND && nodeCount > 10) {
        QMessageBox::warning(this,
                             "Branch & Bound Limit",
                             "Branch & Bound has factorial complexity (O(n!)) and becomes very slow with many nodes.\n\n"
                             "Recommended maximum: 10 nodes.\n"
                             "The system will automatically reduce the number to 10 to prevent freezing.");

        nodeCount = 10;
    }

    canvasWidget->clearPoints();
    canvasWidget->generateRandomPoints(nodeCount);

    points.clear();

    auto canvasPoints = canvasWidget->getPoints();

    for (const auto& p : canvasPoints) {
        points.append(p);
    }

    loadSteps();
    emit algorithmRunning(false);
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

    setInputUIEnabled(false);

    nextStep();
}

void MainWindow::onResetClicked()
{
    reset();
}

void MainWindow::onAlgorithmChanged(int index)
{
    if (currentAlgorithm == TSPAlgorithm::BRANCH_AND_BOUND)
    {
        canvasWidget->setMaxNodeLimit(5);
    }
    else
    {
        canvasWidget->setMaxNodeLimit(10);
    }

    if (currentAlgorithm == TSPAlgorithm::BRANCH_AND_BOUND && points.size() > 10) {
        QMessageBox::information(this,
                                 "Branch & Bound Limit",
                                 "The current number of nodes is too large for Branch & Bound.\n\n"
                                 " Recommended limit: ≤ 10 nodes.\n"
                                 " Please generate fewer nodes or choose another algorithm.");

        return;
    }

    if (isPlaying) {
        pause();
    }

    currentAlgorithm = static_cast<TSPAlgorithm>(index);

    currentStepIndex = 0;
    steps.clear();

    if (!points.isEmpty()) {
        loadSteps();
    }

    if (pseudoPopup)
        pseudoPopup->setAlgorithm(currentAlgorithm);
}

void MainWindow::onSpeedChanged(int value)
{
    double speed = 0.5 + (value - 1) * 0.2;
    speedLabel->setText(tr("Speed: %1x").arg(speed, 0, 'f', 1));

    qreal adaptiveFPS = 24.0 + (value - 1) * 4.0;

    int interval = static_cast<int>(1000.0 / adaptiveFPS);

    interval = std::clamp(interval, 16, 50);

    animationTimer->setInterval(interval);
}

void MainWindow::onTimerTimeout()
{
    if (currentStepIndex == 0) {
        nextStep();
        return;
    }

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

    steps.clear();
    currentStepIndex = 0;

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
        reset();
    }

    isPlaying = true;

    setInputUIEnabled(false);

    runButton->setText(tr("Pause"));
    emit playStateChanged(true);

    int speedValue = speedSlider->value();
    qreal adaptiveFPS = 30.0 + (speedValue - 1) * 7.5;
    int interval = static_cast<int>(1000.0 / adaptiveFPS);
    interval = std::max(11, interval);
    animationTimer->start(interval);
}

void MainWindow::pause()
{
    if (animationTimer->isActive())
    {
        animationTimer->stop();
    }

    isPlaying = false;

    runButton->setText(tr("Run"));
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

    if (currentStepIndex >= steps.size())
    {
        if (animationTimer->isActive())
            animationTimer->stop();

        isPlaying = false;

        setInputUIEnabled(true);
        updateInputModeUI();

        runButton->setText(tr("Run"));
    }
}

void MainWindow::reset()
{
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
    updateInputModeUI();
}

void MainWindow::applyStep(const Step &step)
{
    int animationDuration = step.getAnimationDuration();

    qreal speedValue = speedSlider->value();

    qreal speedMultiplier = 0.3 + speedValue * 0.15;

    canvasWidget->setAnimationParams(animationDuration, speedMultiplier);

    canvasWidget->setStep(step);

    if (pseudoPopup) {
        pseudoPopup->setAlgorithm(currentAlgorithm);
        pseudoPopup->updateForStep(step);
    }
}

void MainWindow::updateInfoPanel()
{
    if (currentStepIndex > 0 && currentStepIndex <= steps.size()) {
        const Step &step = steps[currentStepIndex - 1];

        costLabel->setText(tr("Cost: %1").arg(step.value, 0, 'f', 2));

        stepLabel->setText(tr("Step: %1/%2").arg(currentStepIndex).arg(steps.size()));

        QString actionStr = stepActionToString(step.action);
        timeLabel->setText(tr("Action: %1").arg(actionStr));
    } else {
        costLabel->setText(tr("Cost: -"));
        stepLabel->setText(tr("Step: -"));
        timeLabel->setText(tr("Action: -"));
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

    algorithmCombo->setEnabled(enabled);
}

void MainWindow::updateInputModeUI()
{
    bool isClick = clickModeButton->isChecked();
    bool isManual = inputModeButton->isChecked();

    inputX->setEnabled(isManual);
    inputY->setEnabled(isManual);
    addPointButton->setEnabled(isManual);

    canvasWidget->setEnabled(isClick);
}
