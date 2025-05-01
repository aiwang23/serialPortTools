//
// Created by wang on 25-4-25.
//

#ifndef SFTPWINDOW_H
#define SFTPWINDOW_H


#include "ElaIconButton.h"
#include "loginDialog.h"
#include "sftpClientThread.h"


class ElaMenu;
class QTableView;
class sftpClient;
QT_BEGIN_NAMESPACE

namespace Ui {
    class sftpWindow;
}

QT_END_NAMESPACE

// TODO: 后面完善完功能之后 再加到class mainWindow里面
class sftpWindow : public QWidget {
    Q_OBJECT

public:
    explicit sftpWindow(QWidget *parent = nullptr);

    ~sftpWindow() override;

    void showLoginWindow();

private:
    void initUI();

    void initSignalSlots();

    void initMenu();

    static void appendLine(const QTableView *table, const sftpClient::fileInfo &i);

    static void removeLine(const QTableView *table, int index);

    static void clearWith(QTableView *table);

private Q_SLOTS:
    Q_SLOT void doOpen();

    Q_SLOT void doGet();

    Q_SLOT void doPut();

    Q_SLOT void doCopy();

    Q_SLOT void doMove();

    Q_SLOT void doRename();

    Q_SLOT void doDelete();

private:
    Ui::sftpWindow *ui;
    ElaIconButton *icon_btn_back_ = nullptr;
    ElaIconButton *icon_btn_refresh_ = nullptr;
    ElaIconButton *icon_btn_trans_ = nullptr;
    QString oldPath_;
    loginDialog::loginInfo userInfo_;
    sftpClientThread sftpThread_;
    sftpClientThread sftpGetThread_;
    sftpClientThread sftpPutThread_;

    ElaMenu *menu_ = nullptr;
    QAction *getAction_ = nullptr;
    QAction *putAction_ = nullptr;
    QAction *openAction_ = nullptr;
    QAction *copyAction_ = nullptr;
    QAction *moveAction_ = nullptr;
    QAction *renameAction_ = nullptr;
    QAction *deleteAction_ = nullptr;
};


#endif //SFTPWINDOW_H
