// tcpServer.h
#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QTcpServer>
#include <QDateTime>
#include <QTcpSocket>

class tcpServer : public QTcpServer {
    Q_OBJECT

public:
    explicit tcpServer(QObject *parent = nullptr);

    bool listen(const QHostAddress &address = QHostAddress::Any, quint16 port = 0);

    void close();

    QTcpSocket *nextPendingConnection() override;

    static QString formatIPAddress(const QString &address);

Q_SIGNALS:
    Q_SIGNAL void sigLog(QString text);
};

inline tcpServer::tcpServer(QObject *parent): QTcpServer(parent) {
}

inline bool tcpServer::listen(const QHostAddress &address, quint16 port) {
    QString log;
    bool rs = QTcpServer::listen(address, port);
    if (rs) {
        log = QString{"[%0] %1:%2 listening"}
                .arg(QDateTime::currentDateTime().toString())
                .arg(address.toString()).arg(port);
    } else {
        log = QString{"[%0] %1:%2 listen failed"}
                .arg(QDateTime::currentDateTime().toString())
                .arg(address.toString()).arg(port);
    }
    emit sigLog(log);
    return rs;
}

inline void tcpServer::close() {
    emit sigLog(QString{"[%0] %1:%2 closed"}
        .arg(QDateTime::currentDateTime().toString())
        .arg(serverAddress().toString())
        .arg(serverPort()));
    QTcpServer::close();
}

inline QTcpSocket *tcpServer::nextPendingConnection() {
    QTcpSocket *socket = QTcpServer::nextPendingConnection();
    if (!socket || !socket->isValid()) {
        if (socket) socket->deleteLater();
        return nullptr;
    }

    emit sigLog(QString{"[%0] %1:%2 connect"}
        .arg(QDateTime::currentDateTime().toString())
        .arg(formatIPAddress(socket->peerAddress().toString()))
        .arg(socket->peerPort())
    );
    return socket;
}

inline QString tcpServer::formatIPAddress(const QString &address) {
    QHostAddress addr(address);

    // 如果是 IPv4-mapped IPv6 地址（如 ::ffff:192.168.1.1），转为纯 IPv4
    if (addr.protocol() == QAbstractSocket::IPv6Protocol && addr.toIPv4Address()) {
        return QHostAddress(addr.toIPv4Address()).toString(); // 转为 IPv4
    }

    // 否则，保持原格式（IPv6 或 IPv4）
    return addr.toString();
}


#endif //TCPSERVER_H
