//
// Created by wang on 25-4-2.
//

#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QWidget>
#include <ElaScrollPage.h>

QT_BEGIN_NAMESPACE
namespace Ui { class settingsWindow; }
QT_END_NAMESPACE

class settingsWindow : public QWidget {
Q_OBJECT

public:
    explicit settingsWindow(QWidget *parent = nullptr);
    ~settingsWindow() override;

private:
    Ui::settingsWindow *ui;
};


#endif //SETTINGSWINDOW_H
