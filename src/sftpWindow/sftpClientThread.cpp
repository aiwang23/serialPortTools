//
// Created by wang on 25-4-27.
//

#include "sftpClientThread.h"

#include "sftpClient.h"


sftpClientThread::sftpClientThread(QObject *parent): QObject(parent) {
}

sftpClientThread::~sftpClientThread() {
    stop();
}

void sftpClientThread::start() {
    if (isStop_) {
        thread_ = new std::thread{&sftpClientThread::run, this};
        isStop_ = false;
    }
}

void sftpClientThread::stop() {
    isStop_ = true;
}

std::shared_ptr<sftpClientResponse>
sftpClientThread::try_do(funcType func, std::shared_ptr<funcArgs> args) {
    auto rs = std::make_shared<sftpClientResponse>();
    queue_.enqueue(func_args{func, args, rs});
    return rs;
}

void sftpClientThread::run() {
    sftpClient sftp;

    func_args t;
    while (not isStop_) {
        if (queue_.size_approx() <= 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            continue;
        }
        queue_.try_dequeue(t);
        if (funcType::connectToHost == t.func) {
            // 发起连接
            const funcArgs6 *arg6 = static_cast<funcArgs6 *>(t.args.get());
            bool rs = sftp.connectToHost(arg6->a1, arg6->a2.toUInt(), arg6->a3, arg6->a4, arg6->a5, arg6->a6);
            emit t.rs->connectToHostResponse(rs);
        } else if (funcType::home == t.func) {
            // 获取home目录
            QString home = sftp.home();
            emit t.rs->homeResponse(home);
        } else if (funcType::ls == t.func) {
            // 获取列表
            funcArgs2 *args2 = static_cast<funcArgs2 *>(t.args.get());
            QString path = args2->a1;
            QString a2 = args2->a2;

            QList<sftpClient::listArg> listargs;
            if (a2.contains('a'))
                listargs.append(sftpClient::listArg::all);
            if (a2.contains('l'))
                listargs.append(sftpClient::listArg::list);

            const QList<sftpClient::fileInfo> list = sftp.ls(path, listargs);
            emit t.rs->lsResponse(list);
        } else if (funcType::exist == t.func) {
            // 查看 这个目录是否存在
            funcArgs1 *args1 = static_cast<funcArgs1 *>(t.args.get());
            QString path = args1->a1;
            sftpClient::fileType file_t_rs;
            sftp.exist(path, file_t_rs);

            emit t.rs->existResponse(file_t_rs);
        } else if (funcType::get == t.func) {
            // 下载
            funcArgs2 *args2 = static_cast<funcArgs2 *>(t.args.get());
            QString remotePath = args2->a1;
            QString localPath = args2->a2;

            connect(&sftp, &sftpClient::getTransferProgress, t.rs.get(), &sftpClientResponse::getTransferProgress);
            bool rs = sftp.get(remotePath, localPath);
            disconnect(&sftp, &sftpClient::getTransferProgress, t.rs.get(), &sftpClientResponse::getTransferProgress);
            emit t.rs->getResponse(rs);
        } else if (funcType::put == t.func) {
            // 上传
            funcArgs2 *args2 = static_cast<funcArgs2 *>(t.args.get());
            QString localPath = args2->a1;
            QString remotePath = args2->a2;

            connect(&sftp, &sftpClient::putTransferProgress, t.rs.get(), &sftpClientResponse::putTransferProgress);
            bool rs = sftp.put(localPath, remotePath);
            disconnect(&sftp, &sftpClient::putTransferProgress, t.rs.get(), &sftpClientResponse::putTransferProgress);
            emit t.rs->putResponse(rs);
        }
    }

    sftp.disconnectFromHost();
}
