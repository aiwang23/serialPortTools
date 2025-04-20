//
// Created by wang on 25-4-1.
//

// You may need to build the project (run Qt uic code generator) to get "ui_mainWindow.h" resolved

#include "serialwindow.h"

#include <ElaIconButton.h>

#include "ui_serialwindow.h"
#include <ElaTheme.h>
#include <QSettings>
#include <ElaMessageBar.h>
#include <sstream>
#include <CSerialPort/SerialPortInfo.h>
#include <maddy/parser.h>
#include <QTimer>
#include <bitset>
#include <QBitArray>
#include <ElaToggleButton.h>
#include <QFileDialog>
#include <utility>
#include "threadPool.h"
#include <QPropertyAnimation>
#include <QTcpSocket>

#include "serialserver.h"
#include "nlohmann/json.hpp"

using itas109::CSerialPortInfo;
using itas109::SerialPortInfo;

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

static std::string toString(const itas109::Parity parity) {
    if (itas109::ParityNone == parity) return "none";
    else if (itas109::ParityOdd == parity) return "odd";
    else if (itas109::ParityEven == parity) return "even";
    else return "none";
}

// {"1", "1.5", "2"}
static itas109::StopBits SerialStopBitsFrom(const QString &chars) {
    if (chars == "1.5")
        return itas109::StopOneAndHalf;
    else if (chars == "2")
        return itas109::StopTwo;
    else
        return itas109::StopOne;
}

static std::string toString(itas109::StopBits stop_bits) {
    if (itas109::StopOneAndHalf) return "1.5";
    else if (itas109::StopTwo) return "2";
    else return "1";
}

// text 是否为 html
static bool isHtml(const QString &text) {
    QRegularExpression htmlRegex(R"(<([a-z][a-z0-9]*)(?:[^>]*)?>(.*?)<\/\1>|<[a-z]+\s*\/?>|&[a-z]+;)");
    return htmlRegex.match(text).hasMatch();
}

// 辅助函数：去除字符串前缀（如0x或0b）
static std::string removePrefix(const std::string &s) {
    if (s.size() >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X' || s[1] == 'b' || s[1] == 'B')) {
        return s.substr(2);
    }
    return s;
}

// 按照空格、回车符和换行符切割字符串
static QStringList highPerformanceSplit(const QString &str) {
    QStringList result;
    result.reserve(str.size() / 8); // 预估单词数量

    const QChar *data = str.constData();
    int length = str.length();
    int start = 0;

    for (int i = 0; i < length; ++i) {
        if (data[i].isSpace()) {
            if (i > start) {
                result.append(QString(data + start, i - start));
            }
            start = i + 1;
        }
    }

    if (length > start) {
        result.append(QString(data + start, length - start));
    }

    return result;
}

int convertToHex(QString str, QByteArray &hex) {
    // 去除所有空白字符
    QString str_clean = str.remove(QRegularExpression("\\s"));
    if (str_clean.isEmpty()) return -1;

    // 检查是否为有效十六进制字符
    QRegularExpression hexRegex("^[0-9a-fA-F]*$");
    if (!hexRegex.match(str_clean).hasMatch()) return -1;

    // 补前导零（若长度为奇数）
    if (str_clean.length() % 2 != 0) str_clean.prepend('0');

    hex.clear();
    for (int i = 0; i < str_clean.length(); i += 2) {
        bool ok;
        quint8 byte = str_clean.mid(i, 2).toUShort(&ok, 16);
        if (!ok) {
            hex.clear();
            return -1;
        }
        hex.append(byte);
    }
    return 0;
}

