#include "ui/pages/settings_page.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QFileDialog>
#include <QLabel>
#include <QDir>
#include <QStandardPaths>

namespace shipdesk::ui {

SettingsPage::SettingsPage(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void SettingsPage::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(16);

    // Appearance Box
    auto* appBox = new QGroupBox("Appearance & Theme", this);
    auto* appForm = new QFormLayout(appBox);

    m_themeCombo = new QComboBox(appBox);
    m_themeCombo->addItems({"System Default", "Light Theme", "Dark Theme"});
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsPage::onThemeChanged);
    appForm->addRow("Theme Mode:", m_themeCombo);

    mainLayout->addWidget(appBox);

    // Packaging Preferences Box
    auto* pkgBox = new QGroupBox("Packaging Defaults", this);
    auto* pkgForm = new QFormLayout(pkgBox);

    m_defaultFormatCombo = new QComboBox(pkgBox);
    m_defaultFormatCombo->addItems({"ZIP Archive (.zip)", "Organized Directory Package"});
    pkgForm->addRow("Default Package Format:", m_defaultFormatCombo);

    auto* stagingRow = new QHBoxLayout();
    QString defaultStaging = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/staging";
    m_stagingDirEdit = new QLineEdit(defaultStaging, pkgBox);
    m_browseStagingBtn = new QPushButton("Browse...", pkgBox);
    connect(m_browseStagingBtn, &QPushButton::clicked, this, &SettingsPage::onBrowseStaging);
    stagingRow->addWidget(m_stagingDirEdit, 1);
    stagingRow->addWidget(m_browseStagingBtn);
    pkgForm->addRow("Managed Staging Root:", stagingRow);

    mainLayout->addWidget(pkgBox);

    // Privacy & Security Guarantee Box
    auto* privBox = new QGroupBox("Privacy & Local-First Guarantees", this);
    auto* privLayout = new QVBoxLayout(privBox);

    auto* privLabel = new QLabel(
        "<b>1. Zero Telemetry:</b> ShipDesk never transmits analytics, usage metrics, or crash logs to any remote server.<br>"
        "<b>2. Source Safety:</b> Inspection and staging operations are strictly read-only on your original files.<br>"
        "<b>3. Explicit Approval:</b> Build scripts and external processes never execute automatically without inspection and fingerprint approval.<br>"
        "<b>4. Offline First:</b> All staging, packaging, hashing, verification, and history storage operate completely offline unless you explicitly choose to publish to a remote platform.",
        privBox
    );
    privLabel->setWordWrap(true);
    privLayout->addWidget(privLabel);

    mainLayout->addWidget(privBox);
    mainLayout->addStretch();
}

void SettingsPage::onThemeChanged(int index) {
    emit themeChanged(index);
}

void SettingsPage::onBrowseStaging() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Staging Root Directory");
    if (!dir.isEmpty()) {
        m_stagingDirEdit->setText(dir);
    }
}

} // namespace shipdesk::ui
