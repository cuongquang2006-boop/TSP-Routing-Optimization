#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "tsp_steps.h"
#include "tsp_algorithms.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QComboBox;
class QPushButton;
class QSlider;
class QLabel;
class GraphCanvas;
class QVBoxLayout;
class QHBoxLayout;
class QWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

signals:
    void stepChanged(int currentIndex, int totalSteps);
    void playStateChanged(bool isPlaying);
    void algorithmRunning(bool running);

private slots:
    void onGenerateClicked();
    void onRunClicked();
    void onStepClicked();
    void onResetClicked();
    void onAlgorithmChanged(int index);
    void onSpeedChanged(int value);
    void onTimerTimeout();

private:
    // Setup functions
    void setupWidgets();
    void setupLayouts();
    void setupStyles();
    void setupConnections();

    // Animation engine functions
    void loadSteps();
    void applyStep(const Step &step);
    void updateInfoPanel();
    void play();
    void pause();
    void nextStep();
    void reset();

    Ui::MainWindow *ui;

    // Main widgets
    GraphCanvas *canvasWidget;

    QWidget *controlPanel;
    QComboBox *algorithmCombo;
    QPushButton *generateButton;
    QPushButton *runButton;
    QPushButton *stepButton;
    QPushButton *resetButton;
    QSlider *speedSlider;
    QLabel *speedLabel;

    QWidget *infoPanel;
    QLabel *costLabel;
    QLabel *timeLabel;
    QLabel *stepLabel;

    QVBoxLayout *mainLayout;
    QHBoxLayout *contentLayout;

    // Animation engine data
    QVector<QPointF> points;
    QVector<Step> steps;
    int currentStepIndex;
    TSPAlgorithm currentAlgorithm;
    QTimer *animationTimer;
    bool isPlaying;
};

#endif