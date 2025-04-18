//
// Created by wang on 25-4-19.
//

#ifndef SETTINGS_H
#define SETTINGS_H

#include <mutex>
#include <QObject>

#include "nlohmann/json.hpp"

class settings : public QObject {
    Q_OBJECT

public:
    // 私有化默认构造和析构
    settings() = default;

    settings(const settings &) = delete;

    settings &operator=(const settings &) = delete;

    static settings &instance();

    void set(const nlohmann::json &json);
    nlohmann::json get();

    static nlohmann::json defaultSettings();

    static bool checkFormat(const nlohmann::json& json);

Q_SIGNALS:
    Q_SIGNAL void sigSettingsUpdated(nlohmann::json json);

private:
    std::mutex mutex_;
    const std::string fileName = "settings.json";
};


#endif //SETTINGS_H
