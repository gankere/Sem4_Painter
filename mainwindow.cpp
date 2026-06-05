#include "mainwindow.h"
#include "domain.h" 
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>

MainWindow::MainWindow(ICanvas& canvas, QWidget* parent)
    : QMainWindow(parent), canvas(canvas), activeColor(Qt::black) {
    
    QWidget* centralWidget = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // === ЛЕВАЯ ЧАСТЬ ===
    QWidget* leftPart = new QWidget;
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPart);
    leftLayout->setContentsMargins(0,0,0,0);
    leftLayout->setSpacing(0);

    // Верхняя панель
    QWidget* topToolbar = new QWidget;
    QHBoxLayout* toolbarLayout = new QHBoxLayout(topToolbar);
    toolbarLayout->setContentsMargins(5, 5, 5, 5);
    toolbarLayout->setSpacing(5);

    const int BTN_SIZE = 36;
    const QSize ICON_SIZE(24, 24);

    toolGroup = new QButtonGroup(this);
    toolGroup->setExclusive(true);

    // Лямбда для кнопок ИНСТРУМЕНТОВ (кисть, ластик, ведро, текст)
    auto createToolBtn = [&](const QString& iconPath, int id, bool checked = false) {
        auto* btn = new QPushButton;
        QIcon icon(iconPath); 
        if (!icon.isNull()) {
            btn->setIcon(icon);
            btn->setIconSize(ICON_SIZE);
        } else {
            btn->setText("?"); 
        }
        
        btn->setFixedSize(BTN_SIZE, BTN_SIZE);
        btn->setCheckable(true);
        btn->setChecked(checked);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet("background-color: transparent; border: 1px solid transparent; border-radius: 4px;");
        
        toolGroup->addButton(btn, id);
        toolbarLayout->addWidget(btn);
        return btn;
    };

    // Инструменты
    createToolBtn("icons/brush.png", 0, true);
    createToolBtn("icons/eraser.png", 1);
    createToolBtn("icons/bucket.png", 2);
    createToolBtn("icons/text.png", 3);

    // Разделитель
    auto* sep1 = new QFrame;
    sep1->setFrameShape(QFrame::VLine);
    sep1->setFrameShadow(QFrame::Sunken);
    sep1->setFixedHeight(30);
    sep1->setStyleSheet("background-color: #eef5ff; margin: 0 5px;");
    toolbarLayout->addWidget(sep1);

    shapeGroup = new QButtonGroup(this);
    shapeGroup->setExclusive(true);

    // Лямбда для кнопок ФИГУР (линия, прямоугольник, эллипс)
    auto createShapeBtn = [&](const QString& iconPath, int id) {
        auto* btn = new QPushButton;
        QIcon icon(iconPath); 
        if (!icon.isNull()) {
            btn->setIcon(icon);
            btn->setIconSize(ICON_SIZE);
        } else {
            btn->setText("?"); 
        }
        
        btn->setFixedSize(BTN_SIZE, BTN_SIZE);
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet("background-color: transparent; border: 1px solid transparent; border-radius: 4px;");
        
        shapeGroup->addButton(btn, id);
        toolbarLayout->addWidget(btn);
        return btn;
    };

    createShapeBtn("icons/line.png", 10);
    createShapeBtn("icons/rectangle.png", 11);
    createShapeBtn("icons/ellipse.png", 12);

    // Разделитель
    auto* sep2 = new QFrame;
    sep2->setFrameShape(QFrame::VLine);
    sep2->setFrameShadow(QFrame::Sunken);
    sep2->setFixedHeight(30);
    sep2->setStyleSheet("background-color: #ccc; margin: 0 5px;");
    toolbarLayout->addWidget(sep2);

    // === РАЗМЕР КИСТИ (как RGB) ===
    QLabel* sizeLabel = new QLabel("Размер:");
    sizeLabel->setFixedWidth(50);
    toolbarLayout->addWidget(sizeLabel);
    
    brushSizeSlider = new QSlider(Qt::Horizontal);
    brushSizeSlider->setRange(1, 100);
    brushSizeSlider->setValue(1);
    brushSizeSlider->setFixedWidth(80);
    toolbarLayout->addWidget(brushSizeSlider);
    
    toolbarLayout->addSpacing(5);
    
    brushSizeSpinBox = new QSpinBox;
    brushSizeSpinBox->setRange(1, 100);
    brushSizeSpinBox->setValue(1);
    brushSizeSpinBox->setFixedWidth(80);
    toolbarLayout->addWidget(brushSizeSpinBox);
    
    connect(brushSizeSlider, &QSlider::valueChanged, this, &MainWindow::onBrushSizeChanged);
    connect(brushSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onBrushSizeChanged);

    // Файловые операции
    toolbarLayout->addStretch();
    auto* saveBtn = new QPushButton("💾");
    saveBtn->setToolTip("Сохранить");
    saveBtn->setFixedSize(BTN_SIZE, BTN_SIZE);
    
    auto* loadBtn = new QPushButton("📂");
    loadBtn->setToolTip("Загрузить");
    loadBtn->setFixedSize(BTN_SIZE, BTN_SIZE);
    
    auto* clearBtn = new QPushButton("🗑️");
    clearBtn->setToolTip("Очистить");
    clearBtn->setFixedSize(BTN_SIZE, BTN_SIZE);
    
    toolbarLayout->addWidget(saveBtn);
    toolbarLayout->addWidget(loadBtn);
    toolbarLayout->addWidget(clearBtn);

    // Подключения
    connect(toolGroup, &QButtonGroup::buttonClicked, this, &MainWindow::onToolButtonClicked);
    connect(shapeGroup, &QButtonGroup::buttonClicked, this, &MainWindow::onShapeButtonClicked);
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::onSaveClicked);
    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::onLoadClicked);
    connect(clearBtn, &QPushButton::clicked, this, &MainWindow::onClearClicked);

    leftLayout->addWidget(topToolbar);
    
    // === КОНТЕЙНЕР ДЛЯ ХОЛСТА С СЕРЫМ ФОНОМ И СКРОЛЛЕРАМИ ===
    QWidget* canvasContainer = new QWidget;
    canvasContainer->setStyleSheet("background-color: #c6cfd8;");
    QHBoxLayout* containerLayout = new QHBoxLayout(canvasContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    
    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setStyleSheet("background-color: #bbccde; border: none;");
    scrollArea->setWidgetResizable(false);  // ← ВАЖНО: не растягивать виджет
    
    canvasWidget = new QtCanvasWidget(canvas, scrollArea);
    canvasWidget->setMinimumSize(800, 600);
    canvasWidget->setFocusPolicy(Qt::StrongFocus);
    canvasWidget->setFocus();
    
    scrollArea->setWidget(canvasWidget);
    containerLayout->addWidget(scrollArea, 1);
    
    leftLayout->addWidget(canvasContainer, 1);
    
    mainLayout->addWidget(leftPart, 1);
    
    // === ПРАВАЯ ЧАСТЬ (ПАЛИТРА) ===
    QWidget* rightPalette = new QWidget;
    rightPalette->setFixedWidth(160);
    rightPalette->setStyleSheet("background-color: #eef5ff; border-left: 1px solid #ccc;");
    QVBoxLayout* palLayout = new QVBoxLayout(rightPalette);
    palLayout->setContentsMargins(10, 10, 10, 10);
    
    currentColorBtn = new QPushButton;
    currentColorBtn->setFixedHeight(50);
    currentColorBtn->setStyleSheet("background-color: black; border: 2px solid #95a7ff; border-radius: 4px;");
    connect(currentColorBtn, &QPushButton::clicked, [=]() {
        QColor c = QColorDialog::getColor(activeColor, this, "Выберите цвет");
        if (c.isValid()) onColorChanged(c);
    });
    palLayout->addWidget(currentColorBtn);
    
    QVBoxLayout* rgbCol = new QVBoxLayout;
    rgbCol->setSpacing(5);

    rSpin = new QSpinBox; 
    rSpin->setRange(0, 255); 
    rSpin->setValue(activeColor.red());
    
    gSpin = new QSpinBox; 
    gSpin->setRange(0, 255); 
    gSpin->setValue(activeColor.green());
    
    bSpin = new QSpinBox; 
    bSpin->setRange(0, 255); 
    bSpin->setValue(activeColor.blue());

    auto addRgbRow = [&](const QString& label, QSpinBox* spin) {
        QHBoxLayout* row = new QHBoxLayout;
        QLabel* lbl = new QLabel(label);
        lbl->setFixedWidth(20);
        row->addWidget(lbl);
        row->addWidget(spin);
        rgbCol->addLayout(row);
    };

    addRgbRow("R:", rSpin);
    addRgbRow("G:", gSpin);
    addRgbRow("B:", bSpin);

    palLayout->addLayout(rgbCol);

    connect(rSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onRgbChanged);
    connect(gSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onRgbChanged);
    connect(bSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onRgbChanged);
    
    // Ползунок яркости
    QHBoxLayout* valLayout = new QHBoxLayout;
    valLayout->setSpacing(5);
    
    QLabel* valLabel = new QLabel("Val:");
    valLabel->setFixedWidth(30);
    
    brightnessSlider = new QSlider(Qt::Horizontal);
    brightnessSlider->setRange(0, 100);
    brightnessSlider->setValue(100);
    brightnessSlider->setToolTip("Регулировка яркости выбранного цвета");
    
    valLayout->addWidget(valLabel);
    valLayout->addWidget(brightnessSlider);
    
    palLayout->addLayout(valLayout);
    
    connect(brightnessSlider, &QSlider::valueChanged, this, [this](int val) {
        adjustBrightness(val);
    });

    QGridLayout* grid = new QGridLayout;
    QStringList colors = {"#E53935", "#FB8C00", "#FDD835", "#43A047", 
                          "#1E88E5", "#3949AB", "#8E24AA", "#D81B60", 
                          "#00ACC1", "#00897B", "#5D4037", "#757575"};
    int idx = 0;
    for (const QString& hex : colors) {
        auto* btn = new QPushButton;
        btn->setFixedSize(35, 35);
        btn->setStyleSheet(QString("background-color: %1; border: 1px solid #a1b1ff; border-radius: 6px;").arg(hex));
        connect(btn, &QPushButton::clicked, [=]() { onColorChanged(QColor(hex)); });
        grid->addWidget(btn, idx / 4, idx % 4);
        idx++;
    }
        palLayout->addLayout(grid);
    palLayout->addStretch();
    
    // ← РАЗМЕР ХОЛСТА
    canvasSizeLabel = new QLabel;
    canvasSizeLabel->setAlignment(Qt::AlignCenter);
    canvasSizeLabel->setStyleSheet("background-color: #dde9fa; padding: 5px; border-radius: 4px; font-weight: bold;");
    updateCanvasSizeLabel();  // Обновляем текст
    palLayout->addWidget(canvasSizeLabel);
    
    mainLayout->addWidget(rightPalette);
    
    setCentralWidget(centralWidget);
    resize(1100, 750);
}

