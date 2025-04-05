//
// Created by wang on 25-4-6.
//

// You may need to build the project (run Qt uic code generator) to get "ui_cmdLineEdit.h" resolved

#include "cmdlineedit.h"
#include "ui_cmdLineEdit.h"
#include <ElaIconButton.h>

cmdLineEdit::cmdLineEdit(QWidget *parent) : QWidget(parent), ui(new Ui::cmdLineEdit) {
    ui->setupUi(this);

    iconBtn_send_ = new ElaIconButton{ElaIconType::CaretRight, 22, 35, 30};
    ui->horizontalLayout->addWidget(iconBtn_send_, 0);
    ui->horizontalLayout->setStretch(0, 1);
    ui->lineEdit->setObjectName("ElaLineEdit");

    initSignalSlots();
}

cmdLineEdit::~cmdLineEdit() {
    delete ui;
}

void cmdLineEdit::initSignalSlots() {
    connect(iconBtn_send_, &ElaIconButton::clicked, this, [this]() {
        emit sigCmdSend(ui->lineEdit->text());
    });

    connect(ui->lineEdit, &ElaLineEdit::textEdited, this, [this]() {
        emit sigStatusChanged(this);
    });

    connect(ui->lineEdit, &ElaLineEdit::returnPressed, this, [this]() {
        emit sigCmdSend(ui->lineEdit->text());
    });
}

int cmdLineEdit::findIndex(const std::vector<cmdLineEdit *> &vec, const cmdLineEdit *target) {
    auto it = std::find(vec.begin(), vec.end(), target);
    if (it != vec.end()) {
        return std::distance(vec.begin(), it); // 返回迭代器距离（即下标）
    }
    return -1;
}
