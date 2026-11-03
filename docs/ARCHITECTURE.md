# ShipDesk Architecture

```
   Presentation Layer (Qt 6 Widgets)
┌────────────────────────────────────────────────────────┐
│ MainWindow, Pages (Overview, Workspaces, Releases...), │
│ Dialogs (ReleaseWizardDialog, PublishDialog)           │
└───────────────────────────┬────────────────────────────┘
                            │
                            ▼
   Application Layer (Use Cases & Workflows)
┌────────────────────────────────────────────────────────┐
│ WorkspaceService, SourceInspectionService,             │
│ StagingService, PackagingService, VerificationService, │
│ PublishingService, ReleaseService                      │
└─────────────┬───────────────────────────┬──────────────┘
              │                           │
              ▼                           ▼
   Domain Layer (Pure C++20)       Infrastructure / Adapters
┌───────────────────────────────┐ ┌───────────────────────────────┐
│ Workspace, SourceRoot,        │ │ DatabaseManager (SQLite),     │
│ ReleaseProfile, BuildProfile, │ │ PathUtils, Hasher (SHA-256),  │
│ ReleaseDraft, StateMachine,   │ │ ZipArchiveService (miniz),    │
│ ValidationRule, Findings,     │ │ ProcessRunner (QProcess),     │
│ ReleaseManifest               │ │ GitInspector, DestinationAdapters│
└───────────────────────────────┘ └───────────────────────────────┘
```

## Layer Invariants
1. **Domain Layer**:
   - Pure C++20 standard library (no Qt Widgets, no SQLite headers, no network).
   - Enforces state machine transitions and validation rules.
   - Immutable release record definitions.
2. **Infrastructure Layer**:
   - Implements persistent SQLite database access using parameterized SQL queries.
   - Enforces safe filesystem operations and path containment checks.
   - Encapsulates cryptographic hashing and archive creation/verification.
   - Wraps asynchronous process execution with secret pattern redaction.
3. **Application Layer**:
   - Coordinates end-to-end workflows (inspect -> validate -> stage -> package -> verify -> save).
   - Manages cooperative cancellation and progress reporting.
4. **Presentation Layer**:
   - Clean, native Qt 6 Widgets with zero web components or browser shells.
   - Uses Qt model/view patterns for high performance on large directory structures.
