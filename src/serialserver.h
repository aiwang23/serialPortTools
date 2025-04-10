// serialServer.h

#ifndef SERIALSERVER_H
#define SERIALSERVER_H

#include <QWidget>
#include <CSerialPort/SerialPort.h>
#include "nlohmann/json.hpp"

class QTcpSocket;
class tcpServer;
using itas109::CSerialPortListener;
using itas109::CSerialPort;

struct serialSettings {
    std::string port;
    std::string baud;
    std::string dataBits;
    std::string parity;
    std::string stopBit;
};

QT_BEGIN_NAMESPACE

namespace Ui {
    class serialServer;
}

QT_END_NAMESPACE

class serialServer : public QWidget, public CSerialPortListener {
    Q_OBJECT

public:
    explicit serialServer(QWidget *parent = nullptr);

    ~serialServer() override;

    void initText();

    void initLineEdit();

    void initSignalSlots();

    void onReadEvent(const char *portName, unsigned int readBufferLen) override;

private:
    int64_t writeSerialList(const std::vector<std::string> &list);

    static serialSettings toSerialSettings(nlohmann::json json);

private Q_SLOTS:
    Q_SLOT int64_t writeError(const std::string &string);

    void readNetData();

private:
    Ui::serialServer *ui;
    tcpServer *tcp_server_ = nullptr;
    QTcpSocket *tcp_socket_ = nullptr;
    std::atomic_bool isConnected = false;

    CSerialPort serial_port_;
};


#endif //SERIALSERVER_H
