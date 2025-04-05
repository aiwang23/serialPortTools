//
// Created by wang on 25-4-1.
//

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <ElaWidget.h>
#include <CSerialPort/SerialPort.h>

#include "threadPool.h"
#include "concurrentqueue.h"

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

private:
    void initComboBox();

    void initSignalSlots();

    void initText();

    // 接受serial data回调
    Q_SLOT void onReadEvent(const char *portName, unsigned int readBufferLen) override;

Q_SIGNALS:
    Q_SIGNAL void sigClearComboBoxPort();

    Q_SIGNAL void sigAddSerialPort(QString item);

    // 从串口读取的数据
    Q_SIGNAL void sigSerialPortData(QString data);

    // 从串口读取后 并经过处理后 的数据
    Q_SIGNAL void sigDataCompleted(QString data);

    Q_SIGNAL void sigSendData(QString data);

private Q_SLOTS:
    Q_SLOT void refreshSerialPort();

    // 收到数据后 进行处理 处理完后 直接显示
    Q_SLOT void convertDataAndSend(const QString &data);

    // 发送数据到串口
    Q_SLOT void sendDataToSerial(QString data);

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

    ThreadPool threadPool_;
    std::atomic_bool isStop_ = false;
    moodycamel::ConcurrentQueue<QString> data_queue_;
};




#endif //MAINWINDOW_H
