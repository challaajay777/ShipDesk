# Advanced Mode User Guide

Advanced Mode provides comprehensive release orchestration tools for software engineers, IT professionals, and technical teams.

## 1. Multi-Root Workspaces
- Workspaces support registering multiple independent source directories (e.g. `bin/`, `assets/`, and `docs/`).
- Configure custom exclusion glob rules and default package naming templates per workspace.

## 2. Git Status Inspection
- ShipDesk detects Git repositories located inside source roots without mutating repository state.
- Inspect current branch name, HEAD commit hash, and dirty working tree status.
- Inspect configured remote URLs with credentials automatically masked.

## 3. Build & Test Orchestration Security
- External build tools (CMake, CTest, Make, Cargo) never run automatically.
- Commands require explicit user review and approval bound to a cryptographic fingerprint:
  `SHA256(SOURCE_IDENTITY + EXECUTABLE + ARGS + WORKING_DIR + ENV)`
- If executable paths, arguments, or environment variables change, approvals are invalidated immediately.
- Execution logs redact personal access tokens, private keys, and passwords.

## 4. Standalone Verification Tool
- Navigate to **Tools -> Package Verifier**.
- Select any third-party or generated `.zip` archive.
- Optionally select a `shipdesk-release-manifest.json` file.
- ShipDesk inspects archive structure, tests CRC32 validity of each entry, and checks SHA-256 digests.

## 5. Standalone Source Inspector
- Navigate to **Tools -> Source Inspector**.
- Inspect arbitrary folders on disk to analyze total file counts, sizes, hidden files, and broken symlinks without creating a workspace.

## 6. GitHub Releases Publishing
- Navigate to **Releases**, select any verified release, and click **Publish Release...**.
- Select **GitHub Releases**.
- Preview the target repository, release tag, title, notes, and artifact digests.
- Check the explicit authorization confirmation box.
- ShipDesk interacts with the official GitHub CLI (`gh`), uploads the asset, verifies remote asset existence, and records the permanent release URL.
