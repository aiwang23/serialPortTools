//
// Created by wang on 25-4-1.
//

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <ElaWidget.h>
#include <CSerialPort/SerialPort.h>

namespace maddy {
    class Parser;
    struct ParserConfig;
}

// 输出框渲染类型
enum showType { NONE = -1, TEXT, MARKDOWN, HTML };

// string to showType
inline showType showTypeFrom(std::string str) {
    if ("text" == str) return TEXT;
    else if ("markdown" == str) return MARKDOWN;
    else if ("html" == str) return HTML;
    else return NONE;
}

using itas109::CSerialPortListener;
using itas109::CSerialPort;

QT_BEGIN_NAMESPACE

namespace Ui {
    class mainWindow;
}

QT_END_NAMESPACE

// serial window
// TODO: 后面把 mainWindow 改为 serialWindow
class mainWindow : public QWidget, public CSerialPortListener {
    Q_OBJECT

public:
    explicit mainWindow(QWidget *parent = nullptr);

    ~mainWindow() override;

private:
    void initComboBox();

    void initSignalSlots();

Q_SIGNALS:
    Q_SIGNAL void sigClearComboBoxPort();

    Q_SIGNAL void sigAddSerialPort(QString item);

    Q_SIGNAL void sigSerialPortData(QString data);

private Q_SLOTS:
    // 接受serial data回调
    Q_SLOT void onReadEvent(const char *portName, unsigned int readBufferLen);

private:
    Ui::mainWindow *ui;
    CSerialPort serial_port_;
    showType show_type_ = showType::TEXT;

    // markdown 转 html
    std::shared_ptr<maddy::ParserConfig> config_;
    std::unique_ptr<maddy::Parser> parser_;
};


#endif //MAINWINDOW_H
