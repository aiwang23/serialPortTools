//
// Created by wang on 25-4-2.
//

// You may need to build the project (run Qt uic code generator) to get "ui_tabWindow.h" resolved

#include "mainwindow.h"

#include <qtabbar.h>

#include "ui_mainWindow.h"
#include <ElaIconButton.h>
#include <ElaMessageBar.h>
#include <ElaTheme.h>
#include <qevent.h>

#include "inputdialog.h"
#include <ELaMenu.h>
#include <QSettings>

#include "settingswindow.h"
#include <QTranslator>
#include <ElaToolButton.h>
#include <fstream>

#include "serialserver.h"
#include "settings.h"

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

mainWindow::mainWindow(QWidget *parent) : ElaWidget(parent), ui(new Ui::mainWindow),
                                          default_new_window_(defaultNewWindowType::serialWindow) {
    // 初始化设置
    initSettings();

    ui->setupUi(this);

    setWindowTitle("serialPortTools");
    // 颜色主题|最小化|最大化|关闭
    setWindowButtonFlags(
        ElaAppBarType::ThemeChangeButtonHint |
        ElaAppBarType::MinimizeButtonHint |
        ElaAppBarType::MaximizeButtonHint |
        ElaAppBarType::CloseButtonHint
    );

    ui->tabWidget->setObjectName("ElaTabWidget");
    setObjectName("ElaWidget");

    setWindowIcon(QIcon{":/res/plug.png"});

    eTheme->setThemeMode(
        true == isSystemDarkTheme() ? ElaThemeType::Dark : ElaThemeType::Light
    );

    // 开场白
    ElaMessageBar::information(ElaMessageBarType::TopLeft, tr("hello"), tr("welcome"), 2000, this);

    initButton();
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

    // 新建 window
    connect(new_serial_button_, &QPushButton::clicked, this, &mainWindow::newWindow);

    // 剩余最后一个小窗格之后, 点击关闭，直接退出
    connect(ui->tabWidget, &ElaTabWidget::tabCloseRequested, this, [&]() {
        if (ui->tabWidget->count() <= 1)
            close();
    });

    // 父组件 传子组件
    QList<serialWindow *> w_list = findChildren<serialWindow *>();
    for (auto w: w_list) {
        connect(this, &mainWindow::hideSecondaryWindow, w, &serialWindow::hideSecondaryWindow);
        connect(this, &mainWindow::showSecondaryWindow, w, &serialWindow::showSecondaryWindow);
    }
}

void mainWindow::initButton() {
    QWidget *widget = new QWidget;
    QHBoxLayout *hbox = new QHBoxLayout(widget);
    new_serial_button_ = new ElaIconButton{ElaIconType::PlusLarge, 15, 30, 30};
    more_tools_button_ = new ElaToolButton;
    hbox->addWidget(new_serial_button_);
    hbox->addWidget(more_tools_button_);
    ui->tabWidget->setCornerWidget(widget);

    more_tools_button_->setFixedSize(30, 30);

    more_menu_ = new ElaMenu;
    serial_action_ = more_menu_->addElaIconAction(ElaIconType::Plug, tr("serial"));
    serialServer_action_ = more_menu_->addElaIconAction(ElaIconType::Server, tr("server"));
    settings_action_ = more_menu_->addElaIconAction(ElaIconType::Gear, tr("setting"));
    connect(serial_action_, &QAction::triggered, this, &mainWindow::newSerialWindow);
    connect(serialServer_action_, &QAction::triggered, this, &mainWindow::newSerialSerVer);
    connect(settings_action_, &QAction::triggered, this, [&]() {
        settingsWindow *w = new settingsWindow;
        int idx = ui->tabWidget->addTab(w, tr("setting"));
        ui->tabWidget->setCurrentIndex(idx);

        connect(w, &settingsWindow::sigLanguageChanged, this, [this](const QString &language) {
            // 移除旧的翻译器
            QCoreApplication::removeTranslator(translator_);

            // 加载新的语言文件
            if (translator_->load(":/res/translations/" + language + ".qm")) {
                QCoreApplication::installTranslator(translator_);
            } else {
                // 加载失败，回退到英语
                if (translator_->load(":/res/translations/en_US.qm")) {
                    QCoreApplication::installTranslator(translator_);
                }
            }

            // 通知所有窗口重新翻译UI
            QCoreApplication::postEvent(QCoreApplication::instance(),
                                        new QEvent(QEvent::LanguageChange));
            for (QWidget *w_i: QApplication::allWidgets()) {
                w_i->update();
            }
        });
        connect(w, &settingsWindow::sigDefaultNewWindowChanged, this, [this](const defaultNewWindowType &type) {
            default_new_window_ = type;
        });

        w->settingsUpdate(settings::instance().get());
    });

    more_tools_button_->setMenu(more_menu_);
}

