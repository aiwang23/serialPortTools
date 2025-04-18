//
// Created by wang on 25-4-19.
//

#include "settings.h"

#include <fstream>

settings &settings::instance() {
    static settings settings;
    return settings;
}

void settings::set(const nlohmann::json &json) {

    if (not checkFormat(json)) {
        std::ofstream ofstream{fileName};
        ofstream << defaultSettings().dump();
        ofstream.close();
        return;
    }
    std::ifstream ifStream{fileName};
    nlohmann::json original_json = nlohmann::json::parse(ifStream);
    ifStream.close();

    // 选择性合并（只覆盖存在的字段）
    original_json.merge_patch(json);

    std::ofstream ofstream{fileName};
    ofstream << original_json.dump();
    ofstream.close();

    emit sigSettingsUpdated(json);
}

nlohmann::json settings::get() {

    try {
        std::ifstream ifStream{fileName};
        nlohmann::json json = nlohmann::json::parse(ifStream);
        ifStream.close();
        return json;
    } catch (...) {
        std::ofstream ofstream{fileName};
        nlohmann::json json = defaultSettings();
        ofstream << json.dump();
        ofstream.close();
        return json;
    }
}

nlohmann::json settings::defaultSettings() {
    nlohmann::json json;
    json["language"] = "zh_CN";
    json["defaultNewWindow"] = "serial window";

    return std::move(json);
}

bool settings::checkFormat(const nlohmann::json &json) {
    try {
        if (json.contains("language") and not json["language"].is_null())
            return true;
        else if (json.contains("defaultNewWindow") and not json["defaultNewWindow"].is_null())
            return true;
    } catch (...) {
        return false;
    }
    return false;
}
