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

// 辅助函数：去除字符串前缀（如0x或0b）
static std::string removePrefix(const std::string &s) {
    if (s.size() >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X' || s[1] == 'b' || s[1] == 'B')) {
        return s.substr(2);
    }
    return s;
}

// 二进制转十六进制
static int binaryToHex(const std::string &binaryStr, std::string &hexStr) {
    std::string bin = removePrefix(binaryStr);
    std::string hex;

    // 二进制到十六进制映射表
    std::unordered_map<std::string, char> binToHex = {
        {"0000", '0'}, {"0001", '1'}, {"0010", '2'}, {"0011", '3'},
        {"0100", '4'}, {"0101", '5'}, {"0110", '6'}, {"0111", '7'},
        {"1000", '8'}, {"1001", '9'}, {"1010", 'A'}, {"1011", 'B'},
        {"1100", 'C'}, {"1101", 'D'}, {"1110", 'E'}, {"1111", 'F'}
    };

    // 补齐长度为4的倍数
    int padding = (4 - bin.length() % 4) % 4;
    bin = std::string(padding, '0') + bin;

    // 每4位二进制转换为1位十六进制
    for (size_t i = 0; i < bin.length(); i += 4) {
        std::string nibble = bin.substr(i, 4);
        if (binToHex.find(nibble) == binToHex.end()) {
            // throw std::invalid_argument("Invalid binary string");
            return -1;
        }
        hex += binToHex[nibble];
    }

    // 去除前导0
    hex.erase(0, hex.find_first_not_of('0'));
    if (hex.empty()) hex = "0";

    hexStr = "0x" + hex;
    return 0;
}

