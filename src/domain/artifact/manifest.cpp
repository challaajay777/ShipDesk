#include "domain/artifact/manifest.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>

namespace shipdesk::domain {

std::string ReleaseManifest::toJsonString() const {
    QJsonObject root;
    root["format"] = QString::fromStdString(format);
    root["schema_version"] = QString::fromStdString(schema_version);
    root["product_name"] = QString::fromStdString(product_name);
    root["workspace_id"] = QString::fromStdString(workspace_id);
    root["release_id"] = QString::fromStdString(release_id);
    root["version"] = QString::fromStdString(version);
    root["channel"] = QString::fromStdString(channel);
    root["created_at_utc"] = QString::fromStdString(created_at_utc);
    root["platform"] = QString::fromStdString(platform);

    if (!provenance_reference.empty()) {
        root["provenance_reference"] = QString::fromStdString(provenance_reference);
    }

    // Embed JSON snapshots
    auto parseSnapshot = [](const std::string& json) -> QJsonValue {
        if (json.empty() || json == "{}") return QJsonObject();
        QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromRawData(json.data(), static_cast<qsizetype>(json.size())));
        if (doc.isObject()) return doc.object();
        if (doc.isArray()) return doc.array();
        return QString::fromStdString(json);
    };

    root["source_snapshot"] = parseSnapshot(source_snapshot_json);
    root["profile_snapshot"] = parseSnapshot(profile_snapshot_json);
    root["validation_summary"] = parseSnapshot(validation_summary_json);
    if (!build_summary_json.empty() && build_summary_json != "{}") {
        root["build_summary"] = parseSnapshot(build_summary_json);
    }
    if (!test_summary_json.empty() && test_summary_json != "{}") {
        root["test_summary"] = parseSnapshot(test_summary_json);
    }

    // Artifacts array
    QJsonArray artArr;
    for (const auto& a : artifacts) {
        QJsonObject artObj;
        artObj["artifact_id"] = QString::fromStdString(a.artifact_id);
        artObj["filename"] = QString::fromStdString(a.filename);
        artObj["output_path"] = QString::fromStdString(a.output_path);
        artObj["size_bytes"] = static_cast<qint64>(a.size_bytes);
        artObj["sha256"] = QString::fromStdString(a.sha256);
        artObj["package_format"] = QString::fromStdString(a.package_format);
        artObj["created_at_utc"] = QString::fromStdString(a.created_at_utc);
        artArr.append(artObj);
    }
    root["artifacts"] = artArr;

    // Files array
    QJsonArray fileArr;
    for (const auto& f : files) {
        QJsonObject fileObj;
        fileObj["relative_path"] = QString::fromStdString(f.relative_path);
        fileObj["size_bytes"] = static_cast<qint64>(f.size_bytes);
        fileObj["sha256"] = QString::fromStdString(f.sha256);
        fileObj["is_executable"] = f.is_executable;
        fileObj["source_relative_path"] = QString::fromStdString(f.source_relative_path);
        fileObj["classification"] = QString::fromStdString(f.classification);
        fileObj["inclusion_reason"] = QString::fromStdString(f.inclusion_reason);
        fileArr.append(fileObj);
    }
    root["files"] = fileArr;

    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Indented).toStdString();
}

ReleaseManifest ReleaseManifest::fromJsonString(const std::string& json) {
    ReleaseManifest m;
    if (json.empty()) return m;

    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromRawData(json.data(), static_cast<qsizetype>(json.size())));
    if (!doc.isObject()) return m;

    QJsonObject root = doc.object();
    m.format = root["format"].toString("shipdesk-release-manifest").toStdString();
    m.schema_version = root["schema_version"].toString("1.0.0").toStdString();
    m.product_name = root["product_name"].toString().toStdString();
    m.workspace_id = root["workspace_id"].toString().toStdString();
    m.release_id = root["release_id"].toString().toStdString();
    m.version = root["version"].toString().toStdString();
    m.channel = root["channel"].toString("Draft").toStdString();
    m.created_at_utc = root["created_at_utc"].toString().toStdString();
    m.platform = root["platform"].toString().toStdString();
    m.provenance_reference = root["provenance_reference"].toString().toStdString();

    if (root.contains("source_snapshot")) {
        QJsonDocument subDoc(root["source_snapshot"].toObject());
        m.source_snapshot_json = subDoc.toJson(QJsonDocument::Compact).toStdString();
    }
    if (root.contains("profile_snapshot")) {
        QJsonDocument subDoc(root["profile_snapshot"].toObject());
        m.profile_snapshot_json = subDoc.toJson(QJsonDocument::Compact).toStdString();
    }
    if (root.contains("validation_summary")) {
        QJsonDocument subDoc(root["validation_summary"].toObject());
        m.validation_summary_json = subDoc.toJson(QJsonDocument::Compact).toStdString();
    }
    if (root.contains("build_summary")) {
        QJsonDocument subDoc(root["build_summary"].toObject());
        m.build_summary_json = subDoc.toJson(QJsonDocument::Compact).toStdString();
    }
    if (root.contains("test_summary")) {
        QJsonDocument subDoc(root["test_summary"].toObject());
        m.test_summary_json = subDoc.toJson(QJsonDocument::Compact).toStdString();
    }

    // Parse artifacts
    if (root.contains("artifacts") && root["artifacts"].isArray()) {
        QJsonArray artArr = root["artifacts"].toArray();
        for (const auto& val : artArr) {
            if (val.isObject()) {
                QJsonObject o = val.toObject();
                ManifestArtifactEntry a;
                a.artifact_id = o["artifact_id"].toString().toStdString();
                a.filename = o["filename"].toString().toStdString();
                a.output_path = o["output_path"].toString().toStdString();
                a.size_bytes = static_cast<uint64_t>(o["size_bytes"].toInteger());
                a.sha256 = o["sha256"].toString().toStdString();
                a.package_format = o["package_format"].toString().toStdString();
                a.created_at_utc = o["created_at_utc"].toString().toStdString();
                m.artifacts.push_back(a);
            }
        }
    }

    // Parse files
    if (root.contains("files") && root["files"].isArray()) {
        QJsonArray fileArr = root["files"].toArray();
        for (const auto& val : fileArr) {
            if (val.isObject()) {
                QJsonObject o = val.toObject();
                ManifestFileEntry f;
                f.relative_path = o["relative_path"].toString().toStdString();
                f.size_bytes = static_cast<uint64_t>(o["size_bytes"].toInteger());
                f.sha256 = o["sha256"].toString().toStdString();
                f.is_executable = o["is_executable"].toBool(false);
                f.source_relative_path = o["source_relative_path"].toString().toStdString();
                f.classification = o["classification"].toString("regular").toStdString();
                f.inclusion_reason = o["inclusion_reason"].toString("profile_rule_or_user_selection").toStdString();
                m.files.push_back(f);
            }
        }
    }

    return m;
}

} // namespace shipdesk::domain