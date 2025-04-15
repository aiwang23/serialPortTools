//
// Created by wang on 25-4-2.
//

// You may need to build the project (run Qt uic code generator) to get "ui_settingsWindow.h" resolved

#include "settingswindow.h"
#include "ui_settingsWindow.h"


settingsWindow::settingsWindow(QWidget *parent) : QWidget(parent), ui(new Ui::settingsWindow) {
    ui->setupUi(this);

    setObjectName("settingsWindow");
    setWindowFlags(Qt::FramelessWindowHint);
    setWindowTitle("");

    QString html_about = R"(<h1>about</h1>
    <p> https://github.com/aiwang23/serialPortTools </p>
    <p> version: 1.0.0</p>
    <p>Open Source License: MIT</p>
    <p> Third party libraries:</p>
        <h4> 1.https://github.com/Liniyous/ElaWidgetTools</h4>
        <h4> 2.https://github.com/itas109/CSerialPort</h4>
        <h4> 3.https://github.com/qt</h4>
        <h4> 4.https://github.com/progsource/maddy</h4>
        <h4> 5.https://github.com/cameron314/concurrentqueue</h4>
        <h4> 6.https://github.com/progschj/ThreadPool</h4>
        <h4> 7.https://github.com/nlohmann/json</h4>
    )";
    ui->plainTextEdit->appendHtml(html_about);
    // 只读
    ui->plainTextEdit->setReadOnly(true);
    ui->plainTextEdit->setObjectName("ElaPlainTextEdit");
    ui->scrollArea->setObjectName("ElaScrollArea");
    ui->label_language->setObjectName("ElaText");

    initComboBox();
    initSignalSlots();
}

settingsWindow::~settingsWindow() {
    delete ui;
}

void settingsWindow::initComboBox() {
    ui->comboBox_language->addItems(
        {"zh_CN", "en_US"}
    );

    ui->comboBox_default_new_window->addItems(
        {"serial window", "serial server"}
    );

    defaultNewWindowMap_ = {
        {"serial window", defaultNewWindowType::serialServer},
        {"serial server", defaultNewWindowType::serialServer}
    };
}

void settingsWindow::initSignalSlots() {
    connect(ui->comboBox_language, &ElaComboBox::currentTextChanged, this, [this]() {
        QString text = ui->comboBox_language->currentText();
        emit sigLanguageChanged(text);
    });
    connect(ui->comboBox_default_new_window, &ElaComboBox::currentTextChanged, this, [this]() {
        QString text = ui->comboBox_default_new_window->currentText();
        auto rs = defaultNewWindowMap_.at(text);
        emit sigDefaultNewWindowChanged(rs);
    });
}

void settingsWindow::changeEvent(QEvent *event) {
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
