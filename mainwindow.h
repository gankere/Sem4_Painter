#pragma once
#include <QMainWindow>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QButtonGroup>
#include "qtcanvaswidget.h"
#include "domain.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(ICanvas& canvas, QWidget* parent = nullptr);

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

private:
    ICanvas& canvas;
    QtCanvasWidget* canvasWidget;
    
    QButtonGroup* toolGroup;
    QButtonGroup* shapeGroup;
    
    QPushButton* currentColorBtn;
    QSpinBox* rSpin;
    QSpinBox* gSpin;
    QSpinBox* bSpin;
    QSlider* brightnessSlider;
    QColor activeColor;
    
    QSlider* brushSizeSlider;
    
    void updateActiveToolStyle(QAbstractButton* btn);
};