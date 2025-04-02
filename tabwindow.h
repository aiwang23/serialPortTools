//
// Created by wang on 25-4-2.
//

#ifndef TABWINDOW_H
#define TABWINDOW_H

#include <ElaWidget.h>
#include <QWidget>


class ElaIconButton;
QT_BEGIN_NAMESPACE
namespace Ui { class tabWindow; }
QT_END_NAMESPACE

class tabWindow : public ElaWidget {
Q_OBJECT

public:
    explicit tabWindow(QWidget *parent = nullptr);
    ~tabWindow() override;

    void initSignalSlots();

public Q_SLOTS:
    Q_SLOT void newSerialWindow();

private:
    Ui::tabWindow *ui;
    ElaIconButton *new_icon_button_;
    ElaIconButton *more_icon_button_;
};


#endif //TABWINDOW_H
