#include <QApplication>
#include <ElaApplication.h>
#include "serialwindow.h"
#include "settingswindow.h"
#include "mainwindow.h"
#include "cmdlineedit.h"
#include "cmdWidget.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    eApp->init();
    // mainWindow w;
    // w.show();
    mainWindow w;
    w.show();
    // settingsWindow w;
    // w.show();
    // cmdLineEdit cmd;
    // cmd.show();
    // cmdWidget cmdW;
    // cmdW.show();
    return QApplication::exec();
}
