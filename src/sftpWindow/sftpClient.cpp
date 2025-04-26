//
// Created by wang on 25-4-24.
//

#include "sftpClient.h"

#include <QDateTime>
#include <QDir>
#include <QFile>

QAtomicInteger<int> sftpClient::instanceCount = 0;

sftpClient::sftpClient(QObject *parent) {
    // 初始化libssh2库（仅首次实例化时执行）
    if (instanceCount.fetchAndAddAcquire(1) == 0) {
        libssh2_init(0);
    }

    // 创建TCP socket对象
    m_tcpSocket = new QTcpSocket(this);

    connect(m_tcpSocket, &QTcpSocket::disconnected, [this]() {
        m_connected = false;
        emit disconnected();
    });
}

sftpClient::~sftpClient() {
    // 清理SFTP连接
    disconnectFromHost();

    // 清理libssh2库（最后一个实例时执行）
    if (instanceCount.fetchAndSubRelease(1) == 1) {
        libssh2_exit();
    }
}

bool sftpClient::connectToHost(const QString &host, quint16 port, const QString &username, const QString &password,
                               QString pubKeyPath, QString priKeyPath) {
    // 清理已有连接
    disconnectFromHost();

    // 建立TCP连接
    m_tcpSocket->connectToHost(host, port);
    if (!m_tcpSocket->waitForConnected(5000)) {
        // 5秒超时
        m_lastError = tr("Connection timeout: ") + m_tcpSocket->errorString();
        emit errorOccurred(m_lastError);
        return false;
    }

    // 创建SSH会话
    m_session = libssh2_session_init();
    if (!m_session) {
        m_lastError = tr("Failed to create SSH session");
        emit errorOccurred(m_lastError);
        return false;
    }

    // 关联socket与libssh2
    libssh2_session_set_blocking(m_session, 1); // 设置为阻塞模式
    int rc = libssh2_session_handshake(m_session, (intptr_t) m_tcpSocket->socketDescriptor());
    if (rc != 0) {
        m_lastError = tr("SSH handshake failed: ") + QString::number(rc);
        emit errorOccurred(m_lastError);
        return false;
    }

    // 验证服务器指纹
    const char *fingerprint = libssh2_hostkey_hash(m_session, LIBSSH2_HOSTKEY_HASH_SHA1);
    qDebug() << "Server fingerprint:" << QByteArray(fingerprint, 20).toHex();

    // 用户认证 - 先尝试密钥认证，失败后使用密码
    // 密钥路径智能处理
    if (priKeyPath.isEmpty()) {
        priKeyPath = QDir::homePath() + "/.ssh/id_rsa";
    }
    if (pubKeyPath.isEmpty() && QFile::exists(priKeyPath + ".pub")) {
        pubKeyPath = priKeyPath + ".pub";
    }
    bool authSuccess = false;
    if (QFile::exists(priKeyPath)) {
        // 尝试使用默认私钥认证
        rc = libssh2_userauth_publickey_fromfile(
            m_session,
            username.toUtf8().constData(),
            pubKeyPath.isEmpty() ? nullptr : pubKeyPath.toUtf8().constData(),
            priKeyPath.toUtf8().constData(),
            password.toUtf8().isEmpty() ? nullptr : password.toUtf8().constData()
        );
        authSuccess = (rc == 0);

        // 添加密钥认证错误详情
        if (!authSuccess) {
            char *errmsg;
            libssh2_session_last_error(m_session, &errmsg, nullptr, 0);
            qDebug() << "Public key auth failed:" << errmsg;
        }
    }

    if (not authSuccess) {
        m_lastError.clear(); // 清除前序错误
        // 密码为空时 提示
        if (password.isEmpty()) {
            m_lastError = tr("Authentication required but no password provided");
            libssh2_session_free(m_session);
            emit errorOccurred(m_lastError);
            return false;
        }

        // 用户认证
        rc = libssh2_userauth_password(m_session, username.toUtf8().constData(),
                                       password.toUtf8().constData());
        if (rc != 0) {
            m_lastError = tr("Authentication failed: ") + QString::number(rc);
            libssh2_session_free(m_session); // 新增资源释放
            m_session = nullptr;
            emit errorOccurred(m_lastError);
            return false;
        }
    }

    // 初始化SFTP会话
    m_sftpSession = libssh2_sftp_init(m_session);
    if (!m_sftpSession) {
        m_lastError = tr("Failed to initialize SFTP session");
        emit errorOccurred(m_lastError);
        libssh2_session_free(m_session);
        m_session = nullptr;
        return false;
    }

    m_connected = true;
    emit connected();
    return true;
}

