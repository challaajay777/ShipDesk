# Changelog

All notable changes to ShipDesk will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-11-03

### Added
- **Core Native Application**: C++20 desktop application built on Qt 6 Widgets and CMake.
- **Guided Release Wizard**: 8-step wizard supporting deliverable category selection, directory scanning, inclusion rules, versioning, validation checks, formatting, review, and packaging.
- **Source Read-Only Inspection Engine**: Recursive directory traversal that strictly preserves original source files and timestamps without side effects.
- **Managed Private Staging Boundary**: Safe copy-based staging with `shipdesk.owner` marker validation, path containment checks, and simultaneous SHA-256 calculation.
- **Packaging Engine**: Native Directory and ZIP archive creation with standard Deflate compression and CRC32 checks.
- **Cryptographic Verification**: Automated manifest generation (`shipdesk-release-manifest.json`) and SHA-256 verification of packages and individual content files.
- **Local SQLite History**: Parameterized database with WAL mode, foreign keys, and startup recovery for interrupted operations.
- **Process Runner & Secret Redaction**: Asynchronous non-blocking command execution with automated masking of GitHub tokens, GitLab tokens, AWS keys, and passwords.
- **Publishing Adapters**: Local folder delivery and official GitHub CLI (`gh`) release and asset publishing with explicit confirmation safeguards.
- **Diagnostic Suite**: System diagnostics, SQLite PRAGMA integrity checker, storage volume monitors, and toolchain verification.
- **Test Suite**: 20 automated unit and integration tests covering domain state transitions, path security, hashing, database CRUD, ZIP verification, and end-to-end vertical slices.

### Fixed
- **Manifest Deserialization**: Restored complete `files` and `artifacts` array serialization and deserialization in `ReleaseManifest::fromJsonString` and `toJsonString` with `QJsonDocument`.
- **Payload Verification Consistency**: Correctly populated `PackagingResult::manifest` and distinguished embedded metadata manifest (`shipdesk-release-manifest.json`) from payload files in ZIP and directory verification.
- **Auto-Discovery of Embedded Manifest**: Added automated extraction and verification against embedded manifest in `VerificationService::verifyArtifact` when external manifest path is not provided.
- **Wizard Inventory Filtering Index**: Resolved table row vs original item index mapping in `ReleaseWizardDialog` to preserve user file inclusion choices during search filtering.
- **Case Collision Rule Isolation**: Fixed `ReleaseValidator` to only trigger VAL-009 case collision warning for distinct paths, separating it from VAL-008 duplicate path blocker.
- **Process Runner Race Condition**: Eliminated potential event-loop freeze in `ProcessRunner::executeSync` by connecting `finished` prior to starting child processes and checking process exit state.
- **Crash Recovery Consistency**: Extended `DatabaseManager::recoverInterruptedRuns` to recover transient `release_drafts` to `Interrupted` on startup alongside `release_runs`.
- **Workspace Validation**: Guarded workspace name updates against empty strings with clear user feedback.
- **Compiler Warnings**: Cleaned up all build warnings across MinGW-w64 GCC 16.2.0, upgrading deprecated Qt signals (`QCheckBox::stateChanged` -> `toggled`) and unused parameters.