serialWindow::serialWindow(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::serialWindow),
      threadPool_(1),
      tcp_socket_(new QTcpSocket(this)) {
    ui->setupUi(this);

    threadPool_.enqueue([this]() {
        QByteArray data;
        while (not isStop_) {
            bool rs = data_queue_.try_dequeue(data);
            if (false == rs) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            convertDataAndSend(data);
        }
    });

    // 初始化 UI 组件
    initUI();

    dataWidgetRatio_ = {
        {
            sendMode::MAN, {int(7 * ui->splitter_data->height()), int(3 * ui->splitter_data->height())}
        },
        {
            sendMode::CMD, {int(5 * ui->splitter_data->height()), int(5 * ui->splitter_data->height())}
        }
    };
    // 切换发送模式的动画计时器
    animationTimer = new QTimer(this);
    animationTimer->setInterval(16); // ~60 FPS

    autoSendTimer_ = new QTimer{this};

    // 默认状态下 输出窗口比例70%，输入窗口占30%
    ui->splitter_data->setSizes(dataWidgetRatio_[sendMode::MAN]);

    // init signals 和 slots
    initSignalSlots();

    // init markdown to html converter
    config_ = std::make_shared<maddy::ParserConfig>();
    config_->enabledParsers &= ~maddy::types::EMPHASIZED_PARSER; // disable emphasized parser
    config_->enabledParsers |= maddy::types::HTML_PARSER; // do not wrap HTML in paragraph
    parser_ = std::make_unique<maddy::Parser>(config_);
}

serialWindow::~serialWindow() {
    if (tcp_socket_) {
        tcp_socket_->disconnectFromHost();
    }
    isStop_ = true;
    serial_port_.close();
    delete ui;
}

void serialWindow::changeEvent(QEvent *event) {
    if (event) {
        switch (event->type()) {
            // this event is send if a translator is loaded
            case QEvent::LanguageChange:
                ui->retranslateUi(this);
                ui->toggleswitch_open->setText(tr(ui->toggleswitch_open->getIsToggled()
                                                      ? "close"
                                                      : "open"));
                break;
            default:
                break;
        }
    }

    QWidget::changeEvent(event);
}

void serialWindow::initComboBox() {
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
        {"text", "hex", "markdown", "html"}
    );

    ui->comboBox_send_type->addItems(
        {"text", "hex"}
    );

    ui->comboBox_send_mode->addItems(
        {"MAN", "CMD", "AUTO"}
    );
}


