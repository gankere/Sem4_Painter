#pragma once
#include <queue> 
#include <vector>
#include <string>
#include <stack>
#include <QColor>
#include <QPoint>
#include <QString>
#include <QImage>

// ============================================
// СТРУКТУРЫ ДАННЫХ
// ============================================

struct UndoStep {
    int x, y;
    QRgb previousColor;
    bool isBatchMarker;
    
    UndoStep(int x_ = 0, int y_ = 0, QRgb prev = 0, bool marker = false)
        : x(x_), y(y_), previousColor(prev), isBatchMarker(marker) {}
};

struct Pixel {
    QRgb color;
    
    Pixel() : color(qRgba(0, 0, 0, 0)) {}
    Pixel(QRgb c) : color(c) {}
    Pixel(QColor c) : color(c.rgba()) {}
    
    bool isEmpty() const { return qAlpha(color) == 0; }
};

// ============================================
// CANVAS
// ============================================

class ICanvas {
public:
    virtual ~ICanvas() = default;
    virtual void setPixel(int x, int y, Pixel p) = 0;
    virtual Pixel getPixel(int x, int y) const = 0;
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
    virtual void clear() = 0;
    virtual void undo() = 0;
    virtual void startBatch() = 0;
    virtual void endBatch() = 0;
    virtual void loadFromImage(const QImage& image, int x, int y, int width, int height) = 0;
    virtual int getUndoHistorySize() const = 0;
};

class Canvas : public ICanvas {
private:
    std::vector<Pixel> data;
    int w, h;
    std::stack<UndoStep, std::vector<UndoStep>> undoHistory;
    bool isUndoing;
    bool isBatching;
    int batchCount = 0; // Счётчик действий в истории
    static const int MAX_BATCHES = 500; // Максимум 500 действий в истории
    
public:
    Canvas(int width, int height);
    
    void setPixel(int x, int y, Pixel p) override;
    Pixel getPixel(int x, int y) const override;
    int getWidth() const override;
    int getHeight() const override;
    void clear() override;
    void undo() override;
    void startBatch() override;
    void endBatch() override;
    void loadFromImage(const QImage& image, int x, int y, int width, int height) override;
    int getUndoHistorySize() const override { return undoHistory.size(); }

    // Методы для оптимизации заливки:
    Pixel* getData() { return data.data(); }
    bool getIsUndoing() const { return isUndoing; }
    void setIsUndoing(bool val) { isUndoing = val; }
};

// ============================================
// ИНСТРУМЕНТЫ
// ============================================

class ITool {
public:
    virtual ~ITool() = default;
    virtual void use(ICanvas& canvas, int x, int y) = 0;
    virtual std::string getToolName() const = 0;
    virtual void setColor(const QColor& color) = 0;
    virtual void setSize(int size) = 0;
};

class BrushTool : public ITool {
private:
    QColor color;
    int size;
    
public:
    BrushTool(const QColor& c = Qt::black, int s = 1);
    void use(ICanvas& canvas, int x, int y) override;
    std::string getToolName() const override;
    void setColor(const QColor& c) override;
    void setSize(int s) override;
};

class EraserTool : public ITool {
private:
    int size;
    
public:
    EraserTool(int s = 1);
    void use(ICanvas& canvas, int x, int y) override;
    std::string getToolName() const override;
    void setColor(const QColor& c) override;
    void setSize(int s) override;
};

class ShapeTool : public ITool {
public:
    enum Type { Line, Rect, Ellipse };
    Type getShapeType() const { return shapeType; } 
    
private:
    QColor color;
    Type shapeType;
    int size;
    QPoint startPoint;
    bool isDragging;
    
public:
    ShapeTool(const QColor& c, Type type, int s = 1);
    void use(ICanvas& canvas, int x, int y) override;
    std::string getToolName() const override;
    void setColor(const QColor& c) override;
    void setSize(int s) override;
    
    void startShape(int x, int y);
    void drawShape(ICanvas& canvas, int x, int y);
    void endShape();
    
    void drawLine(ICanvas& canvas, int x1, int y1, int x2, int y2);
    void drawRect(ICanvas& canvas, int x1, int y1, int x2, int y2);
    void drawEllipse(ICanvas& canvas, int x1, int y1, int x2, int y2);
};

class BucketTool : public ITool {
private:
    QRgb color;
    
public:
    BucketTool(const QColor& c = Qt::black);
    void use(ICanvas& canvas, int x, int y) override;
    std::string getToolName() const override;
    void setColor(const QColor& c) override;
    void setSize(int s) override;
    
private:
    void floodFill(Canvas& canvas, int startX, int startY, QRgb fillColor);
};

class TextTool : public ITool {
private:
    QColor color;
    int fontSize;
    
public:
    TextTool(const QColor& c = Qt::black, int size = 12);
    void use(ICanvas& canvas, int x, int y) override;
    std::string getToolName() const override;
    void setColor(const QColor& c) override;
    void setSize(int s) override;
    
    void drawText(ICanvas& canvas, int x, int y, const QString& text);
};

// ============================================
// ФАБРИКИ
// ============================================

class IToolFactory {
public:
    virtual ~IToolFactory() = default;
    virtual ITool* create() = 0;
    virtual std::string getToolName() const = 0;
    virtual void setColor(const QColor& color) = 0;
    virtual void setSize(int size) = 0;
};

class BrushFactory : public IToolFactory {
private:
    QColor color;
    int size;
    
public:
    BrushFactory(const QColor& c = Qt::black, int s = 1);
    ITool* create() override;
    std::string getToolName() const override;
    void setColor(const QColor& c) override;
    void setSize(int s) override;
};

class EraserFactory : public IToolFactory {
private:
    int size;
    
public:
    EraserFactory(int s = 1);
    ITool* create() override;
    std::string getToolName() const override;
    void setColor(const QColor& c) override;
    void setSize(int s) override;
};

class ShapeFactory : public IToolFactory {
private:
    QColor color;
    ShapeTool::Type type;
    int size;
    
public:
    ShapeFactory(const QColor& c, ShapeTool::Type t, int s = 1);
    ITool* create() override;
    std::string getToolName() const override;
    void setColor(const QColor& c) override;
    void setSize(int s) override;
};

class BucketFactory : public IToolFactory {
private:
    QColor color;
    
public:
    BucketFactory(const QColor& c = Qt::black);
    ITool* create() override;
    std::string getToolName() const override;
    void setColor(const QColor& c) override;
    void setSize(int s) override;
};

class TextFactory : public IToolFactory {
private:
    QColor color;
    
public:
    TextFactory(const QColor& c = Qt::black);
    ITool* create() override;
    std::string getToolName() const override;
    void setColor(const QColor& c) override;
    void setSize(int s) override;
};