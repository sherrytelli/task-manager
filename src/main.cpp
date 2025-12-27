#include <qt6/QtWidgets/QApplication>
#include "mainwindow.h"

int main (int argc, char *argv[]) {
    //creates the main QApplication
    QApplication app(argc, argv);

    //creates the mainwindow for the app
    MainWindow window;
    window.show();

    //returns the app event loop
    return app.exec();
}