void serialWindow::initSignalSlots() {
    // 打开/关闭串口
    connect(ui->toggleswitch_open, &ElaToggleButton::toggled, this, &serialWindow::openOrCloseSerialPort);

    // 发送串口
    connect(ui->pushButton_send, &QPushButton::clicked, this, [&]() {
        QString data = ui->plainTextEdit_in->toPlainText();
        emit sigSendData(data.toUtf8());
    });

    // 处理完数据，插入到 plainTextEdit_out
    connect(this, &serialWindow::sigDataCompleted, this, &serialWindow::showSerialData);

    // 显示类型切换
    connect(ui->comboBox_show_type, &ElaComboBox::currentTextChanged, this, [this](const QString &item) {
        show_type_ = dataTypeFrom(item.toStdString());
    });

    // 发送模式切换
    connect(ui->comboBox_send_mode, &ElaComboBox::currentTextChanged, this, &serialWindow::sendModeChange);

    // 自动模式 splitter_data 比例切换
    connect(animationTimer, &QTimer::timeout, this, &serialWindow::dataWidgetAnimation);

    // 用户拖动 ui->splitter_data 的时候，记录一下位置
    connect(ui->splitter_data, &QSplitter::splitterMoved, this, [this](int pos, int index) {
        dataWidgetRatio_[send_mode_] = ui->splitter_data->sizes();
    });

    // 用户点击 发送命令
    connect(ui->cmdwidget, &cmdWidget::sigCmdSend, this, &serialWindow::sigSendData);
    // 把发送的命令 发送到串口
    connect(this, &serialWindow::sigSendData, this, &serialWindow::sendDataToSerial);

    // 接受serial onReadEvent初始化
    serial_port_.connectReadEvent(this);

    // 刷新串口状态
    connect(iconBtn_refresh_, &ElaIconButton::clicked, this, &serialWindow::refreshSerialPort);

    // 保存接收数据到文件
    connect(ui->pushButton_save_file, &ElaPushButton::clicked, this, &serialWindow::saveDataWidgetToFile);

    connect(ui->pushButton_clear, &ElaPushButton::clicked, this, [this]() {
        ui->plainTextEdit_out->clear();
    });

    // 发送类型切换
    connect(ui->comboBox_send_type, &ElaComboBox::currentTextChanged, this, [this](const QString &item) {
        if (dataTypeFrom(item.toStdString()) == dataType::TEXT) {
            send_type_ = dataType::TEXT;
        } else if (dataTypeFrom(item.toStdString()) == dataType::HEX) {
            send_type_ = dataType::HEX;
        }
    });

    // 自动模式 发送数据
    connect(autoSendTimer_, &QTimer::timeout, this, [this]() {
        emit sigSendData(ui->plainTextEdit_in->toPlainText().toUtf8());
    });

    // 自动模式 是否打开
    connect(ui->toggleswitch_auto_mode, &ElaToggleSwitch::toggled, this, &serialWindow::openOrCloseAutoMode);

    // 修改自动模式 周期
    connect(ui->lineEdit_auto_mode, &ElaLineEdit::editingFinished, this, &serialWindow::autoModeCycleChange);

    // 启用远程调试
    connect(ui->toggleswitch_url, &ElaToggleSwitch::toggled, this, &serialWindow::enableRemote);

    // 回车 发起远程调试
    connect(ui->lineEdit_url, &ElaLineEdit::returnPressed, this, &serialWindow::connectToHost);

    // 刷新 远程连接
    connect(iconBtn_url_refresh_, &ElaIconButton::clicked, this, &serialWindow::connectToHost);

    // tcp 连接到了
    connect(tcp_socket_, &QTcpSocket::connected, this, [this]() {
        ElaMessageBar::information(ElaMessageBarType::TopLeft, tr("info"), tr("connected"), 3000, this);
    });

    // tcp 断开连接
    connect(tcp_socket_, &QTcpSocket::disconnected, this, [this]() {
    });

    // tcp error
    connect(tcp_socket_, &QTcpSocket::errorOccurred, this, [this](QTcpSocket::SocketError error) {
        Q_UNUSED(error)
        ElaMessageBar::error(ElaMessageBarType::TopLeft,
                             tr("error"), tr(tcp_socket_->errorString().toUtf8()), 3000,
                             this);
    });

    // tcp 接受数据
    connect(tcp_socket_, &QTcpSocket::readyRead, this, &serialWindow::readTcpData);
}

void serialWindow::initText() {
    ui->label_port->setTextPixelSize(13);
    ui->label_baud->setTextPixelSize(13);
    ui->label_databit->setTextPixelSize(13);
    ui->label_parity->setTextPixelSize(13);
    ui->label_stopbit->setTextPixelSize(13);
    ui->label_show_type->setTextPixelSize(13);
    ui->label_send_type->setTextPixelSize(13);
    ui->label_send_mode->setTextPixelSize(13);
    ui->label_auto_mode_cycle->setTextPixelSize(13);
    ui->label_url->setTextPixelSize(13);
}

