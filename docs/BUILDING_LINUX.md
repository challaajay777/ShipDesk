# Building ShipDesk on Linux

ShipDesk uses standard C++20 and standard Qt 6 Widgets packages available across modern Linux distributions.

## 1. Prerequisites

### Ubuntu 22.04 LTS / 24.04 LTS & Debian 12+
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    qt6-base-dev \
    qt6-base-dev-tools \
    libsqlite3-dev \
    git
```

### Fedora 38+ / RHEL 9+
```bash
sudo dnf install -y \
    gcc-c++ \
    cmake \
    ninja-build \
    qt6-qtbase-devel \
    sqlite-devel \
    git
```

### Arch Linux / Manjaro
```bash
sudo pacman -S --needed base-devel cmake ninja qt6-base sqlite git
```

## 2. Compiling

```bash
git clone https://github.com/challaajay777/ShipDesk.git
cd ShipDesk

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

## 3. Running Tests

```bash
ctest --test-dir build --output-on-failure
```

## 4. Launching Application

```bash
./build/shipdesk
```
ShipDesk respects XDG Base Directory standards (`~/.local/share/ShipDesk/shipdesk.db`).
