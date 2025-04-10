#include <QApplication>
#include <ElaApplication.h>
#include <ElaTheme.h>

// #include "serialwindow.h"
// #include "settingswindow.h"
// #include "mainwindow.h"
// #include "cmdlineedit.h"
// #include "cmdWidget.h"
#include "serialserver.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    eApp->init();
    // mainWindow w;
    // w.show();
    // settingsWindow w;
    // w.show();
    // cmdLineEdit cmd;
    // cmd.show();
    // cmdWidget cmdW;
    // cmdW.show();

    eTheme->setThemeMode(
        ElaThemeType::Dark
    );
    serialServer server;
    server.show();

    return QApplication::exec();
}
