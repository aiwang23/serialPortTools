//
// Created by wang on 25-4-1.
//

// You may need to build the project (run Qt uic code generator) to get "ui_mainWindow.h" resolved

#include "mainwindow.h"

#include <ElaIconButton.h>

#include "ui_mainWindow.h"
#include <ElaTheme.h>
#include <QSettings>
#include <ElaMessageBar.h>
#include <sstream>
#include <CSerialPort/SerialPortInfo.h>
#include <maddy/parser.h>
#include <QTimer>
#include <bitset>
#include <QBitArray>

using itas109::CSerialPortInfo;
using itas109::SerialPortInfo;

// 查看当前系统颜色主题
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

// {"5", "6", "7", "8"}
static itas109::DataBits SerialDataBitsFrom(const QString &chars) {
    return static_cast<itas109::DataBits>(chars.toInt());
}

// {"none", "even", "odd"}
static itas109::Parity SerialParityFrom(const QString &chars) {
    if (chars == "none") {
        return itas109::ParityNone;
    } else if (chars == "odd") {
        return itas109::ParityOdd;
    } else if (chars == "even") {
        return itas109::ParityEven;
    } else {
        return itas109::ParityNone;
    }
}

// {"1", "1.5", "2"}
static itas109::StopBits SerialStopBitsFrom(const QString &chars) {
    if (chars == "1")
        return itas109::StopOne;
    else if (chars == "1.5")
        return itas109::StopOneAndHalf;
    else if (chars == "2")
        return itas109::StopTwo;
}

// text 是否为 html
static bool isHtml(const QString &text) {
    QRegularExpression htmlRegex(R"(<([a-z][a-z0-9]*)(?:[^>]*)?>(.*?)<\/\1>|<[a-z]+\s*\/?>|&[a-z]+;)");
    return htmlRegex.match(text).hasMatch();
}

mainWindow::mainWindow(QWidget *parent) : QWidget(parent), ui(new Ui::mainWindow) {
    ui->setupUi(this);

    // // 颜色主题|最小化|最大化|关闭
    // setWindowButtonFlags(
    //     ElaAppBarType::ThemeChangeButtonHint |
    //     ElaAppBarType::MinimizeButtonHint |
    //     ElaAppBarType::MaximizeButtonHint |
    //     ElaAppBarType::CloseButtonHint
    // );
    // 隐藏 颜色主题|最小化|最大化|关闭
    // setWindowButtonFlags({});
    setWindowTitle("");

    setAttribute(Qt::WA_TranslucentBackground);

    // 切换为当前系统颜色
    // eTheme->setThemeMode(isSystemDarkTheme() == true ? ElaThemeType::Dark : ElaThemeType::Light);
    // 只读 不可编辑
    ui->plainTextEdit_out->setReadOnly(true);

    ui->toggleswitch_open->setText("open");

    // 刷新串口按钮 在 ui->comboBox_port 右侧
    iconBtn_flush_ = new ElaIconButton{ElaIconType::ArrowRotateLeft, 17, 20, 20};
    ui->gridLayout->addWidget(iconBtn_flush_, 0, 2);

    // 初始化 combobox 组件
    initComboBox();
    // init signals 和 slots
    initSignalSlots();

    // init markdown to html converter
    config_ = std::make_shared<maddy::ParserConfig>();
    config_->enabledParsers &= ~maddy::types::EMPHASIZED_PARSER; // disable emphasized parser
    config_->enabledParsers |= maddy::types::HTML_PARSER; // do not wrap HTML in paragraph
    parser_ = std::make_unique<maddy::Parser>(config_);
}

mainWindow::~mainWindow() {
    serial_port_.close();
    delete ui;
}

