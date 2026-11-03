# Troubleshooting & FAQ

## 1. Missing Qt DLLs on Windows
**Symptom**: `The code execution cannot proceed because Qt6Core.dll was not found.`
**Solution**: Ensure `C:\msys64\ucrt64\bin` is added to your environment `PATH`, or run the bundled portable distribution which includes all deployed Qt DLLs alongside `shipdesk.exe`.

## 2. SQLite Database Locked (`database is locked` / `busy`)
**Symptom**: Multi-process contention or crash left SQLite in a locked state.
**Solution**:
1. ShipDesk sets `PRAGMA busy_timeout = 5000;` and uses Write-Ahead Logging (`WAL`).
2. Close any external database browser holding an active write lock.
3. If an ungraceful shutdown occurred, delete `.db-shm` or `.db-wal` temporary journal files after terminating any lingering process.

## 3. GitHub CLI Not Authenticated
**Symptom**: "GitHub CLI ('gh') is not installed or not in PATH" or "Not Authenticated".
**Solution**:
1. Install GitHub CLI from https://cli.github.com/
2. Open terminal and run `gh auth login`.
3. ShipDesk will automatically detect the active account session without asking for your token.

## 4. Path Traversal Blocked (`VAL-007`)
**Symptom**: Validation shows blocker: "Unsafe Relative Path Traversal".
**Solution**: One or more files in the source tree contain relative links pointing outside the root (`..`) or contain absolute path specifiers. Remove or exclude these files from the release inventory.

## 5. Startup Interrupted Jobs
**Symptom**: On launch, console logs: "Startup recovery: Marked 1 abandoned transient run(s) as Interrupted."
**Solution**: This is a core safety feature. If the application closed unexpectedly during staging, packaging, or publishing, ShipDesk prevents corrupted states by safely transitioning the run to `Interrupted` and preserving all verified artifacts on disk.
