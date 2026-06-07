#include "qtcanvaswidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QInputDialog>
#include <QLineEdit>
#include <algorithm>

QtCanvasWidget::QtCanvasWidget(ICanvas* canvas, QWidget* parent)
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
      previewCache(),
      hasPreview(false)
{
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
    setMouseTracking(true);

    if (canvas) {
        updateCanvasSize();
    }
}

QtCanvasWidget::~QtCanvasWidget() {
    delete activeTool;
    delete currentFactory;
}

//Смена инструмента
void QtCanvasWidget::updateFactory(IToolFactory* factory) { 
    delete activeTool;
    delete currentFactory;
    currentFactory = factory;
    activeTool = currentFactory->create();
    updateCursor();
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
    updateCursor();
}

void QtCanvasWidget::setPixelSize(int size) {
    if (size == pixelSize) return;
    
    pixelSize = size;
    updateCanvasSize();
    cacheDirty = true;
    updateCursor();
    update();
}

void QtCanvasWidget::updateCache() {
    if (!canvas) return;

    canvasCache = QPixmap(canvas->getWidth() * pixelSize, canvas->getHeight() * pixelSize);
    canvasCache.fill(Qt::white);
    
    QPainter p(&canvasCache);
    p.setPen(Qt::NoPen);
    
    for (int y = 0; y < canvas->getHeight(); ++y) {
        for (int x = 0; x < canvas->getWidth(); ++x) {
            Pixel px = canvas->getPixel(x, y);
            if (!px.isEmpty()) {
                p.fillRect(x * pixelSize, y * pixelSize, pixelSize, pixelSize, QColor::fromRgba(px.color));
            }
        }
    }
    cacheDirty = false;
}

void QtCanvasWidget::drawDirectlyOnCache(int canvasX, int canvasY, QRgb color, int bSize) {
    if (canvasCache.isNull()) {
        cacheDirty = true;
        return;
    }
    
    QPainter p(&canvasCache);
    p.setPen(Qt::NoPen);
    
    bool isEraser = dynamic_cast<EraserTool*>(activeTool) != nullptr;

    if (isEraser) {
        // Для ластика квадрат
        int radius = bSize / 2;
        int x = (canvasX - radius) * pixelSize;
        int y = (canvasY - radius) * pixelSize;
        int size = bSize * pixelSize;
        p.fillRect(x, y, size, size, QColor::fromRgba(color));
    } else {
        // Для кисти круг
        int radius = bSize / 2;
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                if (dx*dx + dy*dy <= radius*radius) {
                    int screenX = (canvasX + dx) * pixelSize;
                    int screenY = (canvasY + dy) * pixelSize;
                    p.fillRect(screenX, screenY, pixelSize, pixelSize, QColor::fromRgba(color));
                }
            }
        }
    }
    p.end();
}

void QtCanvasWidget::paintEvent(QPaintEvent*) {
    if (cacheDirty) updateCache();
    
    QPainter painter(this);
    painter.drawPixmap(0, 0, canvasCache);
    
    if (hasPreview && !previewCache.isNull()) {
        painter.drawPixmap(0, 0, previewCache);
    }
    
    QFont font;
    font.setPointSize(12);
    painter.setFont(font);
    for (const TextItem& item : textItems) {
        painter.setPen(QPen(item.color, 1));
        painter.drawText(item.pos, item.text);
    }
}

void QtCanvasWidget::drawAtPosition(const QPoint& pos) {
    int x = std::clamp(pos.x() / pixelSize, 0, canvas->getWidth() - 1);
    int y = std::clamp(pos.y() / pixelSize, 0, canvas->getHeight() - 1);
    activeTool->use(*canvas, x, y);
    cacheDirty = true;
}