void mainWindow::initSettings() {
    const char* fileName = "settings.json";

    // 创建默认设置文件（如果不存在）
    if (not std::filesystem::exists(fileName)) {
        try {
            std::ofstream ofs(fileName);
            if (!ofs.is_open()) {
                qWarning() << "Failed to create settings file:" << fileName;
                settings::instance().set(settings::defaultSettings());
                return;
            }
            ofs << settings::defaultSettings().dump(4);  // 使用缩进美化输出
            ofs.close();
        } catch (const std::exception& e) {
            qCritical() << "Error creating settings file:" << e.what();
            settings::instance().set(settings::defaultSettings());
            return;
        }
    }

    // 读取设置文件
    nlohmann::json json;
    try {
        std::ifstream ifs(fileName);
        if (not ifs.is_open()) {
            throw std::runtime_error("Failed to open settings file");
        }

        json = nlohmann::json::parse(ifs);
        ifs.close();
    } catch (const std::exception& e) {
        qWarning() << "Error parsing settings file:" << e.what() << "- Using default settings";
        json = settings::defaultSettings();
        // 设置系统默认语言
        json["language"] = QLocale::system().name().toStdString();
    }

    // 处理语言设置
    QString lang;
    try {
        lang = QString::fromStdString(json.at("language").get<std::string>());
    } catch (const std::exception& e) {
        qWarning() << "Invalid language setting:" << e.what() << "- Using system default";
        lang = QLocale::system().name();
        json["language"] = lang.toStdString();
    }

    // 加载翻译文件
    translator_ = new QTranslator(this);
    bool translationLoaded = false;

    // 尝试加载用户设置的语言
    if (!lang.isEmpty()) {
        QString translationPath = ":/res/translations/" + lang + ".qm";
        if (translator_->load(translationPath)) {
            if (QCoreApplication::installTranslator(translator_)) {
                translationLoaded = true;
            } else {
                qWarning() << "Failed to install translator for language:" << lang;
            }
        }
    }

    // 如果首选语言加载失败，尝试加载英语
    if (!translationLoaded) {
        if (translator_->load(":/res/translations/en_US.qm")) {
            if (QCoreApplication::installTranslator(translator_)) {
                json["language"] = "en_US";  // 更新设置
                qInfo() << "Falling back to English translation";
            } else {
                qWarning() << "Failed to install English translator";
            }
        } else {
            qWarning() << "Could not load any translation files";
        }
    }

    // 保存最终设置
    try {
        settings::instance().set(json);

        // 如果设置文件损坏或被重置，重新保存
        std::ofstream ofs(fileName);
        if (ofs.is_open()) {
            ofs << json.dump(4);
            ofs.close();
        }
    } catch (const std::exception& e) {
        qCritical() << "Error saving settings:" << e.what();
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
        if (QEvent::LanguageChange == event->type()) {
            ui->retranslateUi(this);
            int size = ui->tabWidget->count();
            QString name;
            for (int i = 0; i < size; ++i) {
                name = ui->tabWidget->widget(i)->objectName();
                if ("settingsWindow" == name) {
                    ui->tabWidget->tabBar()->setTabText(i, tr("setting"));
                }
            }

            serial_action_->setText(tr("serial"));
            serialServer_action_->setText(tr("server"));
            settings_action_->setText(tr("setting"));
        }
    }

    ElaWidget::changeEvent(event);
}


void mainWindow::newWindow() {
    QString info;
    if (defaultNewWindowType::serialWindow == default_new_window_) {
        info = tr("new serial window");
    } else if (defaultNewWindowType::serialServer == default_new_window_) {
        info = tr("new serial server");
    }
    inputDialog dialog{info};

    dialog.exec();
    QString text = dialog.getInputText();
    if (not text.isEmpty()) {
        QWidget *w = nullptr;
        if (defaultNewWindowType::serialWindow == default_new_window_) {
            w = new serialWindow;
        } else if (defaultNewWindowType::serialServer == default_new_window_) {
            w = new serialServer;
        }

        int idx = ui->tabWidget->addTab(w, text);
        ui->tabWidget->setCurrentIndex(idx);

        // 开场白
        ElaMessageBar::information(ElaMessageBarType::TopLeft, tr("info"), tr("new window succeed"), 2000, this);
    }
}

void mainWindow::newSerialWindow() {
    inputDialog dialog{QString(tr("new window"))};

    dialog.exec();
    QString text = dialog.getInputText();
    if (not text.isEmpty()) {
        serialWindow *w = new serialWindow;

        int idx = ui->tabWidget->addTab(w, text);
        ui->tabWidget->setCurrentIndex(idx);

        // 开场白
        ElaMessageBar::information(ElaMessageBarType::TopLeft, tr("info"), tr("new window succeed"), 2000, this);
    }
}

void mainWindow::newSerialSerVer() {
    inputDialog dialog{QString(tr("new serial server"))};

    dialog.exec();
    QString text = dialog.getInputText();
    if (not text.isEmpty()) {
        auto *w = new serialServer;
        int idx = ui->tabWidget->addTab(w, text);
        ui->tabWidget->setCurrentIndex(idx);

        // 开场白
        ElaMessageBar::information(ElaMessageBarType::TopLeft, tr("info"), tr("new window succeed"), 2000, this);
    }
}
