#include "domain.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <QImage>

// ============================================
// ВСПОМОГАТЕЛЬНАЯ ФУНКЦИЯ (должна быть до всех классов)
// ============================================

// Единая функция для генерации круглой маски кисти любого размера
static std::vector<std::pair<int, int>> getBrushOffsets(int size) {
    std::vector<std::pair<int, int>> offsets;
    if (size <= 0) return offsets;
    
    // Плавное масштабирование: радиус растёт на 0.5 за каждый шаг размера
    double radius = (size - 1) / 2.0;
    int half = std::ceil(radius); 
    
    for (int dy = -half; dy <= half; ++dy) {
        for (int dx = -half; dx <= half; ++dx) {
            double dist = std::sqrt(dx*dx + dy*dy);
            if (dist <= radius + 0.5) { // +0.5 чтобы захватывать краевые пиксели
                offsets.push_back({dx, dy});
            }
        }
    }
    return offsets;
}

// Единая функция для генерации квадратной маски кисти
static std::vector<std::pair<int, int>> getSquareBrushOffsets(int size) {
    std::vector<std::pair<int, int>> offsets;
    if (size <= 0) return offsets;
    
    int start = -(size - 1) / 2;
    int end = size / 2;
    
    for (int dy = start; dy <= end; ++dy) {
        for (int dx = start; dx <= end; ++dx) {
            offsets.push_back({dx, dy});
        }
    }
    return offsets;
}

// ============================================
// CANVAS
// ============================================

Canvas::Canvas(int width, int height) 
    : w(width), h(height), isUndoing(false), isBatching(false), batchCount(0) {
    if (w <= 0 || h <= 0) throw std::invalid_argument("Size must be positive");
    data.resize(w * h, Pixel());
}