void QtCanvasWidget::drawLine(const QPoint& from, const QPoint& to) {
    int x1 = std::clamp(from.x() / pixelSize, 0, canvas->getWidth() - 1);
    int y1 = std::clamp(from.y() / pixelSize, 0, canvas->getHeight() - 1);
    int x2 = std::clamp(to.x() / pixelSize, 0, canvas->getWidth() - 1);
    int y2 = std::clamp(to.y() / pixelSize, 0, canvas->getHeight() - 1);

    int dx = std::abs(x2 - x1), dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1, sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    QRgb drawColor = qRgb(255, 255, 255);
    EraserTool* eraser = dynamic_cast<EraserTool*>(activeTool);
    if (!eraser) drawColor = activeToolColor.rgba();

    while (true) {
        activeTool->use(*canvas, x1, y1);

        if (eraser) {
            eraseTextAtCanvasPos(x1, y1);
        }

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
    
    int x = std::clamp(event->pos().x() / pixelSize, 0, canvas->getWidth() - 1);
    int y = std::clamp(event->pos().y() / pixelSize, 0, canvas->getHeight() - 1);

    TextTool* textTool = dynamic_cast<TextTool*>(activeTool);
    if (textTool) {
        showTextDialog(event->pos());
        return;
    }

    BucketTool* bucketTool = dynamic_cast<BucketTool*>(activeTool);
    if (bucketTool) {
        canvas->startBatch();
        bucketTool->use(*canvas, x, y);
        canvas->endBatch();
        cacheDirty = true;
        update();
        return;
    }

    ShapeTool* shapeTool = dynamic_cast<ShapeTool*>(activeTool);
    
    if (shapeTool) {
        shapeStartCanvas = QPoint(x, y);
        isDrawing = true;
        hasPreview = false;
    } else {
        isDrawing = true;
        lastPos = event->pos();
        canvas->startBatch();
        activeTool->use(*canvas, x, y);
        
        QRgb drawColor = qRgb(255, 255, 255);
        EraserTool* eraser = dynamic_cast<EraserTool*>(activeTool);
        if (!eraser) drawColor = activeToolColor.rgba();
        
        if (eraser) {
            eraseTextAtCanvasPos(x, y);
        }

        drawDirectlyOnCache(x, y, drawColor, brushSize);
        cacheDirty = false;
        update();
    }
}

void QtCanvasWidget::mouseMoveEvent(QMouseEvent* event) {
    if (!isDrawing) return;
    
    BucketTool* bucketTool = dynamic_cast<BucketTool*>(activeTool);
    if (bucketTool) return;
    
    ShapeTool* shapeTool = dynamic_cast<ShapeTool*>(activeTool);
    if (shapeTool) {
        int x = std::clamp(event->pos().x() / pixelSize, 0, canvas->getWidth() - 1);
        int y = std::clamp(event->pos().y() / pixelSize, 0, canvas->getHeight() - 1);
        
        drawPreview(shapeStartCanvas, QPoint(x, y));
        hasPreview = true;
        update();
    } else {
        drawLine(lastPos, event->pos());
        lastPos = event->pos();
        update();
    }
}

void QtCanvasWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) return;
    isDrawing = false;
    
    BucketTool* bucketTool = dynamic_cast<BucketTool*>(activeTool);
    if (bucketTool) return;
    
    ShapeTool* shapeTool = dynamic_cast<ShapeTool*>(activeTool);
    if (shapeTool) {
        clearPreview();
        
        int x = std::clamp(event->pos().x() / pixelSize, 0, canvas->getWidth() - 1);
        int y = std::clamp(event->pos().y() / pixelSize, 0, canvas->getHeight() - 1);
        
        canvas->startBatch();
        shapeTool->startShape(shapeStartCanvas.x(), shapeStartCanvas.y());
        shapeTool->drawShape(*canvas, x, y);
        shapeTool->endShape();
        canvas->endBatch();
        
        cacheDirty = true;
        update();
    } else {
        canvas->endBatch();
    }
}

void QtCanvasWidget::clearCanvas() {
    canvas->clear();
    canvasCache.fill(Qt::white);
    cacheDirty = false;
    textItems.clear();
    update();
}

void QtCanvasWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Z && (event->modifiers() & Qt::ControlModifier)) {
        canvas->undo();
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
    
    int canvasW = canvas->getWidth();
    int canvasH = canvas->getHeight();
    int cacheW = canvasW * pixelSize;
    int cacheH = canvasH * pixelSize;
    
    // Создаём QImage для preview
    if (previewImage.isNull() || previewImage.size() != QSize(cacheW, cacheH)) {
        previewImage = QImage(cacheW, cacheH, QImage::Format_ARGB32);
    }
    previewImage.fill(qRgba(0, 0, 0, 0));  // Прозрачный фон
    
    // Получаем доступ к памяти
    ShapeTool* shapeTool = dynamic_cast<ShapeTool*>(activeTool);
    if (!shapeTool) return;
    
    QRgb color = activeToolColor.rgba();
    int brushRadius = brushSize / 2;
    
    // Рисуем фигуру пикселями (так же, как в ShapeTool)
    switch (shapeTool->getShapeType()) {
        case ShapeTool::Line: {
            int x1 = startCanvas.x(), y1 = startCanvas.y();
            int x2 = currentCanvas.x(), y2 = currentCanvas.y();
            
            int dx = std::abs(x2 - x1), dy = std::abs(y2 - y1);
            int sx = (x1 < x2) ? 1 : -1, sy = (y1 < y2) ? 1 : -1;
            int err = dx - dy;
            
            while (true) {
                // Рисуем кисть в точке (x1, y1)
                for (int bdy = -brushRadius; bdy <= brushRadius; ++bdy) {
                    for (int bdx = -brushRadius; bdx <= brushRadius; ++bdx) {
                        if (bdx*bdx + bdy*bdy <= brushRadius*brushRadius) {
                            int px = x1 + bdx;
                            int py = y1 + bdy;
                            if (px >= 0 && px < canvasW && py >= 0 && py < canvasH) {
                                // Заполняем блок pixelSize × pixelSize
                                int screenX = px * pixelSize;
                                int screenY = py * pixelSize;
                                for (int py2 = 0; py2 < pixelSize; ++py2) {
                                    QRgb* destLine = reinterpret_cast<QRgb*>(previewImage.scanLine(screenY + py2));
                                    for (int px2 = 0; px2 < pixelSize; ++px2) {
                                        destLine[screenX + px2] = color;
                                    }
                                }
                            }
                        }
                    }
                }
                
                if (x1 == x2 && y1 == y2) break;
                int e2 = 2 * err;
                if (e2 > -dy) { err -= dy; x1 += sx; }
                if (e2 < dx) { err += dx; y1 += sy; }
            }
            break;
        }
        
        case ShapeTool::Rect: {
            int minX = std::min(startCanvas.x(), currentCanvas.x());
            int maxX = std::max(startCanvas.x(), currentCanvas.x());
            int minY = std::min(startCanvas.y(), currentCanvas.y());
            int maxY = std::max(startCanvas.y(), currentCanvas.y());
            
            int halfSize = brushRadius;  // Толщина рамки
            
            // Рисуем РАМКУ (не заполненный!)
            for (int y = minY - halfSize; y <= maxY + halfSize; ++y) {
                for (int x = minX - halfSize; x <= maxX + halfSize; ++x) {
                    if (x >= 0 && x < canvasW && y >= 0 && y < canvasH) {
                        // Проверяем, находится ли пиксель в рамке
                        bool nearTop = (y >= minY - halfSize && y <= minY + halfSize);
                        bool nearBottom = (y >= maxY - halfSize && y <= maxY + halfSize);
                        bool nearLeft = (x >= minX - halfSize && x <= minX + halfSize);
                        bool nearRight = (x >= maxX - halfSize && x <= maxX + halfSize);
                        
                        // Рисуем только если в рамке
                        if (nearTop || nearBottom || nearLeft || nearRight) {
                            int screenX = x * pixelSize;
                            int screenY = y * pixelSize;
                            for (int py = 0; py < pixelSize; ++py) {
                                QRgb* destLine = reinterpret_cast<QRgb*>(previewImage.scanLine(screenY + py));
                                for (int px = 0; px < pixelSize; ++px) {
                                    destLine[screenX + px] = color;
                                }
                            }
                        }
                    }
                }
            }
        break;
}
        
        case ShapeTool::Ellipse: {
            int centerX = (startCanvas.x() + currentCanvas.x()) / 2;
            int centerY = (startCanvas.y() + currentCanvas.y()) / 2;
            int radiusX = std::abs(currentCanvas.x() - startCanvas.x()) / 2;
            int radiusY = std::abs(currentCanvas.y() - startCanvas.y()) / 2;
            
            int steps = std::max(radiusX, radiusY) * 8;
            if (steps < 100) steps = 100;
            
            for (int i = 0; i < steps; ++i) {
                double angle = 2.0 * 3.14159265359 * i / steps;
                int x = centerX + static_cast<int>(radiusX * std::cos(angle) + 0.5);
                int y = centerY + static_cast<int>(radiusY * std::sin(angle) + 0.5);
                
                for (int bdy = -brushRadius; bdy <= brushRadius; ++bdy) {
                    for (int bdx = -brushRadius; bdx <= brushRadius; ++bdx) {
                        if (bdx*bdx + bdy*bdy <= brushRadius*brushRadius) {
                            int px = x + bdx, py = y + bdy;
                            if (px >= 0 && px < canvasW && py >= 0 && py < canvasH) {
                                int screenX = px * pixelSize;
                                int screenY = py * pixelSize;
                                for (int py2 = 0; py2 < pixelSize; ++py2) {
                                    QRgb* destLine = reinterpret_cast<QRgb*>(previewImage.scanLine(screenY + py2));
                                    for (int px2 = 0; px2 < pixelSize; ++px2) {
                                        destLine[screenX + px2] = color;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            break;
        }
    }
    
    // Конвертируем в QPixmap для отображения
    previewCache = QPixmap::fromImage(previewImage);
}

void QtCanvasWidget::clearPreview() {
    if (!previewCache.isNull()) {
        previewCache.fill(Qt::transparent);
    }
    hasPreview = false;
}

void QtCanvasWidget::eraseTextAtCanvasPos(int canvasX, int canvasY) {
    // Центр ластика в экранных координатах
    int centerX = canvasX * pixelSize + pixelSize / 2;
    int centerY = canvasY * pixelSize + pixelSize / 2;
    int radius = (brushSize * pixelSize) / 2;
    
    // Прямоугольник ластика
    QRect eraserRect(centerX - radius, centerY - radius, radius * 2, radius * 2);
    
    // Шрифт для измерения текста
    QFont font;
    font.setPointSize(12);
    QFontMetrics fm(font);
    
    bool changed = false;
    
    // Удаляем текст, если ластик пересекает его рамку
    for (int i = textItems.size() - 1; i >= 0; --i) {
        QRect textRect = fm.boundingRect(textItems[i].text);
        textRect.translate(textItems[i].pos);
        
        if (eraserRect.intersects(textRect)) {
            textItems.removeAt(i);
            changed = true;
        }
    }
    
    if (changed) update();
}

void QtCanvasWidget::updateCursor() {
    if (dynamic_cast<EraserTool*>(activeTool)) {
        int cursorSize = brushSize * pixelSize;
        
        QPixmap pixmap(cursorSize, cursorSize);
        pixmap.fill(Qt::transparent);
        
        QPainter p(&pixmap);
        p.setPen(Qt::black);
        p.drawRect(0, 0, cursorSize - 1, cursorSize - 1);
        p.end();
        
        int radius = brushSize / 2;
        setCursor(QCursor(pixmap, radius * pixelSize, radius * pixelSize));
    } else {
        setCursor(Qt::ArrowCursor);
    }
}

void QtCanvasWidget::setCanvas(ICanvas& newCanvas) {
    canvas = &newCanvas;
    
    canvasCache = QPixmap(); //Очистка кеша
    cacheDirty = true;
    
    textItems.clear(); //текста
    
    updateCanvasSize();
    update();
}

void QtCanvasWidget::updateCanvasSize() {
    int newWidth = canvas->getWidth() * pixelSize;
    int newHeight = canvas->getHeight() * pixelSize;
    setFixedSize(newWidth, newHeight);
    cacheDirty = true;
}

int QtCanvasWidget::getCanvasWidth() const {
    return canvas->getWidth();
}

int QtCanvasWidget::getCanvasHeight() const {
    return canvas->getHeight();
}