void serialWindow::initUI() {
    // 只读 不可编辑
    ui->plainTextEdit_out->setReadOnly(true);
    // Ela 要把ObjectName 的名称改回这个"ElaPlainTextEdit"才能正常使用深色模式
    ui->plainTextEdit_out->setObjectName("ElaPlainTextEdit");
    ui->plainTextEdit_in->setObjectName("ElaPlainTextEdit");

    ui->toggleswitch_open->setText(tr("open"));
    // 开始按钮的默认大小
    ui->toggleswitch_open->setMinimumWidth(191);
    // 刷新串口按钮 在 ui->comboBox_port 右侧
    iconBtn_refresh_ = new ElaIconButton{ElaIconType::ArrowRotateLeft, 17, 20, 20};
    ui->horizontalLayout_0->addWidget(iconBtn_refresh_);

    // 刷新URL按钮 在 serialWindow 顶部
    iconBtn_url_refresh_ = new ElaIconButton{ElaIconType::ArrowRotateLeft, 17, 25, 25};
    ui->horizontalLayout_top->insertWidget(1, iconBtn_url_refresh_);
    ui->lineEdit_url->setEnabled(false);
    iconBtn_url_refresh_->setEnabled(false);
    iconBtn_url_refresh_->hide();
    ui->lineEdit_url->hide();

    // 默认 MAN 手动模式
    ui->cmdwidget->hide();
    ui->plainTextEdit_in->show();
    ui->pushButton_send->show();

    ui->lineEdit_auto_mode->setIsClearButtonEnable(false);
    ui->lineEdit_auto_mode->setFixedWidth(60);
    ui->pushButton_auto_mode_sec->setFixedSize(30, 30);
    // ui->pushButton_auto_mode_sec->setDisabled(true);

    ui->label_auto_mode_cycle->hide();
    ui->lineEdit_auto_mode->hide();
    ui->pushButton_auto_mode_sec->hide();
    ui->toggleswitch_auto_mode->hide();
    ui->splitter_main->setSizes({1000, 100});


    // 初始化 combobox 组件
    initComboBox();
    // 初始化 text 组件
    initText();
}

int64_t serialWindow::writeSerialSettings(const serialSettings &settings) {
    nlohmann::json json;
    json["type"] = "serialSettings";
    json["data"]["port"] = settings.port;
    json["data"]["baud"] = settings.baud;
    json["data"]["dataBits"] = settings.dataBits;
    json["data"]["parity"] = settings.parity;
    json["data"]["stopBit"] = settings.stopBit;

    QByteArray text = QByteArray::fromStdString(json.dump());
    return tcp_socket_->write(text);
}

int64_t serialWindow::writeSerialClose() {
    nlohmann::json json;
    json["type"] = "serialClose";

    QByteArray text = QByteArray::fromStdString(json.dump());
    return tcp_socket_->write(text);
}

void serialWindow::onReadEvent(const char *portName, unsigned int readBufferLen) {
    if (readBufferLen > 0) {
        QByteArray rawData;
        rawData.resize(readBufferLen);
        int recLen = serial_port_.readData(rawData.data(), readBufferLen);

        if (recLen > 0) {
            rawData.resize(recLen); // 调整大小为实际读取长度
            data_queue_.enqueue(rawData); // 存储二进制数据
        }
    }
}

void serialWindow::openOrCloseSerialPort(bool checked) {
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

            if (isRemote) {
                // 远程打开
                serialSettings settings{
                    device_name,
                    std::to_string(baudrate),
                    std::to_string(data_bits),
                    toString(parity),
                    toString(stop_bits)
                };
                int len = writeSerialSettings(settings);
            } else {
                // 本地打开
                serial_port_.init(
                    device_name.c_str(), baudrate, parity, data_bits, stop_bits
                );
                serial_port_.setReadIntervalTimeout(0);
                // 打开 serial
                if (not serial_port_.open()) {
                    ElaMessageBar::error(
                        ElaMessageBarType::TopLeft, tr("error"),
                        QString{"%0 %1"}.arg(serial_port_.getLastError()).arg(
                            serial_port_.getLastErrorMsg()), 3000, this
                    );
                    ui->toggleswitch_open->setText(tr("open"));
                    ui->toggleswitch_open->setIsToggled(false);
                    return;
                }
            }

            // 更新 渲染类型
            std::string type = ui->comboBox_show_type->currentText().toStdString();
            show_type_ = dataTypeFrom(type);
            ui->toggleswitch_open->setText(tr("close"));
            ui->comboBox_baud->setDisabled(true);
            ui->comboBox_databit->setDisabled(true);
            ui->comboBox_parity->setDisabled(true);
            ui->comboBox_stopbit->setDisabled(true);
        } else {
            // 没有可用 serials
            ElaMessageBar::error(ElaMessageBarType::TopLeft, tr("error"), tr("This Computer no avaiable port"),
                                 3000,
                                 this);

            ui->toggleswitch_open->setIsToggled(false);
        }
    } else {
        if (isRemote) {
            // 远程关闭
            writeSerialClose();
        } else {
            // 本地关闭
            serial_port_.close();
        }
        ui->toggleswitch_open->setText(tr("open"));
        ui->comboBox_baud->setEnabled(true);
        ui->comboBox_databit->setEnabled(true);
        ui->comboBox_parity->setEnabled(true);
        ui->comboBox_stopbit->setEnabled(true);
    }
}

