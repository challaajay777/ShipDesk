#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QString>

namespace shipdesk::domain::json {

inline std::string escapeString(const std::string& input) {
    QJsonValue val(QString::fromStdString(input));
    QJsonArray arr;
    arr.append(val);
    QJsonDocument doc(arr);
    QString jsonStr = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    // Strip leading '[' and trailing ']' and surrounding quotes if present
    if (jsonStr.startsWith("[\"") && jsonStr.endsWith("\"]")) {
        return jsonStr.mid(2, jsonStr.length() - 4).toStdString();
    }
    std::ostringstream ss;
    for (char c : input) {
        switch (c) {
            case '"':  ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\b': ss << "\\b";  break;
            case '\f': ss << "\\f";  break;
            case '\n': ss << "\\n";  break;
            case '\r': ss << "\\r";  break;
            case '\t': ss << "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c));
                } else {
                    ss << c;
                }
                break;
        }
    }
    return ss.str();
}

inline std::string extractStringField(const std::string& json, const std::string& key, const std::string& defaultValue = "") {
    if (json.empty()) return defaultValue;
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromRawData(json.data(), static_cast<qsizetype>(json.size())));
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        QString qKey = QString::fromStdString(key);
        if (obj.contains(qKey)) {
            return obj[qKey].toString(QString::fromStdString(defaultValue)).toStdString();
        }
    }
    return defaultValue;
}

inline uint64_t extractUInt64Field(const std::string& json, const std::string& key, uint64_t defaultValue = 0) {
    if (json.empty()) return defaultValue;
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromRawData(json.data(), static_cast<qsizetype>(json.size())));
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        QString qKey = QString::fromStdString(key);
        if (obj.contains(qKey)) {
            return static_cast<uint64_t>(obj[qKey].toInteger(static_cast<qint64>(defaultValue)));
        }
    }
    return defaultValue;
}

inline bool extractBoolField(const std::string& json, const std::string& key, bool defaultValue = false) {
    if (json.empty()) return defaultValue;
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromRawData(json.data(), static_cast<qsizetype>(json.size())));
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        QString qKey = QString::fromStdString(key);
        if (obj.contains(qKey)) {
            return obj[qKey].toBool(defaultValue);
        }
    }
    return defaultValue;
}

inline std::vector<std::string> extractStringArray(const std::string& json, const std::string& key) {
    std::vector<std::string> result;
    if (json.empty()) return result;
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromRawData(json.data(), static_cast<qsizetype>(json.size())));
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        QString qKey = QString::fromStdString(key);
        if (obj.contains(qKey) && obj[qKey].isArray()) {
            QJsonArray arr = obj[qKey].toArray();
            for (const auto& item : arr) {
                result.push_back(item.toString().toStdString());
            }
        }
    }
    return result;
}

} // namespace shipdesk::domain::json