void sftpClient::disconnectFromHost() {
    // 逆序清理原则：先SFTP，再Session，最后socket
    if (m_sftpSession) {
        libssh2_sftp_shutdown(m_sftpSession);
        m_sftpSession = nullptr;
    }

    if (m_session) {
        libssh2_session_disconnect(m_session, "Client disconnecting");
        libssh2_session_free(m_session);
        m_session = nullptr;
    }

    if (m_tcpSocket) {
        if (m_tcpSocket->state() == QAbstractSocket::ConnectedState) {
            m_tcpSocket->disconnectFromHost();
            m_tcpSocket->waitForDisconnected(1000);
        }
    }

    m_connected = false;
}

bool sftpClient::put(const QString &localPath, const QString &remotePath) {
    // 检查连接状态
    if (!m_connected || !m_sftpSession) {
        m_lastError = tr("Not connected to SFTP server");
        emit errorOccurred(m_lastError);
        return false;
    }

    // 打开本地文件
    QFile localFile(localPath);
    if (!localFile.open(QIODevice::ReadOnly)) {
        m_lastError = tr("Failed to open local file: ") + localFile.errorString();
        emit errorOccurred(m_lastError);
        return false;
    }

    // 创建远程文件（使用二进制模式，rw-r--r--权限）
    LIBSSH2_SFTP_HANDLE *remoteHandle = libssh2_sftp_open_ex(
        m_sftpSession,
        remotePath.toUtf8().constData(),
        remotePath.length(),
        LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
        LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR |
        LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IROTH,
        LIBSSH2_SFTP_OPENFILE);

    if (!remoteHandle) {
        m_lastError = tr("Failed to create remote file");
        emit errorOccurred(m_lastError);
        localFile.close();
        return false;
    }

    // 分块传输（8KB块）
    const qint64 bufferSize = 8192;
    char buffer[bufferSize];
    qint64 totalBytes = localFile.size();
    qint64 bytesSent = 0;

    while (!localFile.atEnd()) {
        // 读取本地数据
        qint64 bytesRead = localFile.read(buffer, bufferSize);
        if (bytesRead < 0) {
            m_lastError = tr("Error reading local file");
            break;
        }

        // 写入远程文件
        ssize_t bytesWritten = 0;
        do {
            char *dataPtr = buffer + bytesWritten;
            ssize_t rc = libssh2_sftp_write(remoteHandle,
                                            dataPtr,
                                            bytesRead - bytesWritten);

            if (rc < 0) {
                char *errmsg;
                libssh2_session_last_error(m_session, &errmsg, nullptr, 0);
                m_lastError = tr("SFTP write error: ") + QString::fromLocal8Bit(errmsg);
                break;
            }
            bytesWritten += rc;
        } while (bytesWritten < bytesRead);

        // 错误处理
        if (bytesWritten != bytesRead) {
            break;
        }

        // 更新进度
        bytesSent += bytesRead;
        emit transferProgress(bytesSent, totalBytes);
        fprintf(stdout, "%lld\n", bytesSent / totalBytes);
    }

    // 清理资源
    libssh2_sftp_close(remoteHandle);
    localFile.close();

    // 最终状态检查
    if (bytesSent != totalBytes) {
        emit errorOccurred(m_lastError.isEmpty() ? tr("Incomplete file transfer") : m_lastError);
        return false;
    }

    return true;
}

