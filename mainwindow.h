//
// Created by wang on 25-4-2.
//

#ifndef TABWINDOW_H
#define TABWINDOW_H

#include <ElaWidget.h>
#include <QWidget>


class ElaIconButton;
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

protected:
    void resizeEvent(QResizeEvent *event) override;

Q_SIGNALS:
    Q_SIGNAL void hideSecondaryWindow();
    Q_SIGNAL void showSecondaryWindow();

public Q_SLOTS:
    Q_SLOT void newSerialWindow();

private:
    Ui::mainWindow *ui;
    ElaIconButton *new_icon_button_;
    ElaIconButton *more_icon_button_;
};


#endif //TABWINDOW_H
