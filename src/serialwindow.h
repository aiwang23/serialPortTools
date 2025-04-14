//
// Created by wang on 25-4-1.
//

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <ElaWidget.h>
#include <QTimer>
#include <CSerialPort/SerialPort.h>

#include "threadPool.h"
#include "concurrentqueue.h"

struct serialSettings;
class QTcpSocket;
class QCompleter;
class QStringListModel;
class ElaLineEdit;
class ElaIconButton;
class ElaToggleButton;

namespace maddy {
    class Parser;
    struct ParserConfig;
}

// 输出框渲染类型
enum class dataType { NONE = -1, TEXT, HEX, MARKDOWN, HTML };

// string to showType
inline dataType dataTypeFrom(const std::string &str) {
    if ("text" == str) return dataType::TEXT;
    else if ("hex" == str) return dataType::HEX;
    else if ("markdown" == str) return dataType::MARKDOWN;
    else if ("html" == str) return dataType::HTML;
    else return dataType::NONE;
}

enum class sendMode { NONE = -1, MAN, AUTO, CMD };

inline sendMode sendModeFrom(const std::string &str) {
    if ("MAN" == str) return sendMode::MAN;
    else if ("AUTO" == str) return sendMode::AUTO;
    else if ("CMD" == str) return sendMode::CMD;
    else return sendMode::NONE;
}

using itas109::CSerialPortListener;
using itas109::CSerialPort;

QT_BEGIN_NAMESPACE

namespace Ui {
    class serialWindow;
}

QT_END_NAMESPACE

// serial window
class serialWindow : public QWidget, public CSerialPortListener {
    Q_OBJECT

public:
    explicit serialWindow(QWidget *parent = nullptr);

    ~serialWindow() override;

protected:
    // 国际化 刷新时 语言切换
    void changeEvent(QEvent *event) override;

private:
    void initComboBox();

    void initSignalSlots();

    void initText();

    void initUI();

    // 发送 serial Settings 到server ,请求打开 serial
    int64_t writeSerialSettings(const serialSettings& settings);

Q_SIGNALS:
    // 清空 comboBox_port items
    Q_SIGNAL void sigClearComboBoxPort();

    // 添加 comboBox_port items
    Q_SIGNAL void sigAddSerialPort(QString item);

    // 从串口读取后 并经过处理后 的数据
    Q_SIGNAL void sigDataCompleted(QByteArray data);

    // 发送到本地串口或网络
    Q_SIGNAL void sigSendData(QByteArray data);

private Q_SLOTS:
    //
    Q_SLOT void refreshSerialPort();

    // 收到数据后 进行处理 处理完后 直接显示
    Q_SLOT void convertDataAndSend(const QByteArray &data);

    // 发送数据到串口
    Q_SLOT void sendDataToSerial(const QByteArray &data);
    // 发起连接
    Q_SLOT void connectToHost();
    // 接受 tcp网络数据
    Q_SLOT void readTcpData();

    // 接受serial data回调
    Q_SLOT void onReadEvent(const char *portName, unsigned int readBufferLen) override;

    // 打开或关闭 串口(本地/远程)
    Q_SLOT void openOrCloseSerialPort(bool check);

    // 发送模式切换
    Q_SLOT void sendModeChange(const QString &item);
    // 收到数据显示到窗口上
    Q_SLOT void showSerialData(const QString &data);
    // 数据窗口 发送模式切换时的动画
    Q_SLOT void dataWidgetAnimation();
    // 把收到的数据保存成文件
    Q_SLOT void saveDataWidgetToFile();
    // 打开或关闭 自动发送模式
    Q_SLOT void openOrCloseAutoMode(bool check);
    // 改变自动发送模式的周期
    Q_SLOT void autoModeCycleChange();
    // 打开或关闭远程模式
    Q_SLOT void enableRemote(bool check);

public Q_SLOT:
    Q_SLOT void hideSecondaryWindow();

    Q_SLOT void showSecondaryWindow();

private:
    Ui::serialWindow *ui;
    CSerialPort serial_port_;
    dataType show_type_ = dataType::TEXT;
    dataType send_type_ = dataType::TEXT;
    sendMode send_mode_ = sendMode::MAN;

    // markdown 转 html
    std::shared_ptr<maddy::ParserConfig> config_;
    std::unique_ptr<maddy::Parser> parser_;
    ElaIconButton *iconBtn_refresh_;
    ElaIconButton *iconBtn_url_refresh_;

    ThreadPool threadPool_;
    std::atomic_bool isStop_ = false;
    moodycamel::ConcurrentQueue<QByteArray> data_queue_;

    // 数据窗口比例 MAN模式 CMD模式
    std::unordered_map<sendMode, QList<int> > dataWidgetRatio_;
    float animationProgress = 0;
    QTimer *animationTimer;
    QTimer *autoSendTimer_;

    QTcpSocket *tcp_socket_ = nullptr;
    std::atomic_bool isRemote = false; // true: 启用远程调试, false: 启用本地调试(不启用远程调试)
};


#endif //MAINWINDOW_H