bool sftpClient::get(const QString &remotePath, const QString &localPath) {
    // 检查连接状态
    if (!m_connected || !m_sftpSession) {
        m_lastError = tr("Not connected to SFTP server");
        emit errorOccurred(m_lastError);
        return false;
    }

    // 打开远程文件
    LIBSSH2_SFTP_HANDLE *remoteHandle = libssh2_sftp_open_ex(
        m_sftpSession,
        remotePath.toUtf8().constData(),
        remotePath.length(),
        LIBSSH2_FXF_READ,
        0,
        LIBSSH2_SFTP_OPENFILE);

    if (!remoteHandle) {
        m_lastError = tr("Failed to open remote file");
        emit errorOccurred(m_lastError);
        return false;
    }

    // 获取远程文件大小
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    if (libssh2_sftp_fstat(remoteHandle, &attrs) != 0) {
        m_lastError = tr("Failed to get file size");
        libssh2_sftp_close(remoteHandle);
        emit errorOccurred(m_lastError);
        return false;
    }
    quint64 totalBytes = attrs.filesize;

    // 创建本地文件
    QFile localFile(localPath);
    if (!localFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_lastError = tr("Failed to create local file: ") + localFile.errorString();
        libssh2_sftp_close(remoteHandle);
        emit errorOccurred(m_lastError);
        return false;
    }

    // 分块传输（16KB块）
    const int bufferSize = 16384;
    char buffer[bufferSize];
    qint64 bytesReceived = 0;
    bool transferError = false;

    while (true) {
        // 从远程读取数据
        ssize_t bytesRead = libssh2_sftp_read(remoteHandle, buffer, bufferSize);

        if (bytesRead < 0) {
            // 错误处理
            m_lastError = tr("SFTP read error: ") + QString::number(bytesRead);
            transferError = true;
            break;
        }
        if (bytesRead == 0) {
            // 传输完成
            break;
        }

        // 写入本地文件
        qint64 bytesWritten = localFile.write(buffer, bytesRead);
        if (bytesWritten != bytesRead) {
            m_lastError = tr("Local write error: ") + localFile.errorString();
            transferError = true;
            break;
        }

        // 更新进度
        bytesReceived += bytesWritten;
        emit transferProgress(bytesReceived, totalBytes);
    }

    // 强制刷新写入缓存
    localFile.flush();

    // 关闭资源
    libssh2_sftp_close(remoteHandle);
    localFile.close();

    // 最终验证
    if (transferError) {
        emit errorOccurred(m_lastError);
        QFile::remove(localPath); // 删除不完整文件
        return false;
    }

    // 验证文件大小
    if (bytesReceived != totalBytes) {
        m_lastError = tr("Incomplete transfer (") +
                      QString::number(bytesReceived) + "/" +
                      QString::number(totalBytes) + " bytes)";
        emit errorOccurred(m_lastError);
        QFile::remove(localPath);
        return false;
    }

    return true;
}

bool sftpClient::mkdir(const QString &path) {
    // 检查连接状态
    if (!m_connected || !m_sftpSession) {
        m_lastError = tr("Not connected to SFTP server");
        emit errorOccurred(m_lastError);
        return false;
    }

    // 转换路径格式
    QByteArray pathBytes = path.toUtf8();
    const char *sftpPath = pathBytes.constData();
    unsigned int pathLen = static_cast<unsigned int>(pathBytes.length());

    // 设置目录权限（rwxr-xr-x）
    const long mode = LIBSSH2_SFTP_S_IRWXU | // user: rwx
                      LIBSSH2_SFTP_S_IRGRP | // group: r
                      LIBSSH2_SFTP_S_IXGRP | // group: x
                      LIBSSH2_SFTP_S_IROTH | // others: r
                      LIBSSH2_SFTP_S_IXOTH; // others: x

    // 创建目录
    int rc = libssh2_sftp_mkdir_ex(m_sftpSession,
                                   sftpPath,
                                   pathLen,
                                   static_cast<int>(mode));

    // 错误处理
    if (rc != 0) {
        char *errmsg;
        libssh2_session_last_error(m_session, &errmsg, nullptr, 0);
        m_lastError = tr("Create directory failed: ") +
                      QString::fromLocal8Bit(errmsg);
        emit errorOccurred(m_lastError);
        return false;
    }

    return true;
}

bool sftpClient::rmdir(const QString &path) {
    // 检查连接状态
    if (!m_connected || !m_sftpSession) {
        m_lastError = tr("Not connected to SFTP server");
        emit errorOccurred(m_lastError);
        return false;
    }

    // 转换路径格式
    QByteArray pathBytes = path.toUtf8();
    const char *sftpPath = pathBytes.constData();
    unsigned int pathLen = static_cast<unsigned int>(pathBytes.size());

    // 执行目录删除
    int rc = libssh2_sftp_rmdir_ex(m_sftpSession, sftpPath, pathLen);

    // 错误处理
    if (rc != 0) {
        // 获取详细的错误信息
        char *errmsg;
        int errcode = libssh2_session_last_error(m_session, &errmsg, nullptr, 0);

        // 转换常见错误代码
        switch (errcode) {
            case LIBSSH2_FX_NO_SUCH_FILE:
                m_lastError = tr("Directory not exists: ") + path;
                break;
            case LIBSSH2_FX_PERMISSION_DENIED:
                m_lastError = tr("Permission denied: ") + path;
                break;
            case LIBSSH2_FX_DIR_NOT_EMPTY:
                m_lastError = tr("Directory not empty: ") + path;
                break;
            default:
                m_lastError = tr("Failed to remove directory: ") +
                              QString::fromLocal8Bit(errmsg);
        }

        emit errorOccurred(m_lastError);
        return false;
    }

    return true;
}

