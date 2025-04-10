//
// Created by wang on 25-4-2.
//

// You may need to build the project (run Qt uic code generator) to get "ui_inputDialog.h" resolved

#include "inputdialog.h"
#include "ui_inputDialog.h"
#include <ElaMessageBar.h>
#include <QFocusEvent>

inputDialog::inputDialog(const QString &titleName, const QString &text, QWidget *parent) : ElaWidget(parent),
    ui(new Ui::inputDialog) {
    ui->setupUi(this);

    setWindowTitle(titleName);
    setWindowButtonFlags(ElaAppBarType::CloseButtonHint);
    ui->lineEdit->setText(text);
    ui->lineEdit->installEventFilter(this);

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

void inputDialog::showEvent(QShowEvent *event) {
    ElaWidget::showEvent(event);

    ui->lineEdit->setFocus();
    // 手动发送一个 FocusIn event, 播放动画
    QFocusEvent focusEvent(QEvent::FocusIn, Qt::MouseFocusReason);
    QCoreApplication::sendEvent(ui->lineEdit, &focusEvent);
}

void inputDialog::changeEvent(QEvent *event) {
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

    ElaWidget::changeEvent(event);
}

bool inputDialog::eventFilter(QObject *watched, QEvent *event) {
    if (watched == ui->lineEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            // ESC键被按下
            inputText_.clear();
            close();
            emit finished();

            return true; // 表示事件已处理
        }
    }
    return ElaWidget::eventFilter(watched, event);
}

void inputDialog::keyReleaseEvent(QKeyEvent *event) {
    if (Qt::Key_Escape == event->key()) {
        inputText_.clear();
        close();
        emit finished();
    }
    ElaWidget::keyReleaseEvent(event);
}
