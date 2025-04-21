#include <QApplication>
#include <ElaApplication.h>
#include "mainWindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    eApp->init();
    mainWindow w;
    w.show();
    return QApplication::exec();
}
