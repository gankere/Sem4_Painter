#include <QApplication>
#include "domain.h"
#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    Canvas canvas(200, 200); // размер сетки

    MainWindow window(canvas);
    window.setWindowTitle("Pixel Editor Qt");
    window.resize(1100, 750);
    window.show();

    return app.exec();
}