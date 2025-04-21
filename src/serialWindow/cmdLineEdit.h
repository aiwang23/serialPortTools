//
// Created by wang on 25-4-6.
//

#ifndef CMDLINEEDIT_H
#define CMDLINEEDIT_H

#include <QWidget>
#include <ui_cmdLineEdit.h>


class ElaIconButton;
QT_BEGIN_NAMESPACE

namespace Ui {
    class cmdLineEdit;
}

QT_END_NAMESPACE

class cmdLineEdit : public QWidget {
    Q_OBJECT

public:
    explicit cmdLineEdit(QWidget *parent = nullptr);

    ~cmdLineEdit() override;

    void initSignalSlots();

    bool isEmpty() const { return ui->lineEdit->text().isEmpty(); }

    static int findIndex(const std::vector<cmdLineEdit *> &vec, const cmdLineEdit *target);

protected:
    void changeEvent(QEvent *event) override;

Q_SIGNALS:
    Q_SIGNAL void sigCmdSend(QByteArray cmd);

    Q_SIGNAL void sigStatusChanged(cmdLineEdit *edit);

private:
    Ui::cmdLineEdit *ui;
    ElaIconButton *iconBtn_send_;
};


#endif //CMDLINEEDIT_H
