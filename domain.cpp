#include "domain.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <QImage>

Canvas::Canvas(int width, int height) 
    : w(width), h(height), isUndoing(false), isBatching(false) {
    if (w <= 0 || h <= 0) throw std::invalid_argument("Size must be positive");
    data.resize(w * h, Pixel());
}

void Canvas::setPixel(int x, int y, Pixel p) {
    if (x >= 0 && x < w && y >= 0 && y < h) {
        if (!isUndoing) {
            if (undoHistory.size() > 100000) { //Ограничение истории
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
                canvas.setPixel(x + dx, y + dy, Pixel(color.rgba()));
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
                    canvas.setPixel(x1 + dx, y1 + dy, Pixel(color.rgba()));
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
    
    int width = maxX - minX + 1;
    int height = maxY - minY + 1;

    // точка
    if (width == 1 && height == 1) {
        int half = size / 2;
        for (int dy = -half; dy < size - half; ++dy) {
            for (int dx = -half; dx < size - half; ++dx) {
                canvas.setPixel(minX + dx, minY + dy, Pixel(color.rgba()));
            }
        }
        return;
    }
    
    // горизонтальная линия
    if (height == 1) {
        for (int x = minX; x <= maxX; ++x) {
            for (int dy = -size/2; dy < size - size/2; ++dy) {
                canvas.setPixel(x, minY + dy, Pixel(color.rgba()));
            }
        }
        return;
    }
    
    // вертикальная линия
    if (width == 1) {
        for (int y = minY; y <= maxY; ++y) {
            for (int dx = -size/2; dx < size - size/2; ++dx) {
                canvas.setPixel(minX + dx, y, Pixel(color.rgba()));
            }
        }
        return;
    }

    // маленький прямоугольник
    if (width <= size || height <= size) {
        for (int y = minY - size/2; y <= maxY + size - size/2; ++y) {
            for (int x = minX - size/2; x <= maxX + size - size/2; ++x) {
                canvas.setPixel(x, y, Pixel(color.rgba()));
            }
        }
        return;
    }

    // Обычный случай: большой прямоугольник
    for (int x = minX; x <= maxX; ++x) {
        for (int i = 0; i < size; ++i) {
            canvas.setPixel(x, minY + i, Pixel(color.rgba()));
        }
    }

    for (int x = minX; x <= maxX; ++x) {
        for (int i = 0; i < size; ++i) {
            canvas.setPixel(x, maxY - i, Pixel(color.rgba()));
        }
    }
    
    for (int y = minY; y <= maxY; ++y) {
        for (int i = 0; i < size; ++i) {
            canvas.setPixel(minX + i, y, Pixel(color.rgba()));
        }
    }

    for (int y = minY; y <= maxY; ++y) {
        for (int i = 0; i < size; ++i) {
            canvas.setPixel(maxX - i, y, Pixel(color.rgba()));
        }
    }
}

void ShapeTool::drawEllipse(ICanvas& canvas, int x1, int y1, int x2, int y2) {
    int centerX = (x1 + x2) / 2;
    int centerY = (y1 + y2) / 2;
    int radiusX = std::abs(x2 - x1) / 2;
    int radiusY = std::abs(y2 - y1) / 2;
    int brushRadius = size / 2;

    // Вырожденный случай: точка (клик без движения)
    if (radiusX == 0 && radiusY == 0) {
        for (int dy = -brushRadius; dy <= brushRadius; ++dy) {
            for (int dx = -brushRadius; dx <= brushRadius; ++dx) {
                if (dx*dx + dy*dy <= brushRadius*brushRadius) {
                    canvas.setPixel(centerX + dx, centerY + dy, Pixel(color.rgba()));
                }
            }
        }
        return;
    }
    
    // Вырожденный случай: вертикальная линия
    if (radiusX == 0) {
        for (int y = centerY - radiusY; y <= centerY + radiusY; ++y) {
            for (int dy = -brushRadius; dy <= brushRadius; ++dy) {
                for (int dx = -brushRadius; dx <= brushRadius; ++dx) {
                    if (dx*dx + dy*dy <= brushRadius*brushRadius) {
                        canvas.setPixel(centerX + dx, y + dy, Pixel(color.rgba()));
                    }
                }
            }
        }
        return;
    }
    
    // Вырожденный случай: горизонтальная линия
    if (radiusY == 0) {
        for (int x = centerX - radiusX; x <= centerX + radiusX; ++x) {
            for (int dy = -brushRadius; dy <= brushRadius; ++dy) {
                for (int dx = -brushRadius; dx <= brushRadius; ++dx) {
                    if (dx*dx + dy*dy <= brushRadius*brushRadius) {
                        canvas.setPixel(x + dx, centerY + dy, Pixel(color.rgba()));
                    }
                }
            }
        }
        return;
    }

    // ← Обычный случай: эллипс с круглой кистью
    int steps = std::max(radiusX, radiusY) * 8;
    if (steps < 100) steps = 100;
    
    for (int i = 0; i < steps; ++i) {
        double angle = 2.0 * 3.14159265359 * i / steps;
        int x = centerX + static_cast<int>(radiusX * std::cos(angle) + 0.5);
        int y = centerY + static_cast<int>(radiusY * std::sin(angle) + 0.5);

        // Круглая кисть в каждой точке эллипса
        for (int dy = -brushRadius; dy <= brushRadius; ++dy) {
            for (int dx = -brushRadius; dx <= brushRadius; ++dx) {
                if (dx*dx + dy*dy <= brushRadius*brushRadius) {
                    canvas.setPixel(x + dx, y + dy, Pixel(color.rgba()));
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
    
    // Прямой доступ к данным холста
    Pixel* data = canvas.getData();
    int targetColor = data[startY * width + startX].color;
    if (targetColor == fillColor) return;

    bool wasUndoing = canvas.getIsUndoing();
    canvas.setIsUndoing(true);
    
    std::vector<std::pair<int, int>> stack;
    stack.reserve(width * height / 10);
    stack.push_back({startX, startY});
    
    while (!stack.empty()) {
        auto [x, y] = stack.back();
        stack.pop_back();
        
        
        int x1 = x; // Находим начало строки слева
        while (x1 > 0 && data[y * width + (x1 - 1)].color == targetColor) {
            x1--;
        }
              
        int x2 = x;// Находим конец строки справа
        while (x2 < width - 1 && data[y * width + (x2 + 1)].color == targetColor) {
            x2++;
        }       
        
        Pixel* row = data + y * width + x1;// Заливаем всю строку
        for (int i = 0; i <= x2 - x1; ++i) {
            row[i].color = fillColor;
        }
               
        if (y > 0) {// Проверяем строку выше
            Pixel* aboveRow = data + (y - 1) * width;
            for (int i = x1; i <= x2; ++i) {
                if (aboveRow[i].color == targetColor) {
                    stack.push_back({i, y - 1});
                }
            }
        }
        
        if (y < height - 1) {// Проверяем строку ниже
            Pixel* belowRow = data + (y + 1) * width;
            for (int i = x1; i <= x2; ++i) {
                if (belowRow[i].color == targetColor) {
                    stack.push_back({i, y + 1});
                }
            }
        }
    }
    
    canvas.setIsUndoing(wasUndoing);
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