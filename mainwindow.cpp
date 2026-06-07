#include "mainwindow.h"
#include "domain.h" 
#include "canvasdialog.h" 
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QtConcurrent>
#include <QThreadPool>
#include <QRunnable>


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), canvas(nullptr), activeColor(Qt::black) {
    
    QWidget* centralWidget = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0); //отступы
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
        btn->setCheckable(true); //состояние кнопки
        btn->setChecked(checked);
        btn->setCursor(Qt::PointingHandCursor); //изменение курсора (кликабельный эл-т)
        btn->setStyleSheet("background-color: transparent; border: 1px solid transparent; border-radius: 4px;");
        
        toolGroup->addButton(btn, id);
        toolbarLayout->addWidget(btn);
        return btn;
    };

    // Иконки инструментов
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

    // === РАЗМЕР КИСТИ ===
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
    
    connect(brushSizeSlider, &QSlider::valueChanged, this, &MainWindow::onBrushSizeChanged); //срабатывает, когда пользователь двигает ползунок
    connect(brushSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onBrushSizeChanged); //срабатывает при изменении числа

    // === ФАЙЛОВЫЕ ОПЕРАЦИИ ===
    toolbarLayout->addStretch();

    auto* newCanvasBtn = new QPushButton("📄");
    newCanvasBtn->setToolTip("Новый холст");
    newCanvasBtn->setFixedSize(BTN_SIZE, BTN_SIZE);

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
    
    toolbarLayout->addWidget(newCanvasBtn);
    toolbarLayout->addWidget(saveBtn);
    toolbarLayout->addWidget(loadBtn);
    toolbarLayout->addWidget(clearBtn);

    // Подключения
    connect(newCanvasBtn, &QPushButton::clicked, this, &MainWindow::onNewCanvasClicked);
    connect(toolGroup, &QButtonGroup::buttonClicked, this, &MainWindow::onToolButtonClicked);
    connect(shapeGroup, &QButtonGroup::buttonClicked, this, &MainWindow::onShapeButtonClicked);
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::onSaveClicked);
    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::onLoadClicked);
    connect(clearBtn, &QPushButton::clicked, this, &MainWindow::onClearClicked);

    leftLayout->addWidget(topToolbar);
    
    // === КОНТЕЙНЕР ДЛЯ ХОЛСТА С СЕРЫМ ФОНОМ И СКРОЛЛЕРАМИ ===
    QWidget* canvasContainer = new QWidget; //фон за холстом
    canvasContainer->setStyleSheet("background-color: #c6cfd8;"); 
    QHBoxLayout* containerLayout = new QHBoxLayout(canvasContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    
    scrollArea = new QScrollArea; //скроллеры
    scrollArea->setStyleSheet("background-color: #bbccde; border: none;"); 
    scrollArea->setWidgetResizable(false);

    canvasWidget = new QtCanvasWidget(nullptr, scrollArea);
    scrollArea->hide();

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
    
    //=== ЯРКОСТЬ ===
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
        btn->setStyleSheet(QString("background-color: %1; border: 1px solid #dde9fa; border-radius: 6px;").arg(hex));
        connect(btn, &QPushButton::clicked, [=]() { onColorChanged(QColor(hex)); });
        grid->addWidget(btn, idx / 4, idx % 4);
        idx++;
    }
        palLayout->addLayout(grid);
    palLayout->addStretch();
    
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
    if (!canvas) return;
    QAbstractButton* activeBtn = toolGroup->button(id);
    updateActiveToolStyle(activeBtn);
    
    IToolFactory* factory = nullptr;
    switch (id) {
        case 0: // КИСТЬ
            factory = new BrushFactory(activeColor, brushSizeSlider->value());
            canvasWidget->setActiveColor(activeColor);
            break;
        case 1: // ЛАСТИК
            factory = new EraserFactory(brushSizeSlider->value());
            break;
        case 2: // ЗАЛИВКА
            factory = new BucketFactory(activeColor);
            canvasWidget->setActiveColor(activeColor);
            break;
        case 3: // ТЕКСТ
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
    if (!canvas) {
        QMessageBox::warning(this, "Внимание", "Сначала создайте холст!");
        return;
    }
    
    if (QMessageBox::question(this, "Подтверждение", "Очистить холст?") == QMessageBox::Yes) {
        canvasWidget->clearCanvas();
        updateCanvasSizeLabel();
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

void MainWindow::updateCanvasSizeLabel() {
    if (canvas) {
        int width = canvas->getWidth();
        int height = canvas->getHeight();
        canvasSizeLabel->setText(QString("%1 x %2 px").arg(width).arg(height));
    } else {
        canvasSizeLabel->setText("0 x 0 px");
    }
}

void MainWindow::onSaveClicked() {
    if (!canvas) {
        QMessageBox::warning(this, "Внимание", "Сначала создайте холст!");
        return;
    }
    
    QString path = QFileDialog::getSaveFileName(
        this, 
        "Сохранить изображение", 
        "", 
        "PNG (*.png);;JPG (*.jpg *.jpeg);;BMP (*.bmp)"
    );
    
    if (path.isEmpty()) return;

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
    QImage image(canvas->getWidth(), canvas->getHeight(), QImage::Format_RGB32);
    
    for (int y = 0; y < canvas->getHeight(); ++y) {
        for (int x = 0; x < canvas->getWidth(); ++x) {
            Pixel px = canvas->getPixel(x, y);
            if (px.isEmpty()) {
                image.setPixel(x, y, qRgb(255, 255, 255));
            } else {
                image.setPixel(x, y, px.color);
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
        this, "Загрузить изображение", "", 
        "Images (*.png *.jpg *.jpeg *.bmp)"
    );
    
    if (path.isEmpty()) return;
    
    QImage image;
    if (!image.load(path)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить изображение!");
        return;
    }
    
    int imgWidth = image.width();
    int imgHeight = image.height();
    
    if (!canvas || canvas->getWidth() < imgWidth || canvas->getHeight() < imgHeight) {
        QMessageBox::StandardButton reply;
        if (canvas) {
            reply = QMessageBox::question(
                this, "Создать новый холст?", 
                QString("Размер изображения (%1x%2) больше текущего холста.\nСоздать новый холст?")
                    .arg(imgWidth).arg(imgHeight),
                QMessageBox::Yes | QMessageBox::No
            );
        } else {
            reply = QMessageBox::Yes;
        }
        
        if (reply == QMessageBox::Yes) {
            delete canvas;
            canvas = new Canvas(imgWidth, imgHeight);
            canvasWidget->setCanvas(*canvas);
            scrollArea->show();
            updateCanvasSizeLabel();
        } else {
            return;
        }
    }
    
    setCursor(Qt::WaitCursor);
    
    int width = std::min(imgWidth, canvas->getWidth());
    int height = std::min(imgHeight, canvas->getHeight());
    
    canvas->loadFromImage(image, 0, 0, width, height);
    
    setCursor(Qt::ArrowCursor);
    
    canvasWidget->updateCanvasSize();
    canvasWidget->update();
}

void MainWindow::createCanvas(int width, int height) {
    canvas = new Canvas(width, height);
    canvasWidget->setCanvas(*canvas);
    canvasWidget->show();
    scrollArea->show();
    updateCanvasSizeLabel();
}

void MainWindow::onNewCanvasClicked() {
    CanvasDialog sizeDialog(this);
    if (sizeDialog.exec() != QDialog::Accepted) {
        return;
    }
    
    int width = sizeDialog.getWidth();
    int height = sizeDialog.getHeight();
    
    delete canvas;
    
    canvas = new Canvas(width, height);
    
    canvasWidget->setCanvas(*canvas); //обновление виджета
    
    updateCanvasSizeLabel();
}
