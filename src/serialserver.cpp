// serialServer.cpp

#include "serialserver.h"

#include <ElaMessageBar.h>
#include <iostream>
#include <CSerialPort/SerialPortInfo.h>

#include "ui_serialServer.h"
#include "tcpServer.h"
#include "nlohmann/json.hpp"

// {"none", "even", "odd"}
static itas109::Parity SerialParityFrom(const std::string &chars) {
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
static itas109::StopBits SerialStopBitsFrom(const std::string &chars) {
    if (chars == "1")
        return itas109::StopOne;
    else if (chars == "1.5")
        return itas109::StopOneAndHalf;
    else if (chars == "2")
        return itas109::StopTwo;
}

serialServer::serialServer(QWidget *parent) : QWidget(parent), ui(new Ui::serialServer) {
    ui->setupUi(this);

    tcp_server_ = new tcpServer{this};

    ui->plainTextEdit_log->setObjectName("ElaPlainTextEdit");
    ui->toggleswitch_net_open->setText(tr("open"));

    initText();
    initLineEdit();
    initSignalSlots();
}

serialServer::~serialServer() {
    if (tcp_server_ and tcp_server_->isListening()) {
        tcp_server_->close();
    }
    if (serial_port_.isOpen()) {
        serial_port_.close();
    }
    delete ui;
    ui = nullptr;
}

void serialServer::initText() {
    ui->label_port->setTextPixelSize(13);
    ui->label_baud->setTextPixelSize(13);
    ui->label_databit->setTextPixelSize(13);
    ui->label_parity->setTextPixelSize(13);
    ui->label_stopbit->setTextPixelSize(13);
    ui->label_net_port->setTextPixelSize(13);
}

void serialServer::initLineEdit() {
    ui->lineEdit_serial_port->setEnabled(false);
    ui->lineEdit_baud_rate->setEnabled(false);
    ui->lineEdit_data_bit->setEnabled(false);
    ui->lineEdit_parity->setEnabled(false);
    ui->lineEdit_stop_bit->setEnabled(false);
}

void serialServer::initSignalSlots() {
    // 打开/关闭 监听
    connect(ui->toggleswitch_net_open, &ElaToggleButton::toggled, this, [this](bool check) {
        if (check) {
            // 打开
            bool ok;
            quint16 port = ui->lineEdit_net_port->text().toUShort(&ok);
            if (!ok || port < 1 || port > 65535) {
                ElaMessageBar::error(ElaMessageBarType::TopLeft, tr("error"), tr("Invalid port"), 3000, this);
                ui->toggleswitch_net_open->setIsToggled(false);
                return;
            }

            if (tcp_server_->listen(QHostAddress::Any, port)) {
                ui->toggleswitch_net_open->setText(tr("close"));
            } else {
                ui->toggleswitch_net_open->setIsToggled(false);
            }
        } else {
            // 关闭
            if (tcp_server_->isListening()) {
                tcp_server_->close();
                if (tcp_socket_) {
                    tcp_socket_->disconnectFromHost();
                    tcp_socket_->deleteLater();
                    tcp_socket_ = nullptr;
                }
                ui->toggleswitch_net_open->setText(tr("open"));
            }
        }
    });

    // 新连接
    connect(tcp_server_, &QTcpServer::newConnection, this, [this]() {
        // 解析所有客户连接
        while (tcp_server_->hasPendingConnections()) {
            // 不重复连接
            if (isConnected) {
                continue;
            }
            // 连接上后通过socket（QTcpSocket对象）获取连接信息
            tcp_socket_ = tcp_server_->nextPendingConnection();
            isConnected = true;
            // 接受数据
            connect(tcp_socket_, &QTcpSocket::readyRead, this, &serialServer::readNetData);
            // 断开连接
            connect(tcp_socket_, &QTcpSocket::disconnected, this, [this]() {
                if (not ui) {
                    return;
                }
                ui->plainTextEdit_log->appendPlainText(QString{"[%0] %1:%2 %3"}
                    .arg(QDateTime::currentDateTime().toString())
                    .arg(tcpServer::formatIPAddress(tcp_socket_->peerAddress().toString()))
                    .arg(tcp_socket_->peerPort())
                    .arg("disconnected"));
                ui->lineEdit_serial_port->setText("");
                ui->lineEdit_baud_rate->setText("");
                ui->lineEdit_data_bit->setText("");
                ui->lineEdit_parity->setText("");
                ui->lineEdit_stop_bit->setText("");
                if (serial_port_.isOpen())
                    serial_port_.close();
                isConnected = false;
            });

            std::vector<itas109::SerialPortInfo> list = itas109::CSerialPortInfo::availablePortInfos();
            std::vector<std::string> portList;
            for (const itas109::SerialPortInfo port: list) {
                portList.push_back(port.portName);
            }
            // 把 serial list 发送到 client
            writeSerialList(portList);
        }
    });

    // log
    connect(tcp_server_, &tcpServer::sigLog, this, [this](const QString &log) {
        ui->plainTextEdit_log->appendPlainText(log);
    });

    // 接受serial onReadEvent初始化
    serial_port_.connectReadEvent(this);
}

void serialServer::onReadEvent(const char *portName, unsigned int readBufferLen) {
    if (readBufferLen > 0 && tcp_socket_ && tcp_socket_->state() == QAbstractSocket::ConnectedState) {
        QByteArray rawData(readBufferLen, 0);
        int recLen = serial_port_.readData(rawData.data(), readBufferLen);

        if (recLen > 0) {
            rawData.resize(recLen);
            qint64 written = tcp_socket_->write(rawData);
            if (written == -1) {
                qDebug() << "Write error:" << tcp_socket_->errorString();
            } else if (!tcp_socket_->waitForBytesWritten(1000)) {
                qDebug() << "Write timeout";
            }
        }
    }
}

int64_t serialServer::writeSerialList(const std::vector<std::string> &list) {
    nlohmann::json json;
    json["type"] = "serialList";
    json["data"] = list;

    QByteArray text = QByteArray::fromStdString(json.dump());
    return tcp_socket_->write(text);
}

serialSettings serialServer::toSerialSettings(nlohmann::json json) {
    return {
        json.at("port").get<std::string>(),
        json.at("baud").get<std::string>(),
        json.at("dataBits").get<std::string>(),
        json.at("parity").get<std::string>(),
        json.at("stopBit").get<std::string>()
    };
}

int64_t serialServer::writeError(const std::string &string) {
    nlohmann::json json;
    json["type"] = "error";
    json["data"] = string;

    QByteArray text = QByteArray::fromStdString(json.dump());
    return tcp_socket_->write(text);
}

void serialServer::readNetData() {
    QByteArray text = tcp_socket_->readAll();

    nlohmann::json json;
    try {
        json = nlohmann::json::parse(text.toStdString());
    } catch (const nlohmann::json::parse_error &e) {
        if (serial_port_.isOpen()) {
            serial_port_.writeData(text.constData(), text.size());
        }
        return;
    }

    std::string type = json.at("type").get<std::string>();

    if ("serialSettings" == type) {
        // 请求打开
        if (serial_port_.isOpen()) {
            return;
        }

        serialSettings settings = toSerialSettings(json.at("data"));
        // 填入args
        serial_port_.init(
            settings.port.c_str(),
            std::stoi(settings.baud),
            SerialParityFrom(settings.parity),
            itas109::DataBits(std::stoi(settings.dataBits)),
            SerialStopBitsFrom(settings.stopBit)
        );
        serial_port_.setReadIntervalTimeout(0);

        // 打开 serial
        if (not serial_port_.open()) {
            QString err = QString{"[%0] %1 %2"}
                    .arg(QDateTime::currentDateTime().toString())
                    .arg(serial_port_.getLastError())
                    .arg(serial_port_.getLastErrorMsg());
            writeError(err.toStdString());
            ui->plainTextEdit_log->appendPlainText(err);
            return;
        }

        ui->plainTextEdit_log->appendPlainText(
            QString{"[%0] %1 %2"}.arg(QDateTime::currentDateTime().toString())
            .arg(settings.port.c_str())
            .arg("opening")
        );
        ui->lineEdit_serial_port->setText(QString::fromStdString(settings.port));
        ui->lineEdit_baud_rate->setText(QString::fromStdString(settings.baud));
        ui->lineEdit_data_bit->setText(QString::fromStdString(settings.dataBits));
        ui->lineEdit_parity->setText(QString::fromStdString(settings.parity));
        ui->lineEdit_stop_bit->setText(QString::fromStdString(settings.stopBit));
    } else if ("serialData" == type) {
        // 发给串口的文本数据
        if (not serial_port_.isOpen()) {
            QString err = QString{"[%0] %1 %2"}
                    .arg(QDateTime::currentDateTime().toString())
                    .arg("serial not open");
            ui->plainTextEdit_log->appendPlainText(err);
            writeError(err.toStdString());
            return;
        }
        std::string data = json.at("data").get<std::string>();
        serial_port_.writeData(data.c_str(), data.size());
    } else if ("serialClose" == type) {
        // 串口关闭
        if (not serial_port_.isOpen()) {
            return;
        }
        ui->lineEdit_serial_port->setText("");
        ui->lineEdit_baud_rate->setText("");
        ui->lineEdit_data_bit->setText("");
        ui->lineEdit_parity->setText("");
        ui->lineEdit_stop_bit->setText("");
        serial_port_.close();
    }
}