void serialWindow::sendModeChange(const QString &item) {
    send_mode_ = sendModeFrom(item.toStdString());
    if (sendMode::MAN == send_mode_) {
        ui->cmdwidget->hide();
        ui->plainTextEdit_in->show();
        ui->pushButton_send->show();

        ui->label_auto_mode_cycle->hide();
        ui->lineEdit_auto_mode->hide();
        ui->pushButton_auto_mode_sec->hide();
        ui->toggleswitch_auto_mode->hide();
        // 重置动画进度
        animationProgress = 0;
        animationTimer->start();
    } else if (sendMode::CMD == send_mode_) {
        ui->cmdwidget->show();
        ui->plainTextEdit_in->hide();
        ui->pushButton_send->hide();

        ui->label_auto_mode_cycle->hide();
        ui->lineEdit_auto_mode->hide();
        ui->pushButton_auto_mode_sec->hide();
        ui->toggleswitch_auto_mode->hide();
        // 重置动画进度
        animationProgress = 0;
        animationTimer->start();
    } else if (sendMode::AUTO == send_mode_) {
        ui->cmdwidget->hide();
        ui->plainTextEdit_in->show();
        ui->pushButton_send->show();

        ui->label_auto_mode_cycle->show();
        ui->lineEdit_auto_mode->show();
        ui->pushButton_auto_mode_sec->show();
        ui->toggleswitch_auto_mode->show();
    }
}

void serialWindow::showSerialData(const QString &data) {
    if (dataType::TEXT == show_type_) {
        ui->plainTextEdit_out->moveCursor(QTextCursor::End);
        ui->plainTextEdit_out->insertPlainText(data);
    } else if (dataType::HEX == show_type_) {
        ui->plainTextEdit_out->moveCursor(QTextCursor::End);
        ui->plainTextEdit_out->insertPlainText(data);
    } else if (dataType::HTML == show_type_) {
        ui->plainTextEdit_out->appendHtml(data);
    } else if (dataType::MARKDOWN == show_type_) {
        ui->plainTextEdit_out->appendHtml(data);
    }
}

void serialWindow::dataWidgetAnimation() {
    animationProgress += 0.02; // 每帧增加2%的进度
    // 获取当前尺寸
    QList<int> currentSizes = ui->splitter_data->sizes();
    // 计算目标尺寸
    QList<int> targetSizes = dataWidgetRatio_[send_mode_];

    if (animationProgress >= 1.0) {
        animationProgress = 1.0;
        animationTimer->stop();
    }

    // 计算中间值
    QList<int> intermediateSizes;
    intermediateSizes << currentSizes[0] + (targetSizes[0] - currentSizes[0]) * animationProgress
            << currentSizes[1] + (targetSizes[1] - currentSizes[1]) * animationProgress;

    ui->splitter_data->setSizes(intermediateSizes);
}

