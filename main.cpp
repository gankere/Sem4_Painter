#include <QApplication>
#include "domain.h"
#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    Canvas canvas(570, 300); // размер холста

    MainWindow window(canvas);
    window.setWindowTitle("Pixel Editor Qt");
    window.resize(1100, 750); // размер окна
    window.show();

    return app.exec();
}