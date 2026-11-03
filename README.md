# ShipDesk

> **Prepare, check, package, verify, and deliver digital work without losing control of originals.**

ShipDesk is a native C++20 desktop workspace built with Qt 6 Widgets and SQLite. It provides a local-first, source-read-only release pipeline that turns digital work into structured, validated, packaged, verified, versioned, and deliverable releases.

```
+---------------------------------------------------------------------------------------------------+
|  Overview    Workspaces    Releases    Activity    Source Inspector    Package Verifier   Diagnostics |
|---------------------------------------------------------------------------------------------------|
|  Workspace: DevTools-v1.0  |  Deliverable Type: Software Application  |  Status: Verified & Ready |
|                                                                                                   |
|  [Source Roots]           --> [Validation Engine]     --> [Private Staging]  --> [Verification]   |
|   C:/Projects/App/src          VAL-001 Files Count: OK     shipdesk.owner         CRC32: PASS     |
|   (Strictly Read-Only)         VAL-007 Traversal: OK       SHA-256 Streamed       SHA-256: PASS   |
|                                VAL-008 Duplicates: OK      Zero Mutation Check    Manifest: PASS  |
|                                                                                                   |
|  [First-Class Outputs]                                    [Publishing Delivery]                   |
|   - Standard ZIP Archive (.zip)                            - Local Directory / Network Share      |
|   - Organized Directory Package                            - GitHub Releases via Official CLI (gh)|
+---------------------------------------------------------------------------------------------------+
```

---

## Key Capabilities

- **Strict Source Read-Only Boundary**: Inspection and staging treat original files as immutable. ShipDesk never modifies, deletes, moves, or changes permissions on source folders.
- **Guided 8-Step Release Wizard**: Step-by-step deliverable preparation for everyday creators, educators, students, and developers.
- **Managed Private Staging**: Safe copy-based staging with `shipdesk.owner` marker validation, short-write detection, disk-full protection, and mutation detection between inspection and copy.
- **Cryptographic Release Manifest**: Automatically generates schema-compliant `shipdesk-release-manifest.json` with per-file SHA-256 checksums, uncompressed sizes, and platform metadata.
- **Archive Verification Engine**: Verifies package integrity, compares actual entries against recorded manifests, checks CRC32 and SHA-256 digests, and detects missing or unexpected files.
- **Untrusted Command Execution Security**: Never auto-executes project scripts upon opening folders. Advanced mode command approval binds to exact executable, argument arrays, working directories, and environment fingerprints with secret redaction.
- **Local SQLite History**: Parameterized database with Write-Ahead Logging (WAL), foreign keys, and startup crash recovery that automatically marks interrupted jobs.
- **Local-First & Zero Telemetry**: Operates completely offline without tracking, analytics, or background uploads. Online publishing to GitHub Releases occurs only with explicit user confirmation.

---

## Supported Modes

### Guided Mode (Default)
1. **Deliverable Type**: Choose from 11 categories (*General File Bundle, Software Application, Document Bundle, Educational Resources, Media Collection, Source Code Release, Game Mod, Resource Pack, Plugin/Extension, Static Website Output, Custom*).
2. **Source Selection**: Browse local folders or link an existing workspace.
3. **Inventory & Inclusions**: Interactive tree/table with search filtering and inclusion checkboxes.
4. **Metadata & Versioning**: Configure title, semantic version, channel (*Draft, Preview, Alpha, Beta, Stable*), description, and release notes.
5. **Validation Engine**: Real-time checking against rule violations (empty packages, path traversal, file count limits, size limits, case collisions, missing README recommendations).
6. **Format & Destination**: Choose ZIP Archive or Directory package, and select output directory.
7. **Specification Review**: Review complete release parameters before writing output.
8. **Execute & Verify**: Watch real-time staging and packaging progress, followed by instant cryptographic verification.

### Advanced Mode
- Workspace configuration and multi-root source management.
- Git repository status inspection (branch, HEAD commit, dirty status, credential-redacted remotes).
- Command review and fingerprint approval for build orchestration.
- Standalone tools: **Source Inspector**, **Package Verifier**, and **System Diagnostics**.
- Publishing adapter for GitHub Releases via official GitHub CLI (`gh`).

---

## Tested Platforms & Verification Status

| Platform | Architecture | Compiler | Qt Version | Build Status | Test Suite Status |
|:---|:---|:---|:---|:---|:---|
| **Windows 11** | `x86_64` (AMD64) | GCC 16.2.0 (UCRT64 MinGW-w64) | Qt 6.11.2 | **Verified** | **20 / 20 PASS (100%)** |
| **Linux Desktop** | `x86_64` | GCC 11+ / Clang 14+ | Qt 6.4+ | Ready for Build | Portable ISO C++20 |
| **macOS** | Any | Apple Clang | Qt 6.4+ | *Future Target* | *Deferred until hardware test* |

