#include "domain.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>

// === Canvas ===
Canvas::Canvas(int width, int height) 
    : w(width), h(height), isUndoing(false), isBatching(false) {
    if (w <= 0 || h <= 0) throw std::invalid_argument("Size must be positive");
    data.resize(w * h, Pixel());  // ← Прозрачные пиксели
}

void Canvas::setPixel(int x, int y, Pixel p) {
    if (x >= 0 && x < w && y >= 0 && y < h) {
        if (!isUndoing) {
            // ← Ограничиваем историю 10000 шагов
            if (undoHistory.size() > 1000) {
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
    return Pixel();  // ← Прозрачный
}

int Canvas::getWidth() const { return w; }
int Canvas::getHeight() const { return h; }

void Canvas::startBatch() {
    isBatching = true;
    undoHistory.push(UndoStep(-1, -1, QColor(0, 0, 0, 0), true));  // ← Прозрачный
}

void Canvas::endBatch() {
    isBatching = false;
}

void Canvas::undo() {
    if (undoHistory.empty()) return;
    isUndoing = true;
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
    // ← МГНОВЕННАЯ очистка истории (swap работает за O(1))
    std::stack<UndoStep>().swap(undoHistory);
    
    // ← Быстрая очистка данных
    std::fill(data.begin(), data.end(), Pixel());
}

// === BrushTool ===
BrushTool::BrushTool(const QColor& c, int s) : color(c), size(s) {}

void BrushTool::use(ICanvas& canvas, int x, int y) {
    int radius = size / 2;
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            // Проверяем, находится ли точка внутри круга
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
    if (!isDragging) {
        startShape(x, y);
    } else {
        drawShape(canvas, x, y);
    }
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
        // Круглая кисть для линии
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

    if (radiusX == 0 || radiusY == 0) return;

    int x = 0, y = radiusY;
    int d1 = (radiusY * radiusY) - (radiusX * radiusX * radiusY) + (0.25 * radiusX * radiusX);
    int dx = 2 * radiusY * radiusY * x;
    int dy = 2 * radiusX * radiusY * y;

    while (dx < dy) {
        int brushRadius = size / 2;
        for (int i = -brushRadius; i <= brushRadius; ++i) {
            for (int j = -brushRadius; j <= brushRadius; ++j) {
                if (i*i + j*j <= brushRadius*brushRadius) {
                    canvas.setPixel(centerX + x + i, centerY + y + j, Pixel(color));
                    canvas.setPixel(centerX - x + i, centerY + y + j, Pixel(color));
                    canvas.setPixel(centerX + x + i, centerY - y + j, Pixel(color));
                    canvas.setPixel(centerX - x + i, centerY - y + j, Pixel(color));
                }
            }
        }

        if (d1 < 0) {
            x++;
            dx = dx + 2 * radiusY * radiusY;
            d1 = d1 + dx + radiusY * radiusY;
        } else {
            x++;
            y--;
            dx = dx + 2 * radiusY * radiusY;
            dy = dy - 2 * radiusX * radiusX;
            d1 = d1 + dx - dy + radiusY * radiusY;
        }
    }

    int d2 = ((radiusY * radiusY) * ((x + 0.5) * (x + 0.5))) + 
             ((radiusX * radiusX) * ((y - 1) * (y - 1))) - 
             (radiusX * radiusX * radiusY * radiusY);

    while (y >= 0) {
        int brushRadius = size / 2;
        for (int i = -brushRadius; i <= brushRadius; ++i) {
            for (int j = -brushRadius; j <= brushRadius; ++j) {
                if (i*i + j*j <= brushRadius*brushRadius) {
                    canvas.setPixel(centerX + x + i, centerY + y + j, Pixel(color));
                    canvas.setPixel(centerX - x + i, centerY + y + j, Pixel(color));
                    canvas.setPixel(centerX + x + i, centerY - y + j, Pixel(color));
                    canvas.setPixel(centerX - x + i, centerY - y + j, Pixel(color));
                }
            }
        }

        if (d2 > 0) {
            y--;
            dy = dy - 2 * radiusX * radiusX;
            d2 = d2 + radiusX * radiusX - dy;
        } else {
            y--;
            x++;
            dx = dx + 2 * radiusY * radiusY;
            dy = dy - 2 * radiusX * radiusX;
            d2 = d2 + dx - dy + radiusX * radiusX;
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