#include <QApplication>
#include <ElaApplication.h>
#include "serialwindow.h"
#include "settingswindow.h"
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    eApp->init();
    // mainWindow w;
    // w.show();
    mainWindow w;
    w.show();
    // settingsWindow w;
    // w.show();
    return QApplication::exec();
}
