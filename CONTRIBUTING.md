# Contributing to ShipDesk

Thank you for your interest in contributing to ShipDesk!

ShipDesk is a native C++20 desktop application built with Qt 6 Widgets, CMake, and SQLite.

## Core Development Principles

1. **Local-First & Privacy**: No telemetry, analytics, or silent network calls.
2. **Source Safety**: Inspection and staging must never modify original user sources. Staging copies; it never moves or deletes original files.
3. **Explicit Control**: External commands and remote publishing require user review and confirmation.
4. **Clean Native C++20**: Standard ISO C++20, portable across Windows and Linux.
5. **No Web Wrappers**: ShipDesk is purely native desktop software (Qt Widgets). No WebView, Electron, Tauri, or embedded browser frameworks.

## Building from Source

### Prerequisites
- C++20 compliant compiler: GCC 11+, Clang 13+, or MSVC 2019+
- CMake 3.24+
- Ninja build system
- Qt 6.4+ (Core, Gui, Widgets, Sql, Test)
- SQLite 3.35+

### Build Instructions
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Running Tests
```bash
ctest --test-dir build --output-on-failure
```
Ensure all automated tests pass before submitting pull requests.

## Code Style
- Use 4 spaces for C++ code, 2 spaces for CMake and Markdown.
- Keep domain entities independent of UI or persistence libraries.
- Run `git diff --check` before committing.
