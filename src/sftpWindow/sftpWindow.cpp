//
// Created by wang on 25-4-25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_sftpWindow.h" resolved

#include "sftpWindow.h"

#include <iso646.h>
#include <QFileDialog>
#include <QMouseEvent>
#include <QStandardItemModel>

#include "ElaIcon.h"
#include "ElaIconButton.h"
#include "ElaMenu.h"
#include "ElaMessageBar.h"
#include "inputDialog.h"
#include "loginDialog.h"
#include "sftpClient.h"
#include "ui_sftpWindow.h"


sftpWindow::sftpWindow(QWidget *parent)
    : QWidget(parent), ui(new Ui::sftpWindow) {
    ui->setupUi(this);
    initUI();
    initSignalSlots();
    initMenu();

    sftpThread_.start();
}

sftpWindow::~sftpWindow() {
    sftpThread_.stop();
    sftpGetThread_.stop();
    sftpPutThread_.stop();
    delete ui;
}

void sftpWindow::showLoginWindow() {
    loginDialog dialog(loginDialog::loginInfo{
        "127.0.0.1",
        "22",
        "wang",
        loginDialog::connectionType::password,
        "123456"
    });
    dialog.exec();
    userInfo_ = dialog.getLoginInfo();

    // 连接
    auto args6 = std::make_shared<funcArgs6>();
    args6->a1 = userInfo_.ip;
    args6->a2 = userInfo_.port;
    args6->a3 = userInfo_.user;
    args6->a4 = userInfo_.password;
    args6->a5 = userInfo_.pubkey_path;
    args6->a6 = userInfo_.prikey_path;
    auto rs = sftpThread_.try_do(funcType::connectToHost, args6);
    connect(rs.get(), &sftpClientResponse::connectToHostResponse, this, [this](bool ok) {
        if (not ok) {
            ElaMessageBar::error(ElaMessageBarType::TopLeft, tr("error"), tr("connectToHost OK"), 3000, this);
            return;
        }

        // 获取 home path
        auto args = std::make_shared<funcArgs>();
        auto home_rs = sftpThread_.try_do(funcType::home, args);
        connect(home_rs.get(), &sftpClientResponse::homeResponse, this, [this](QString home) {
            if (home.isEmpty()) {
                return;
            }
            ui->lineEdit_url->setText(home);
            oldPath_ = home;

            // 获取 ls 更新列表
            auto ls_args = std::make_shared<funcArgs2>();
            ls_args->a1 = home;
            ls_args->a2 = "l";
            auto ls_rs = sftpThread_.try_do(funcType::ls, ls_args);
            connect(ls_rs.get(), &sftpClientResponse::lsResponse, this, [this](QList<sftpClient::fileInfo> list) {
                clearWith(ui->tableWidget_list);
                for (auto i: list) {
                    appendLine(ui->tableWidget_list, i);
                }
            }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
        }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
    }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
}

void sftpWindow::initUI() {
    icon_btn_back_ = new ElaIconButton{ElaIconType::ArrowUp, 17, 20, 20};
    icon_btn_refresh_ = new ElaIconButton{ElaIconType::ArrowRotateRight, 17, 20, 20};
    icon_btn_trans_ = new ElaIconButton{ElaIconType::ArrowDownArrowUp, 17, 20, 20};
    ui->horizontalLayout_top_left_btn->addWidget(icon_btn_back_);
    ui->horizontalLayout_top_left_btn->addWidget(icon_btn_refresh_);
    ui->horizontalLayout_top_right_btn->addWidget(icon_btn_trans_);

    ui->lineEdit_url->setObjectName("ElaLineEdit");
    ui->tableWidget_list->setObjectName("ElaTableView");

    QStringList headers = {
        tr("filename"),
        tr("permissions"),
        tr("user"),
        tr("userGroups"),
        tr("fileSize"),
        tr("dateModified")
    };
    // 创建模型对象
    QStandardItemModel *model = new QStandardItemModel(0, headers.size()); // 初始0行2列
    model->setHorizontalHeaderLabels(headers);
    // 关联模型与视图
    ui->tableWidget_list->setModel(model);
    ui->tableWidget_list->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_list->horizontalHeader()->setDefaultAlignment(Qt::AlignmentFlag::AlignLeft);
    ui->tableWidget_list->setColumnWidth(0, 200);
    ui->tableWidget_list->setColumnWidth(5, 200);
}

void sftpWindow::initSignalSlots() {
    // 回车 更新路径
    connect(ui->lineEdit_url, &ElaLineEdit::returnPressed, this, [this]() {
        QString path = ui->lineEdit_url->text();
        std::shared_ptr<funcArgs1> args1 = std::make_shared<funcArgs1>();
        args1->a1 = path;
        auto rs = sftpThread_.try_do(funcType::exist, args1);

        // 接收exist() 的结果
        connect(rs.get(), &sftpClientResponse::existResponse, this, [this](sftpClient::fileType file_t) {
            if (sftpClient::fileType::dir != file_t) {
                ui->lineEdit_url->setText(oldPath_);
                return;
            }
            // 尝试获取 目录列表
            auto args2 = std::make_shared<funcArgs2>();
            args2->a1 = ui->lineEdit_url->text();
            args2->a2 = "l";
            auto ls_rs = sftpThread_.try_do(funcType::ls, args2);
            connect(ls_rs.get(), &sftpClientResponse::lsResponse, this, [this](QList<sftpClient::fileInfo> list) {
                clearWith(ui->tableWidget_list);
                for (auto i: list) {
                    appendLine(ui->tableWidget_list, i);
                }
            }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
        }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
    });

    // 刷新
    connect(icon_btn_refresh_, &ElaIconButton::clicked, this, [this]() {
        // 尝试获取 目录列表
        auto args2 = std::make_shared<funcArgs2>();
        args2->a1 = ui->lineEdit_url->text();
        args2->a2 = "l";
        auto ls_rs = sftpThread_.try_do(funcType::ls, args2);
        connect(ls_rs.get(), &sftpClientResponse::lsResponse, this, [this](QList<sftpClient::fileInfo> list) {
            clearWith(ui->tableWidget_list);
            for (auto i: list) {
                appendLine(ui->tableWidget_list, i);
            }
        }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
    });

    // 双击跳转
    connect(ui->tableWidget_list, &ElaTableView::doubleClicked, this, [this](const QModelIndex &index) {
        int rows = index.row();
        auto model = static_cast<QStandardItemModel *>(ui->tableWidget_list->model());
        QString fileName = model->item(rows, 0)->text();
        QString thisPath = ui->lineEdit_url->text();

        // 检查上级目录是否存在
        std::shared_ptr<funcArgs1> args1 = std::make_shared<funcArgs1>();
        args1->a1 = thisPath + "/" + fileName;
        ui->lineEdit_url->setText(args1->a1);
        auto exist_rs = sftpThread_.try_do(funcType::exist, args1);
        // 接收exist() 的结果
        connect(exist_rs.get(), &sftpClientResponse::existResponse, this, [this](sftpClient::fileType file_t) {
            if (sftpClient::fileType::dir != file_t) {
                ui->lineEdit_url->setText(oldPath_);
                return;
            }

            oldPath_ = ui->lineEdit_url->text();
            // 尝试获取 目录列表
            auto args2 = std::make_shared<funcArgs2>();
            args2->a1 = oldPath_;
            args2->a2 = "l";
            auto ls_rs = sftpThread_.try_do(funcType::ls, args2);
            connect(ls_rs.get(), &sftpClientResponse::lsResponse, this, [this](QList<sftpClient::fileInfo> list) {
                clearWith(ui->tableWidget_list);
                for (auto i: list) {
                    appendLine(ui->tableWidget_list, i);
                }
            }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
        }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
    });

    // 回到上级目录
    connect(icon_btn_back_, &ElaIconButton::clicked, this, [this]() {
        std::filesystem::path path{ui->lineEdit_url->text().toStdString()};
        QString par_path = QString::fromStdString(path.parent_path().string());
        ui->lineEdit_url->setText(par_path);

        // 检查上级目录是否存在
        std::shared_ptr<funcArgs1> args1 = std::make_shared<funcArgs1>();
        args1->a1 = par_path;
        auto rs = sftpThread_.try_do(funcType::exist, args1);

        // 接收exist() 的结果
        connect(rs.get(), &sftpClientResponse::existResponse, this, [this](sftpClient::fileType file_t) {
            if (sftpClient::fileType::dir != file_t) {
                ui->lineEdit_url->setText(oldPath_);
                return;
            }
            // 尝试获取 目录列表
            auto args2 = std::make_shared<funcArgs2>();
            args2->a1 = ui->lineEdit_url->text();
            args2->a2 = "l";
            auto ls_rs = sftpThread_.try_do(funcType::ls, args2);
            connect(ls_rs.get(), &sftpClientResponse::lsResponse, this, [this](QList<sftpClient::fileInfo> list) {
                clearWith(ui->tableWidget_list);
                for (auto i: list) {
                    appendLine(ui->tableWidget_list, i);
                }
            }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
        }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
    });

    // 下载 上传进度
    connect(icon_btn_trans_, &ElaIconButton::clicked, this, [this]() {
    });
}

void sftpWindow::initMenu() {
    ui->tableWidget_list->setContextMenuPolicy(Qt::CustomContextMenu);
    menu_ = new ElaMenu{ui->tableWidget_list};
    menu_->setFixedWidth(120);
    openAction_ = menu_->addElaIconAction(ElaIconType::FolderOpen, tr("open"));
    menu_->addSeparator();
    getAction_ = menu_->addElaIconAction(ElaIconType::ArrowDownToBracket, tr("get"));
    putAction_ = menu_->addElaIconAction(ElaIconType::ArrowUpFromBracket, tr("put"));
    menu_->addSeparator();
    copyAction_ = menu_->addElaIconAction(ElaIconType::Copy, tr("copy"));
    moveAction_ = menu_->addElaIconAction(ElaIconType::Copy, tr("move"));
    renameAction_ = menu_->addElaIconAction(ElaIconType::PenLine, tr("rename"));
    deleteAction_ = menu_->addElaIconAction(ElaIconType::TrashCan, tr("delete"));

    // 打开文件或 打开文件夹
    connect(openAction_, &QAction::triggered, this, &sftpWindow::doOpen);
    // 下载
    connect(getAction_, &QAction::triggered, this, &sftpWindow::doGet);
    // 上传
    connect(putAction_, &QAction::triggered, this, &sftpWindow::doPut);
    // 复制
    connect(copyAction_, &QAction::triggered, this, &sftpWindow::doCopy);
    // 移动
    connect(moveAction_, &QAction::triggered, this, &sftpWindow::doMove);
    // 重命名
    connect(renameAction_, &QAction::triggered, this, &sftpWindow::doRename);
    // 删除
    connect(deleteAction_, &QAction::triggered, this, &sftpWindow::doDelete);

    // 右键弹出菜单
    connect(ui->tableWidget_list, &ElaTableView::customContextMenuRequested, this, [this](QPoint point) {
        auto index = ui->tableWidget_list->indexAt(point);
        if (not index.isValid()) {
            return;
        }
        menu_->exec(QCursor::pos());
    });
}

void sftpWindow::appendLine(const QTableView *table, const sftpClient::fileInfo &i) {
    if (!table) return;

    QStandardItemModel *model = static_cast<QStandardItemModel *>(table->model());
    if (!model) return;

    auto new_func = [&](const QString &str)-> QStandardItem * {
        QStandardItem *t = new QStandardItem(str);
        t->setFlags(t->flags() & ~Qt::ItemIsEditable);
        return t;
    };

    auto icon_func = [&](QStandardItem *item)-> QStandardItem * {
        const QChar type = i.permissions.at(0);
        QIcon icon;
        if (type.isNull())
            return item;
        else if ('l' == type)
            icon = QIcon{":/res/link.png"};
        else if ('d' == type)
            icon = QIcon{":/res/dir.png"};
        else
            icon = QIcon{":/res/file.png"};
        item->setIcon(icon);
        return item;
    };

    auto size_func = [&](QStandardItem *item) -> QStandardItem * {
        const QChar type = i.permissions.at(0);
        if (type.isNull() or
            'l' == type or
            '-' == type)
            return item;
        else if ('d' == type)
            item->setText("");
        return item;
    };

    // 单次插入整行
    model->appendRow(QList{
        icon_func(new_func(i.filename)),
        new_func(i.permissions),
        new_func(i.user),
        new_func(i.userGroups),
        size_func(new_func(i.fileSize)),
        new_func(i.dateModified),
    });
}

void sftpWindow::removeLine(const QTableView *table, int index) {
    if (not table) {
        return;
    }

    QStandardItemModel *model = static_cast<QStandardItemModel *>(table->model());
    int rows = model->rowCount();
    if (index < rows or rows <= index)
        return;
    model->removeRow(index);
}

void sftpWindow::clearWith(QTableView *table) {
    if (!table) return;

    // 使用 static_cast 替代 dynamic_cast（假设模型类型确定）
    if (auto *model = static_cast<QStandardItemModel *>(table->model())) {
        // 阻止视图刷新信号
        model->blockSignals(true);

        // 保留表头结构仅清数据（比 clear() 快 30%+）
        model->removeRows(0, model->rowCount());

        // 恢复信号并触发单次更新
        model->blockSignals(false);
        model->headerDataChanged(Qt::Vertical, 0, model->rowCount() - 1);
    }
}

void sftpWindow::doOpen() {
    auto model = static_cast<QStandardItemModel *>(ui->tableWidget_list->model());
    int row = ui->tableWidget_list->currentIndex().row();
    QString remoteName = model->item(row, 0)->text();
    QString thisPath = ui->lineEdit_url->text();

    QChar fileType = model->item(row, 1)->text().at(0);
    if ('d' == fileType) {
        // 检查上级目录是否存在
        std::shared_ptr<funcArgs1> args1 = std::make_shared<funcArgs1>();
        args1->a1 = thisPath + "/" + remoteName;
        ui->lineEdit_url->setText(args1->a1);
        auto exist_rs = sftpThread_.try_do(funcType::exist, args1);
        // 接收exist() 的结果
        connect(exist_rs.get(), &sftpClientResponse::existResponse, this, [this](sftpClient::fileType file_t) {
            if (sftpClient::fileType::dir != file_t) {
                ui->lineEdit_url->setText(oldPath_);
                return;
            }

            oldPath_ = ui->lineEdit_url->text();
            // 尝试获取 目录列表
            auto args2 = std::make_shared<funcArgs2>();
            args2->a1 = oldPath_;
            args2->a2 = "l";
            auto ls_rs = sftpThread_.try_do(funcType::ls, args2);
            connect(ls_rs.get(), &sftpClientResponse::lsResponse, this, [this](QList<sftpClient::fileInfo> list) {
                clearWith(ui->tableWidget_list);
                for (auto i: list) {
                    appendLine(ui->tableWidget_list, i);
                }
            }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
        }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
    } else if ('-' == fileType) {
    } else if ('l' == fileType) {
    }
}

void sftpWindow::doGet() {
    static QString locName = QFileDialog::getSaveFileName(this, tr("save file"), QDir::homePath(), "all (*)");
    if (locName.isEmpty()) {
        return;
    }

    auto model = static_cast<QStandardItemModel *>(ui->tableWidget_list->model());
    int row = ui->tableWidget_list->currentIndex().row();
    QString remoteName = model->item(row, 0)->text();
    QString this_path = ui->lineEdit_url->text();

    // 查看remote 文件是否存在
    auto args1 = std::make_shared<funcArgs1>();
    args1->a1 = this_path + "/" + remoteName;;
    auto exist_rs = sftpThread_.try_do(funcType::exist, args1);
    connect(exist_rs.get(), &sftpClientResponse::existResponse, this, [this](sftpClient::fileType file_t) {
        if (sftpClient::fileType::file != file_t) {
            ElaMessageBar::warning(ElaMessageBarType::TopLeft, tr("warning"), tr("remote file not exist"), 3000,
                                   this);
            return;
        }
        sftpGetThread_.start();
        // 连接
        auto args6 = std::make_shared<funcArgs6>();
        args6->a1 = userInfo_.ip;
        args6->a2 = userInfo_.port;
        args6->a3 = userInfo_.user;
        args6->a4 = userInfo_.password;
        args6->a5 = userInfo_.pubkey_path;
        args6->a6 = userInfo_.prikey_path;
        auto connect_rs = sftpGetThread_.try_do(funcType::connectToHost, args6);
        connect(connect_rs.get(), &sftpClientResponse::connectToHostResponse, this, [this](bool ok) {
            auto model = static_cast<QStandardItemModel *>(ui->tableWidget_list->model());
            int row = ui->tableWidget_list->currentIndex().row();
            QString remoteName = model->item(row, 0)->text();
            QString this_path = ui->lineEdit_url->text();

            // 下载
            std::shared_ptr<funcArgs2> args2 = std::make_shared<funcArgs2>();
            args2->a1 = this_path + "/" + remoteName;
            args2->a2 = locName;
            auto get_rs = sftpGetThread_.try_do(funcType::get, args2);
            connect(get_rs.get(), &sftpClientResponse::getTransferProgress, this,
                    [this](qint64 bytesSent, qint64 bytesTotal) {
                        ElaMessageBar::information(ElaMessageBarType::TopLeft,
                                                   tr("info"),
                                                   QString("%1 / %2")
                                                   .arg(bytesSent, bytesTotal),
                                                   3000, this);
                    }, Qt::QueuedConnection);
            // 下载结果
            connect(get_rs.get(), &sftpClientResponse::getResponse, this, [this](bool ok) {
                if (ok) {
                    ElaMessageBar::information(ElaMessageBarType::TopLeft, tr("info"), tr("get OK"), 3000, this);
                } else {
                    ElaMessageBar::error(ElaMessageBarType::TopLeft, tr("error"), tr("get failed"), 3000, this);
                }
            }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
        }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
    }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
}

void sftpWindow::doPut() {
    // 要上传的文件
    static QString locName = QFileDialog::getOpenFileName(this, tr("save file"), QDir::homePath(), "all (*)");
    if (locName.isEmpty()) {
        return;
    }

    // 上传到哪里
    inputDialog input_dialog{tr("put name"), QFileInfo(locName).fileName()};
    input_dialog.exec();
    static QString remoteName = input_dialog.getInputText();
    if (remoteName.isEmpty()) {
        return;
    }

    static QString this_path = ui->lineEdit_url->text();

    // 查看remote 文件是
    auto args1 = std::make_shared<funcArgs1>();
    args1->a1 = this_path + "/" + remoteName;;
    auto exist_rs = sftpThread_.try_do(funcType::exist, args1);
    connect(exist_rs.get(), &sftpClientResponse::existResponse, this, [this](sftpClient::fileType file_t) {
        if (sftpClient::fileType::none != file_t) {
            ElaMessageBar::warning(ElaMessageBarType::TopLeft, tr("warning"), tr("remote file exist"), 3000,
                                   this);
            return;
        }

        sftpPutThread_.start();
        // 连接
        auto args6 = std::make_shared<funcArgs6>();
        args6->a1 = userInfo_.ip;
        args6->a2 = userInfo_.port;
        args6->a3 = userInfo_.user;
        args6->a4 = userInfo_.password;
        args6->a5 = userInfo_.pubkey_path;
        args6->a6 = userInfo_.prikey_path;
        auto connect_rs = sftpPutThread_.try_do(funcType::connectToHost, args6);
        connect(connect_rs.get(), &sftpClientResponse::connectToHostResponse, this, [this](bool ok) {
            // 下载
            std::shared_ptr<funcArgs2> args2 = std::make_shared<funcArgs2>();
            args2->a1 = locName;
            args2->a2 = this_path + "/" + remoteName;
            auto put_rs = sftpPutThread_.try_do(funcType::put, args2);
            connect(put_rs.get(), &sftpClientResponse::putTransferProgress, this,
                    [this](qint64 bytesSent, qint64 bytesTotal) {
                        ElaMessageBar::information(ElaMessageBarType::TopLeft,
                                                   tr("info"),
                                                   QString("%1 / %2")
                                                   .arg(bytesSent, bytesTotal),
                                                   3000, this);
                    }, Qt::QueuedConnection);
            // 下载结果
            connect(put_rs.get(), &sftpClientResponse::putResponse, this, [this](bool ok) {
                if (not ok) {
                    ElaMessageBar::error(ElaMessageBarType::TopLeft, tr("error"), tr("put  failed"), 3000, this);
                }
                ElaMessageBar::information(ElaMessageBarType::TopLeft, tr("info"), tr("put OK"), 3000, this);
                // 更新目录列表
                auto args2 = std::make_shared<funcArgs2>();
                args2->a1 = ui->lineEdit_url->text();
                args2->a2 = "l";
                auto ls_rs = sftpThread_.try_do(funcType::ls, args2);
                connect(ls_rs.get(), &sftpClientResponse::lsResponse, this, [this](QList<sftpClient::fileInfo> list) {
                    clearWith(ui->tableWidget_list);
                    for (const auto &i: list) {
                        appendLine(ui->tableWidget_list, i);
                    }
                }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
            }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
        }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
    }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
}

void sftpWindow::doCopy() {
    auto model = static_cast<QStandardItemModel *>(ui->tableWidget_list->model());
    int row = ui->tableWidget_list->currentIndex().row();
    QString srcName = model->item(row, 0)->text();
    QString this_path = ui->lineEdit_url->text();
    static QString srcPath = this_path + "/" + srcName;
    static QString dstPath = this_path + "/" + srcName;

    // 查看 src remote 文件是否存在
    auto args1 = std::make_shared<funcArgs1>();
    args1->a1 = srcPath;
    auto exist_rs = sftpThread_.try_do(funcType::exist, args1);
    connect(exist_rs.get(), &sftpClientResponse::existResponse, this, [this](sftpClient::fileType file_t) {
        // 不是文件
        if (sftpClient::fileType::file != file_t) {
            ElaMessageBar::warning(ElaMessageBarType::TopLeft, tr("warning"), tr("remote file not exist"), 3000,
                                   this);
            return;
        }

        sftpGetThread_.start();
        // 连接
        auto args6 = std::make_shared<funcArgs6>();
        args6->a1 = userInfo_.ip;
        args6->a2 = userInfo_.port;
        args6->a3 = userInfo_.user;
        args6->a4 = userInfo_.password;
        args6->a5 = userInfo_.pubkey_path;
        args6->a6 = userInfo_.prikey_path;
        auto connect_rs = sftpGetThread_.try_do(funcType::connectToHost, args6);
        connect(connect_rs.get(), &sftpClientResponse::connectToHostResponse, this, [this](bool ok) {
            auto model = static_cast<QStandardItemModel *>(ui->tableWidget_list->model());
            int row = ui->tableWidget_list->currentIndex().row();
            QString remoteName = model->item(row, 0)->text();
            QString this_path = ui->lineEdit_url->text();

            // 复制
            std::shared_ptr<funcArgs2> args2 = std::make_shared<funcArgs2>();
            args2->a1 = srcPath;
            args2->a2 = dstPath;
            auto get_rs = sftpGetThread_.try_do(funcType::get, args2);
            connect(get_rs.get(), &sftpClientResponse::getTransferProgress, this,
                    [this](qint64 bytesSent, qint64 bytesTotal) {
                        ElaMessageBar::information(ElaMessageBarType::TopLeft,
                                                   tr("info"),
                                                   QString("%1 / %2")
                                                   .arg(bytesSent, bytesTotal),
                                                   3000, this);
                    }, Qt::QueuedConnection);
            // 下载结果
            connect(get_rs.get(), &sftpClientResponse::getResponse, this, [this](bool ok) {
                if (ok) {
                    ElaMessageBar::information(ElaMessageBarType::TopLeft, tr("info"), tr("get OK"), 3000, this);
                } else {
                    ElaMessageBar::error(ElaMessageBarType::TopLeft, tr("error"), tr("get failed"), 3000, this);
                }
                sftpGetThread_.stop();
            }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
        }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
    }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
}

void sftpWindow::doMove() {
}

void sftpWindow::doRename() {
}

void sftpWindow::doDelete() {
}