void serialWindow::saveDataWidgetToFile() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save file"), "",
                                                    "Text file (*.txt);;HTML file(*.html);;Markdown file(*.md);;All file(*)");
    if (fileName.isEmpty()) {
        return;
    }
    // 确保文件名可以 .txt 结尾（可选）
    if (!fileName.endsWith(".txt", Qt::CaseInsensitive)) {
        fileName += ".txt";
    } else if (fileName.endsWith(".html", Qt::CaseInsensitive)) {
        fileName += ".html";
    } else if (fileName.endsWith(".md", Qt::CaseInsensitive)) {
        fileName += ".md";
    }

    // 创建文件并写入文本
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        ElaMessageBar::error(ElaMessageBarType::TopLeft, tr("error"), tr("can not save ~~").arg(fileName), 3000,
                             this);
        return;
    }

    QTextStream out(&file);
    out << ui->plainTextEdit_out->toPlainText(); // 写入文本
    file.close();

    ElaMessageBar::information(ElaMessageBarType::TopLeft, tr("info"), tr("save successfully").arg(fileName), 3000,
                               this);
}

void serialWindow::openOrCloseAutoMode(bool check) {
    if (check) {
        bool rs;
        double sec = ui->lineEdit_auto_mode->text().toDouble(&rs);
        if (not rs) {
            ElaMessageBar::error(ElaMessageBarType::TopLeft, tr("error"), tr("The input cycle is invalid"), 3000,
                                 this);
            ui->toggleswitch_auto_mode->setIsToggled(false);
            return;
        }
        autoSendTimer_->start(static_cast<int>(sec * 1000));
    } else {
        autoSendTimer_->stop();
    }
}

void serialWindow::autoModeCycleChange() {
    if (not autoSendTimer_->isActive()) {
        return;
    }
    bool rs;
    double sec = ui->lineEdit_auto_mode->text().toDouble(&rs);
    if (not rs) {
        ElaMessageBar::error(ElaMessageBarType::TopLeft, tr("error"), tr("The input cycle is invalid"), 3000, this);
        ui->toggleswitch_auto_mode->setIsToggled(false);
        return;
    }
    autoSendTimer_->stop();
    autoSendTimer_->start(static_cast<int>(sec * 1000));
}

void serialWindow::enableRemote(bool check) {
    if (check) {
        // 打开远程调试
        isRemote = true;
        if (serial_port_.isOpen()) {
            // 假如本地的串口已经打开了,要断开连接
            serial_port_.close();
            ui->toggleswitch_open->setIsToggled(false);
        }
        ui->lineEdit_url->setEnabled(true);
        iconBtn_url_refresh_->setEnabled(true);
        iconBtn_url_refresh_->show();
        ui->lineEdit_url->show();
    } else {
        // 关闭远程调试
        isRemote = false;
        if (tcp_socket_->state() == QAbstractSocket::ConnectedState) {
            tcp_socket_->disconnectFromHost();
            ui->toggleswitch_open->setIsToggled(false);
        }
        ui->lineEdit_url->setEnabled(false);
        iconBtn_url_refresh_->setEnabled(false);
        iconBtn_url_refresh_->hide();
        ui->lineEdit_url->hide();
    }
}

void serialWindow::refreshSerialPort() {
    // 获取可用串口list
    std::vector<SerialPortInfo> portNameList = CSerialPortInfo::availablePortInfos();
    ui->comboBox_port->clear();
    for (SerialPortInfo portName: portNameList) {
        ui->comboBox_port->addItem(QString::fromLocal8Bit(portName.portName));
    }
}

void serialWindow::convertDataAndSend(const QByteArray &data) {
    if (show_type_ == dataType::HEX) {
        // 十六进制显示
        emit sigDataCompleted(data.toHex(' ').toUpper());
    } else if (show_type_ == dataType::TEXT) {
        // 文本显示
        emit sigDataCompleted(data);
    } else if (dataType::HTML == show_type_) {
        // 优化后的HTML处理
        static QString buffer;
        buffer.append(data);

        // 查找完整的HTML标签
        QRegularExpression htmlTag(R"(<[^>]*>)"); // 简单匹配HTML标签
        int lastPos = 0;
        bool hasCompleteHtml = false;

        // 检查是否有完整的HTML标签
        QRegularExpressionMatchIterator it = htmlTag.globalMatch(buffer);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            lastPos = match.capturedEnd();
            hasCompleteHtml = true;
        }

        // 如果有完整的HTML内容，发送并清空缓冲区
        if (hasCompleteHtml && lastPos > 0) {
            QString completeHtml = buffer.left(lastPos);
            emit sigDataCompleted(completeHtml.toUtf8());
            buffer = buffer.mid(lastPos);
        }
    } else if (dataType::MARKDOWN == show_type_) {
        // 优化后的Markdown处理
        static QString buffer;
        buffer.append(data);

        // 尝试在换行处分割，确保完整段落
        int lastNewline = buffer.lastIndexOf('\n');
        if (lastNewline != -1) {
            QString completeContent = buffer.left(lastNewline + 1);
            buffer = buffer.mid(lastNewline + 1);

            // 转换为HTML
            std::stringstream markdownStream;
            markdownStream << completeContent.toStdString();
            std::string html = parser_->Parse(markdownStream);

            if (!html.empty()) {
                emit sigDataCompleted(QByteArray::fromStdString(html));
            }
        }
    }
}

