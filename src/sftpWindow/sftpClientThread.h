//
// Created by wang on 25-4-27.
//

#ifndef SFTPCLIENTTHREAD_H
#define SFTPCLIENTTHREAD_H

#include <QObject>
#include <thread>

#include "sftpClient.h"
#include "sftpClientResponse.h"
#include "moodycamel/concurrentqueue.h"

enum class funcType {
    none = -1, connectToHost, disconnectFromHost, put, get, mkdir, rmdir, rm, rmAuto, ls, rename, cp, lastError, home,
    exist
};

struct funcArgs {
};

struct funcArgs1 : funcArgs {
    QString a1;
};

struct funcArgs2 : funcArgs1 {
    QString a2;
};

struct funcArgs3 : funcArgs2 {
    QString a3;
};

struct funcArgs4 : funcArgs3 {
    QString a4;
};

struct funcArgs5 : funcArgs4 {
    QString a5;
};

struct funcArgs6 : funcArgs5 {
    QString a6;
};

class sftpClientThread : public QObject {
    Q_OBJECT

public:
    sftpClientThread(QObject *parent = nullptr);

    ~sftpClientThread();

    void start();

    void stop();

    std::shared_ptr<sftpClientResponse> try_do(funcType func, std::shared_ptr<funcArgs> args = nullptr);

private:
    void run();

private:
    std::thread *thread_ = nullptr;
    std::atomic_bool isStop_ = true;

    struct func_args {
        funcType func;
        std::shared_ptr<funcArgs> args;
        std::shared_ptr<sftpClientResponse> rs;
    };

    moodycamel::ConcurrentQueue<func_args> queue_;

    funcArgs args;
};


#endif //SFTPCLIENTTHREAD_H
