#pragma once

#include <QApplication>
#include <QString>

namespace shipdesk::ui {

class ThemeManager {
public:
    enum class ThemeMode {
        Light,
        Dark,
        System
    };

    static void applyTheme(QApplication& app, ThemeMode mode = ThemeMode::System);
    static QString getMonospaceFontFamily();
};

} // namespace shipdesk::ui
