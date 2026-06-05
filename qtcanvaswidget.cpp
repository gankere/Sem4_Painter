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
      shapeStartPoint()
{
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
    setMouseTracking(true);  // ← ВАЖНО: отслеживание мыши даже без нажатия
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
    if (activeTool) {
        activeTool->setColor(color);
    }
    if (currentFactory) {
        currentFactory->setColor(color);
    }
}

void QtCanvasWidget::setBrushSize(int size) {
    brushSize = size;
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
            Pixel px = canvas.getPixel(x, y);
            if (!px.isEmpty()) {
                p.fillRect(x * pixelSize, y * pixelSize, pixelSize, pixelSize, px.color);
            }
        }
    }
    cacheDirty = false;
}

void QtCanvasWidget::drawDirectlyOnCache(int canvasX, int canvasY, const QColor& color, int brushSize) {
    if (canvasCache.isNull()) {
        cacheDirty = true;
        return;
    }
    
    QPainter p(&canvasCache);
    p.setPen(Qt::NoPen);
    
    int radius = brushSize / 2;
    
    // Точно так же как в BrushTool::use()
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

void QtCanvasWidget::paintEvent(QPaintEvent* event) {
    if (cacheDirty) {
        updateCache();
    }
    
    QPainter painter(this);
    painter.drawPixmap(0, 0, canvasCache);
    
    // ← Рисуем весь текст поверх кэша
    painter.setPen(QPen(Qt::black, 1));
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

    // ← ДОБАВИТЬ: определяем цвет
    QColor drawColor = Qt::white;  // Ластик
    EraserTool* eraser = dynamic_cast<EraserTool*>(activeTool);
    if (!eraser) {
        drawColor = activeToolColor;  // Кисть
    }

    while (true) {
        activeTool->use(canvas, x1, y1);
        drawDirectlyOnCache(x1, y1, drawColor, brushSize);  // ← Используем drawColor
        
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
    cacheDirty = false;
}

void QtCanvasWidget::mouseMoveEvent(QMouseEvent* event) {
    if (isDrawing) {
        ShapeTool* shapeTool = dynamic_cast<ShapeTool*>(activeTool);
        if (shapeTool) {
            // Для фигур - полная перерисовка
            canvas.undo();
            canvas.startBatch();
            
            int x = std::clamp(event->pos().x() / pixelSize, 0, canvas.getWidth() - 1);
            int y = std::clamp(event->pos().y() / pixelSize, 0, canvas.getHeight() - 1);
            shapeTool->drawShape(canvas, x, y);
            cacheDirty = true;
        } else {
            // Для кисти/ластика: рисуем линию и сразу на кэше
            drawLine(lastPos, event->pos());
            lastPos = event->pos();
        }
        
        update();
    }
}

void QtCanvasWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        int x = std::clamp(event->pos().x() / pixelSize, 0, canvas.getWidth() - 1);
        int y = std::clamp(event->pos().y() / pixelSize, 0, canvas.getHeight() - 1);

        // Проверяем, является ли инструмент TextTool
        TextTool* textTool = dynamic_cast<TextTool*>(activeTool);
        if (textTool) {
            showTextDialog(event->pos());
            return;  // ← Для текста НЕ устанавливаем isDrawing
        }

        // Для всех остальных инструментов
        isDrawing = true;  // ← Перенесли сюда
        lastPos = event->pos();

        ShapeTool* shapeTool = dynamic_cast<ShapeTool*>(activeTool);
        if (shapeTool) {
            shapeStartPoint = QPoint(x, y);
            canvas.startBatch();
            shapeTool->startShape(x, y);
            cacheDirty = true;
        } else {
            canvas.startBatch();
            activeTool->use(canvas, x, y);
            
            QColor drawColor = Qt::white;
            EraserTool* eraser = dynamic_cast<EraserTool*>(activeTool);
            if (!eraser) {
                drawColor = activeToolColor;
            }
            
            drawDirectlyOnCache(x, y, drawColor, brushSize);
            cacheDirty = false;
        }
        
        update();
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
        cacheDirty = true;  // На всякий случай пересоздадим кэш
        update();
    }
}

void QtCanvasWidget::clearCanvas() {
    canvas.clear();
    canvasCache.fill(Qt::white);
    cacheDirty = false;
    textItems.clear();  // ← Очищаем текст при очистке холста
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
                                         QLineEdit::Normal,
                                         "", &ok);
    
    if (ok && !text.isEmpty()) {
        // ← Сохраняем текст в список
        TextItem item;
        item.text = text;
        item.pos = pos;
        item.color = activeToolColor;
        textItems.append(item);
        
        // Перерисовываем
        cacheDirty = true;
        update();
    }
}