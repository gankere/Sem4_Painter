#include "canvasdialog.h"

CanvasDialog::CanvasDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Новый холст");
    setFixedSize(300, 150);

    auto* mainLayout = new QVBoxLayout(this);

    auto* widthLayout = new QHBoxLayout();
    widthLayout->addWidget(new QLabel("Ширина:"));
    widthSpinBox = new QSpinBox();
    widthSpinBox->setRange(10, 2000);
    widthSpinBox->setValue(500);
    widthLayout->addWidget(widthSpinBox);
    mainLayout->addLayout(widthLayout);

    auto* heightLayout = new QHBoxLayout();
    heightLayout->addWidget(new QLabel("Высота:"));
    heightSpinBox = new QSpinBox();
    heightSpinBox->setRange(10, 2000);
    heightSpinBox->setValue(500);
    heightLayout->addWidget(heightSpinBox);
    mainLayout->addLayout(heightLayout);

    // Кнопки меню
    auto* buttonLayout = new QHBoxLayout();
    okButton = new QPushButton("ОК");
    cancelButton = new QPushButton("Отмена");
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

int CanvasDialog::getWidth() const {
    return widthSpinBox->value();
}

int CanvasDialog::getHeight() const {
    return heightSpinBox->value();
}