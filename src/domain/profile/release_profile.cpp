#include "domain/profile/release_profile.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>

namespace shipdesk::domain {

ReleaseProfile::ReleaseProfile(std::string id, std::string workspace_id, std::string name, std::string category)
    : m_id(std::move(id))
    , m_workspaceId(std::move(workspace_id))
    , m_name(std::move(name))
    , m_category(std::move(category))
    , m_createdAtUtc(currentUtcTimestamp())
    , m_updatedAtUtc(m_createdAtUtc)
{
}

ReleaseProfile ReleaseProfile::createDefault(const std::string& category, const std::string& workspaceId) {
    ReleaseProfile p("profile-default", workspaceId, category + " Profile", category);
    if (category == "Software Application") {
        p.m_defaultFormat = PackageFormat::ZipArchive;
        p.m_excludePatterns.push_back("*.obj");
        p.m_excludePatterns.push_back("*.o");
        p.m_excludePatterns.push_back("*.pdb");
    } else if (category == "Static Website Output") {
        p.m_defaultFormat = PackageFormat::ZipArchive;
        p.m_excludePatterns.push_back("node_modules/*");
        p.m_excludePatterns.push_back(".git*");
    } else if (category == "Document Bundle") {
        p.m_defaultFormat = PackageFormat::ZipArchive;
        p.m_excludePatterns.push_back("~$*");
    }
    return p;
}

std::string ReleaseProfile::serializeSettingsJson() const {
    QJsonObject root;
    root["schema_version"] = QString::fromStdString(m_schemaVersion);
    root["naming_template"] = QString::fromStdString(m_namingTemplate);
    root["default_format"] = QString::fromStdString(packageFormatToString(m_defaultFormat));
    root["include_hidden_files"] = m_includeHiddenFiles;
    root["follow_symlinks"] = m_followSymlinks;
    root["max_total_bytes"] = static_cast<qint64>(m_maxTotalBytes);
    root["max_file_count"] = static_cast<qint64>(m_maxFileCount);
    root["max_per_file_bytes"] = static_cast<qint64>(m_maxPerFileBytes);

    QJsonArray incArr;
    for (const auto& pat : m_includePatterns) {
        incArr.append(QString::fromStdString(pat));
    }
    root["include_patterns"] = incArr;

    QJsonArray excArr;
    for (const auto& pat : m_excludePatterns) {
        excArr.append(QString::fromStdString(pat));
    }
    root["exclude_patterns"] = excArr;

    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Indented).toStdString();
}

void ReleaseProfile::deserializeSettingsJson(const std::string& json) {
    if (json.empty()) return;

    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromRawData(json.data(), static_cast<qsizetype>(json.size())));
    if (!doc.isObject()) return;

    QJsonObject root = doc.object();
    m_schemaVersion = root["schema_version"].toString("1.0.0").toStdString();
    m_namingTemplate = root["naming_template"].toString("{product}-{version}").toStdString();
    m_defaultFormat = packageFormatFromString(root["default_format"].toString("ZIP").toStdString());
    m_includeHiddenFiles = root["include_hidden_files"].toBool(false);
    m_followSymlinks = root["follow_symlinks"].toBool(false);
    m_maxTotalBytes = static_cast<uint64_t>(root["max_total_bytes"].toInteger(10ULL * 1024 * 1024 * 1024));
    m_maxFileCount = static_cast<uint64_t>(root["max_file_count"].toInteger(500000));
    m_maxPerFileBytes = static_cast<uint64_t>(root["max_per_file_bytes"].toInteger(4ULL * 1024 * 1024 * 1024));

    if (root.contains("include_patterns") && root["include_patterns"].isArray()) {
        m_includePatterns.clear();
        for (const auto& item : root["include_patterns"].toArray()) {
            m_includePatterns.push_back(item.toString().toStdString());
        }
    }

    if (root.contains("exclude_patterns") && root["exclude_patterns"].isArray()) {
        m_excludePatterns.clear();
        for (const auto& item : root["exclude_patterns"].toArray()) {
            m_excludePatterns.push_back(item.toString().toStdString());
        }
    }
}

} // namespace shipdesk::domain