void mainWindow::initComboBox() {
    ui->comboBox_port->setToolTip(tr("serial port"));
    // 刷新串口，更新状态
    refreshSerialPort();

    ui->comboBox_baud->setToolTip(tr("baud rate"));
    ui->comboBox_baud->addItems(
        QStringList{"9600", "19200", "38400", "57600", "115200", "230400"}
    );

    ui->comboBox_databit->setToolTip(tr("data bit"));
    ui->comboBox_databit->addItems(
        {"8", "5", "6", "7"}
    );

    ui->comboBox_parity->setToolTip(tr("parity"));
    ui->comboBox_parity->addItems(
        {"none", "even", "odd"}
    );

    ui->comboBox_stopbit->setToolTip(tr("stop bits"));
    ui->comboBox_stopbit->addItems(
        {"1", "1.5", "2"}
    );

    ui->comboBox_show_type->addItems(
        {"text", "bin", "hex", "markdown", "html"}
    );
}


void mainWindow::initSignalSlots() {
    // 主题切换
    // connect(this, &ElaWidget::themeChangeButtonClicked, this, [&]() {
    //     eTheme->setThemeMode(
    //         eTheme->getThemeMode() == ElaThemeType::Light
    //             ? ElaThemeType::Dark
    //             : ElaThemeType::Light
    //     );
    // });

    // 打开/关闭串口
    connect(ui->toggleswitch_open, &ElaToggleButton::toggled, this, [this](bool checked) {
        if (checked) {
            // 打开
            if (ui->comboBox_port->count()) {
                // 有可用serial
                // 填入args
                std::string device_name = ui->comboBox_port->currentText().toStdString();
                int baudrate = ui->comboBox_baud->currentText().toInt();
                itas109::DataBits data_bits = SerialDataBitsFrom(ui->comboBox_databit->currentText());
                itas109::Parity parity = SerialParityFrom(ui->comboBox_parity->currentText());
                itas109::StopBits stop_bits = SerialStopBitsFrom(ui->comboBox_stopbit->currentText());
                serial_port_.init(
                    device_name.c_str(), baudrate, parity, data_bits, stop_bits
                );
                serial_port_.setReadIntervalTimeout(0);

                // 打开 serial
                if (not serial_port_.open()) {
                    ElaMessageBar::error(
                        ElaMessageBarType::TopLeft, "error",
                        QString{"%0 %1"}.arg(serial_port_.getLastError()).arg(
                            serial_port_.getLastErrorMsg()), 3000, this
                    );
                    ui->toggleswitch_open->setText(tr("open"));
                    ui->toggleswitch_open->setIsToggled(false);
                    return;
                }
                // 更新 渲染类型
                std::string type = ui->comboBox_show_type->currentText().toStdString();
                show_type_ = showTypeFrom(type);
                ui->toggleswitch_open->setText(tr("close"));
            } else {
                // 没有可用 serials
                ElaMessageBar::error(ElaMessageBarType::TopLeft, "error", tr("This Computer no avaiable port"), 3000,
                                     this);

                ui->toggleswitch_open->setIsToggled(false);
            }
        } else {
            // 关闭
            serial_port_.close();
            ui->toggleswitch_open->setText(tr("open"));
        }
    });

    // 发送串口
    connect(ui->pushButton_send, &QPushButton::clicked, this, [&]() {
        if (serial_port_.isOpen()) {
            QString sendStr = ui->plainTextEdit_in->toPlainText();

            QByteArray ba = sendStr.toLocal8Bit();
            const char *s = ba.constData();

            // 支持中文并获取正确的长度
            serial_port_.writeData(s, ba.length());
        } else {
            // QMessageBox::information(NULL,tr("information"),tr("please open serial port first"));
            ElaMessageBar::error(ElaMessageBarType::TopLeft, "error", tr("please open serial port first"), 3000, this);
        }
    });

    // 清空 comboBox_port items
    connect(this, &mainWindow::sigClearComboBoxPort, this, [this]() {
        ui->comboBox_port->clear();
    });

    // 添加 comboBox_port items
    connect(this, &mainWindow::sigAddSerialPort, this, [this](const QString &item) {
        ui->comboBox_port->addItem(item);
    });

    // 收到串口数据
    connect(this, &mainWindow::sigSerialPortData, this, [this](const QString &msg) {
        if (show_type_ == TEXT) {
            ui->plainTextEdit_out->moveCursor(QTextCursor::End);
            ui->plainTextEdit_out->insertPlainText(msg);
        } else if (show_type_ == BIN) {
            QString bin;
            for (size_t i = 0; i < msg.size(); ++i) {
                std::bitset<8> bits(msg.at(i).toLatin1());
                bin += bits.to_string().c_str();
            }
            ui->plainTextEdit_out->moveCursor(QTextCursor::End);
            ui->plainTextEdit_out->insertPlainText(bin);
        } else if (show_type_ == HEX) {
            constexpr char hexChars[] = "0123456789abcdef";
            std::string hex;
            hex.reserve(msg.size() * 2);
            std::string bin = msg.toStdString();
            for (unsigned char byte: bin) {
                hex.push_back(hexChars[byte >> 4]);
                hex.push_back(hexChars[byte & 0x0F]);
            }
            ui->plainTextEdit_out->moveCursor(QTextCursor::End);
            ui->plainTextEdit_out->insertPlainText(QString::fromStdString(hex));
        } else if (show_type_ == HTML) {
            static QString lastLine;
            QStringList lines = msg.split("\n");
            lines[0] = lastLine + lines[0];
            for (int i = 0; i < lines.size(); ++i) {
                if (i != lines.size() - 1) {
                    ui->plainTextEdit_out->appendHtml(lines[i]);
                }
            }
            lastLine = lines.back();
            if (isHtml(lastLine)) {
                ui->plainTextEdit_out->appendHtml(lastLine);
                lastLine = "";
            }
        } else if (show_type_ == MARKDOWN) {
            static QString lastLine;
            std::stringstream strSt;
            QStringList lines = msg.split("\n");
            lines[0] = lastLine + lines[0];
            for (int i = 0; i < lines.size(); ++i) {
                if (i != lines.size() - 1) {
                    strSt << lines[i].toStdString();
                }
            }
            std::string html = parser_->Parse(strSt);
            ui->plainTextEdit_out->appendHtml(QString::fromStdString(html));

            lastLine = lines.back();
        }
    }, Qt::QueuedConnection);

    // 显示类型切换
    connect(ui->comboBox_show_type, &QComboBox::currentTextChanged, this, [this](QString item) {
        if (showTypeFrom(item.toStdString()) == TEXT) {
            show_type_ = TEXT;
        } else if (showTypeFrom(item.toStdString()) == BIN) {
            show_type_ = BIN;
        } else if (showTypeFrom(item.toStdString()) == HEX) {
            show_type_ = HEX;
        } else if (showTypeFrom(item.toStdString()) == MARKDOWN) {
            show_type_ = MARKDOWN;
        } else if (showTypeFrom(item.toStdString()) == HTML) {
            show_type_ = HTML;
        }
    });

    // 接受serial onReadEvent初始化
    serial_port_.connectReadEvent(this);

    // 刷新串口状态
    connect(iconBtn_flush_, &ElaIconButton::clicked, this, &mainWindow::refreshSerialPort);
}


void mainWindow::onReadEvent(const char *portName, unsigned int readBufferLen) {
    if (readBufferLen > 0) {
        int recLen = 0;
        char *str = nullptr;
        str = new char[readBufferLen];
        recLen = serial_port_.readData(str, readBufferLen);

        if (recLen > 0) {
            //* 中文需要由两个字符拼接，否则显示为空""
            QString m_str = QString::fromLocal8Bit(str, recLen);
            emit sigSerialPortData(m_str);
        } else {
        }

        if (str) {
            delete[] str;
            str = nullptr;
        }
    }
}

void mainWindow::refreshSerialPort() {
    // 获取可用串口list
    std::vector<SerialPortInfo> portNameList = CSerialPortInfo::availablePortInfos();
    ui->comboBox_port->clear();
    for (SerialPortInfo portName: portNameList) {
        ui->comboBox_port->addItem(QString::fromLocal8Bit(portName.portName));
    }
}
