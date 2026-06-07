#pragma once
#include <QWidget>
#include <QPixmap>
#include <QList>
#include <QString>
#include "domain.h"

class QtCanvasWidget : public QWidget {
    Q_OBJECT
public:
    explicit QtCanvasWidget(ICanvas* canvas = nullptr, QWidget* parent = nullptr);
    ~QtCanvasWidget();
    int getCanvasWidth() const;
    int getCanvasHeight() const;

    void updateFactory(IToolFactory* factory);
    void clearCanvas();
    
    void setActiveColor(const QColor& color);
    void setBrushSize(int size);
    void setPixelSize(int size);
    void updateCanvasSize();
    void showTextDialog(const QPoint& pos);
    void setCanvas(ICanvas& newCanvas);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

signals:
    void pixelClicked(int x, int y);

private:
    struct TextItem {
        QString text;
        QPoint pos;
        QColor color;
    };
    
    ICanvas* canvas;
    IToolFactory* currentFactory;
    ITool* activeTool;
    bool isDrawing;
    QPoint lastPos;
    int pixelSize;
    QColor activeToolColor;
    int brushSize;

    QPixmap canvasCache;
    bool cacheDirty;
    void updateCache();

    void drawAtPosition(const QPoint& pos);
    void drawLine(const QPoint& from, const QPoint& to);
    void drawDirectlyOnCache(int canvasX, int canvasY, const QColor& color, int brushSize);
    
    // Для фигур
    QPoint shapeStartCanvas;
    
    // Для preview
    QPixmap previewCache;
    bool hasPreview;
    void drawPreview(const QPoint& startCanvas, const QPoint& currentCanvas);
    void clearPreview();
    
    QList<TextItem> textItems;
    void eraseTextAtCanvasPos(int canvasX, int canvasY);

    void updateCursor();
};