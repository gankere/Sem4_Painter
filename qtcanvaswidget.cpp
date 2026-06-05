#include "qtcanvaswidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <algorithm>

QtCanvasWidget::QtCanvasWidget(ICanvas& canvas, QWidget* parent)
    : QWidget(parent), canvas(canvas), currentFactory(new BrushFactory()), 
      activeTool(new BrushTool()), isDrawing(false), pixelSize(3), 
      cacheDirty(true) 
{
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
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
    if (activeTool) {
        activeTool->setColor(color);
    }
    if (currentFactory) {
        currentFactory->setColor(color);
    }
}

void QtCanvasWidget::setBrushSize(int size) {
    if (activeTool) {
        activeTool->setSize(size);
    }
    if (currentFactory) {
        currentFactory->setSize(size);
    }
}

void QtCanvasWidget::updateCache() {
    canvasCache = QPixmap(canvas.getWidth() * pixelSize, canvas.getHeight() * pixelSize);
    canvasCache.fill(Qt::white);
    
    QPainter p(&canvasCache);
    p.setPen(Qt::NoPen);
    
    for (int y = 0; y < canvas.getHeight(); ++y) {
        for (int x = 0; x < canvas.getWidth(); ++x) {
            QColor color = canvas.getPixel(x, y).color;
            if (color != Qt::white) {
                p.fillRect(x * pixelSize, y * pixelSize, pixelSize, pixelSize, color);
            }
        }
    }
    cacheDirty = false;
}

void QtCanvasWidget::paintEvent(QPaintEvent* event) {
    if (cacheDirty) {
        updateCache();
    }
    QPainter painter(this);
    painter.drawPixmap(0, 0, canvasCache);
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

    while (true) {
        activeTool->use(canvas, x1, y1);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
    cacheDirty = true;
}

void QtCanvasWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        isDrawing = true;
        lastPos = event->pos();

        int x = std::clamp(event->pos().x() / pixelSize, 0, canvas.getWidth() - 1);
        int y = std::clamp(event->pos().y() / pixelSize, 0, canvas.getHeight() - 1);

        // Проверяем, является ли инструмент ShapeTool
        ShapeTool* shapeTool = dynamic_cast<ShapeTool*>(activeTool);
        if (shapeTool) {
            shapeStartPoint = QPoint(x, y);
            canvas.startBatch();
            shapeTool->startShape(x, y);
        } else {
            canvas.startBatch();
            drawAtPosition(event->pos());
        }
        repaint();
    }
}

void QtCanvasWidget::mouseMoveEvent(QMouseEvent* event) {
    if (isDrawing) {
        ShapeTool* shapeTool = dynamic_cast<ShapeTool*>(activeTool);
        if (shapeTool) {
            // Для фигур: откатываем предыдущий preview и рисуем новый
            canvas.undo();
            canvas.startBatch();
            
            int x = std::clamp(event->pos().x() / pixelSize, 0, canvas.getWidth() - 1);
            int y = std::clamp(event->pos().y() / pixelSize, 0, canvas.getHeight() - 1);
            shapeTool->drawShape(canvas, x, y);
        } else {
            // Для обычных инструментов: рисуем линию
            drawLine(lastPos, event->pos());
            lastPos = event->pos();
        }
        repaint();
    }
}

void QtCanvasWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        isDrawing = false;
        
        ShapeTool* shapeTool = dynamic_cast<ShapeTool*>(activeTool);
        if (shapeTool) {
            int x = std::clamp(event->pos().x() / pixelSize, 0, canvas.getWidth() - 1);
            int y = std::clamp(event->pos().y() / pixelSize, 0, canvas.getHeight() - 1);
            shapeTool->drawShape(canvas, x, y);
            shapeTool->endShape();
        }
        
        canvas.endBatch();
        repaint();
    }
}

void QtCanvasWidget::clearCanvas() {
    canvas.clear();
    cacheDirty = true;
    repaint();
}

void QtCanvasWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Z && (event->modifiers() & Qt::ControlModifier)) {
        canvas.undo();
        cacheDirty = true;
        repaint();
        return;
    }
    QWidget::keyPressEvent(event);
}