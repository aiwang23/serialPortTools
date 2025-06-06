//
// Created by wang on 25-4-2.
//

#ifndef TABWINDOW_H
#define TABWINDOW_H

#include <ElaWidget.h>
#include <qcoreevent.h>
#include <QWidget>

#include "nlohmann/json.hpp"


enum class defaultNewWindowType;
class ElaMenu;
class QTranslator;
class ElaIconButton;
class ElaToolButton;
QT_BEGIN_NAMESPACE

namespace Ui {
    class mainWindow;
}

QT_END_NAMESPACE

class mainWindow : public ElaWidget {
    Q_OBJECT

public:
    explicit mainWindow(QWidget *parent = nullptr);

    ~mainWindow() override;

    void initSignalSlots();

    void initButton();

    void initSettings();

protected:
    void resizeEvent(QResizeEvent *event) override;

    void changeEvent(QEvent *event) override;

Q_SIGNALS:
    Q_SIGNAL void hideSecondaryWindow();

    Q_SIGNAL void showSecondaryWindow();

public Q_SLOTS:
    Q_SLOT void newWindow();
    Q_SLOT void newSerialWindow();
    Q_SLOT void newSerialSerVer();

private:
    Ui::mainWindow *ui;
    ElaIconButton *new_serial_button_;
    ElaToolButton *more_tools_button_;
    ElaMenu *more_menu_;
    QAction *serial_action_;
    QAction *settings_action_;
    QAction * serialServer_action_;
    QTranslator *translator_;
    defaultNewWindowType default_new_window_;
};


#endif //TABWINDOW_H