void Canvas::setPixel(int x, int y, Pixel p) {
    if (x >= 0 && x < w && y >= 0 && y < h) {
        if (!isUndoing) {
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
    if (batchCount >= MAX_BATCHES) {
        // Если лимит превышен — очищаем всю историю
        std::stack<UndoStep, std::vector<UndoStep>>().swap(undoHistory);
        batchCount = 0;
    }
    
    isBatching = true;
    undoHistory.push(UndoStep(-1, -1, 0, true)); // Маркер начала действия
    batchCount++;
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
        
        if (step.isBatchMarker) {
            batchCount--; // Уменьшаем количество действий в истории
            break;
        }
        
        if (step.x >= 0 && step.x < w && step.y >= 0 && step.y < h) {
            data[step.y * w + step.x] = Pixel(step.previousColor);
        }
    }
    isUndoing = false;
}

void Canvas::clear() {
    std::stack<UndoStep, std::vector<UndoStep>>().swap(undoHistory);
    batchCount = 0;
    std::fill(data.begin(), data.end(), Pixel());
}

void Canvas::loadFromImage(const QImage& image, int x, int y, int width, int height) {
    QImage converted = image.convertToFormat(QImage::Format_ARGB32);
    
    isUndoing = true;
    
    int actualWidth = std::min(width, w - x);
    int actualHeight = std::min(height, h - y);
    
    Pixel* canvasData = data.data();
    
    for (int row = 0; row < actualHeight; ++row) {
        const QRgb* srcLine = reinterpret_cast<const QRgb*>(converted.scanLine(row));
        Pixel* destLine = canvasData + (y + row) * w + x;
        
        for (int col = 0; col < actualWidth; ++col) {
            destLine[col].color = srcLine[col];
        }
    }
    
    isUndoing = false;
}

// ============================================
// BRUSHTOOL
// ============================================

BrushTool::BrushTool(const QColor& c, int s) : color(c), size(s) {}

void BrushTool::use(ICanvas& canvas, int x, int y) {
    double radius = size / 2.0;
    int half = size / 2;
    
    for (int dy = -half; dy <= half; ++dy) {
        for (int dx = -half; dx <= half; ++dx) {
            double dist = sqrt(dx*dx + dy*dy);
            if (dist <= radius) {
                canvas.setPixel(x + dx, y + dy, Pixel(color.rgba()));
            }
        }
    }
}

std::string BrushTool::getToolName() const { return "Draw"; }
void BrushTool::setColor(const QColor& c) { color = c; }
void BrushTool::setSize(int s) { size = s; }

// ============================================
// ERASERTOOL
// ============================================

EraserTool::EraserTool(int s) : size(s) {}

void EraserTool::use(ICanvas& canvas, int x, int y) {
    int startX = x - (size - 1) / 2;
    int startY = y - (size - 1) / 2;
    for (int dy = 0; dy < size; ++dy) {
        for (int dx = 0; dx < size; ++dx) {
            canvas.setPixel(startX + dx, startY + dy, Pixel());
        }
    }
}

std::string EraserTool::getToolName() const { return "Erase"; }
void EraserTool::setColor(const QColor&) {}
void EraserTool::setSize(int s) { size = s; }

// ============================================
// SHAPETOOL
// ============================================

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
    
    auto offsets = getBrushOffsets(size);

    while (true) {
        for (const auto& [ox, oy] : offsets) {
            canvas.setPixel(x1 + ox, y1 + oy, Pixel(color.rgba()));
        }
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

void ShapeTool::drawRect(ICanvas& canvas, int x1, int y1, int x2, int y2) {
    int minX = std::min(x1, x2), maxX = std::max(x1, x2);
    int minY = std::min(y1, y2), maxY = std::max(y1, y2);
    
    auto offsets = getSquareBrushOffsets(size);

    //верхняя и нижняя границы
    for (int x = minX; x <= maxX; ++x) {
        for (const auto& [ox, oy] : offsets) {
            canvas.setPixel(x + ox, minY + oy, Pixel(color.rgba()));
            canvas.setPixel(x + ox, maxY + oy, Pixel(color.rgba()));
        }
    }
    //левая и правая границы
    for (int y = minY + 1; y < maxY; ++y) {
        for (const auto& [ox, oy] : offsets) {
            canvas.setPixel(minX + ox, y + oy, Pixel(color.rgba()));
            canvas.setPixel(maxX + ox, y + oy, Pixel(color.rgba()));
        }
    }
}

void ShapeTool::drawEllipse(ICanvas& canvas, int x1, int y1, int x2, int y2) {
    int centerX = (x1 + x2) / 2, centerY = (y1 + y2) / 2;
    int radiusX = std::abs(x2 - x1) / 2, radiusY = std::abs(y2 - y1) / 2;
    
    auto offsets = getBrushOffsets(size);

    if (radiusX == 0 && radiusY == 0) {
        for (const auto& [ox, oy] : offsets) {
            canvas.setPixel(centerX + ox, centerY + oy, Pixel(color.rgba()));
        }
        return;
    }

    int steps = std::max(radiusX, radiusY) * 8;
    if (steps < 100) steps = 100;
    if (steps > 2000) steps = 2000;
    
    // Рисуем эллипс по параметрическому уравнению, штампуя кисть
    for (int i = 0; i < steps; ++i) {
        double angle = 2.0 * 3.14159265359 * i / steps;
        int x = centerX + static_cast<int>(radiusX * std::cos(angle) + 0.5);
        int y = centerY + static_cast<int>(radiusY * std::sin(angle) + 0.5);

        for (const auto& [ox, oy] : offsets) {
            canvas.setPixel(x + ox, y + oy, Pixel(color.rgba()));
        }
    }
}

// ============================================
// FACTORIES
// ============================================

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

// ============================================
// BUCKETTOOL
// ============================================

BucketTool::BucketTool(const QColor& c) : color(c.rgba()) {}

void BucketTool::use(ICanvas& canvas, int x, int y) {
    Canvas* realCanvas = dynamic_cast<Canvas*>(&canvas);
    if (realCanvas) {
        floodFill(*realCanvas, x, y, color);
    }
}

std::string BucketTool::getToolName() const { return "Bucket"; }
void BucketTool::setColor(const QColor& c) { color = c.rgba(); }
void BucketTool::setSize(int) {}

void BucketTool::floodFill(Canvas& canvas, int startX, int startY, QRgb fillColor) {
    int width = canvas.getWidth();
    int height = canvas.getHeight();

    if (startX < 0 || startX >= width || startY < 0 || startY >= height) return;
    
    Pixel* data = canvas.getData();
    QRgb targetColor = data[startY * width + startX].color;
    if (targetColor == fillColor) return;

    std::vector<std::pair<int, int>> stack;
    stack.reserve(10000);
    stack.push_back({startX, startY});
    
    while (!stack.empty()) {
        auto [x, y] = stack.back();
        stack.pop_back();
        
        if (data[y * width + x].color != targetColor) {// пропускаем пиксели, которые уже залиты
            continue;
        }
        
        int x1 = x;
        while (x1 > 0 && data[y * width + (x1 - 1)].color == targetColor) {
            x1--;
        }
              
        int x2 = x;
        while (x2 < width - 1 && data[y * width + (x2 + 1)].color == targetColor) {
            x2++;
        }       

        for (int i = x1; i <= x2; ++i) { //Записб в историю заливки малых областей
            canvas.setPixel(i, y, Pixel(fillColor));
        }
               
        if (y > 0) {
            Pixel* aboveRow = data + (y - 1) * width;
            for (int i = x1; i <= x2; ++i) {
                if (aboveRow[i].color == targetColor) {
                    stack.push_back({i, y - 1});
                }
            }
        }
        
        if (y < height - 1) {
            Pixel* belowRow = data + (y + 1) * width;
            for (int i = x1; i <= x2; ++i) {
                if (belowRow[i].color == targetColor) {
                    stack.push_back({i, y + 1});
                }
            }
        }
    }
}

// ============================================
// BUCKETFACTORY
// ============================================

BucketFactory::BucketFactory(const QColor& c) : color(c) {}

ITool* BucketFactory::create() { 
    return new BucketTool(color); 
}

std::string BucketFactory::getToolName() const { return "Bucket"; }
void BucketFactory::setColor(const QColor& c) { color = c; }
void BucketFactory::setSize(int) {}

// ============================================
// TEXTTOOL
// ============================================

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

// ============================================
// TEXTFACTORY
// ============================================

TextFactory::TextFactory(const QColor& c) : color(c) {}

ITool* TextFactory::create() { 
    return new TextTool(color); 
}

std::string TextFactory::getToolName() const { return "Text"; }
void TextFactory::setColor(const QColor& c) { color = c; }
void TextFactory::setSize(int) {}