> *Note on Platform Claims: In accordance with our honesty contract, macOS is strictly listed as a future platform until native compilation and runtime tests are executed on physical Apple silicon hardware.*

---

## Building from Source

### Prerequisites

- **C++ Compiler**: GCC 11+, Clang 14+, or MSVC 2019+ with full C++20 support.
- **CMake**: 3.24 or higher.
- **Ninja**: Fast build system.
- **Qt 6**: `Qt6Core`, `Qt6Gui`, `Qt6Widgets`, `Qt6Sql`, `Qt6Test`.
- **SQLite**: 3.35+ (standard Qt SQLite driver).

### Windows (MSYS2 UCRT64 / MinGW-w64)

```powershell
# Clone repository
git clone https://github.com/challaajay777/ShipDesk.git
cd ShipDesk

# Configure with CMake and Ninja
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/msys64/ucrt64"

# Build executable and test suite
cmake --build build

# Run automated tests
$env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH
ctest --test-dir build --output-on-failure
```

### Linux (Ubuntu 22.04+ / Fedora 38+)

```bash
sudo apt-get install -y build-essential cmake ninja-build qt6-base-dev qt6-base-dev-tools libsqlite3-dev
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

---

## Architecture & Codebase Structure

ShipDesk follows clean hexagonal/layered architectural boundaries:

```
src/
├── app/                        # Application entry point (main.cpp)
├── domain/                     # Pure C++20 domain logic (zero GUI/DB dependencies)
│   ├── workspace/              # Workspace and SourceRoot entities
│   ├── profile/                # ReleaseProfile and BuildProfile entities
│   ├── release/                # ReleaseDraft, State Machine, and ReleaseRecord
│   ├── validation/             # ValidationRule and ValidationFinding engine
│   ├── artifact/               # ReleaseManifest and VerificationReport
│   └── destination/            # Destination and PublishAttempt records
├── infrastructure/             # Concrete I/O and standard implementations
│   ├── database/               # SQLite DatabaseManager with WAL and migrations
│   ├── filesystem/             # PathUtils (containment, normalization, glob)
│   ├── hashing/                # Hasher (streaming SHA-256)
│   ├── archives/               # ZipArchiveService (miniz native ZIP engine)
│   ├── process/                # ProcessRunner (asynchronous QProcess with redaction)
│   └── git/                    # GitInspector (safe read-only inspection)
├── adapters/                   # Packaging and delivery channel adapters
│   └── destinations/           # LocalFolderDestination and GitHubDestination
└── ui/                         # Qt 6 Widgets presentation layer
    ├── windows/                # MainWindow with navigation sidebar and status bar
    ├── pages/                  # Overview, Workspaces, Releases, Activity, Tools, Settings
    ├── dialogs/                # ReleaseWizardDialog, PublishDialog, ApprovalDialog
    └── themes/                 # ThemeManager (clean light/dark native styling)
```

---

## Database Design & Storage

ShipDesk stores metadata in a local SQLite 3 database located in your standard application directory:
- **Windows**: `%LOCALAPPDATA%\ShipDesk\shipdesk.db`
- **Linux**: `~/.local/share/ShipDesk/shipdesk.db`

### Key Database Tables
- `workspaces`: Workspace entities and active/archived status.
- `source_roots`: Registered source directories and symlink policies.
- `release_profiles`: Configured inclusion/exclusion glob rules and size bounds.
- `release_drafts`: Release metadata, version labels, and state machine state.
- `release_runs`: Execution runs with immutable source snapshots and error logs.
- `staged_files`: Staged items with relative paths and SHA-256 hashes.
- `artifacts`: Packaged outputs with verification status and digests.
- `release_records`: Immutable historical release snapshots.
- `events`: Audit event log with automatic secret redaction.

---

## Security & Privacy Highlights

- **Source Read-Only**: Original source folders are never modified during any phase of execution.
- **No Script Execution on Open**: Opening a workspace or inspecting a directory never runs scripts.
- **Redaction Filter**: Masks GitHub tokens (`gho_`, `ghp_`), GitLab tokens, AWS keys, and passwords.
- **Zero Telemetry**: No crash-report transmission, analytics, or background pings.
- **Ownership Verification**: Staging directories verify ownership marker before deletion; unverified folders are retained for review.

---

## License

ShipDesk is open source software licensed under the [MIT License](LICENSE).
Copyright &copy; 2026 Challa Ajay.
