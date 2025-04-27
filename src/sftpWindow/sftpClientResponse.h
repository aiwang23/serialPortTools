//
// Created by wang on 25-4-27.
//

#ifndef SFTPCLIENTRESPONSE_H
#define SFTPCLIENTRESPONSE_H

#include <QObject>

#include "sftpClient.h"

class sftpClientResponse : public QObject {
    Q_OBJECT

public:
    sftpClientResponse() = default;

    ~sftpClientResponse() = default;

Q_SIGNALS:
    Q_SIGNAL void connectToHostResponse(bool rs);

    Q_SIGNAL void homeResponse(QString path);

    Q_SIGNAL void lsResponse(QList<sftpClient::fileInfo>);

    Q_SIGNAL void existResponse(sftpClient::fileType file_t);

    Q_SIGNAL void getResponse(bool rs);

    Q_SIGNAL void putResponse(bool rs);

    Q_SIGNAL void getTransferProgress(qint64 bytesSent, qint64 bytesTotal);

    Q_SIGNAL void putTransferProgress(qint64 bytesSent, qint64 bytesTotal);

};


#endif //SFTPCLIENTRESPONSE_H
