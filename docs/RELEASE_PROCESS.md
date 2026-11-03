# ShipDesk Release Process

This document describes how ShipDesk releases are prepared, packaged, and published.

## 1. Local Pre-Flight Checklist
1. All local changes are committed cleanly. Run `git status` and `git diff --check`.
2. Clean build in Release mode:
   ```bash
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ```
3. Run the automated test suite and ensure 100% pass:
   ```bash
   ctest --test-dir build --output-on-failure
   ```
4. Verify database migrations and `PRAGMA integrity_check`.

## 2. Packaging the Binary Distribution
1. Deploy necessary Qt runtime libraries using `windeployqt6` on Windows.
2. Include third-party licenses (Qt, miniz, SQLite) in the bundle.
3. Generate standalone ZIP archive `ShipDesk-v<version>-windows-x64.zip`.
4. Calculate SHA-256 digest:
   ```powershell
   Get-FileHash -Algorithm SHA256 ShipDesk-v0.1.0-windows-x64.zip
   ```

## 3. GitHub Publishing
1. Verify `gh auth status`.
2. Inspect target repository.
3. Create release draft on GitHub with generated release notes.
4. Upload verified release archive.
5. Verify remote asset name and size.
6. Publish after explicit confirmation.
