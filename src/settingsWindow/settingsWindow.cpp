//
// Created by wang on 25-4-2.
//

// You may need to build the project (run Qt uic code generator) to get "ui_settingsWindow.h" resolved

#include "settingsWindow.h"

#include "settings.h"
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
        {"serial window", defaultNewWindowType::serialWindow},
        {"serial server", defaultNewWindowType::serialServer}
    };
}

void settingsWindow::initSignalSlots() {
    connect(ui->comboBox_language, &ElaComboBox::currentTextChanged, this, [this](QString currentText) {
        if (currentText == oldLangText) {
            return;
        }
        emit sigLanguageChanged(currentText);

        nlohmann::json json;
        json["language"] = currentText.toStdString();
        settings::instance().set(json);
        oldLangText = currentText;
    });

    connect(ui->comboBox_default_new_window, &ElaComboBox::currentTextChanged, this, [this](QString currentText) {
        if (currentText == oldDefaultNewWindowText) {
            return;
        }
        auto rs = defaultNewWindowMap_.at(currentText);
        emit sigDefaultNewWindowChanged(rs);

        nlohmann::json json;
        json["defaultNewWindow"] = currentText.toStdString();
        settings::instance().set(json);
        oldDefaultNewWindowText = currentText;
    });

    connect(&settings::instance(), &settings::sigSettingsUpdated, this, &settingsWindow::settingsUpdate);
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

void settingsWindow::settingsUpdate(nlohmann::json json) {
    for (auto &[key, val]: json.items()) {
        if (key == "language") {
            QString text = QString::fromStdString(val.get<std::string>());
            ui->comboBox_language->setCurrentText(text);
            emit sigLanguageChanged(text);
        } else if (key == "defaultNewWindow") {
            QString text = QString::fromStdString(val.get<std::string>());
            ui->comboBox_default_new_window->setCurrentText(text);
            const defaultNewWindowType rs = defaultNewWindowMap_.at(text);
            emit sigDefaultNewWindowChanged(rs);
        }
    }
}
