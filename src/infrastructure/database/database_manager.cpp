#include "infrastructure/database/database_manager.h"
#include <QSqlError>
#include <QSqlRecord>
#include <QVariant>
#include <QFileInfo>
#include <QDir>
#include <stdexcept>

namespace shipdesk::infrastructure {

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager s_instance;
    return s_instance;
}

DatabaseManager::~DatabaseManager() {
    close();
}

void DatabaseManager::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) {
        {
            auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
            if (db.isOpen()) {
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(QString::fromStdString(m_connectionName));
        m_initialized = false;
    }
}

bool DatabaseManager::isInitialized() const {
    return m_initialized;
}

bool DatabaseManager::initialize(const std::string& dbPath) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized) {
        close();
    }

    m_dbPath = dbPath;

    if (dbPath != ":memory:") {
        QFileInfo fi(QString::fromStdString(dbPath));
        QDir().mkpath(fi.absolutePath());
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", QString::fromStdString(m_connectionName));
    db.setDatabaseName(QString::fromStdString(dbPath));

    if (!db.open()) {
        return false;
    }

    // Set Pragmas
    QSqlQuery qPragma(db);
    qPragma.exec("PRAGMA foreign_keys = ON;");
    qPragma.exec("PRAGMA busy_timeout = 5000;");
    if (dbPath != ":memory:") {
        qPragma.exec("PRAGMA journal_mode = WAL;");
    }

    if (!runMigrations()) {
        db.close();
        return false;
    }

    m_initialized = true;
    return true;
}

bool DatabaseManager::runIntegrityCheck(std::string& outDetails) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return false;

    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery query(db);
    if (query.exec("PRAGMA integrity_check;") && query.next()) {
        outDetails = query.value(0).toString().toStdString();
        return outDetails == "ok";
    }
    outDetails = query.lastError().text().toStdString();
    return false;
}