void serialWindow::sendDataToSerial(const QByteArray &data) {
    // 本地串口 和 远程连接 都没打开
    if (not serial_port_.isOpen() and tcp_socket_->state() == QAbstractSocket::UnconnectedState) {
        ElaMessageBar::error(ElaMessageBarType::TopLeft, tr("Error"), tr("Please open serial port first"), 3000, this);
        return;
    }

    QByteArray sendData;
    if (send_type_ == dataType::HEX) {
        // 十六进制模式：转换输入为二进制数据
        if (convertToHex(data, sendData) != 0) {
            ElaMessageBar::error(ElaMessageBarType::TopLeft, tr("Error"), tr("Invalid HEX format"), 3000, this);
            return;
        }
    } else {
        // 文本模式：直接发送原始数据
        sendData = data;
    }

    if (isRemote) {
        nlohmann::json json;
        json["type"] = "serialData";
        json["data"] = sendData.toStdString();
        tcp_socket_->write(QByteArray::fromStdString(json.dump()));
    } else {
        // 发送二进制数据
        serial_port_.writeData(sendData.constData(), sendData.size());
    }
}

void serialWindow::connectToHost() {
    if (tcp_socket_->state() == QAbstractSocket::ConnectedState) {
        tcp_socket_->disconnectFromHost();
        return;
    }
    QString addressPort = ui->lineEdit_url->text().trimmed();
    QStringList parts = addressPort.split(":");
    if (parts.size() != 2) {
        ElaMessageBar::error(ElaMessageBarType::TopLeft, tr("error"),
                             tr("Please enter the correct IP: port format"), 3000, this);
        return;
    }

    bool ok;
    quint16 port = parts[1].toUShort(&ok);
    if (!ok) {
        ElaMessageBar::warning(ElaMessageBarType::TopLeft, tr("error"),
                               tr("Invalid port"), 3000, this);
        return;
    }
    tcp_socket_->connectToHost(parts[0], port);
}

void serialWindow::readTcpData() {
    QByteArray data = tcp_socket_->readAll();

    nlohmann::json json;
    try {
        json = nlohmann::json::parse(data.toStdString());
    } catch (const nlohmann::json::parse_error &e) {
        // show binary
        data_queue_.enqueue(data);
        return;
    }

    std::string type = json.at("type").get<std::string>();
    if ("serialList" == type) {
        // 可用串口列表
        auto list = json.at("data").get<std::vector<std::string> >();
        if (list.empty()) {
            return;
        }
        ui->comboBox_port->clear();
        for (const std::string &port: list) {
            ui->comboBox_port->addItem(QString::fromStdString(port));
        }
    } else if ("serialData" == type) {
        // 接收 串口的文本数据
        std::string showData = json.at("data").get<std::string>();
        data_queue_.enqueue(std::move(QByteArray::fromStdString(showData)));
    }
}

void serialWindow::hideSecondaryWindow() {
    ui->splitter_main->setSizes({1000, 0});
}

void serialWindow::showSecondaryWindow() {
    ui->splitter_main->setSizes({1000, 100});
}
