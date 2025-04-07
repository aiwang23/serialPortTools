//
// Created by wang on 25-4-2.
//

#ifndef INPUTDIALOG_H
#define INPUTDIALOG_H

#include <QWidget>
#include <ElaWidget.h>
#include <QDialog>

QT_BEGIN_NAMESPACE

namespace Ui {
    class inputDialog;
}

QT_END_NAMESPACE

// 文本输入弹窗
class inputDialog : public ElaWidget {
    Q_OBJECT

public:
    explicit inputDialog(const QString &titleName = {}, const QString &text = {}, QWidget *parent = nullptr);

    ~inputDialog() override;

    QString getInputText() const { return inputText_; }

    void exec();

protected:
    void showEvent(QShowEvent *event) override;

    void changeEvent(QEvent *event) override;

Q_SIGNALS:
    Q_SIGNAL void finished();

private:
    Ui::inputDialog *ui;
    QString inputText_;
};


#endif //INPUTDIALOG_H
