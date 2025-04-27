//
// Created by wang on 25-4-25.
//

#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QWidget>
#include <ElaWidget.h>

class ElaIconButton;
QT_BEGIN_NAMESPACE

namespace Ui {
    class loginDialog;
}

QT_END_NAMESPACE

class loginDialog : public ElaWidget {
    Q_OBJECT

public:
    enum class connectionType { none = -1, key, password };

    static QString stringFrom(connectionType en);

    static connectionType connectionTypeFrom(QString str);

    struct loginInfo {
        QString ip;
        QString port;
        QString user;
        connectionType connection_t;
        QString password;
        QString pubkey_path;
        QString prikey_path;
    };

public:
    explicit loginDialog(const loginInfo &info, QWidget *parent = nullptr);

    ~loginDialog() override;

    loginInfo getLoginInfo();

    void exec();

private:
    void initUI();
    void initValues();

protected:
    void keyReleaseEvent(QKeyEvent *event) override;

    void closeEvent(QCloseEvent *event) override;

Q_SIGNALS:
    Q_SIGNAL void finished();

private:
    Ui::loginDialog *ui;

    ElaIconButton *icon_btn_pubkey_ = nullptr;
    ElaIconButton *icon_btn_prikey_ = nullptr;
    loginInfo info_;
};


#endif //LOGINDIALOG_H
