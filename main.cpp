#include <QApplication>
#include <ElaApplication.h>
#include "mainwindow.h"
#include "settingswindow.h"
#include "tabwindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    eApp->init();
    // mainWindow w;
    // w.show();
    tabWindow tab;
    tab.show();
    // settingsWindow w;
    // w.show();
    return QApplication::exec();
}
