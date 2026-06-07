#pragma once
#include <QDialog>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

class CanvasDialog : public QDialog {
    Q_OBJECT

public:
    explicit CanvasDialog(QWidget* parent = nullptr);
    int getWidth() const;
    int getHeight() const;

private:
    QSpinBox* widthSpinBox;
    QSpinBox* heightSpinBox;
    QPushButton* okButton;
    QPushButton* cancelButton;
};