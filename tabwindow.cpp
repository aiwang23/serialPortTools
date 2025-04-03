//
// Created by wang on 25-4-2.
//

// You may need to build the project (run Qt uic code generator) to get "ui_tabWindow.h" resolved

#include "tabwindow.h"

#include <qtabbar.h>

#include "ui_tabWindow.h"
#include <ElaIconButton.h>
#include <ElaContentDialog.h>
#include <ElaMessageBar.h>
#include <ElaTheme.h>
#include <qevent.h>

#include "inputdialog.h"
#include <ELaMenu.h>

#include "settingswindow.h"

tabWindow::tabWindow(QWidget *parent) : ElaWidget(parent), ui(new Ui::tabWindow) {
    ui->setupUi(this);

    setWindowTitle("serialPortTools");
    // 颜色主题|最小化|最大化|关闭
    setWindowButtonFlags(
        ElaAppBarType::ThemeChangeButtonHint |
        ElaAppBarType::MinimizeButtonHint |
        ElaAppBarType::MaximizeButtonHint |
        ElaAppBarType::CloseButtonHint
    );
    ui->tabWidget->setStyleSheet("QTabWidget::pane{border:none;}");

    auto *widget = new QWidget(this);
    auto *hbox = new QHBoxLayout(widget);
    new_icon_button_ = new ElaIconButton{ElaIconType::PlusLarge, 20, 30, 30};
    more_icon_button_ = new ElaIconButton{ElaIconType::AngleDown, 23, 30, 30};
    hbox->addWidget(new_icon_button_);
    hbox->addWidget(more_icon_button_);
    // widget->setStyleSheet("QWidget::pane{border:none;}");
    ui->tabWidget->setCornerWidget(widget);

    // 开场白
    ElaMessageBar::information(ElaMessageBarType::TopLeft, "hello", "welcome", 2000, this);

    // 连接signals slots
    initSignalSlots();

    // 移动到屏幕中心
    moveToCenter();
}

tabWindow::~tabWindow() {
    delete ui;
}

void tabWindow::initSignalSlots() {
    // 主题切换
    connect(this, &ElaWidget::themeChangeButtonClicked, this, [&]() {
        eTheme->setThemeMode(
            eTheme->getThemeMode() == ElaThemeType::Light
                ? ElaThemeType::Dark
                : ElaThemeType::Light
        );
    });

    // 添加 serial window
    connect(new_icon_button_, &QPushButton::clicked, this, &tabWindow::newSerialWindow);

    // add settings window
    connect(more_icon_button_, &QPushButton::clicked, this, [&]() {
        ElaMenu menu;
        auto serial_action = menu.addElaIconAction(ElaIconType::Plug, "serial");
        auto settings_action = menu.addElaIconAction(ElaIconType::Gear, tr("setting"));
        connect(serial_action, &QAction::triggered, this, &tabWindow::newSerialWindow);
        connect(settings_action, &QAction::triggered, this, [&]() {
            settingsWindow *w = new settingsWindow;
            int idx = ui->tabWidget->addTab(w, tr("setting"));
            ui->tabWidget->setCurrentIndex(idx);
        });

        menu.exec(QCursor::pos());
    });

    // 剩余最后一个小窗格之后, 点击关闭，直接退出
    connect(ui->tabWidget, &ElaTabWidget::tabCloseRequested, this, [&]() {
        if (ui->tabWidget->count() <= 1)
            close();
    });
}

void tabWindow::newSerialWindow() {
    inputDialog dialog{QString("new window name")};

    dialog.exec();
    QString text = dialog.getInputText();
    if (not text.isEmpty()) {
        auto *w = new serialWindow;
        int idx = ui->tabWidget->addTab(w, text);
        ui->tabWidget->setCurrentIndex(idx);
        // 开场白
        ElaMessageBar::information(ElaMessageBarType::TopLeft, "info", "new window succeed", 2000, this);
    }
}


