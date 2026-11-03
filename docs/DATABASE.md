# ShipDesk Database Design

ShipDesk utilizes a local SQLite 3 database to persist workspaces, release profiles, drafts, runs, staged items, artifacts, immutable release records, and audit events.

## SQLite Pragmas
Every database connection executes:
- `PRAGMA foreign_keys = ON;` (Strict referential integrity)
- `PRAGMA busy_timeout = 5000;` (Handles concurrent locking gracefully)
- `PRAGMA journal_mode = WAL;` (Write-Ahead Logging for high performance and durability)

## Table Schemas

### 1. `workspaces`
- `id`: TEXT PRIMARY KEY (UUID)
- `name`: TEXT NOT NULL
- `description`: TEXT
- `category`: TEXT NOT NULL
- `status`: TEXT NOT NULL ("Active", "Archived")
- `created_at_utc`: TEXT NOT NULL (ISO 8601)
- `updated_at_utc`: TEXT NOT NULL
- `archived_at_utc`: TEXT

### 2. `source_roots`
- `id`: TEXT PRIMARY KEY
- `workspace_id`: TEXT NOT NULL REFERENCES workspaces(id) ON DELETE CASCADE
- `display_name`: TEXT NOT NULL
- `configured_path`: TEXT NOT NULL
- `path_kind`: TEXT NOT NULL ("Directory" or "File")
- `follow_symlinks`: INTEGER NOT NULL DEFAULT 0
- `created_at_utc`: TEXT NOT NULL

### 3. `release_profiles`
- `id`: TEXT PRIMARY KEY
- `workspace_id`: TEXT NOT NULL
- `name`: TEXT NOT NULL
- `category`: TEXT NOT NULL
- `schema_version`: TEXT NOT NULL ("1.0.0")
- `settings_json`: TEXT NOT NULL (Inclusion/exclusion patterns, size limits, naming templates)
- `created_at_utc`: TEXT NOT NULL
- `updated_at_utc`: TEXT NOT NULL

### 4. `release_drafts`
- `id`: TEXT PRIMARY KEY
- `workspace_id`: TEXT NOT NULL REFERENCES workspaces(id) ON DELETE CASCADE
- `profile_id`: TEXT NOT NULL
- `title`: TEXT NOT NULL
- `version`: TEXT NOT NULL
- `channel`: TEXT NOT NULL ("Draft", "Preview", "Alpha", "Beta", "Stable")
- `description`: TEXT
- `release_notes`: TEXT
- `metadata_json`: TEXT NOT NULL
- `status`: TEXT NOT NULL (ReleaseState string)
- `created_at_utc`: TEXT NOT NULL
- `updated_at_utc`: TEXT NOT NULL

### 5. `release_runs`
- `id`: TEXT PRIMARY KEY
- `release_draft_id`: TEXT NOT NULL REFERENCES release_drafts(id) ON DELETE CASCADE
- `status`: TEXT NOT NULL
- `source_snapshot_json`: TEXT NOT NULL
- `profile_snapshot_json`: TEXT NOT NULL
- `started_at_utc`: TEXT NOT NULL
- `finished_at_utc`: TEXT
- `summary_json`: TEXT
- `error_json`: TEXT

### 6. `release_records` (Immutable History)
- `id`: TEXT PRIMARY KEY
- `release_run_id`: TEXT NOT NULL REFERENCES release_runs(id)
- `workspace_snapshot_json`: TEXT NOT NULL
- `metadata_snapshot_json`: TEXT NOT NULL
- `manifest_path`: TEXT NOT NULL
- `completed_at`: TEXT NOT NULL

### 7. `events` (Audit Timeline)
- `id`: TEXT PRIMARY KEY
- `workspace_id`: TEXT
- `release_run_id`: TEXT
- `severity`: TEXT NOT NULL ("Info", "Warning", "Error")
- `event_type`: TEXT NOT NULL
- `message`: TEXT NOT NULL
- `safe_details_json`: TEXT
- `created_at_utc`: TEXT NOT NULL

## Startup Recovery Policy
On startup, `DatabaseManager::recoverInterruptedRuns()` queries for any `release_runs` left in transient states (`Inspecting`, `Validating`, `Building`, `Staging`, `Packaging`, `Verifying`, `Publishing`) and transitions them to `Interrupted`. Completed artifacts on disk are preserved; build and publish operations are never re-run blindly.