bool sftpClient::rm(const QString &path) {
    // 检查连接状态
    if (!m_connected || !m_sftpSession) {
        m_lastError = tr("Not connected to SFTP server");
        emit errorOccurred(m_lastError);
        return false;
    }
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    if (libssh2_sftp_stat(m_sftpSession, path.toUtf8(), &attrs) == 0) {
        if (LIBSSH2_SFTP_S_ISDIR(attrs.permissions)) {
            m_lastError = tr("Target is a directory");
            emit errorOccurred(m_lastError);
            return false;
        }
    }

    // 转换路径格式
    QByteArray pathBytes = path.toUtf8();
    const char *sftpPath = pathBytes.constData();
    unsigned int pathLen = static_cast<unsigned int>(pathBytes.size());

    // 执行文件删除
    int rc = libssh2_sftp_unlink_ex(m_sftpSession, sftpPath, pathLen);

    // 错误处理
    if (rc != 0) {
        char *errmsg;
        int errcode = libssh2_session_last_error(m_session, &errmsg, nullptr, 0);

        switch (errcode) {
            case LIBSSH2_FX_NO_SUCH_FILE: // 2UL
                m_lastError = tr("File not found: ") + path;
                break;
            case LIBSSH2_FX_PERMISSION_DENIED: // 3UL
                m_lastError = tr("Permission denied: ") + path;
                break;
            case LIBSSH2_FX_NOT_A_DIRECTORY: // 19UL（当尝试删除目录时返回）
                m_lastError = tr("Cannot delete directory with removeFile(): ") + path;
                break;
            case LIBSSH2_FX_DIR_NOT_EMPTY: // 18UL（虽然主要用于rmdir，但保持完整性）
                m_lastError = tr("Directory not empty: ") + path;
                break;
            default:
                m_lastError = tr("Failed to remove file: ") +
                              QString::fromLocal8Bit(errmsg);
        }

        emit errorOccurred(m_lastError);
        return false;
    }

    return true;
}

bool sftpClient::rmAuto(const QString &path) {
    // 检查连接状态
    if (!m_connected || !m_sftpSession) {
        m_lastError = tr("Not connected to SFTP server");
        emit errorOccurred(m_lastError);
        return false;
    }

    int isDir = 0;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    if (libssh2_sftp_stat(m_sftpSession, path.toUtf8(), &attrs) == 0) {
        isDir = LIBSSH2_SFTP_S_ISDIR(attrs.permissions);
    }

    if (not isDir) {
        return rm(path);
    } else {
        return rmdir(path);
    }

    return false;
}

