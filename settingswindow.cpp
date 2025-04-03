//
// Created by wang on 25-4-2.
//

// You may need to build the project (run Qt uic code generator) to get "ui_settingsWindow.h" resolved

#include "settingswindow.h"
#include "ui_settingsWindow.h"


settingsWindow::settingsWindow(QWidget *parent) : QWidget(parent), ui(new Ui::settingsWindow) {
    ui->setupUi(this);

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
    )";
    ui->plainTextEdit->appendHtml(html_about);
    // 只读
    ui->plainTextEdit->setReadOnly(true);

    // 无边框
    ui->scrollArea->setStyleSheet("border:none;");
}

settingsWindow::~settingsWindow() {
    delete ui;
}
