#include "ui/themes/theme_manager.h"
#include <QPalette>
#include <QColor>
#include <QFontDatabase>

namespace shipdesk::ui {

void ThemeManager::applyTheme(QApplication& app, ThemeMode mode) {
    // Neutral palette with clean dark or light styling and a single muted slate-blue accent (#2563EB or #3B82F6)
    bool isDark = (mode == ThemeMode::Dark);
    if (mode == ThemeMode::System) {
        // Detect system window color lightness
        isDark = app.palette().color(QPalette::Window).lightness() < 128;
    }

    QPalette p;
    if (isDark) {
        p.setColor(QPalette::Window, QColor(32, 33, 36));
        p.setColor(QPalette::WindowText, QColor(232, 234, 237));
        p.setColor(QPalette::Base, QColor(24, 25, 27));
        p.setColor(QPalette::AlternateBase, QColor(41, 42, 45));
        p.setColor(QPalette::ToolTipBase, QColor(24, 25, 27));
        p.setColor(QPalette::ToolTipText, QColor(232, 234, 237));
        p.setColor(QPalette::Text, QColor(232, 234, 237));
        p.setColor(QPalette::Button, QColor(45, 47, 51));
        p.setColor(QPalette::ButtonText, QColor(232, 234, 237));
        p.setColor(QPalette::Highlight, QColor(59, 130, 246));
        p.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
        p.setColor(QPalette::PlaceholderText, QColor(154, 160, 166));
    } else {
        p.setColor(QPalette::Window, QColor(248, 249, 250));
        p.setColor(QPalette::WindowText, QColor(32, 33, 36));
        p.setColor(QPalette::Base, QColor(255, 255, 255));
        p.setColor(QPalette::AlternateBase, QColor(241, 243, 244));
        p.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
        p.setColor(QPalette::ToolTipText, QColor(32, 33, 36));
        p.setColor(QPalette::Text, QColor(32, 33, 36));
        p.setColor(QPalette::Button, QColor(241, 243, 244));
        p.setColor(QPalette::ButtonText, QColor(32, 33, 36));
        p.setColor(QPalette::Highlight, QColor(37, 99, 235));
        p.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
        p.setColor(QPalette::PlaceholderText, QColor(128, 134, 139));
    }

    app.setPalette(p);

    // Clean, subtle stylesheet: conventional desktop controls, modest 3px corners, crisp borders
    QString qss = isDark ? R"(
        QMainWindow { background-color: #202124; }
        QWidget { font-size: 13px; color: #E8EAED; }
        QListWidget, QTreeWidget, QTableWidget {
            background-color: #18191B;
            border: 1px solid #3C4043;
            border-radius: 3px;
            gridline-color: #2D2F33;
        }
        QHeaderView::section {
            background-color: #292A2D;
            color: #BDC1C6;
            padding: 4px 8px;
            border: 1px solid #3C4043;
            font-weight: 600;
        }
        QPushButton {
            background-color: #2D2F33;
            border: 1px solid #5F6368;
            border-radius: 3px;
            padding: 6px 14px;
            min-height: 18px;
        }
        QPushButton:hover { background-color: #3C4043; }
        QPushButton:pressed { background-color: #202124; }
        QPushButton:default {
            background-color: #2563EB;
            color: #FFFFFF;
            border: 1px solid #1D4ED8;
            font-weight: 600;
        }
        QPushButton:default:hover { background-color: #3B82F6; }
        QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QComboBox {
            background-color: #18191B;
            border: 1px solid #5F6368;
            border-radius: 3px;
            padding: 5px;
            color: #E8EAED;
        }
        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QComboBox:focus {
            border: 1px solid #3B82F6;
        }
        QProgressBar {
            border: 1px solid #3C4043;
            border-radius: 3px;
            text-align: center;
            background-color: #18191B;
        }
        QProgressBar::chunk {
            background-color: #3B82F6;
        }
        QSplitter::handle {
            background-color: #3C4043;
        }
        QStatusBar {
            background-color: #292A2D;
            border-top: 1px solid #3C4043;
        }
    )" : R"(
        QMainWindow { background-color: #F8F9FA; }
        QWidget { font-size: 13px; color: #202124; }
        QListWidget, QTreeWidget, QTableWidget {
            background-color: #FFFFFF;
            border: 1px solid #DADCE0;
            border-radius: 3px;
            gridline-color: #E8EAED;
        }
        QHeaderView::section {
            background-color: #F1F3F4;
            color: #3C4043;
            padding: 4px 8px;
            border: 1px solid #DADCE0;
            font-weight: 600;
        }
        QPushButton {
            background-color: #F1F3F4;
            border: 1px solid #DADCE0;
            border-radius: 3px;
            padding: 6px 14px;
            min-height: 18px;
        }
        QPushButton:hover { background-color: #E8EAED; }
        QPushButton:pressed { background-color: #DADCE0; }
        QPushButton:default {
            background-color: #2563EB;
            color: #FFFFFF;
            border: 1px solid #1D4ED8;
            font-weight: 600;
        }
        QPushButton:default:hover { background-color: #1D4ED8; }
        QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QComboBox {
            background-color: #FFFFFF;
            border: 1px solid #DADCE0;
            border-radius: 3px;
            padding: 5px;
            color: #202124;
        }
        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QComboBox:focus {
            border: 1px solid #2563EB;
        }
        QProgressBar {
            border: 1px solid #DADCE0;
            border-radius: 3px;
            text-align: center;
            background-color: #FFFFFF;
        }
        QProgressBar::chunk {
            background-color: #2563EB;
        }
        QSplitter::handle {
            background-color: #DADCE0;
        }
        QStatusBar {
            background-color: #F1F3F4;
            border-top: 1px solid #DADCE0;
        }
    )";

    app.setStyleSheet(qss);
}

QString ThemeManager::getMonospaceFontFamily() {
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    return font.family();
}

} // namespace shipdesk::ui
