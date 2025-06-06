//
// Created by wang on 25-4-2.
//

#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QWidget>
#include <ElaScrollPage.h>

#include "nlohmann/json.hpp"

QT_BEGIN_NAMESPACE

enum class defaultNewWindowType { serialWindow, serialServer };

namespace Ui {
    class settingsWindow;
}

QT_END_NAMESPACE

class settingsWindow : public QWidget {
    Q_OBJECT

public:
    explicit settingsWindow(QWidget *parent = nullptr);

    ~settingsWindow() override;

    void initComboBox();

    void initSignalSlots();

protected:
    void changeEvent(QEvent *event) override;

Q_SIGNALS:
    Q_SIGNAL void sigLanguageChanged(QString language);

    Q_SIGNAL void sigDefaultNewWindowChanged(defaultNewWindowType type);

public Q_SLOTS:
    void settingsUpdate(nlohmann::json json);

private:
    Ui::settingsWindow *ui;

    std::unordered_map<QString, defaultNewWindowType> defaultNewWindowMap_;

    QString oldLangText;
    QString oldDefaultNewWindowText;
};


#endif //SETTINGSWINDOW_H
