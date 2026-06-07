#pragma once
#include <QMainWindow>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QLabel>
#include <QButtonGroup>
#include <QScrollArea>
#include "qtcanvaswidget.h"
#include "domain.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    void createCanvas(int width, int height);    

private slots:
    void onToolButtonClicked(QAbstractButton* btn);
    void onShapeButtonClicked(QAbstractButton* btn);
    void onToolClicked(int id);
    void onShapeClicked(int id);
    void onBrushSizeChanged(int size);
    void onColorChanged(const QColor& color);
    void onRgbChanged();
    void onSaveClicked();
    void onLoadClicked();
    void onClearClicked();
    void adjustBrightness(int value);
    void onNewCanvasClicked();

private:
    ICanvas* canvas;
    QtCanvasWidget* canvasWidget;
    QScrollArea* scrollArea;
    
    QButtonGroup* toolGroup;
    QButtonGroup* shapeGroup;
    
    QPushButton* currentColorBtn;
    QSpinBox* rSpin;
    QSpinBox* gSpin;
    QSpinBox* bSpin;
    QSlider* brightnessSlider;
    QColor activeColor;
    
    QSlider* brushSizeSlider;
    QSpinBox* brushSizeSpinBox;

    QLabel* canvasSizeLabel;
    
    void updateActiveToolStyle(QAbstractButton* btn);
    void updateCanvasSizeLabel();
};