//
// Created by wang on 25-4-2.
//

// You may need to build the project (run Qt uic code generator) to get "ui_tabWindow.h" resolved

#include "mainwindow.h"

#include <qtabbar.h>

#include "ui_mainWindow.h"
#include <ElaIconButton.h>
#include <ElaContentDialog.h>
#include <ElaMessageBar.h>
#include <ElaTheme.h>
#include <qevent.h>

#include "inputdialog.h"
#include <ELaMenu.h>
#include <QSettings>

#include "settingswindow.h"
#include <QTranslator>

static bool isSystemDarkTheme() {
    // 1. 先检查调色板（通用方法）
    bool isDark = QGuiApplication::palette().color(QPalette::Window).lightness() < 128;

    // 2. 平台特定检测（覆盖通用结果）
#ifdef Q_OS_WIN
    QSettings registry("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                       QSettings::NativeFormat);
    isDark = registry.value("AppsUseLightTheme", 1).toInt() == 0;
#elif defined(Q_OS_MACOS)
    QSettings settings("Apple Global Domain", QSettings::NativeFormat);
    isDark = settings.value("AppleInterfaceStyle").toString().toLower() == "dark";
#elif defined(Q_OS_LINUX)
    if (qEnvironmentVariableIsSet("GTK_THEME")) {
        isDark = QString(qgetenv("GTK_THEME")).contains("dark", Qt::CaseInsensitive);
    }
#endif

    return isDark;
}

mainWindow::mainWindow(QWidget *parent) : ElaWidget(parent), ui(new Ui::mainWindow) {
    // 获取系统语言
    QString systemLang = QLocale::system().name(); // 例如: "zh_CN", "en_US"
    // 尝试加载对应的.qm文件
    translator_ = new QTranslator{this};
    if (translator_->load(":/translations/" + systemLang + ".qm")) {
        QCoreApplication::installTranslator(translator_);
    } else {
        // 加载失败，使用默认语言(如英语)
        if (translator_->load(":/translations/en_US.qm")) {
            QCoreApplication::installTranslator(translator_);
        }
    }

    ui->setupUi(this);

    setWindowTitle("serialPortTools");
    // 颜色主题|最小化|最大化|关闭
    setWindowButtonFlags(
        ElaAppBarType::ThemeChangeButtonHint |
        ElaAppBarType::MinimizeButtonHint |
        ElaAppBarType::MaximizeButtonHint |
        ElaAppBarType::CloseButtonHint
    );

    QWidget *widget = new QWidget;
    auto *hbox = new QHBoxLayout(widget);
    new_icon_button_ = new ElaIconButton{ElaIconType::PlusLarge, 20, 30, 30};
    more_icon_button_ = new ElaIconButton{ElaIconType::AngleDown, 23, 30, 30};
    hbox->addWidget(new_icon_button_);
    hbox->addWidget(more_icon_button_);
    ui->tabWidget->setCornerWidget(widget);
    ui->tabWidget->setObjectName("ElaTabWidget");
    setObjectName("ElaWidget");

    eTheme->setThemeMode(
        true == isSystemDarkTheme() ? ElaThemeType::Dark : ElaThemeType::Light
    );

    // 开场白
    ElaMessageBar::information(ElaMessageBarType::TopLeft, tr("hello"), tr("welcome"), 2000, this);

    // 连接signals slots
    initSignalSlots();

    // 移动到屏幕中心
    moveToCenter();
}

mainWindow::~mainWindow() {
    delete ui;
}

void mainWindow::initSignalSlots() {
    // 主题切换
    connect(this, &ElaWidget::themeChangeButtonClicked, this, [&]() {
        eTheme->setThemeMode(
            eTheme->getThemeMode() == ElaThemeType::Light
                ? ElaThemeType::Dark
                : ElaThemeType::Light
        );
    });

    // 添加 serial window
    connect(new_icon_button_, &QPushButton::clicked, this, &mainWindow::newSerialWindow);

    // add settings window
    connect(more_icon_button_, &QPushButton::clicked, this, [&]() {
        ElaMenu menu;
        auto serial_action = menu.addElaIconAction(ElaIconType::Plug, tr("serial"));
        auto settings_action = menu.addElaIconAction(ElaIconType::Gear, tr("setting"));
        connect(serial_action, &QAction::triggered, this, &mainWindow::newSerialWindow);
        connect(settings_action, &QAction::triggered, this, [&]() {
            settingsWindow *w = new settingsWindow;
            int idx = ui->tabWidget->addTab(w, tr("setting"));
            ui->tabWidget->setCurrentIndex(idx);

            connect(w, &settingsWindow::sigLanguageChanged, this, [this](QString language) {
                // 移除旧的翻译器
                QCoreApplication::removeTranslator(translator_);

                // 加载新的语言文件
                if (translator_->load(":/translations/" + language + ".qm")) {
                    QCoreApplication::installTranslator(translator_);
                } else {
                    // 加载失败，回退到英语
                    if (translator_->load(":/translations/en_US.qm")) {
                        QCoreApplication::installTranslator(translator_);
                    }
                }

                // 通知所有窗口重新翻译UI
                QCoreApplication::postEvent(QCoreApplication::instance(),
                                            new QEvent(QEvent::LanguageChange));
                for (QWidget *widget: QApplication::allWidgets()) {
                    widget->update();
                }
            });
        });

        menu.exec(QCursor::pos());
    });

    // 剩余最后一个小窗格之后, 点击关闭，直接退出
    connect(ui->tabWidget, &ElaTabWidget::tabCloseRequested, this, [&]() {
        if (ui->tabWidget->count() <= 1)
            close();
    });

    // 父组件 传子组件
    // serialWindow *w = findChild<serialWindow *>();
    // if (w) {
    //     connect(this, &mainWindow::hideSecondaryWindow, w, &serialWindow::hideSecondaryWindow);
    //     connect(this, &mainWindow::showSecondaryWindow, w, &serialWindow::showSecondaryWindow);
    // }
    QList<serialWindow *> w_list = findChildren<serialWindow *>();
    for (auto w: w_list) {
        connect(this, &mainWindow::hideSecondaryWindow, w, &serialWindow::hideSecondaryWindow);
        connect(this, &mainWindow::showSecondaryWindow, w, &serialWindow::showSecondaryWindow);
    }
}

void mainWindow::resizeEvent(QResizeEvent *event) {
    ElaWidget::resizeEvent(event);
    // 获取屏幕的宽度
    QScreen *screen = QApplication::primaryScreen();
    int screenWidth = screen->geometry().width();

    // 检查当前窗口宽度是否是屏幕宽度的一半 隐藏副窗口
    if (this->width() <= screenWidth / 2 + 1) {
        emit hideSecondaryWindow();
    } else {
        emit showSecondaryWindow();
    }
}


void mainWindow::changeEvent(QEvent *event) {
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


void mainWindow::newSerialWindow() {
    inputDialog dialog{QString(tr("new serial window"))};

    dialog.exec();
    QString text = dialog.getInputText();
    if (not text.isEmpty()) {
        auto *w = new serialWindow;
        int idx = ui->tabWidget->addTab(w, text);
        ui->tabWidget->setCurrentIndex(idx);

        // 开场白
        ElaMessageBar::information(ElaMessageBarType::TopLeft, tr("info"), tr("new window succeed"), 2000, this);
    }
}