bool DatabaseManager::runMigrations() {
    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery q(db);

    // Schema migrations table
    if (!q.exec("CREATE TABLE IF NOT EXISTS schema_migrations ("
                "version INTEGER PRIMARY KEY, "
                "applied_at_utc TEXT NOT NULL);")) {
        return false;
    }

    // Workspaces
    if (!q.exec("CREATE TABLE IF NOT EXISTS workspaces ("
                "id TEXT PRIMARY KEY, "
                "name TEXT NOT NULL, "
                "description TEXT, "
                "category TEXT NOT NULL, "
                "status TEXT NOT NULL, "
                "created_at_utc TEXT NOT NULL, "
                "updated_at_utc TEXT NOT NULL, "
                "archived_at_utc TEXT);")) {
        return false;
    }

    // Source Roots
    if (!q.exec("CREATE TABLE IF NOT EXISTS source_roots ("
                "id TEXT PRIMARY KEY, "
                "workspace_id TEXT NOT NULL REFERENCES workspaces(id) ON DELETE CASCADE, "
                "display_name TEXT NOT NULL, "
                "configured_path TEXT NOT NULL, "
                "path_kind TEXT NOT NULL, "
                "follow_symlinks INTEGER NOT NULL DEFAULT 0, "
                "created_at_utc TEXT NOT NULL);")) {
        return false;
    }

    // Release Profiles
    if (!q.exec("CREATE TABLE IF NOT EXISTS release_profiles ("
                "id TEXT PRIMARY KEY, "
                "workspace_id TEXT NOT NULL, "
                "name TEXT NOT NULL, "
                "category TEXT NOT NULL, "
                "schema_version TEXT NOT NULL, "
                "settings_json TEXT NOT NULL, "
                "created_at_utc TEXT NOT NULL, "
                "updated_at_utc TEXT NOT NULL);")) {
        return false;
    }

    // Build Profiles
    if (!q.exec("CREATE TABLE IF NOT EXISTS build_profiles ("
                "id TEXT PRIMARY KEY, "
                "workspace_id TEXT NOT NULL, "
                "name TEXT NOT NULL, "
                "configuration_json TEXT NOT NULL, "
                "approval_fingerprint TEXT, "
                "approval_expires_at_utc TEXT);")) {
        return false;
    }

    // Build Steps
    if (!q.exec("CREATE TABLE IF NOT EXISTS build_steps ("
                "id TEXT PRIMARY KEY, "
                "build_profile_id TEXT NOT NULL REFERENCES build_profiles(id) ON DELETE CASCADE, "
                "step_order INTEGER NOT NULL, "
                "executable TEXT NOT NULL, "
                "arguments_json TEXT NOT NULL, "
                "working_directory TEXT NOT NULL, "
                "environment_json TEXT NOT NULL, "
                "timeout_seconds INTEGER NOT NULL, "
                "enabled INTEGER NOT NULL DEFAULT 1);")) {
        return false;
    }

    // Release Drafts
    if (!q.exec("CREATE TABLE IF NOT EXISTS release_drafts ("
                "id TEXT PRIMARY KEY, "
                "workspace_id TEXT NOT NULL REFERENCES workspaces(id) ON DELETE CASCADE, "
                "profile_id TEXT NOT NULL, "
                "title TEXT NOT NULL, "
                "version TEXT NOT NULL, "
                "channel TEXT NOT NULL, "
                "description TEXT, "
                "release_notes TEXT, "
                "metadata_json TEXT NOT NULL, "
                "status TEXT NOT NULL, "
                "created_at_utc TEXT NOT NULL, "
                "updated_at_utc TEXT NOT NULL);")) {
        return false;
    }

    // Release Runs
    if (!q.exec("CREATE TABLE IF NOT EXISTS release_runs ("
                "id TEXT PRIMARY KEY, "
                "release_draft_id TEXT NOT NULL REFERENCES release_drafts(id) ON DELETE CASCADE, "
                "status TEXT NOT NULL, "
                "source_snapshot_json TEXT NOT NULL, "
                "profile_snapshot_json TEXT NOT NULL, "
                "started_at_utc TEXT NOT NULL, "
                "finished_at_utc TEXT, "
                "summary_json TEXT, "
                "error_json TEXT);")) {
        return false;
    }

    // Staged Files
    if (!q.exec("CREATE TABLE IF NOT EXISTS staged_files ("
                "id TEXT PRIMARY KEY, "
                "release_run_id TEXT NOT NULL REFERENCES release_runs(id) ON DELETE CASCADE, "
                "relative_path TEXT NOT NULL, "
                "staged_path TEXT NOT NULL, "
                "size_bytes INTEGER NOT NULL, "
                "hash_algorithm TEXT NOT NULL, "
                "hash_value TEXT NOT NULL, "
                "source_modified_at_utc TEXT NOT NULL, "
                "status TEXT NOT NULL);")) {
        return false;
    }

    // Artifacts
    if (!q.exec("CREATE TABLE IF NOT EXISTS artifacts ("
                "id TEXT PRIMARY KEY, "
                "release_run_id TEXT NOT NULL REFERENCES release_runs(id) ON DELETE CASCADE, "
                "filename TEXT NOT NULL, "
                "local_path TEXT NOT NULL, "
                "format TEXT NOT NULL, "
                "size_bytes INTEGER NOT NULL, "
                "hash_algorithm TEXT NOT NULL, "
                "hash_value TEXT NOT NULL, "
                "verification_status TEXT NOT NULL, "
                "created_at_utc TEXT NOT NULL);")) {
        return false;
    }

    // Destinations
    if (!q.exec("CREATE TABLE IF NOT EXISTS destinations ("
                "id TEXT PRIMARY KEY, "
                "workspace_id TEXT NOT NULL REFERENCES workspaces(id) ON DELETE CASCADE, "
                "kind TEXT NOT NULL, "
                "display_name TEXT NOT NULL, "
                "safe_configuration_json TEXT NOT NULL, "
                "created_at_utc TEXT NOT NULL);")) {
        return false;
    }

    // Publish Attempts
    if (!q.exec("CREATE TABLE IF NOT EXISTS publish_attempts ("
                "id TEXT PRIMARY KEY, "
                "artifact_id TEXT NOT NULL REFERENCES artifacts(id) ON DELETE CASCADE, "
                "destination_id TEXT NOT NULL REFERENCES destinations(id) ON DELETE CASCADE, "
                "status TEXT NOT NULL, "
                "external_id TEXT, "
                "external_url TEXT, "
                "started_at_utc TEXT NOT NULL, "
                "finished_at_utc TEXT, "
                "result_json TEXT, "
                "error_json TEXT);")) {
        return false;
    }

    // Immutable Release Records
    if (!q.exec("CREATE TABLE IF NOT EXISTS release_records ("
                "id TEXT PRIMARY KEY, "
                "release_run_id TEXT NOT NULL REFERENCES release_runs(id), "
                "workspace_snapshot_json TEXT NOT NULL, "
                "metadata_snapshot_json TEXT NOT NULL, "
                "manifest_path TEXT NOT NULL, "
                "completed_at TEXT NOT NULL);")) {
        return false;
    }

    // Events Log
    if (!q.exec("CREATE TABLE IF NOT EXISTS events ("
                "id TEXT PRIMARY KEY, "
                "workspace_id TEXT, "
                "release_run_id TEXT, "
                "severity TEXT NOT NULL, "
                "event_type TEXT NOT NULL, "
                "message TEXT NOT NULL, "
                "safe_details_json TEXT, "
                "created_at_utc TEXT NOT NULL);")) {
        return false;
    }

    // Create indexes for high query performance
    q.exec("CREATE INDEX IF NOT EXISTS idx_workspaces_status ON workspaces(status);");
    q.exec("CREATE INDEX IF NOT EXISTS idx_source_roots_ws ON source_roots(workspace_id);");
    q.exec("CREATE INDEX IF NOT EXISTS idx_release_drafts_ws ON release_drafts(workspace_id);");
    q.exec("CREATE INDEX IF NOT EXISTS idx_release_runs_draft ON release_runs(release_draft_id);");
    q.exec("CREATE INDEX IF NOT EXISTS idx_artifacts_run ON artifacts(release_run_id);");
    q.exec("CREATE INDEX IF NOT EXISTS idx_events_ws ON events(workspace_id);");

    return true;
}

