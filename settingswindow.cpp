//
// Created by wang on 25-4-2.
//

// You may need to build the project (run Qt uic code generator) to get "ui_settingsWindow.h" resolved

#include "settingswindow.h"
#include "ui_settingsWindow.h"


settingsWindow::settingsWindow(QWidget *parent) :
    QWidget(parent), ui(new Ui::settingsWindow) {
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);
    setWindowTitle("");
    // setWindowButtonFlags({});
    setAttribute(Qt::WA_TranslucentBackground);

    QString html_about = R"(    <h1>about</h1>
    <p> version: 1.0.0</p>
    <p>Open Source License: MIT</p>
    <p> Third party libraries:</p>
    <ul>
        <li>https://github.com/Liniyous/ElaWidgetTools</li>
        <li>https://github.com/itas109/CSerialPort</li>
        <li>https://github.com/qt</li>
    </ul>)";
    ui->plainTextEdit->appendHtml(html_about);
}

settingsWindow::~settingsWindow() {
    delete ui;
}
