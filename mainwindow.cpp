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
    
    // Лямбда для создания кнопок с иконками
    auto createIconBtn = [&](const QString& iconPath, int id, bool checked = false) {
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
    createIconBtn("icons/brush.png", 0, true);
    createIconBtn("icons/eraser.png", 1);
    createIconBtn("icons/bucket.png", 2);
    createIconBtn("icons/text.png", 3);

    // Разделитель
    auto* sep1 = new QFrame;
    sep1->setFrameShape(QFrame::VLine);
    sep1->setFrameShadow(QFrame::Sunken);
    sep1->setFixedHeight(30);
    sep1->setStyleSheet("background-color: #eef5ff; margin: 0 5px;");
    toolbarLayout->addWidget(sep1);

    shapeGroup = new QButtonGroup(this);
    shapeGroup->setExclusive(true);

    createIconBtn("icons/line.png", 10);
    createIconBtn("icons/rectangle.png", 11);
    createIconBtn("icons/ellipse.png", 12);

       // Разделитель
    auto* sep2 = new QFrame;
    sep2->setFrameShape(QFrame::VLine);
    sep2->setFrameShadow(QFrame::Sunken);
    sep2->setFixedHeight(30);
    sep2->setStyleSheet("background-color: #ccc; margin: 0 5px;");
    toolbarLayout->addWidget(sep2);

    // === РАЗМЕР КИСТИ (как RGB) ===
    // Лейбл "Размер:"
    QLabel* sizeLabel = new QLabel("Размер:");
    sizeLabel->setFixedWidth(50);
    toolbarLayout->addWidget(sizeLabel);
    
    // Слайдер
    brushSizeSlider = new QSlider(Qt::Horizontal);
    brushSizeSlider->setRange(1, 100);
    brushSizeSlider->setValue(1);
    brushSizeSlider->setFixedWidth(80);
    toolbarLayout->addWidget(brushSizeSlider);
    
    // Отступ
    toolbarLayout->addSpacing(5);
    
    // QSpinBox
    brushSizeSpinBox = new QSpinBox;
    brushSizeSpinBox->setRange(1, 100);
    brushSizeSpinBox->setValue(1);
    brushSizeSpinBox->setFixedWidth(80);
    toolbarLayout->addWidget(brushSizeSpinBox);
    
    // СВЯЗЫВАЕМ
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
    
    canvasWidget = new QtCanvasWidget(canvas, this);
    canvasWidget->setMinimumSize(800, 600);
    canvasWidget->setFocusPolicy(Qt::StrongFocus);
    canvasWidget->setFocus();
    leftLayout->addWidget(canvasWidget, 1);
    
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
        case 0: factory = new BrushFactory(activeColor, brushSizeSlider->value()); break;
        case 1: factory = new EraserFactory(brushSizeSlider->value()); break;
        case 2: // Bucket - пока заглушка
        case 3: // Text - пока заглушка
            factory = new BrushFactory(activeColor, brushSizeSlider->value()); break;
    }
    
    if (factory) {
        canvasWidget->updateFactory(factory);
        canvasWidget->setActiveColor(activeColor);
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
    
    // Синхронизируем как RGB
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

void MainWindow::onSaveClicked() {
    QString path = QFileDialog::getSaveFileName(this, "Сохранить изображение", "", "PNG (*.png);;JPG (*.jpg);;BMP (*.bmp)");
    if (!path.isEmpty()) {
        QMessageBox::information(this, "Инфо", "Функция сохранения будет реализована далее.");
    }
}

void MainWindow::onLoadClicked() {
    QString path = QFileDialog::getOpenFileName(this, "Загрузить изображение", "", "Images (*.png *.jpg *.bmp)");
    if (!path.isEmpty()) {
        QMessageBox::information(this, "Инфо", "Функция загрузки будет реализована далее.");
    }
}

void MainWindow::onClearClicked() {
    if (QMessageBox::question(this, "Подтверждение", "Очистить холст?") == QMessageBox::Yes) {
        canvasWidget->clearCanvas();  // ← Вызываем оптимизированный метод
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