bool DatabaseManager::saveWorkspace(const domain::Workspace& ws) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return false;

    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery q(db);
    q.prepare("INSERT INTO workspaces (id, name, description, category, status, created_at_utc, updated_at_utc, archived_at_utc) "
              "VALUES (:id, :name, :desc, :cat, :status, :created, :updated, :archived) "
              "ON CONFLICT(id) DO UPDATE SET "
              "name=excluded.name, description=excluded.description, category=excluded.category, "
              "status=excluded.status, updated_at_utc=excluded.updated_at_utc, archived_at_utc=excluded.archived_at_utc;");
    q.bindValue(":id", QString::fromStdString(ws.id()));
    q.bindValue(":name", QString::fromStdString(ws.name()));
    q.bindValue(":desc", QString::fromStdString(ws.description()));
    q.bindValue(":cat", QString::fromStdString(ws.category()));
    q.bindValue(":status", QString::fromStdString(ws.status()));
    q.bindValue(":created", QString::fromStdString(ws.createdAtUtc()));
    q.bindValue(":updated", QString::fromStdString(ws.updatedAtUtc()));
    q.bindValue(":archived", QString::fromStdString(ws.archivedAtUtc()));

    return q.exec();
}

std::optional<domain::Workspace> DatabaseManager::getWorkspace(const std::string& id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return std::nullopt;

    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery q(db);
    q.prepare("SELECT id, name, description, category, status, created_at_utc, updated_at_utc, archived_at_utc "
              "FROM workspaces WHERE id = :id;");
    q.bindValue(":id", QString::fromStdString(id));

    if (q.exec() && q.next()) {
        domain::Workspace ws(
            q.value(0).toString().toStdString(),
            q.value(1).toString().toStdString(),
            q.value(2).toString().toStdString(),
            q.value(3).toString().toStdString()
        );
        ws.setTimestamps(
            q.value(5).toString().toStdString(),
            q.value(6).toString().toStdString(),
            q.value(7).toString().toStdString()
        );

        // Load source roots for this workspace
        QSqlQuery qRoots(db);
        qRoots.prepare("SELECT id, workspace_id, display_name, configured_path, path_kind, follow_symlinks, created_at_utc "
                       "FROM source_roots WHERE workspace_id = :wsId;");
        qRoots.bindValue(":wsId", QString::fromStdString(id));
        if (qRoots.exec()) {
            while (qRoots.next()) {
                domain::SourceRoot root;
                root.id = qRoots.value(0).toString().toStdString();
                root.workspace_id = qRoots.value(1).toString().toStdString();
                root.display_name = qRoots.value(2).toString().toStdString();
                root.configured_path = qRoots.value(3).toString().toStdString();
                root.path_kind = qRoots.value(4).toString().toStdString();
                root.follow_symlinks = qRoots.value(5).toInt() != 0;
                root.created_at_utc = qRoots.value(6).toString().toStdString();
                ws.addSourceRoot(root);
            }
        }
        return ws;
    }
    return std::nullopt;
}

