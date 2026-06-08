#pragma once
#include <QWidget>
#include <QPixmap>
#include <QList>
#include <QString>
#include <QVector>
#include <QPoint>
#include "domain.h"

class QtCanvasWidget : public QWidget {
    Q_OBJECT

public:
    explicit QtCanvasWidget(ICanvas* canvas = nullptr, QWidget* parent = nullptr);
    ~QtCanvasWidget();

    // Canvas info
    int getCanvasWidth() const;
    int getCanvasHeight() const;
    void setCanvas(ICanvas& newCanvas);
    void updateCanvasSize();

    // Tools
    void updateFactory(IToolFactory* factory);
    void setActiveColor(const QColor& color);
    void setBrushSize(int size);
    void setPixelSize(int size);
    void updateCursor();

    void undoText();

    // Canvas operations
    void clearCanvas();
    void showTextDialog(const QPoint& pos);
    void setCacheDirty() { cacheDirty = true; }
    void setCacheDirty(bool val) { cacheDirty = val; }

    // Static mask generators
    static QVector<QPoint> getBrushMask(int brushSize);
    static QVector<QPoint> getEraserMask(int brushSize);

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

    // Кеш
    QPixmap canvasCache;
    bool cacheDirty;

    // Для фигур
    QPoint shapeStartCanvas;
    
    // Для preview    
    QImage previewImage;
    bool hasPreview;
    
    // Текст
    QList<TextItem> textItems;
    QList<QList<TextItem>> textUndoStack;  // Стек снимков текста для undo

    // === PRIVATE METHODS ===
    void updateCache();
    void drawAtPosition(const QPoint& pos);
    void drawLine(const QPoint& from, const QPoint& to);
    void drawDirectlyOnCache(int canvasX, int canvasY, QRgb color, int brushSize);
    
    void drawPreview(const QPoint& startCanvas, const QPoint& currentCanvas);
    void clearPreview();
    
    void eraseTextAtCanvasPos(int canvasX, int canvasY);
    
    // Text undo
    void saveTextState();
    void restoreTextState();
};