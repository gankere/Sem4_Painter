#include "domain.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>

Canvas::Canvas(int width, int height) 
    : w(width), h(height), isUndoing(false), isBatching(false) {
    if (w <= 0 || h <= 0) throw std::invalid_argument("Size must be positive");
    data.resize(w * h, Pixel());
}

void Canvas::setPixel(int x, int y, Pixel p) {
    if (x >= 0 && x < w && y >= 0 && y < h) {
        if (!isUndoing) {
            if (undoHistory.size() > 10000000) { //Ограничение истории
                std::stack<UndoStep> temp;
                std::swap(undoHistory, temp);
            }
            undoHistory.push(UndoStep(x, y, data[y * w + x].color, false));
        }
        data[y * w + x] = p;
    }
}
Pixel Canvas::getPixel(int x, int y) const {
    if (x >= 0 && x < w && y >= 0 && y < h)
        return data[y * w + x];
    return Pixel();
}

int Canvas::getWidth() const { return w; }
int Canvas::getHeight() const { return h; }

void Canvas::startBatch() {
    isBatching = true;
    undoHistory.push(UndoStep(-1, -1, QColor(0, 0, 0, 0), true));
}

void Canvas::endBatch() {
    isBatching = false;
}

void Canvas::undo() {
    if (undoHistory.empty()) return;
    isUndoing = true; // Флаг: не пишем в историю при отмене

    while (!undoHistory.empty()) {
        UndoStep step = undoHistory.top();
        undoHistory.pop();
        if (step.isBatchMarker) break;
        if (step.x >= 0 && step.x < w && step.y >= 0 && step.y < h) {
            data[step.y * w + step.x] = Pixel(step.previousColor);
        }
    }
    isUndoing = false;
}

void Canvas::clear() {
    std::stack<UndoStep>().swap(undoHistory);
    
    std::fill(data.begin(), data.end(), Pixel());
}

// === BrushTool ===
BrushTool::BrushTool(const QColor& c, int s) : color(c), size(s) {}

void BrushTool::use(ICanvas& canvas, int x, int y) {
    int radius = size / 2;
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            if (dx*dx + dy*dy <= radius*radius) {
                canvas.setPixel(x + dx, y + dy, Pixel(color));
            }
        }
    }
}

std::string BrushTool::getToolName() const { return "Draw"; }

void BrushTool::setColor(const QColor& c) { color = c; }
void BrushTool::setSize(int s) { size = s; }

// === EraserTool ===
EraserTool::EraserTool(int s) : size(s) {}

void EraserTool::use(ICanvas& canvas, int x, int y) {
    int radius = size / 2;
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            // Круглый ластик
            if (dx*dx + dy*dy <= radius*radius) {
                canvas.setPixel(x + dx, y + dy, Pixel());
            }
        }
    }
}

std::string EraserTool::getToolName() const { return "Erase"; }

void EraserTool::setColor(const QColor&) {}
void EraserTool::setSize(int s) { size = s; }

// === ShapeTool ===
ShapeTool::ShapeTool(const QColor& c, Type type, int s) 
    : color(c), shapeType(type), size(s), isDragging(false) {}

void ShapeTool::use(ICanvas& canvas, int x, int y) {
    (void)canvas; (void)x; (void)y;
}

std::string ShapeTool::getToolName() const {
    switch (shapeType) {
        case Line: return "Line";
        case Rect: return "Rectangle";
        case Ellipse: return "Ellipse";
        default: return "Shape";
    }
}

void ShapeTool::setColor(const QColor& c) { color = c; }
void ShapeTool::setSize(int s) { size = s; }

void ShapeTool::startShape(int x, int y) {
    startPoint = QPoint(x, y);
    isDragging = true;
}

void ShapeTool::drawShape(ICanvas& canvas, int x, int y) {
    switch (shapeType) {
        case Line:
            drawLine(canvas, startPoint.x(), startPoint.y(), x, y);
            break;
        case Rect:
            drawRect(canvas, startPoint.x(), startPoint.y(), x, y);
            break;
        case Ellipse:
            drawEllipse(canvas, startPoint.x(), startPoint.y(), x, y);
            break;
    }
}

void ShapeTool::endShape() {
    isDragging = false;
}

void ShapeTool::drawLine(ICanvas& canvas, int x1, int y1, int x2, int y2) {
    int dx = std::abs(x2 - x1), dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1, sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    int radius = size / 2;

    while (true) {
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                if (dx*dx + dy*dy <= radius*radius) {
                    canvas.setPixel(x1 + dx, y1 + dy, Pixel(color));
                }
            }
        }
        
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

void ShapeTool::drawRect(ICanvas& canvas, int x1, int y1, int x2, int y2) {
    int minX = std::min(x1, x2);
    int maxX = std::max(x1, x2);
    int minY = std::min(y1, y2);
    int maxY = std::max(y1, y2);

    for (int x = minX; x <= maxX; ++x) {
        for (int i = 0; i < size; ++i) {
            canvas.setPixel(x, minY + i, Pixel(color));
            canvas.setPixel(x, maxY - i, Pixel(color));
        }
    }
    for (int y = minY; y <= maxY; ++y) {
        for (int i = 0; i < size; ++i) {
            canvas.setPixel(minX + i, y, Pixel(color));
            canvas.setPixel(maxX - i, y, Pixel(color));
        }
    }
}