std::vector<domain::Workspace> DatabaseManager::listWorkspaces(bool includeArchived) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<domain::Workspace> result;
    if (!m_initialized) return result;

    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery q(db);
    QString queryStr = "SELECT id, name, description, category, status, created_at_utc, updated_at_utc, archived_at_utc FROM workspaces";
    if (!includeArchived) {
        queryStr += " WHERE status != 'Archived'";
    }
    queryStr += " ORDER BY updated_at_utc DESC;";

    if (q.exec(queryStr)) {
        while (q.next()) {
            domain::Workspace ws(
                q.value(0).toString().toStdString(),
                q.value(1).toString().toStdString(),
                q.value(2).toString().toStdString(),
                q.value(3).toString().toStdString()
            );
            ws.setTimestamps(
                q.value(5).toString().toStdString(),
                q.value(6).toString().toStdString(),
                q.value(7).toString().toStdString()
            );
            result.push_back(ws);
        }
    }
    return result;
}

bool DatabaseManager::deleteWorkspace(const std::string& id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return false;

    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery q(db);
    q.prepare("DELETE FROM workspaces WHERE id = :id;");
    q.bindValue(":id", QString::fromStdString(id));
    return q.exec();
}

bool DatabaseManager::saveSourceRoot(const domain::SourceRoot& root) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return false;

    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery q(db);
    q.prepare("INSERT INTO source_roots (id, workspace_id, display_name, configured_path, path_kind, follow_symlinks, created_at_utc) "
              "VALUES (:id, :wsId, :disp, :path, :kind, :symlinks, :created) "
              "ON CONFLICT(id) DO UPDATE SET "
              "display_name=excluded.display_name, configured_path=excluded.configured_path, "
              "path_kind=excluded.path_kind, follow_symlinks=excluded.follow_symlinks;");
    q.bindValue(":id", QString::fromStdString(root.id));
    q.bindValue(":wsId", QString::fromStdString(root.workspace_id));
    q.bindValue(":disp", QString::fromStdString(root.display_name));
    q.bindValue(":path", QString::fromStdString(root.configured_path));
    q.bindValue(":kind", QString::fromStdString(root.path_kind));
    q.bindValue(":symlinks", root.follow_symlinks ? 1 : 0);
    q.bindValue(":created", QString::fromStdString(root.created_at_utc));
    return q.exec();
}

std::vector<domain::SourceRoot> DatabaseManager::getSourceRoots(const std::string& workspaceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<domain::SourceRoot> roots;
    if (!m_initialized) return roots;

    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery q(db);
    q.prepare("SELECT id, workspace_id, display_name, configured_path, path_kind, follow_symlinks, created_at_utc "
              "FROM source_roots WHERE workspace_id = :wsId;");
    q.bindValue(":wsId", QString::fromStdString(workspaceId));

    if (q.exec()) {
        while (q.next()) {
            domain::SourceRoot r;
            r.id = q.value(0).toString().toStdString();
            r.workspace_id = q.value(1).toString().toStdString();
            r.display_name = q.value(2).toString().toStdString();
            r.configured_path = q.value(3).toString().toStdString();
            r.path_kind = q.value(4).toString().toStdString();
            r.follow_symlinks = q.value(5).toInt() != 0;
            r.created_at_utc = q.value(6).toString().toStdString();
            roots.push_back(r);
        }
    }
    return roots;
}

bool DatabaseManager::deleteSourceRoot(const std::string& rootId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return false;

    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery q(db);
    q.prepare("DELETE FROM source_roots WHERE id = :id;");
    q.bindValue(":id", QString::fromStdString(rootId));
    return q.exec();
}

