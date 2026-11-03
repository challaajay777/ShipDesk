# Privacy Policy

ShipDesk is built from the ground up as a **local-first** native desktop product.

## 1. Zero Telemetry & Tracking
- ShipDesk contains **no analytics, no telemetry, no tracking pixels, and no crash-report uploaders**.
- The application makes zero unsolicited network connections.
- Diagnostics and audit events are stored strictly on your local computer in your local SQLite database (`shipdesk.db`).

## 2. Local-First Operations
- All inspection, inclusion filtering, staging, archive creation, and cryptographic hashing take place entirely offline on your local CPU and storage devices.
- Manifest files and verification reports are written locally on your computer.

## 3. Remote Delivery & Publishing
- Network connections occur exclusively when you explicitly select a remote publishing destination (such as GitHub Releases) and confirm the action.
- Before publishing, you are shown an exact preview of the target account, repository, tag, title, notes, artifact filenames, and SHA-256 digests.
- Publication requires checking an explicit authorization box; ShipDesk will never publish automatically.

## 4. Local Data Storage & User Control
- All workspace configurations, profile settings, and release history records are stored in standard user app data directories:
  - Windows: `%LOCALAPPDATA%\ShipDesk\shipdesk.db`
  - Linux: `~/.local/share/ShipDesk/shipdesk.db`
- You retain complete ownership and control over your SQLite database and artifacts. Deleting a workspace removes its metadata from the database without touching your original files or generated release archives on disk.
