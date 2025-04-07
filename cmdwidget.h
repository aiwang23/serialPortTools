//
// Created by wang on 25-4-6.
//

#ifndef CMDWIDGET_H
#define CMDWIDGET_H

#include <QWidget>


class cmdLineEdit;
QT_BEGIN_NAMESPACE

namespace Ui {
    class cmdWidget;
}

QT_END_NAMESPACE

class cmdWidget : public QWidget {
    Q_OBJECT

public:
    explicit cmdWidget(QWidget *parent = nullptr);

    ~cmdWidget() override;

    void initSignalSlots();

protected:
    void changeEvent(QEvent *event) override;

Q_SIGNALS:
    Q_SIGNAL void sigCmdSend(QByteArray cmd);

private Q_SLOTS:
    Q_SLOT void addOrRemoveLine(cmdLineEdit *edit);

private:
    Ui::cmdWidget *ui;
    std::vector<cmdLineEdit*> editVec_;
};


#endif //CMDWIDGET_H
