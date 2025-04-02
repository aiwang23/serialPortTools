//
// Created by wang on 25-4-2.
//

// You may need to build the project (run Qt uic code generator) to get "ui_inputDialog.h" resolved

#include "inputdialog.h"
#include "ui_inputDialog.h"
#include <ElaMessageBar.h>

inputDialog::inputDialog(const QString &titleName, const QString &text, QWidget *parent) : ElaWidget(parent),
    ui(new Ui::inputDialog) {
    ui->setupUi(this);

    setWindowTitle(titleName);
    setWindowButtonFlags(ElaAppBarType::CloseButtonHint);
    ui->lineEdit->setText(text);

    // 设置窗口模态（阻止其他窗口操作）
    setWindowModality(Qt::ApplicationModal);
    connect(ui->lineEdit, &QLineEdit::returnPressed, this, [this]() {
        inputText_ = ui->lineEdit->text();
        if (inputText_.isEmpty()) {
            ElaMessageBar::warning(ElaMessageBarType::TopLeft, tr("warning"), tr("text is empty"), 4000, this);
            return;
        }
        close();
        emit finished();
    });
}

inputDialog::~inputDialog() {
    delete ui;
}

void inputDialog::exec() {
    show();
    QEventLoop loop;
    connect(this, &inputDialog::finished, &loop, &QEventLoop::quit);
    loop.exec();
}