QList<sftpClient::fileInfo> sftpClient::ls(const QString &path, const QList<listArg> &args) {
    QList<fileInfo> fileList;
    fileList.reserve(20);
    LIBSSH2_SFTP_HANDLE *dirHandle = nullptr;

    if (!m_connected || !m_sftpSession) {
        m_lastError = tr("Not connected to SFTP server");
        emit errorOccurred(m_lastError);
        return std::move(fileList);
    }

    QByteArray pathBytes = path.toUtf8();
    const char *sftpPath = pathBytes.constData();

    dirHandle = libssh2_sftp_opendir(m_sftpSession, sftpPath);
    if (!dirHandle) {
        char *errmsg;
        int errcode = libssh2_session_last_error(m_session, &errmsg, nullptr, 0);
        switch (errcode) {
            case LIBSSH2_FX_NO_SUCH_FILE:
                m_lastError = tr("Directory not exists: ") + path;
                break;
            case LIBSSH2_FX_PERMISSION_DENIED:
                m_lastError = tr("Permission denied: ") + path;
                break;
            case LIBSSH2_FX_NOT_A_DIRECTORY:
                m_lastError = tr("Not a directory: ") + path;
                break;
            default:
                m_lastError = tr("Failed to open directory: ") + QString::fromLocal8Bit(errmsg);
        }
        emit errorOccurred(m_lastError);
        return std::move(fileList);
    }

    bool showAll = args.contains(listArg::all);
    bool showDetails = args.contains(listArg::list);

    char buffer[4096] = {0};
    LIBSSH2_SFTP_ATTRIBUTES attrs;

    while (true) {
        int rc = libssh2_sftp_readdir(dirHandle, buffer, sizeof(buffer), &attrs);
        if (rc <= 0) {
            if (rc < 0) {
                m_lastError = tr("Directory read error: ") + QString::number(rc);
                emit errorOccurred(m_lastError);
                fileList.clear();
            }
            break;
        }

        QString fileName = QString::fromUtf8(buffer);
        if (!showAll && (fileName[0] == '.')) {
            continue;
        }

        fileInfo info;
        info.filename = fileName;

        if (showDetails) {
            // 处理权限
            info.permissions.clear();
            if (attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) {
                info.permissions += LIBSSH2_SFTP_S_ISDIR(attrs.permissions)
                                        ? "d"
                                        : LIBSSH2_SFTP_S_ISLNK(attrs.permissions)
                                              ? "l"
                                              : "-";
                info.permissions += (attrs.permissions & 0400) ? "r" : "-";
                info.permissions += (attrs.permissions & 0200) ? "w" : "-";
                info.permissions += (attrs.permissions & 0100) ? "x" : "-";
                info.permissions += (attrs.permissions & 0040) ? "r" : "-";
                info.permissions += (attrs.permissions & 0020) ? "w" : "-";
                info.permissions += (attrs.permissions & 0010) ? "x" : "-";
                info.permissions += (attrs.permissions & 0004) ? "r" : "-";
                info.permissions += (attrs.permissions & 0002) ? "w" : "-";
                info.permissions += (attrs.permissions & 0001) ? "x" : "-";
            } else {
                info.permissions = "??????????";
            }

            // 用户和组
            if (attrs.flags & LIBSSH2_SFTP_ATTR_UIDGID) {
                info.user = userNameFromUid(attrs.uid);
                info.userGroups = groupNameFromGid(attrs.gid);
            } else {
                info.user.clear();
                info.userGroups.clear();
            }

            // 文件大小
            if (attrs.flags & LIBSSH2_SFTP_ATTR_SIZE) {
                info.fileSize = QString::number(attrs.filesize);
            } else {
                info.fileSize.clear();
            }

            // 修改时间
            if (attrs.flags & LIBSSH2_SFTP_ATTR_ACMODTIME) {
                QDateTime dt = QDateTime::fromSecsSinceEpoch(attrs.mtime);
                info.dateModified = dt.toString("yyyy-MM-dd hh:mm:ss");
            } else {
                info.dateModified.clear();
            }
        } else {
            info.permissions.clear();
            info.user.clear();
            info.userGroups.clear();
            info.fileSize.clear();
            info.dateModified.clear();
        }

        fileList.append(info);
    }

    libssh2_sftp_closedir(dirHandle);
    return std::move(fileList);
}

bool sftpClient::rename(const QString &oldName, const QString &newName) {
    // 检查连接状态
    if (!m_connected || !m_sftpSession) {
        m_lastError = tr("Not connected to SFTP server");
        emit errorOccurred(m_lastError);
        return false;
    }

    QByteArray oldPath = oldName.toUtf8();
    QByteArray newPath = newName.toUtf8();

    // 优先尝试 POSIX 重命名（如果服务器支持）
    int rc = libssh2_sftp_posix_rename_ex(m_sftpSession,
                                          oldPath.constData(),
                                          oldPath.size(),
                                          newPath.constData(),
                                          newPath.size());

    if (rc == 0) {
        return true;
    }

    // 如果 POSIX 重命名失败，回退到标准重命名方法
    rc = libssh2_sftp_rename_ex(m_sftpSession,
                                oldPath.constData(),
                                oldPath.size(),
                                newPath.constData(),
                                newPath.size(),
                                0); // flags 参数设为 0，根据需求调整
    if (rc < 0) {
        // 错误处理
        char *errmsg;
        libssh2_session_last_error(m_session, &errmsg, nullptr, 0);
        m_lastError = errmsg;
        emit errorOccurred(m_lastError);
        return false;
    }
    return true;
}

