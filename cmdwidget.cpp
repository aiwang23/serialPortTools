//
// Created by wang on 25-4-6.
//

// You may need to build the project (run Qt uic code generator) to get "ui_cmdWidget.h" resolved

#include "cmdwidget.h"

#include <QFocusEvent>

#include "ui_cmdWidget.h"
#include "cmdlineedit.h"

cmdWidget::cmdWidget(QWidget *parent) : QWidget(parent), ui(new Ui::cmdWidget) {
    ui->setupUi(this);

    editVec_.reserve(10);
    editVec_.push_back(ui->edit_first);
    // 无边框
    ui->scrollArea->setObjectName("ElaScrollArea");
    initSignalSlots();
}

cmdWidget::~cmdWidget() {
    delete ui;
}

void cmdWidget::initSignalSlots() {
    connect(ui->edit_first, &cmdLineEdit::sigCmdSend, this, &cmdWidget::sigCmdSend);
    connect(ui->edit_first, &cmdLineEdit::sigStatusChanged, this, &cmdWidget::addOrRemoveLine);
}

void cmdWidget::addOrRemoveLine(cmdLineEdit *edit) {
    int idx = cmdLineEdit::findIndex(editVec_, edit);
    int size = editVec_.size();
    if (edit->isEmpty()) {
        // 想要删除该行
        // 当前输入框没有文本
        if (size > 1) {
            if (idx < editVec_.size()) {
                // 检查下标是否有效
                editVec_.erase(editVec_.begin() + idx); // 删除指定位置的元素
            }
            edit->hide();
            edit->setParent(nullptr); // 解除父子关系
            edit->deleteLater(); // 让Qt在适当时候删除

            // 删除成功后。 把光标移到上一行
            int nextIdx;
            if (idx > 0) {
                nextIdx = idx - 1;
            } else {
                nextIdx = idx;
            }
            QFocusEvent focusEvent(QEvent::FocusIn, Qt::MouseFocusReason);
            QCoreApplication::sendEvent(editVec_[nextIdx], &focusEvent);
        }
    } else {
        // 想要在最后插入一行
        // 当前输入框有文本
        // 确保当前输入框是最后一行
        if (editVec_.size() - 1 == idx) {
            auto layout = ui->scrollAreaWidgetContents->layout();
            auto newEdit = new cmdLineEdit;
            layout->addWidget(newEdit);
            editVec_.push_back(newEdit);

            connect(newEdit, &cmdLineEdit::sigCmdSend, this, &cmdWidget::sigCmdSend);
            connect(newEdit, &cmdLineEdit::sigStatusChanged, this, &cmdWidget::addOrRemoveLine);
        }
    }
}