// === СЛОТЫ ===

void MainWindow::onToolButtonClicked(QAbstractButton* btn) {
    int id = toolGroup->id(btn);
    onToolClicked(id);
}

void MainWindow::onShapeButtonClicked(QAbstractButton* btn) {
    int id = shapeGroup->id(btn);
    onShapeClicked(id);
}

void MainWindow::onToolClicked(int id) {
    QAbstractButton* activeBtn = toolGroup->button(id);
    updateActiveToolStyle(activeBtn);
    
    IToolFactory* factory = nullptr;
    switch (id) {
        case 0: // Кисть
            factory = new BrushFactory(activeColor, brushSizeSlider->value());
            canvasWidget->setActiveColor(activeColor);
            break;
        case 1: // Ластик
            factory = new EraserFactory(brushSizeSlider->value());
            break;
        case 2: // ВЕДРО!
            factory = new BucketFactory(activeColor);
            canvasWidget->setActiveColor(activeColor);
            break;
        case 3: // ТЕКСТ!
            factory = new TextFactory(activeColor);
            canvasWidget->setActiveColor(activeColor);
            break;
    }
    
    if (factory) {
        canvasWidget->updateFactory(factory);
        canvasWidget->setBrushSize(brushSizeSlider->value());
    }
}

void MainWindow::onShapeClicked(int id) {
    QAbstractButton* activeBtn = shapeGroup->button(id);
    updateActiveToolStyle(activeBtn);
    
    ShapeTool::Type shapeType;
    switch (id) {
        case 10: shapeType = ShapeTool::Line; break;
        case 11: shapeType = ShapeTool::Rect; break;
        case 12: shapeType = ShapeTool::Ellipse; break;
        default: return;
    }
    
    auto* factory = new ShapeFactory(activeColor, shapeType, brushSizeSlider->value());
    canvasWidget->updateFactory(factory);
    
    canvasWidget->setActiveColor(activeColor);
    canvasWidget->setBrushSize(brushSizeSlider->value());
}

