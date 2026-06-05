#include "qtcanvaswidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QInputDialog>
#include <QLineEdit>
#include <algorithm>

QtCanvasWidget::QtCanvasWidget(ICanvas& canvas, QWidget* parent)
    : QWidget(parent), 
      canvas(canvas), 
      currentFactory(new BrushFactory()), 
      activeTool(new BrushTool()), 
      isDrawing(false), 
      lastPos(),
      pixelSize(3), 
      activeToolColor(Qt::black),
      brushSize(1),
      canvasCache(),
      cacheDirty(true),
      shapeStartCanvas(),
      hasPreview(false)  // ← Добавь
{
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
    setMouseTracking(true);
}

QtCanvasWidget::~QtCanvasWidget() {
    delete activeTool;
    delete currentFactory;
}

void QtCanvasWidget::updateFactory(IToolFactory* factory) {
    delete activeTool;
    delete currentFactory;
    currentFactory = factory;
    activeTool = currentFactory->create();
}

void QtCanvasWidget::setActiveColor(const QColor& color) {
    activeToolColor = color;
    if (activeTool) activeTool->setColor(color);
    if (currentFactory) currentFactory->setColor(color);
}

void QtCanvasWidget::setBrushSize(int size) {
    brushSize = size;
    if (activeTool) activeTool->setSize(size);
    if (currentFactory) currentFactory->setSize(size);
}

void QtCanvasWidget::updateCache() {
    canvasCache = QPixmap(canvas.getWidth() * pixelSize, canvas.getHeight() * pixelSize);
    canvasCache.fill(Qt::white);
    
    QPainter p(&canvasCache);
    p.setPen(Qt::NoPen);
    
    for (int y = 0; y < canvas.getHeight(); ++y) {
        for (int x = 0; x < canvas.getWidth(); ++x) {
            Pixel px = canvas.getPixel(x, y);
            if (!px.isEmpty()) {
                p.fillRect(x * pixelSize, y * pixelSize, pixelSize, pixelSize, px.color);
            }
        }
    }
    cacheDirty = false;
}

void QtCanvasWidget::drawDirectlyOnCache(int canvasX, int canvasY, const QColor& color, int bSize) {
    if (canvasCache.isNull()) {
        cacheDirty = true;
        return;
    }
    
    QPainter p(&canvasCache);
    p.setPen(Qt::NoPen);
    
    int radius = bSize / 2;
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            if (dx*dx + dy*dy <= radius*radius) {
                int screenX = (canvasX + dx) * pixelSize;
                int screenY = (canvasY + dy) * pixelSize;
                p.fillRect(screenX, screenY, pixelSize, pixelSize, color);
            }
        }
    }
    p.end();
}

void QtCanvasWidget::paintEvent(QPaintEvent*) {
    if (cacheDirty) updateCache();
    
    QPainter painter(this);
    painter.drawPixmap(0, 0, canvasCache);
    
    // ← Рисуем preview поверх кэша
    if (hasPreview && !previewCache.isNull()) {
        painter.drawPixmap(0, 0, previewCache);
    }
    
    // Рисуем текст
    QFont font;
    font.setPointSize(12);
    painter.setFont(font);
    for (const TextItem& item : textItems) {
        painter.setPen(QPen(item.color, 1));
        painter.drawText(item.pos, item.text);
    }
}

void QtCanvasWidget::drawAtPosition(const QPoint& pos) {
    int x = std::clamp(pos.x() / pixelSize, 0, canvas.getWidth() - 1);
    int y = std::clamp(pos.y() / pixelSize, 0, canvas.getHeight() - 1);
    activeTool->use(canvas, x, y);
    cacheDirty = true;
}

void QtCanvasWidget::drawLine(const QPoint& from, const QPoint& to) {
    int x1 = std::clamp(from.x() / pixelSize, 0, canvas.getWidth() - 1);
    int y1 = std::clamp(from.y() / pixelSize, 0, canvas.getHeight() - 1);
    int x2 = std::clamp(to.x() / pixelSize, 0, canvas.getWidth() - 1);
    int y2 = std::clamp(to.y() / pixelSize, 0, canvas.getHeight() - 1);

    int dx = std::abs(x2 - x1), dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1, sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    QColor drawColor = Qt::white;
    EraserTool* eraser = dynamic_cast<EraserTool*>(activeTool);
    if (!eraser) drawColor = activeToolColor;

    while (true) {
        activeTool->use(canvas, x1, y1);
        drawDirectlyOnCache(x1, y1, drawColor, brushSize);
        
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
    cacheDirty = false;
}

void QtCanvasWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) return;
    
    int x = std::clamp(event->pos().x() / pixelSize, 0, canvas.getWidth() - 1);
    int y = std::clamp(event->pos().y() / pixelSize, 0, canvas.getHeight() - 1);

    TextTool* textTool = dynamic_cast<TextTool*>(activeTool);
    if (textTool) {
        showTextDialog(event->pos());
        return;
    }

    ShapeTool* shapeTool = dynamic_cast<ShapeTool*>(activeTool);
    
    if (shapeTool) {
        // Для фигур: запоминаем первую точку
        shapeStartCanvas = QPoint(x, y);
        isDrawing = true;
        hasPreview = false;
    } else {
        // Для кисти/ластика/ведра
        isDrawing = true;
        lastPos = event->pos();
        canvas.startBatch();
        activeTool->use(canvas, x, y);
        
        QColor drawColor = Qt::white;
        EraserTool* eraser = dynamic_cast<EraserTool*>(activeTool);
        if (!eraser) drawColor = activeToolColor;
        
        drawDirectlyOnCache(x, y, drawColor, brushSize);
        cacheDirty = false;
        update();
    }
}