bool DatabaseManager::saveReleaseProfile(const domain::ReleaseProfile& profile) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return false;

    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery q(db);
    q.prepare("INSERT INTO release_profiles (id, workspace_id, name, category, schema_version, settings_json, created_at_utc, updated_at_utc) "
              "VALUES (:id, :wsId, :name, :cat, :schema, :settings, :created, :updated) "
              "ON CONFLICT(id) DO UPDATE SET "
              "name=excluded.name, category=excluded.category, schema_version=excluded.schema_version, "
              "settings_json=excluded.settings_json, updated_at_utc=excluded.updated_at_utc;");
    q.bindValue(":id", QString::fromStdString(profile.id()));
    q.bindValue(":wsId", QString::fromStdString(profile.workspaceId()));
    q.bindValue(":name", QString::fromStdString(profile.name()));
    q.bindValue(":cat", QString::fromStdString(profile.category()));
    q.bindValue(":schema", QString::fromStdString(profile.schemaVersion()));
    q.bindValue(":settings", QString::fromStdString(profile.serializeSettingsJson()));
    q.bindValue(":created", QString::fromStdString(domain::currentUtcTimestamp()));
    q.bindValue(":updated", QString::fromStdString(domain::currentUtcTimestamp()));
    return q.exec();
}

std::optional<domain::ReleaseProfile> DatabaseManager::getReleaseProfile(const std::string& id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return std::nullopt;

    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery q(db);
    q.prepare("SELECT id, workspace_id, name, category, schema_version, settings_json FROM release_profiles WHERE id = :id;");
    q.bindValue(":id", QString::fromStdString(id));

    if (q.exec() && q.next()) {
        domain::ReleaseProfile p(
            q.value(0).toString().toStdString(),
            q.value(1).toString().toStdString(),
            q.value(2).toString().toStdString(),
            q.value(3).toString().toStdString()
        );
        p.deserializeSettingsJson(q.value(5).toString().toStdString());
        return p;
    }
    return std::nullopt;
}

std::vector<domain::ReleaseProfile> DatabaseManager::listReleaseProfiles(const std::string& workspaceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<domain::ReleaseProfile> list;
    if (!m_initialized) return list;

    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery q(db);
    q.prepare("SELECT id, workspace_id, name, category, schema_version, settings_json FROM release_profiles WHERE workspace_id = :wsId OR workspace_id = '';");
    q.bindValue(":wsId", QString::fromStdString(workspaceId));

    if (q.exec()) {
        while (q.next()) {
            domain::ReleaseProfile p(
                q.value(0).toString().toStdString(),
                q.value(1).toString().toStdString(),
                q.value(2).toString().toStdString(),
                q.value(3).toString().toStdString()
            );
            p.deserializeSettingsJson(q.value(5).toString().toStdString());
            list.push_back(p);
        }
    }
    return list;
}

bool DatabaseManager::saveReleaseDraft(const domain::ReleaseDraft& draft) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return false;

    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery q(db);
    q.prepare("INSERT INTO release_drafts (id, workspace_id, profile_id, title, version, channel, description, release_notes, metadata_json, status, created_at_utc, updated_at_utc) "
              "VALUES (:id, :wsId, :profId, :title, :ver, :ch, :desc, :notes, :meta, :status, :created, :updated) "
              "ON CONFLICT(id) DO UPDATE SET "
              "title=excluded.title, version=excluded.version, channel=excluded.channel, "
              "description=excluded.description, release_notes=excluded.release_notes, "
              "metadata_json=excluded.metadata_json, status=excluded.status, updated_at_utc=excluded.updated_at_utc;");
    q.bindValue(":id", QString::fromStdString(draft.id()));
    q.bindValue(":wsId", QString::fromStdString(draft.workspaceId()));
    q.bindValue(":profId", QString::fromStdString(draft.profileId()));
    q.bindValue(":title", QString::fromStdString(draft.title()));
    q.bindValue(":ver", QString::fromStdString(draft.version()));
    q.bindValue(":ch", QString::fromStdString(domain::releaseChannelToString(draft.channel())));
    q.bindValue(":desc", QString::fromStdString(draft.description()));
    q.bindValue(":notes", QString::fromStdString(draft.releaseNotes()));
    q.bindValue(":meta", QString::fromStdString(draft.metadataJson()));
    q.bindValue(":status", QString::fromStdString(domain::releaseStateToString(draft.state())));
    q.bindValue(":created", QString::fromStdString(draft.createdAtUtc()));
    q.bindValue(":updated", QString::fromStdString(draft.updatedAtUtc()));
    return q.exec();
}

