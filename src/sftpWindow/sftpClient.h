//
// Created by wang on 25-4-24.
//

#ifndef SFTPCLIENT_H
#define SFTPCLIENT_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTcpSocket>

#include <libssh2.h>
#include <libssh2_sftp.h>
#include <QDir>

class sftpClient : public QObject {
    Q_OBJECT

public:
    struct fileInfo {
        QString filename;
        QString permissions;
        QString user;
        QString userGroups;
        QString fileSize;
        QString dateModified;
    };

    enum class listArg { none = -1, all, list };

    enum class fileType { none = -1, file, link, dir };

public:
    explicit sftpClient(QObject *parent = nullptr);

    ~sftpClient() override;

    // 连接
    bool connectToHost(const QString &host, quint16 port = 22,
                       const QString &username = {}, const QString &password = {},
                       QString pubKeyPath = {},
                       QString priKeyPath = QDir::homePath() + "/.ssh/id_rsa"
    );

    // 断开连接
    void disconnectFromHost();

    // 上传文件
    bool put(const QString &localPath, const QString &remotePath);

    // 下载文件
    bool get(const QString &remotePath, const QString &localPath);

    // 创建目录
    bool mkdir(const QString &path);

    // 删除目录
    bool rmdir(const QString &path);

    // 删除文件
    bool rm(const QString &path);

    // 自动识别类型 删除文件或目录
    bool rmAuto(const QString &path);

    // 列出目录内容
    QList<fileInfo> ls(const QString &path, const QList<listArg> &args = {});

    // 重命名目录或文件
    bool rename(const QString &oldName, const QString &newName);

    // 复制文件
    bool cp(const QString &src, const QString &dst);

    // 错误处理
    QString lastError() const;

    // 获取当前用户的home目录
    QString home();

    // 查看该目录是否存在 是否有效
    void exist(const QString &path, fileType &type);

private:
    QString userNameFromUid(uint uid);

    QString groupNameFromGid(uint gid);

signals:
    // 连接状态信号
    void connected();

    void disconnected();

    // 传输进度信号 (需要手动在实现中触发)
    void transferProgress(qint64 bytesSent, qint64 bytesTotal);

    // 错误信号
    void errorOccurred(const QString &errorString);

private:
    // libssh2资源
    LIBSSH2_SESSION *m_session = nullptr;
    LIBSSH2_SFTP *m_sftpSession = nullptr;

    // 网络连接
    QTcpSocket *m_tcpSocket = nullptr;

    // 状态标志
    std::atomic_bool m_connected = false;

    // 错误信息存储
    QString m_lastError;

    static QAtomicInteger<int> instanceCount;
};


#endif //SFTPCLIENT_H