void QtCanvasWidget::mouseMoveEvent(QMouseEvent* event) {
    if (!isDrawing) return;
    
    ShapeTool* shapeTool = dynamic_cast<ShapeTool*>(activeTool);
    if (shapeTool) {
        // ← Для фигур: рисуем preview в реальном времени
        int x = std::clamp(event->pos().x() / pixelSize, 0, canvas.getWidth() - 1);
        int y = std::clamp(event->pos().y() / pixelSize, 0, canvas.getHeight() - 1);
        
        drawPreview(shapeStartCanvas, QPoint(x, y));
        hasPreview = true;
        update();  // Перерисовываем экран
    } else {
        // Для кисти/ластика
        drawLine(lastPos, event->pos());
        lastPos = event->pos();
        update();
    }
}

void QtCanvasWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) return;
    isDrawing = false;
    
    ShapeTool* shapeTool = dynamic_cast<ShapeTool*>(activeTool);
    if (shapeTool) {
        // Очищаем preview
        clearPreview();
        
        // Берём вторую точку
        int x = std::clamp(event->pos().x() / pixelSize, 0, canvas.getWidth() - 1);
        int y = std::clamp(event->pos().y() / pixelSize, 0, canvas.getHeight() - 1);
        
        // Рисуем фигуру на canvas
        canvas.startBatch();
        shapeTool->startShape(shapeStartCanvas.x(), shapeStartCanvas.y());
        shapeTool->drawShape(canvas, x, y);
        shapeTool->endShape();
        canvas.endBatch();
        
        cacheDirty = true;
        update();
    } else {
        canvas.endBatch();
    }
}

void QtCanvasWidget::clearCanvas() {
    canvas.clear();
    canvasCache.fill(Qt::white);
    cacheDirty = false;
    textItems.clear();
    update();
}

void QtCanvasWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Z && (event->modifiers() & Qt::ControlModifier)) {
        canvas.undo();
        cacheDirty = true;
        update();
        return;
    }
    QWidget::keyPressEvent(event);
}

void QtCanvasWidget::showTextDialog(const QPoint& pos) {
    bool ok;
    QString text = QInputDialog::getText(this, "Ввод текста",
                                         "Введите текст:",
                                         QLineEdit::Normal, "", &ok);
    
    if (ok && !text.isEmpty()) {
        TextItem item;
        item.text = text;
        item.pos = pos;
        item.color = activeToolColor;
        textItems.append(item);
        update();
    }
}

void QtCanvasWidget::drawPreview(const QPoint& startCanvas, const QPoint& currentCanvas) {
    if (previewCache.isNull() || previewCache.size() != canvasCache.size()) {
        previewCache = QPixmap(canvasCache.size());
    }
    previewCache.fill(Qt::transparent);
    
    QPainter p(&previewCache);
    p.setRenderHint(QPainter::Antialiasing, false);
    
    int penWidth = brushSize * pixelSize;
    
    QPen pen(activeToolColor, penWidth);
    pen.setJoinStyle(Qt::MiterJoin);  // ← Прямые углы (или Qt::BevelJoin)
    // pen.setCapStyle(Qt::FlatCap);      // ← Плоские концы для линий
    pen.setCapStyle(Qt::RoundCap);

    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    
    ShapeTool* shapeTool = dynamic_cast<ShapeTool*>(activeTool);
    if (!shapeTool) {
        p.end();
        return;
    }
    
    QPoint startScreen = QPoint(startCanvas.x() * pixelSize, startCanvas.y() * pixelSize);
    QPoint currentScreen = QPoint(currentCanvas.x() * pixelSize, currentCanvas.y() * pixelSize);
    
    switch (shapeTool->getShapeType()) {
        case ShapeTool::Line:
            p.drawLine(startScreen, currentScreen);
            break;
        case ShapeTool::Rect:
            p.drawRect(QRect(startScreen, currentScreen).normalized());
            break;
        case ShapeTool::Ellipse:
            p.drawEllipse(QRect(startScreen, currentScreen).normalized());
            break;
    }
    p.end();
}

void QtCanvasWidget::clearPreview() {
    if (!previewCache.isNull()) {
        previewCache.fill(Qt::transparent);
    }
    hasPreview = false;
}