std::optional<domain::ReleaseDraft> DatabaseManager::getReleaseDraft(const std::string& id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return std::nullopt;

    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery q(db);
    q.prepare("SELECT id, workspace_id, profile_id, title, version, channel, description, release_notes, metadata_json, status, created_at_utc, updated_at_utc "
              "FROM release_drafts WHERE id = :id;");
    q.bindValue(":id", QString::fromStdString(id));

    if (q.exec() && q.next()) {
        domain::ReleaseDraft d(
            q.value(0).toString().toStdString(),
            q.value(1).toString().toStdString(),
            q.value(2).toString().toStdString(),
            q.value(3).toString().toStdString(),
            q.value(4).toString().toStdString(),
            domain::releaseChannelFromString(q.value(5).toString().toStdString())
        );
        d.setDescription(q.value(6).toString().toStdString());
        d.setReleaseNotes(q.value(7).toString().toStdString());
        d.setMetadataJson(q.value(8).toString().toStdString());
        d.setStateDirect(domain::releaseStateFromString(q.value(9).toString().toStdString()));
        d.setTimestamps(q.value(10).toString().toStdString(), q.value(11).toString().toStdString());
        return d;
    }
    return std::nullopt;
}

std::vector<domain::ReleaseDraft> DatabaseManager::listReleaseDrafts(const std::string& workspaceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<domain::ReleaseDraft> drafts;
    if (!m_initialized) return drafts;

    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery q(db);
    q.prepare("SELECT id, workspace_id, profile_id, title, version, channel, description, release_notes, metadata_json, status, created_at_utc, updated_at_utc "
              "FROM release_drafts WHERE workspace_id = :wsId ORDER BY updated_at_utc DESC;");
    q.bindValue(":wsId", QString::fromStdString(workspaceId));

    if (q.exec()) {
        while (q.next()) {
            domain::ReleaseDraft d(
                q.value(0).toString().toStdString(),
                q.value(1).toString().toStdString(),
                q.value(2).toString().toStdString(),
                q.value(3).toString().toStdString(),
                q.value(4).toString().toStdString(),
                domain::releaseChannelFromString(q.value(5).toString().toStdString())
            );
            d.setDescription(q.value(6).toString().toStdString());
            d.setReleaseNotes(q.value(7).toString().toStdString());
            d.setMetadataJson(q.value(8).toString().toStdString());
            d.setStateDirect(domain::releaseStateFromString(q.value(9).toString().toStdString()));
            d.setTimestamps(q.value(10).toString().toStdString(), q.value(11).toString().toStdString());
            drafts.push_back(d);
        }
    }
    return drafts;
}

bool DatabaseManager::saveReleaseRun(const domain::ReleaseRunRecord& run) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return false;

    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery q(db);
    q.prepare("INSERT INTO release_runs (id, release_draft_id, status, source_snapshot_json, profile_snapshot_json, started_at_utc, finished_at_utc, summary_json, error_json) "
              "VALUES (:id, :draftId, :status, :src, :prof, :started, :finished, :summary, :err) "
              "ON CONFLICT(id) DO UPDATE SET "
              "status=excluded.status, finished_at_utc=excluded.finished_at_utc, summary_json=excluded.summary_json, error_json=excluded.error_json;");
    q.bindValue(":id", QString::fromStdString(run.id));
    q.bindValue(":draftId", QString::fromStdString(run.release_draft_id));
    q.bindValue(":status", QString::fromStdString(domain::releaseStateToString(run.status)));
    q.bindValue(":src", QString::fromStdString(run.source_snapshot_json));
    q.bindValue(":prof", QString::fromStdString(run.profile_snapshot_json));
    q.bindValue(":started", QString::fromStdString(run.started_at_utc));
    q.bindValue(":finished", QString::fromStdString(run.finished_at_utc));
    q.bindValue(":summary", QString::fromStdString(run.summary_json));
    q.bindValue(":err", QString::fromStdString(run.error_json));
    return q.exec();
}

