//
// Created by wang on 25-4-6.
//

// You may need to build the project (run Qt uic code generator) to get "ui_cmdLineEdit.h" resolved

#include "cmdLineEdit.h"
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
        emit sigCmdSend(ui->lineEdit->text().toUtf8());
    });

    connect(ui->lineEdit, &ElaLineEdit::textEdited, this, [this]() {
        emit sigStatusChanged(this);
    });

    connect(ui->lineEdit, &ElaLineEdit::returnPressed, this, [this]() {
        emit sigCmdSend(ui->lineEdit->text().toUtf8());
    });
}

int cmdLineEdit::findIndex(const std::vector<cmdLineEdit *> &vec, const cmdLineEdit *target) {
    auto it = std::find(vec.begin(), vec.end(), target);
    if (it != vec.end()) {
        return std::distance(vec.begin(), it); // 返回迭代器距离（即下标）
    }
    return -1;
}

void cmdLineEdit::changeEvent(QEvent *event) {
    if (event) {
        switch (event->type()) {
            // this event is send if a translator is loaded
            case QEvent::LanguageChange:
                ui->retranslateUi(this);
                break;
            default:
                break;
        }
    }

    QWidget::changeEvent(event);
}
