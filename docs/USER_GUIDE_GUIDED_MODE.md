# Guided Mode User Guide

Guided Mode is the default interface designed for everyday users, educators, students, content creators, and developers who need to package deliverables quickly and reliably.

## Walkthrough: Creating Your First Release

### Step 1: Deliverable Category
Click **Create Release (Guided)** on the Overview page. Select the deliverable category that best fits your content:
- **General File Bundle**: Everyday file collections and assets.
- **Document Bundle**: Business deliverables, reports, and PDFs (ignores temporary Office lockfiles `~$*`).
- **Software Application**: Compiled binaries and distributions (ignores object files and compiler intermediates).
- **Educational Resources**: Course materials, lecture notes, and assignments.
- **Media Collection**: High-resolution images, video packs, and audio stems.
- **Static Website Output**: Built static websites (ignores `.git` and `node_modules`).

### Step 2: Source Selection
- Pick an existing workspace or choose **(New Workspace from Folder...)**.
- Click **Browse...** to select your source directory on disk.
- *Safety Reminder: Original files are never modified, moved, or deleted.*

### Step 3: Inventory & Inclusion Filtering
- ShipDesk automatically scans the selected directory.
- Review the generated inventory list with exact sizes and types.
- Check or uncheck files to include or exclude them from the package.
- Use the **Filter** box to search specific filenames or paths.

### Step 4: Metadata & Versioning
- Enter a **Release Title** (e.g. `Physics101-LectureNotes` or `PhotoViewer`).
- Specify a **Version Label** (e.g. `1.0.0` or `2026.11`).
- Select a **Channel** (`Draft`, `Preview`, `Alpha`, `Beta`, `Stable`).
- Add an optional description and release notes for recipients.

### Step 5: Pre-Packaging Validation
- ShipDesk executes automated rule checks:
  - Verifies that at least one file is selected (`VAL-001`).
  - Verifies that paths do not attempt directory traversal (`VAL-007`).
  - Checks for duplicate output paths and case collisions (`VAL-008`, `VAL-009`).
  - Verifies file size and byte thresholds.
- If blockers exist, adjust your files or settings. Once clean, click **Next**.

### Step 6: Package Format & Output Destination
- Choose your package output format:
  - **ZIP Archive (.zip)**: Recommended for delivery and sharing.
  - **Organized Directory Package**: Clean folder structure on disk.
- Select your target output directory.

### Step 7: Specification Review
- Inspect the complete release review summary before writing the package.

### Step 8: Execution & Instant Verification
- Click **Start Release Execution**.
- Watch real-time staging (copying to managed staging with SHA-256 calculation), archive creation, and cryptographic verification.
- Upon completion, ShipDesk displays the package path, exact byte size, and full SHA-256 checksum.