std::optional<domain::ReleaseRunRecord> DatabaseManager::getReleaseRun(const std::string& id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return std::nullopt;

    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery q(db);
    q.prepare("SELECT id, release_draft_id, status, source_snapshot_json, profile_snapshot_json, started_at_utc, finished_at_utc, summary_json, error_json "
              "FROM release_runs WHERE id = :id;");
    q.bindValue(":id", QString::fromStdString(id));

    if (q.exec() && q.next()) {
        domain::ReleaseRunRecord r;
        r.id = q.value(0).toString().toStdString();
        r.release_draft_id = q.value(1).toString().toStdString();
        r.status = domain::releaseStateFromString(q.value(2).toString().toStdString());
        r.source_snapshot_json = q.value(3).toString().toStdString();
        r.profile_snapshot_json = q.value(4).toString().toStdString();
        r.started_at_utc = q.value(5).toString().toStdString();
        r.finished_at_utc = q.value(6).toString().toStdString();
        r.summary_json = q.value(7).toString().toStdString();
        r.error_json = q.value(8).toString().toStdString();
        return r;
    }
    return std::nullopt;
}

std::vector<domain::ReleaseRunRecord> DatabaseManager::listReleaseRuns(const std::string& releaseDraftId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<domain::ReleaseRunRecord> runs;
    if (!m_initialized) return runs;

    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery q(db);
    q.prepare("SELECT id, release_draft_id, status, source_snapshot_json, profile_snapshot_json, started_at_utc, finished_at_utc, summary_json, error_json "
              "FROM release_runs WHERE release_draft_id = :draftId ORDER BY started_at_utc DESC;");
    q.bindValue(":draftId", QString::fromStdString(releaseDraftId));

    if (q.exec()) {
        while (q.next()) {
            domain::ReleaseRunRecord r;
            r.id = q.value(0).toString().toStdString();
            r.release_draft_id = q.value(1).toString().toStdString();
            r.status = domain::releaseStateFromString(q.value(2).toString().toStdString());
            r.source_snapshot_json = q.value(3).toString().toStdString();
            r.profile_snapshot_json = q.value(4).toString().toStdString();
            r.started_at_utc = q.value(5).toString().toStdString();
            r.finished_at_utc = q.value(6).toString().toStdString();
            r.summary_json = q.value(7).toString().toStdString();
            r.error_json = q.value(8).toString().toStdString();
            runs.push_back(r);
        }
    }
    return runs;
}

bool DatabaseManager::saveReleaseRecord(const domain::ImmutableReleaseRecord& record) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return false;

    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery q(db);
    q.prepare("INSERT INTO release_records (id, release_run_id, workspace_snapshot_json, metadata_snapshot_json, manifest_path, completed_at) "
              "VALUES (:id, :runId, :ws, :meta, :manifest, :completed);");
    q.bindValue(":id", QString::fromStdString(record.id));
    q.bindValue(":runId", QString::fromStdString(record.release_run_id));
    q.bindValue(":ws", QString::fromStdString(record.workspace_snapshot_json));
    q.bindValue(":meta", QString::fromStdString(record.metadata_snapshot_json));
    q.bindValue(":manifest", QString::fromStdString(record.manifest_path));
    q.bindValue(":completed", QString::fromStdString(record.completed_at));
    return q.exec();
}

std::optional<domain::ImmutableReleaseRecord> DatabaseManager::getReleaseRecord(const std::string& id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return std::nullopt;

    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery q(db);
    q.prepare("SELECT id, release_run_id, workspace_snapshot_json, metadata_snapshot_json, manifest_path, completed_at "
              "FROM release_records WHERE id = :id;");
    q.bindValue(":id", QString::fromStdString(id));

    if (q.exec() && q.next()) {
        domain::ImmutableReleaseRecord r;
        r.id = q.value(0).toString().toStdString();
        r.release_run_id = q.value(1).toString().toStdString();
        r.workspace_snapshot_json = q.value(2).toString().toStdString();
        r.metadata_snapshot_json = q.value(3).toString().toStdString();
        r.manifest_path = q.value(4).toString().toStdString();
        r.completed_at = q.value(5).toString().toStdString();
        return r;
    }
    return std::nullopt;
}