// 十六进制转二进制
static int hexToBinary(const std::string &hexStr, std::string &binaryStr) {
    std::string hex = removePrefix(hexStr);
    std::string binary;

    // 十六进制到二进制映射表
    std::unordered_map<char, std::string> hexToBin = {
        {'0', "0000"}, {'1', "0001"}, {'2', "0010"}, {'3', "0011"},
        {'4', "0100"}, {'5', "0101"}, {'6', "0110"}, {'7', "0111"},
        {'8', "1000"}, {'9', "1001"}, {'A', "1010"}, {'B', "1011"},
        {'C', "1100"}, {'D', "1101"}, {'E', "1110"}, {'F', "1111"},
        {'a', "1010"}, {'b', "1011"}, {'c', "1100"}, {'d', "1101"},
        {'e', "1110"}, {'f', "1111"}
    };

    for (char c: hex) {
        if (hexToBin.find(c) == hexToBin.end()) {
            // throw std::invalid_argument("Invalid hexadecimal string");
            return -1;
        }
        binary += hexToBin[c];
    }

    // 去除前导0
    binary.erase(0, binary.find_first_not_of('0'));
    if (binary.empty()) binary = "0";

    binaryStr = "0b" + binary;
    return 0;
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

serialWindow::serialWindow(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::serialWindow),
      threadPool_(1) {
    ui->setupUi(this);

    threadPool_.enqueue([this]() {
        QString data;
        while (not isStop_) {
            bool rs = data_queue_.try_dequeue(data);
            if (false == rs) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            convertDataAndSend(std::move(data));
        }
    });

    // 只读 不可编辑
    ui->plainTextEdit_out->setReadOnly(true);
    // TODO: Ela 这个似乎有bug, 要把ObjectName的名称改回这个"ElaPlainTextEdit"才能正常使用深色模式
    ui->plainTextEdit_out->setObjectName("ElaPlainTextEdit");
    ui->plainTextEdit_in->setObjectName("ElaPlainTextEdit");

    ui->toggleswitch_open->setText("open");
    // 开始按钮的默认大小
    ui->toggleswitch_open->setMinimumWidth(191);
    // 刷新串口按钮 在 ui->comboBox_port 右侧
    iconBtn_refresh_ = new ElaIconButton{ElaIconType::ArrowRotateLeft, 17, 20, 20};
    ui->horizontalLayout_0->addWidget(iconBtn_refresh_);

    // 默认 MAN 手动模式
    ui->cmdwidget->hide();
    ui->plainTextEdit_in->show();
    ui->pushButton_send->show();

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

    // 默认状态下 输出窗口比例70%，输入窗口占30%
    ui->splitter_data->setSizes(dataWidgetRatio_[sendMode::MAN]);

    // 初始化 combobox 组件
    initComboBox();
    // 初始化 text 组件
    initText();
    // init signals 和 slots
    initSignalSlots();

    // init markdown to html converter
    config_ = std::make_shared<maddy::ParserConfig>();
    config_->enabledParsers &= ~maddy::types::EMPHASIZED_PARSER; // disable emphasized parser
    config_->enabledParsers |= maddy::types::HTML_PARSER; // do not wrap HTML in paragraph
    parser_ = std::make_unique<maddy::Parser>(config_);
}

serialWindow::~serialWindow() {
    isStop_ = true;
    serial_port_.close();
    delete ui;
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
                show_type_ = dataTypeFrom(type);
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
        QString data = ui->plainTextEdit_in->toPlainText();
        emit sigSendData(data);
    });

    // 清空 comboBox_port items
    connect(this, &serialWindow::sigClearComboBoxPort, this, [this]() {
        ui->comboBox_port->clear();
    });

    // 添加 comboBox_port items
    connect(this, &serialWindow::sigAddSerialPort, this, [this](const QString &item) {
        ui->comboBox_port->addItem(item);
    });

    // 处理完数据，插入到 plainTextEdit_out
    connect(this, &serialWindow::sigDataCompleted, this, [this](const QString &data) {
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
    });

    // 显示类型切换
    connect(ui->comboBox_show_type, &ElaComboBox::currentTextChanged, this, [this](const QString &item) {
        show_type_ = dataTypeFrom(item.toStdString());
    });

    // 发送模式切换
    connect(ui->comboBox_send_mode, &ElaComboBox::currentTextChanged, this, [this](const QString &item) {
        send_mode_ = sendModeFrom(item.toStdString());
        if (sendMode::MAN == send_mode_) {
            ui->cmdwidget->hide();
            ui->plainTextEdit_in->show();
            ui->pushButton_send->show();

            // 重置动画进度
            animationProgress = 0;
            animationTimer->start();

            // ui->splitter_data->setSizes(dataWidgetRatio_["MAN"]);
        } else if (sendMode::CMD == send_mode_) {
            ui->cmdwidget->show();
            ui->plainTextEdit_in->hide();
            ui->pushButton_send->hide();

            // 重置动画进度
            animationProgress = 0;
            animationTimer->start();

            // ui->splitter_data->setSizes(dataWidgetRatio_["CMD"]);
        }
    });

    connect(animationTimer, &QTimer::timeout, this, [this]() {
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
    });

    connect(ui->cmdwidget, &cmdWidget::sigCmdSend, this, &serialWindow::sigSendData);

    connect(this, &serialWindow::sigSendData, this, &serialWindow::sendDataToSerial);

    // 接受serial onReadEvent初始化
    serial_port_.connectReadEvent(this);

    // 刷新串口状态
    connect(iconBtn_refresh_, &ElaIconButton::clicked, this, &serialWindow::refreshSerialPort);

    // 保存接收数据到文件
    connect(ui->pushButton_save_file, &ElaPushButton::clicked, this, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save file"), "",
                                                        tr(
                                                            "Text file (*.txt);;HTML file(*.html);;Markdown file(*.md);;All file(*)"));
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
            ElaMessageBar::error(ElaMessageBarType::TopLeft, "error", tr("can not save ~~").arg(fileName), 3000,
                                 this);
            return;
        }

        QTextStream out(&file);
        out << ui->plainTextEdit_out->toPlainText(); // 写入文本
        file.close();

        ElaMessageBar::information(ElaMessageBarType::TopLeft, "info", tr("save successfully").arg(fileName), 3000,
                                   this);
    });

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
}