bool sftpClient::cp(const QString &src, const QString &dst) {
    // 检查连接状态
    if (!m_connected || !m_sftpSession) {
        m_lastError = tr("Not connected to SFTP server");
        emit errorOccurred(m_lastError);
        return false;
    }

    LIBSSH2_SFTP_HANDLE *srcHandle = nullptr;
    LIBSSH2_SFTP_HANDLE *dstHandle = nullptr;
    QByteArray srcBytes = src.toUtf8();
    QByteArray dstBytes = dst.toUtf8();
    bool result = false;

    // 清空资源
    auto cleanup = [&]()-> bool {
        // 6. 资源清理
        if (srcHandle)
            libssh2_sftp_close(srcHandle);
        if (dstHandle)
            libssh2_sftp_close(dstHandle);

        if (!result && m_lastError.isEmpty()) {
            m_lastError = tr("Unknown error during file copy");
        }
        if (!result) {
            emit errorOccurred(m_lastError);
        }
        return result;
    };

    // 1. 打开源文件
    srcHandle = libssh2_sftp_open_ex(m_sftpSession,
                                     srcBytes.constData(),
                                     srcBytes.size(),
                                     LIBSSH2_FXF_READ,
                                     0,
                                     LIBSSH2_SFTP_OPENFILE);
    if (!srcHandle) {
        char *errmsg;
        libssh2_session_last_error(m_session, &errmsg, nullptr, 0);
        m_lastError = tr("Failed to open source file: ") + QString::fromLocal8Bit(errmsg);
        return cleanup();
    }

    // 2. 获取源文件属性
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    if (libssh2_sftp_fstat(srcHandle, &attrs) != 0) {
        char *errmsg;
        libssh2_session_last_error(m_session, &errmsg, nullptr, 0);
        m_lastError = tr("Failed to get file attributes: ") + QString::fromLocal8Bit(errmsg);
        return cleanup();
    }

    // 3. 创建目标文件（使用源文件权限，添加用户写权限确保可写）
    long mode = attrs.permissions | LIBSSH2_SFTP_S_IWUSR;
    dstHandle = libssh2_sftp_open_ex(m_sftpSession,
                                     dstBytes.constData(),
                                     dstBytes.size(),
                                     LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
                                     mode,
                                     LIBSSH2_SFTP_OPENFILE);
    if (!dstHandle) {
        char *errmsg;
        libssh2_session_last_error(m_session, &errmsg, nullptr, 0);
        m_lastError = tr("Failed to create destination file: ") + QString::fromLocal8Bit(errmsg);
        return cleanup();
    }

    // 4. 分块传输（8KB缓冲区）
    const int bufferSize = 8192;
    char buffer[bufferSize] = {0};
    quint64 totalBytes = attrs.filesize;
    quint64 bytesCopied = 0;
    bool transferError = false;

    while (!transferError) {
        // 读取源文件
        ssize_t bytesRead = libssh2_sftp_read(srcHandle, buffer, bufferSize);
        if (bytesRead < 0) {
            char *errmsg;
            libssh2_session_last_error(m_session, &errmsg, nullptr, 0);
            m_lastError = tr("Read error: ") + QString::fromLocal8Bit(errmsg);
            transferError = true;
            break;
        }
        if (bytesRead == 0) break; // 文件结束

        // 写入目标文件（确保完整写入）
        ssize_t bytesWritten = 0;
        while (bytesWritten < bytesRead && !transferError) {
            ssize_t rc = libssh2_sftp_write(dstHandle,
                                            buffer + bytesWritten,
                                            bytesRead - bytesWritten);
            if (rc < 0) {
                char *errmsg;
                libssh2_session_last_error(m_session, &errmsg, nullptr, 0);
                m_lastError = tr("Write error: ") + QString::fromLocal8Bit(errmsg);
                transferError = true;
                break;
            }
            bytesWritten += rc;
        }

        // 更新进度
        bytesCopied += bytesRead;
        emit transferProgress(bytesCopied, totalBytes);
    }

    // 5. 最终校验
    if (transferError) {
        // 删除不完整的目标文件
        libssh2_sftp_unlink_ex(m_sftpSession,
                               dstBytes.constData(),
                               dstBytes.size());
    } else if (bytesCopied != totalBytes) {
        m_lastError = tr("Incomplete copy: %1/%2 bytes transferred")
                .arg(bytesCopied).arg(totalBytes);
        libssh2_sftp_unlink_ex(m_sftpSession,
                               dstBytes.constData(),
                               dstBytes.size());
        transferError = true;
    }

    result = !transferError;

    return cleanup();
}

