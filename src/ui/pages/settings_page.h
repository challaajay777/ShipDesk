#pragma once

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>

namespace shipdesk::ui {

class SettingsPage : public QWidget {
    Q_OBJECT
public:
    explicit SettingsPage(QWidget* parent = nullptr);

signals:
    void themeChanged(int themeIndex);

private slots:
    void onThemeChanged(int index);
    void onBrowseStaging();

private:
    void setupUi();

    QComboBox* m_themeCombo{nullptr};
    QComboBox* m_defaultFormatCombo{nullptr};
    QLineEdit* m_stagingDirEdit{nullptr};
    QPushButton* m_browseStagingBtn{nullptr};
};

} // namespace shipdesk::ui