void serialWindow::onReadEvent(const char *portName, unsigned int readBufferLen) {
    if (readBufferLen > 0) {
        int recLen = 0;
        char *str = nullptr;
        str = new char[readBufferLen];
        recLen = serial_port_.readData(str, readBufferLen);

        if (recLen > 0) {
            //* 中文需要由两个字符拼接，否则显示为空""
            QString m_str = QString::fromLocal8Bit(str, recLen);
            // data_queue_.enqueue(std::move(m_str));
            data_queue_.try_enqueue(std::move(m_str));
        } else {
        }

        if (str) {
            delete[] str;
            str = nullptr;
        }
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

void serialWindow::convertDataAndSend(const QString &data) {
    if (dataType::TEXT == show_type_) {
        // 文本
        // ui->plainTextEdit_out->moveCursor(QTextCursor::End);
        // ui->plainTextEdit_out->insertPlainText(data);
        emit sigDataCompleted(data);
    } else if (dataType::HEX == show_type_) {
        emit sigDataCompleted(data.toLocal8Bit().toHex() + " "); // 添加空格分隔符提高可读性
    } else if (dataType::HTML == show_type_) {
        // HTML
        static QString lastLine;
        QStringList lines = data.split("\n");
        lines[0] = lastLine + lines[0];
        for (int i = 0; i < lines.size(); ++i) {
            if (i != lines.size() - 1) {
                // ui->plainTextEdit_out->appendHtml(lines[i]);
                emit sigDataCompleted(lines[i]);
            }
        }
        lastLine = lines.back();
        if (isHtml(lastLine)) {
            // ui->plainTextEdit_out->appendHtml(lastLine);
            emit sigDataCompleted(lastLine);
            lastLine = "";
        }
    } else if (dataType::MARKDOWN == show_type_) {
        // markdown
        static QString lastLine;
        std::stringstream strSt;
        QStringList lines = data.split("\n");
        lines[0] = lastLine + lines[0];
        for (int i = 0; i < lines.size(); ++i) {
            if (i != lines.size() - 1) {
                strSt << lines[i].toStdString();
            }
        }
        std::string html = parser_->Parse(strSt);
        // ui->plainTextEdit_out->appendHtml(QString::fromStdString(html));
        emit sigDataCompleted(QString::fromStdString(html));
        lastLine = lines.back();
    }
}

void serialWindow::sendDataToSerial(QString data) {
    if (serial_port_.isOpen()) {
        // 串口正常打开
        QString sendStr = ui->plainTextEdit_in->toPlainText();
        // 16进制发送
        if (send_type_ == dataType::HEX) {
            // 按照 '\n' '\r' ' ' 来切割
            QStringList strList = highPerformanceSplit(sendStr);
            std::string tmp;
            tmp.reserve(sendStr.size());
            for (const QString &str: strList) {
                tmp += removePrefix(str.toStdString());
            }
            std::string bin;
            int rs = hexToBinary(tmp, bin);
            if (-1 == rs) {
                ElaMessageBar::error(ElaMessageBarType::TopLeft, "error", "invalid hex data", 4000, this);
                return;
            }
            sendStr = QString::fromStdString(bin);
        }
        // 正常发送 文本发送
        QByteArray ba = sendStr.toLocal8Bit();
        const char *s = ba.constData();
        // 支持中文并获取正确的长度
        serial_port_.writeData(s, ba.length());
    } else {
        // 串口没有打开
        // QMessageBox::information(NULL,tr("information"),tr("please open serial port first"));
        ElaMessageBar::error(ElaMessageBarType::TopLeft, "error", tr("please open serial port first"), 3000, this);
    }
}

void serialWindow::hideSecondaryWindow() {
    ui->groupBox_port->hide();
    ui->groupBox_recv->hide();
    ui->groupBox_send->hide();
}

void serialWindow::showSecondaryWindow() {
    ui->groupBox_port->show();
    ui->groupBox_recv->show();
    ui->groupBox_send->show();
}
