//
// Created by wang on 25-4-25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_loginDialog.h" resolved

#include "loginDialog.h"
#include "loginDialog.h"
#include "ui_loginDialog.h"
#include <ElaIconButton.h>
#include <QDir>
#include <QFileDialog>
#include <QKeyEvent>
#include <magic_enum/magic_enum.hpp>

QString loginDialog::stringFrom(const connectionType en) {
    return QString::fromStdString(std::string{magic_enum::enum_name(en)});
}

loginDialog::connectionType loginDialog::connectionTypeFrom(QString str) {
    return magic_enum::enum_cast<connectionType>(str.toStdString()).value();
}

loginDialog::loginDialog(const loginInfo &info, QWidget *parent)
    : ElaWidget(parent), ui(new Ui::loginDialog), info_(info) {
    ui->setupUi(this);

    initUI();
    initValues();

    setWindowTitle(tr("login sftp"));
    setWindowButtonFlags(ElaAppBarType::CloseButtonHint);

    // 设置窗口模态（阻止其他窗口操作）
    setWindowModality(Qt::ApplicationModal);
    connect(ui->pushButton_start, &ElaPushButton::clicked, this, [this]() {
        QString ip = ui->lineEdit_ip->text();
        QString port = ui->lineEdit_port->text();
        QString user = ui->lineEdit_user->text();
        connectionType type = connectionTypeFrom(ui->comboBox_connection->currentText());
        QString password = ui->lineEdit_password->text();
        QString pubkey = ui->lineEdit_pubkey->text();
        QString prikey = ui->lineEdit_prikey->text();

        info_ = loginInfo{
            ip,
            port,
            user,
            type,
            password,
            pubkey,
            prikey
        };
        close();
    });

    connect(icon_btn_pubkey_, &ElaIconButton::clicked, this, [this]() {
        QString defaultPath = QDir::homePath();
        defaultPath = std::filesystem::exists((defaultPath + "/.ssh/").toStdString()) ? defaultPath + "/.ssh/" : defaultPath;
        QString filePath = QFileDialog::getOpenFileName(
            this, // 父窗口
            tr("public key"), // 对话框标题
            defaultPath, // 默认打开目录（用户主目录）
            "all (*)" // 文件过滤器
        );

        if (!filePath.isEmpty()) {
            ui->lineEdit_pubkey->setText(filePath);
        }
    });
    connect(icon_btn_prikey_, &ElaIconButton::clicked, this, [this]() {
        QString defaultPath = QDir::homePath();
        defaultPath = std::filesystem::exists((defaultPath + "/.ssh/").toStdString()) ? defaultPath + "/.ssh/" : defaultPath;
        QString filePath = QFileDialog::getOpenFileName(
            this, // 父窗口
            tr("private key"), // 对话框标题
            defaultPath, // 默认打开目录（用户主目录）
            "all (*)" // 文件过滤器
        );

        if (!filePath.isEmpty()) {
            ui->lineEdit_prikey->setText(filePath);
        }
    });


}

loginDialog::~loginDialog() {
    delete ui;
}

loginDialog::loginInfo loginDialog::getLoginInfo() {
    return info_;
}

void loginDialog::exec() {
    show();
    QEventLoop loop;
    connect(this, &loginDialog::finished, &loop, &QEventLoop::quit);
    loop.exec();
}

void loginDialog::initUI() {
    ui->label_ip->setTextPixelSize(17);
    ui->label_port->setTextPixelSize(17);
    ui->label_user->setTextPixelSize(17);
    ui->label_connection->setTextPixelSize(17);
    ui->label_password->setTextPixelSize(17);
    ui->label_pubkey->setTextPixelSize(17);
    ui->label_prikey->setTextPixelSize(17);

    ui->label_ip->setObjectName("ElaText");
    ui->label_port->setObjectName("ElaText");
    ui->label_user->setObjectName("ElaText");
    ui->label_connection->setObjectName("ElaText");
    ui->label_password->setObjectName("ElaText");
    ui->label_pubkey->setObjectName("ElaText");
    ui->label_prikey->setObjectName("ElaText");

    ui->lineEdit_ip->setObjectName("ElaLineEdit");
    ui->lineEdit_port->setObjectName("ElaLineEdit");
    ui->lineEdit_user->setObjectName("ElaLineEdit");
    ui->comboBox_connection->setObjectName("ElaComboBox");
    ui->lineEdit_password->setObjectName("ElaLineEdit");
    ui->lineEdit_pubkey->setObjectName("ElaLineEdit");
    ui->lineEdit_prikey->setObjectName("ElaLineEdit");

    icon_btn_pubkey_ = new ElaIconButton{ElaIconType::Folder, 17, 20, 20};
    icon_btn_prikey_ = new ElaIconButton{ElaIconType::Folder, 17, 20, 20};
    ui->horizontalLayout_pubkey->addWidget(icon_btn_pubkey_);
    ui->horizontalLayout_prikey->addWidget(icon_btn_prikey_);

    ui->comboBox_connection->addItems({"key", "password"});
}

void loginDialog::initValues() {
    ui->lineEdit_ip->setText(info_.ip);
    ui->lineEdit_port->setText(info_.port);
    ui->lineEdit_user->setText(info_.user);
    ui->comboBox_connection->setCurrentText(stringFrom(info_.connection_t));
    ui->lineEdit_password->setText(info_.password);
    ui->lineEdit_pubkey->setText(info_.pubkey_path);
    ui->lineEdit_prikey->setText(info_.prikey_path);
}

void loginDialog::keyReleaseEvent(QKeyEvent *event) {
    if (Qt::Key_Escape == event->key()) {
        info_ = {};
        close();
    }
    ElaWidget::keyReleaseEvent(event);
}

void loginDialog::closeEvent(QCloseEvent *event) {
    emit finished();
    ElaWidget::closeEvent(event);
}
