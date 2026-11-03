# Privacy, Storage, and Retention Policy

## 1. Storage Locations
ShipDesk persists all local state within standard user configuration and app data directories:
- **Windows**: `%LOCALAPPDATA%\ShipDesk\shipdesk.db`
- **Linux**: `~/.local/share/ShipDesk/shipdesk.db`
- **Staging Root**: Defaults to `<AppDataLocation>/staging` (configurable in Settings).

## 2. Managed Staging Boundary & Ownership Verification
When files are staged for packaging:
1. ShipDesk generates a unique staging folder `stage_<RUN_ID>`.
2. Writes an ownership marker file `shipdesk.owner` containing:
   ```json
   {
     "job_id": "<RUN_ID>",
     "created_at": "<ISO_8601_TIMESTAMP>",
     "owner": "ShipDesk Staging Engine"
   }
   ```
3. During cleanup, ShipDesk reads `shipdesk.owner`. If the marker is missing or contains a mismatched job ID, **the directory is never deleted** and is retained for user review.

## 3. Database Retention & Deletion Policy
- Deleting a workspace removes its metadata records from SQLite using `ON DELETE CASCADE`.
- Deleting a workspace **never deletes or modifies your source files** on disk.
- Historical release records are immutable. Artifact files written to your designated release folder are never deleted automatically.

## 4. Zero Remote Tracking
- ShipDesk contains zero telemetry, analytics, or background reporting mechanisms.
- All operations (scan, stage, hash, package, verify) operate completely offline.
- Remote connections occur strictly upon explicit user confirmation in the publishing dialog.
