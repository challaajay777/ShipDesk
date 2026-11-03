# Building ShipDesk on Windows

This guide covers building ShipDesk on Windows 10/11 x64 using MinGW-w64 (MSYS2 UCRT64) or MSVC.

## 1. Toolchain Prerequisites

### Recommended: MSYS2 UCRT64
1. Install MSYS2 from https://www.msys2.org/
2. Open the **MSYS2 UCRT64** terminal and install packages:
   ```bash
   pacman -S mingw-w64-ucrt-x86_64-gcc \
             mingw-w64-ucrt-x86_64-ninja \
             mingw-w64-ucrt-x86_64-cmake \
             mingw-w64-ucrt-x86_64-qt6-base
   ```
3. Add `C:\msys64\ucrt64\bin` to your Windows user `PATH`.

## 2. Configuration & Build

```powershell
# Open PowerShell in the project root
cd C:\Users\ajays\.gemini\antigravity\scratch\ShipDesk

# Configure with CMake and Ninja
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/msys64/ucrt64"

# Compile ShipDesk executable and test suite
cmake --build build
```

## 3. Running Automated Tests

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH
ctest --test-dir build --output-on-failure
```

## 4. Running ShipDesk

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH
.\build\shipdesk.exe
```