void MainWindow::onBrushSizeChanged(int size) {
    canvasWidget->setBrushSize(size);
    
    brushSizeSpinBox->blockSignals(true);
    brushSizeSlider->blockSignals(true);
    
    brushSizeSpinBox->setValue(size);
    brushSizeSlider->setValue(size);
    
    brushSizeSpinBox->blockSignals(false);
    brushSizeSlider->blockSignals(false);
}

void MainWindow::onColorChanged(const QColor& color) {
    activeColor = color;
    currentColorBtn->setStyleSheet(
        QString("background-color: %1; border: 2px solid #999; border-radius: 4px;").arg(color.name())
    );
    rSpin->blockSignals(true); rSpin->setValue(color.red()); rSpin->blockSignals(false);
    gSpin->blockSignals(true); gSpin->setValue(color.green()); gSpin->blockSignals(false);
    bSpin->blockSignals(true); bSpin->setValue(color.blue()); bSpin->blockSignals(false);

    canvasWidget->setActiveColor(color);
}

void MainWindow::onRgbChanged() {
    QColor c(rSpin->value(), gSpin->value(), bSpin->value());
    onColorChanged(c);
}

void MainWindow::updateActiveToolStyle(QAbstractButton* btn) {
    QString defaultStyle = "background-color: transparent; border: 1px solid transparent; border-radius: 4px;";
    for (auto* b : toolGroup->buttons()) b->setStyleSheet(defaultStyle);
    for (auto* b : shapeGroup->buttons()) b->setStyleSheet(defaultStyle);
    
    if (btn) {
        btn->setStyleSheet("background-color: #cdf1eb; border: 1px solid #d2e6e2; border-radius: 4px;");
    }
}