std::vector<domain::ImmutableReleaseRecord> DatabaseManager::listReleaseRecords() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<domain::ImmutableReleaseRecord> list;
    if (!m_initialized) return list;

    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery q(db);
    if (q.exec("SELECT id, release_run_id, workspace_snapshot_json, metadata_snapshot_json, manifest_path, completed_at "
               "FROM release_records ORDER BY completed_at DESC;")) {
        while (q.next()) {
            domain::ImmutableReleaseRecord r;
            r.id = q.value(0).toString().toStdString();
            r.release_run_id = q.value(1).toString().toStdString();
            r.workspace_snapshot_json = q.value(2).toString().toStdString();
            r.metadata_snapshot_json = q.value(3).toString().toStdString();
            r.manifest_path = q.value(4).toString().toStdString();
            r.completed_at = q.value(5).toString().toStdString();
            list.push_back(r);
        }
    }
    return list;
}

bool DatabaseManager::logEvent(const domain::EventLogRecord& event) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return false;

    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery q(db);
    q.prepare("INSERT INTO events (id, workspace_id, release_run_id, severity, event_type, message, safe_details_json, created_at_utc) "
              "VALUES (:id, :wsId, :runId, :sev, :type, :msg, :details, :created);");
    q.bindValue(":id", QString::fromStdString(event.id));
    q.bindValue(":wsId", QString::fromStdString(event.workspace_id));
    q.bindValue(":runId", QString::fromStdString(event.release_run_id));
    q.bindValue(":sev", QString::fromStdString(event.severity));
    q.bindValue(":type", QString::fromStdString(event.event_type));
    q.bindValue(":msg", QString::fromStdString(event.message));
    q.bindValue(":details", QString::fromStdString(event.safe_details_json));
    q.bindValue(":created", QString::fromStdString(event.created_at_utc));
    return q.exec();
}

std::vector<domain::EventLogRecord> DatabaseManager::getEvents(const std::string& workspaceId, size_t limit) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<domain::EventLogRecord> list;
    if (!m_initialized) return list;

    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery q(db);
    QString qStr = "SELECT id, workspace_id, release_run_id, severity, event_type, message, safe_details_json, created_at_utc FROM events ";
    if (!workspaceId.empty()) {
        qStr += "WHERE workspace_id = :wsId ";
    }
    qStr += "ORDER BY created_at_utc DESC LIMIT " + QString::number(limit) + ";";

    q.prepare(qStr);
    if (!workspaceId.empty()) {
        q.bindValue(":wsId", QString::fromStdString(workspaceId));
    }

    if (q.exec()) {
        while (q.next()) {
            domain::EventLogRecord e;
            e.id = q.value(0).toString().toStdString();
            e.workspace_id = q.value(1).toString().toStdString();
            e.release_run_id = q.value(2).toString().toStdString();
            e.severity = q.value(3).toString().toStdString();
            e.event_type = q.value(4).toString().toStdString();
            e.message = q.value(5).toString().toStdString();
            e.safe_details_json = q.value(6).toString().toStdString();
            e.created_at_utc = q.value(7).toString().toStdString();
            list.push_back(e);
        }
    }
    return list;
}

int DatabaseManager::recoverInterruptedRuns() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return 0;

    auto db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    QSqlQuery q(db);

    // Any run left in transient states must be marked 'Interrupted'
    QString updateRunsQuery =
        "UPDATE release_runs SET status = 'Interrupted', "
        "error_json = '{\"recovery\": \"Marked Interrupted during application startup recovery\"}' "
        "WHERE status IN ('Inspecting', 'Validating', 'Building', 'Staging', 'Packaging', 'Verifying', 'Publishing');";

    int affected = 0;
    if (q.exec(updateRunsQuery)) {
        affected = q.numRowsAffected();
    }

    // Any draft left in transient states must also be marked 'Interrupted'
    QString updateDraftsQuery =
        "UPDATE release_drafts SET status = 'Interrupted' "
        "WHERE status IN ('Inspecting', 'Validating', 'Building', 'Staging', 'Packaging', 'Verifying', 'Publishing');";
    q.exec(updateDraftsQuery);

    return affected;
}

} // namespace shipdesk::infrastructure