QString sftpClient::lastError() const {
    return m_lastError;
}

QString sftpClient::home() {
    if (!m_connected || !m_sftpSession) {
        m_lastError = tr("Not connected to SFTP server");
        return {};
    }

    // 预分配缓冲区（支持最大 4095 字符路径）
    constexpr int BUFFER_SIZE = 4096;
    char cpath[BUFFER_SIZE] = {0};

    // 解析当前目录的绝对路径
    int rc = libssh2_sftp_symlink_ex(
        m_sftpSession,
        ".", 1,
        cpath, BUFFER_SIZE,
        LIBSSH2_SFTP_REALPATH
    );

    // 错误处理（严格区分返回码语义）
    if (rc < 0) {
        m_lastError = tr("SFTP realpath error: ") +
                      QString::number(libssh2_sftp_last_error(m_sftpSession));
        return {};
    }

    // 显式终止符保证
    if (rc >= BUFFER_SIZE) rc = BUFFER_SIZE - 1;
    cpath[rc >= 0 ? rc : 0] = '\0';

    // 路径标准化处理
    QString homePath = QString::fromUtf8(cpath)
            .trimmed()
            .replace("//", "/");

    return homePath.isEmpty() ? "/" : homePath; // 默认回退根目录
}

void sftpClient::exist(const QString &path, fileType &type) {
    // 检查连接状态
    if (!m_connected || !m_sftpSession) {
        m_lastError = tr("Not connected to SFTP server");
        type = fileType::none;
        return;
    }

    QByteArray pathBytes = path.toUtf8();
    const char *sftpPath = pathBytes.constData();
    LIBSSH2_SFTP_ATTRIBUTES attrs;

    // 使用 lstat 检查路径元数据（不跟随符号链接）
    int rc = libssh2_sftp_lstat(m_sftpSession, sftpPath, &attrs);
    if (rc == 0) {
        type = LIBSSH2_SFTP_S_ISDIR(attrs.permissions)
                   ? fileType::dir
                   : LIBSSH2_SFTP_S_ISLNK(attrs.permissions)
                         ? fileType::link
                         : fileType::file;
        return; // 路径存在（文件/目录/符号链接）
    }
    type = fileType::none;

    // 获取具体错误码
    int sftpError = libssh2_sftp_last_error(m_sftpSession);

    // 处理不存在的情况
    if (sftpError == LIBSSH2_FX_NO_SUCH_FILE) {
        return;
    }

    // 其他错误处理
    char *errmsg = nullptr;
    libssh2_session_last_error(m_session, &errmsg, nullptr, 0);

    // 将错误信息格式化为可读字符串
    m_lastError = tr("SFTP error [%1]: %2")
            .arg(sftpError)
            .arg(QString::fromLocal8Bit(errmsg));
}

QString sftpClient::userNameFromUid(uint uid) {
    LIBSSH2_CHANNEL *channel = libssh2_channel_open_session(m_session);
    if (!channel) return QString::number(uid);

    QString username;
    QString command = QString("getent passwd %1 | cut -d: -f1").arg(uid);

    if (libssh2_channel_exec(channel, command.toUtf8().constData()) == 0) {
        char buffer[128];
        ssize_t bytesRead = libssh2_channel_read(channel, buffer, sizeof(buffer)-1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            username = QString::fromUtf8(buffer).trimmed();
        }
    }

    libssh2_channel_close(channel);
    libssh2_channel_free(channel);
    return username.isEmpty() ? QString::number(uid) : username;
}

QString sftpClient::groupNameFromGid(uint gid) {
    LIBSSH2_CHANNEL *channel = libssh2_channel_open_session(m_session);
    if (!channel) return QString::number(gid);

    QString groupname;
    QString command = QString("getent group %1 | cut -d: -f1").arg(gid);

    if (libssh2_channel_exec(channel, command.toUtf8().constData()) == 0) {
        char buffer[128];
        ssize_t bytesRead = libssh2_channel_read(channel, buffer, sizeof(buffer)-1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            groupname = QString::fromUtf8(buffer).trimmed();
        }
    }

    libssh2_channel_close(channel);
    libssh2_channel_free(channel);
    return groupname.isEmpty() ? QString::number(gid) : groupname;
}
