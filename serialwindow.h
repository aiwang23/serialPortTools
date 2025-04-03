//
// Created by wang on 25-4-1.
//

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <ElaWidget.h>
#include <CSerialPort/SerialPort.h>

class ElaIconButton;
class ElaToggleButton;

namespace maddy {
    class Parser;
    struct ParserConfig;
}

// 输出框渲染类型
enum showType { NONE = -1, TEXT, HEX, MARKDOWN, HTML };

// string to showType
inline showType showTypeFrom(const std::string &str) {
    if ("text" == str) return TEXT;
    else if ("hex" == str) return HEX;
    else if ("markdown" == str) return MARKDOWN;
    else if ("html" == str) return HTML;
    else return NONE;
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
    Q_SLOT void onReadEvent(const char *portName, unsigned int readBufferLen);

Q_SIGNALS:
    Q_SIGNAL void sigClearComboBoxPort();

    Q_SIGNAL void sigAddSerialPort(QString item);

    Q_SIGNAL void sigSerialPortData(QString data);

private Q_SLOTS:
    void refreshSerialPort();

public Q_SLOT:
    void hideSecondaryWindow();

    void showSecondaryWindow();

private:
    Ui::serialWindow *ui;
    CSerialPort serial_port_;
    showType show_type_ = showType::TEXT;

    // markdown 转 html
    std::shared_ptr<maddy::ParserConfig> config_;
    std::unique_ptr<maddy::Parser> parser_;
    ElaIconButton *iconBtn_flush_;
    bool isSendHex_ = false;
};


#endif //MAINWINDOW_H
