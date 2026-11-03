# ShipDesk Validation Profiles & Rules Engine

ShipDesk evaluates deliverables against formal rules to catch errors before staging and packaging.

## Severity Levels
- **Info**: Helpful suggestions or best practice recommendations (e.g. recommending a README or LICENSE).
- **Warning**: Potential issues that do not strictly prevent packaging (e.g. large file size, case collision on Windows, inclusion of hidden files).
- **Error**: Violations that exceed hard profile limits (e.g. total byte ceiling exceeded, file count ceiling exceeded).
- **Blocker**: Critical security or integrity violations that strictly prohibit packaging or publishing (e.g. empty inventory, path traversal attempts, duplicate normalized output paths).

## Core Validation Rules

| Rule ID | Rule Title | Default Severity | Description |
|:---|:---|:---|:---|
| `VAL-001` | No Files Selected | **Blocker** | Fired when inventory contains zero included regular files. |
| `VAL-002` | Total Size Exceeds Limit | **Error** | Fired when total uncompressed bytes exceed profile `maxTotalBytes`. |
| `VAL-003` | Large File Exceeds Limit | **Warning** | Fired when an individual file exceeds `maxPerFileBytes`. |
| `VAL-004` | File Count Limit Exceeded | **Error** | Fired when file count exceeds profile `maxFileCount`. |
| `VAL-005` | Hidden File Included | **Warning** | Fired when dotfiles or hidden files are included without explicit opt-in. |
| `VAL-006` | Broken Symlink Detected | **Error** | Fired when a symlink target does not exist. |
| `VAL-007` | Unsafe Path Traversal | **Blocker** | Fired when relative paths contain `..`, drive letters, or escape roots. |
| `VAL-008` | Duplicate Output Path | **Blocker** | Fired when two files resolve to the same normalized destination path. |
| `VAL-009` | Case-Only Path Collision | **Warning** | Fired when paths differ only by character casing (risk on Windows/macOS). |
| `VAL-010-README` | README Recommendation | **Info** | Advises providing documentation for software releases. |
| `VAL-010-LICENSE` | License Recommendation | **Info** | Advises clarifying usage rights for software distributions. |
| `VAL-011` | Version Label Required | **Blocker** | Fired when release version string is empty. |
