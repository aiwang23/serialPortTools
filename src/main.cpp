#include <QApplication>
#include <ElaApplication.h>

#include "ElaTheme.h"
#include "mainWindow.h"
#include "sftpWindow/sftpWindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    eApp->init();
    mainWindow w;
    w.show();
    // eTheme->setThemeMode(ElaThemeType::Dark);
    // sftpWindow window;
    // window.showLoginWindow();
    // window.show();
    return QApplication::exec();
}