void ShapeTool::drawEllipse(ICanvas& canvas, int x1, int y1, int x2, int y2) {
    int centerX = (x1 + x2) / 2;
    int centerY = (y1 + y2) / 2;
    int radiusX = std::abs(x2 - x1) / 2;
    int radiusY = std::abs(y2 - y1) / 2;

    if (radiusX == 0 && radiusY == 0) return;
    
    // Если один из радиусов очень маленький — рисуем линию или точку
    if (radiusX == 0) {
        for (int y = centerY - radiusY; y <= centerY + radiusY; ++y) {
            canvas.setPixel(centerX, y, Pixel(color));
        }
        return;
    }
    if (radiusY == 0) {
        for (int x = centerX - radiusX; x <= centerX + radiusX; ++x) {
            canvas.setPixel(x, centerY, Pixel(color));
        }
        return;
    }

    int brushRadius = size / 2;
    
    int steps = std::max(radiusX, radiusY) * 8;
    if (steps < 100) steps = 100;
    
    for (int i = 0; i < steps; ++i) {
        double angle = 2.0 * 3.14159265359 * i / steps;
        int x = centerX + static_cast<int>(radiusX * std::cos(angle) + 0.5);
        int y = centerY + static_cast<int>(radiusY * std::sin(angle) + 0.5);

        for (int dy = -brushRadius; dy <= brushRadius; ++dy) {
            for (int dx = -brushRadius; dx <= brushRadius; ++dx) {
                if (dx*dx + dy*dy <= brushRadius*brushRadius) {
                    canvas.setPixel(x + dx, y + dy, Pixel(color));
                }
            }
        }
    }
}
// === Factories ===
BrushFactory::BrushFactory(const QColor& c, int s) : color(c), size(s) {}

ITool* BrushFactory::create() { 
    return new BrushTool(color, size); 
}

std::string BrushFactory::getToolName() const { return "Draw"; }

void BrushFactory::setColor(const QColor& c) { color = c; }
void BrushFactory::setSize(int s) { size = s; }

EraserFactory::EraserFactory(int s) : size(s) {}

ITool* EraserFactory::create() { 
    return new EraserTool(size); 
}

std::string EraserFactory::getToolName() const { return "Erase"; }

void EraserFactory::setColor(const QColor&) {}
void EraserFactory::setSize(int s) { size = s; }

ShapeFactory::ShapeFactory(const QColor& c, ShapeTool::Type t, int s) 
    : color(c), type(t), size(s) {}

ITool* ShapeFactory::create() { 
    return new ShapeTool(color, type, size); 
}

std::string ShapeFactory::getToolName() const { return "Shape"; }

void ShapeFactory::setColor(const QColor& c) { color = c; }
void ShapeFactory::setSize(int s) { size = s; }

// === BucketTool ===
BucketTool::BucketTool(const QColor& c) : color(c) {}

void BucketTool::use(ICanvas& canvas, int x, int y) {
    floodFill(canvas, x, y, color);
}

std::string BucketTool::getToolName() const { return "Bucket"; }

void BucketTool::setColor(const QColor& c) { color = c; }
void BucketTool::setSize(int) {}  // Ведро не использует размер

void BucketTool::floodFill(ICanvas& canvas, int startX, int startY, const QColor& fillColor) {
    int width = canvas.getWidth();
    int height = canvas.getHeight();
    
    QColor targetColor = canvas.getPixel(startX, startY).color;
    
    // Если цвета одинаковые — ничего не делаем
    if (targetColor == fillColor) return;
    
    std::queue<std::pair<int, int>> pixels;
    pixels.push({startX, startY});
    
    while (!pixels.empty()) {
        auto [x, y] = pixels.front();
        pixels.pop();
        
        if (x < 0 || x >= width || y < 0 || y >= height) continue;
        
        Pixel currentPixel = canvas.getPixel(x, y);
        
        // Если цвет не совпадает с целевым — пропускаем
        if (currentPixel.color != targetColor) continue;
        
        // Закрашиваем пиксель
        canvas.setPixel(x, y, Pixel(fillColor));
        
        // Добавляем соседей
        pixels.push({x + 1, y});
        pixels.push({x - 1, y});
        pixels.push({x, y + 1});
        pixels.push({x, y - 1});
    }
}

// === BucketFactory ===
BucketFactory::BucketFactory(const QColor& c) : color(c) {}

ITool* BucketFactory::create() { 
    return new BucketTool(color); 
}

std::string BucketFactory::getToolName() const { return "Bucket"; }

void BucketFactory::setColor(const QColor& c) { color = c; }
void BucketFactory::setSize(int) {}

// === TextTool ===
TextTool::TextTool(const QColor& c, int size) : color(c), fontSize(size) {}

void TextTool::use(ICanvas& canvas, int x, int y) {
    (void)canvas; (void)x; (void)y;
}

std::string TextTool::getToolName() const { return "Text"; }

void TextTool::setColor(const QColor& c) { color = c; }
void TextTool::setSize(int s) { fontSize = s; }

void TextTool::drawText(ICanvas& canvas, int x, int y, const QString& text) {
    QColor textColor = color;
    
    int xPos = x;
    for (QChar ch : text) {
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 10; ++j) {
                if (i == 0 || i == 7 || j == 0 || j == 9) {
                    canvas.setPixel(xPos + i, y + j, Pixel(textColor));
                }
            }
        }
        xPos += 10; // Отступ между символами
    }
}

// === TextFactory ===
TextFactory::TextFactory(const QColor& c) : color(c) {}

ITool* TextFactory::create() { 
    return new TextTool(color); 
}

std::string TextFactory::getToolName() const { return "Text"; }

void TextFactory::setColor(const QColor& c) { color = c; }
void TextFactory::setSize(int) {}