#include <QApplication>
#include "mainwindow.h"
#include "canvasdialog.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindow.show();

    CanvasDialog sizeDialog(&mainWindow); //диалог поверх главного окна
    if (sizeDialog.exec() != QDialog::Accepted) {
        return 0;
    }

    mainWindow.createCanvas(sizeDialog.getWidth(), sizeDialog.getHeight());

    return app.exec();
}