void MainWindow::onClearClicked() {
    if (QMessageBox::question(this, "Подтверждение", "Очистить холст?") == QMessageBox::Yes) {
        canvasWidget->clearCanvas();
        updateCanvasSizeLabel();  // ← Добавь
    }
}

void MainWindow::adjustBrightness(int value) {
    if (!currentColorBtn) return;
    
    int h, s, v;
    activeColor.getHsv(&h, &s, &v);
    
    int newV = static_cast<int>(255.0 * value / 100.0);
    
    QColor newColor = QColor::fromHsv(h, s, newV);
    onColorChanged(newColor);
}

void QtCanvasWidget::updateCanvasSize() {
    int newWidth = canvas.getWidth() * pixelSize;
    int newHeight = canvas.getHeight() * pixelSize;
    setFixedSize(newWidth, newHeight);  // ← Фиксированный размер виджета
    cacheDirty = true;
}

void MainWindow::updateCanvasSizeLabel() {
    int width = canvas.getWidth();
    int height = canvas.getHeight();
    canvasSizeLabel->setText(QString("%1 x %2 px").arg(width).arg(height));
}

void MainWindow::onSaveClicked() {
    QString path = QFileDialog::getSaveFileName(
        this, 
        "Сохранить изображение", 
        "", 
        "PNG (*.png);;JPG (*.jpg *.jpeg);;BMP (*.bmp)"
    );
    
    if (path.isEmpty()) return;
    
    // Определяем формат по расширению
    QString format;
    if (path.endsWith(".png", Qt::CaseInsensitive)) {
        format = "PNG";
    } else if (path.endsWith(".jpg", Qt::CaseInsensitive) || path.endsWith(".jpeg", Qt::CaseInsensitive)) {
        format = "JPG";
    } else if (path.endsWith(".bmp", Qt::CaseInsensitive)) {
        format = "BMP";
    } else {
        // По умолчанию PNG
        path += ".png";
        format = "PNG";
    }
    
    // Создаём изображение из canvas
    QImage image(canvas.getWidth(), canvas.getHeight(), QImage::Format_RGB32);
    
    for (int y = 0; y < canvas.getHeight(); ++y) {
        for (int x = 0; x < canvas.getWidth(); ++x) {
            Pixel px = canvas.getPixel(x, y);
            if (px.isEmpty()) {
                image.setPixelColor(x, y, Qt::white);  // Прозрачные = белые
            } else {
                image.setPixelColor(x, y, px.color);
            }
        }
    }
    
    // Сохраняем
    if (image.save(path, format.toUtf8().constData())) {
        QMessageBox::information(this, "Успех", "Изображение сохранено!");
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить изображение!");
    }
}

void MainWindow::onLoadClicked() {
    QString path = QFileDialog::getOpenFileName(
        this, 
        "Загрузить изображение", 
        "", 
        "Images (*.png *.jpg *.jpeg *.bmp)"
    );
    
    if (path.isEmpty()) return;
    
    QImage image;
    if (!image.load(path)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить изображение!");
        return;
    }
    
    // ← Сначала очищаем canvas
    canvas.clear();
    
    // Загружаем пиксели
    int width = std::min(image.width(), canvas.getWidth());
    int height = std::min(image.height(), canvas.getHeight());
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            QColor color = image.pixelColor(x, y);
            canvas.setPixel(x, y, Pixel(color));
        }
    }
    
    // ← Обновляем отображение (НЕ clearCanvas!)
    canvasWidget->updateCanvasSize();
    canvasWidget->update();
    
    QMessageBox::information(this, "Успех", "Изображение загружено!");
}