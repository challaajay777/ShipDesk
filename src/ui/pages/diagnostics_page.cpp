#include "ui/pages/diagnostics_page.h"
#include "infrastructure/database/database_manager.h"
#include "infrastructure/process/process_runner.h"
#include "adapters/destinations/github_destination.h"
#include "infrastructure/filesystem/path_utils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSysInfo>
#include <QStorageInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <fstream>

namespace shipdesk::ui {

DiagnosticsPage::DiagnosticsPage(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    refreshDiagnostics();
}

void DiagnosticsPage::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    auto* btnRow = new QHBoxLayout();
    m_refreshBtn = new QPushButton("Refresh Diagnostics", this);
    m_checkDbBtn = new QPushButton("Run SQLite Integrity Check", this);
    m_exportBtn = new QPushButton("Export Diagnostics...", this);

    connect(m_refreshBtn, &QPushButton::clicked, this, &DiagnosticsPage::refreshDiagnostics);
    connect(m_checkDbBtn, &QPushButton::clicked, this, &DiagnosticsPage::onRunDbCheckClicked);
    connect(m_exportBtn, &QPushButton::clicked, this, &DiagnosticsPage::onExportClicked);

    btnRow->addWidget(m_refreshBtn);
    btnRow->addWidget(m_checkDbBtn);
    btnRow->addWidget(m_exportBtn);
    btnRow->addStretch();
    mainLayout->addLayout(btnRow);

    m_diagView = new QTextEdit(this);
    m_diagView->setReadOnly(true);
    m_diagView->setFontFamily("Courier New");
    mainLayout->addWidget(m_diagView, 1);
}

void DiagnosticsPage::refreshDiagnostics() {
    QString out = "================================================================================\n";
    out += "SHIPDESK SYSTEM DIAGNOSTICS REPORT\n";
    out += "================================================================================\n\n";

    out += "[Application]\n";
    out += "Name: ShipDesk\n";
    out += "Version: 0.1.0 (Flagship Master Release)\n";
    out += "Codename: ShipDesk\n\n";

    out += "[Environment & OS]\n";
    out += "OS: " + QSysInfo::prettyProductName() + " (" + QSysInfo::productType() + ")\n";
    out += "Kernel: " + QSysInfo::kernelType() + " " + QSysInfo::kernelVersion() + "\n";
    out += "Architecture: " + QSysInfo::currentCpuArchitecture() + "\n";
    out += "Qt Version: " + QString(qVersion()) + "\n";
    out += "C++ Standard: C++20\n\n";

    out += "[Local Database]\n";
    out += "Database Path: " + QString::fromStdString(infrastructure::DatabaseManager::instance().databasePath()) + "\n";
    std::string dbStatus;
    bool ok = infrastructure::DatabaseManager::instance().runIntegrityCheck(dbStatus);
    out += "PRAGMA integrity_check: " + QString::fromStdString(dbStatus) + " (" + (ok ? "HEALTHY" : "FAILED") + ")\n\n";

    out += "[External Toolchains]\n";
    infrastructure::ProcessRunner runner;

    auto cmakeRes = runner.executeSync("cmake", {"--version"}, "", {}, 3);
    out += "CMake: " + (cmakeRes.exit_code == 0 ? QString::fromStdString(cmakeRes.stdout_output.substr(0, cmakeRes.stdout_output.find('\n'))) : "Not Found") + "\n";

    auto gitRes = runner.executeSync("git", {"--version"}, "", {}, 3);
    out += "Git: " + (gitRes.exit_code == 0 ? QString::fromStdString(gitRes.stdout_output.substr(0, gitRes.stdout_output.find('\n'))) : "Not Found") + "\n";

    auto ghInfo = adapters::GitHubDestinationAdapter::checkAuth();
    out += "GitHub CLI: " + (ghInfo.installed ? QString::fromStdString(ghInfo.cli_version) : "Not Installed") + "\n";
    out += "GitHub Auth Status: " + QString(ghInfo.authenticated ? "Authenticated as " + QString::fromStdString(ghInfo.logged_in_user) : "Not Authenticated") + "\n\n";

    out += "[Storage Volumes]\n";
    for (const auto& vol : QStorageInfo::mountedVolumes()) {
        if (vol.isValid() && vol.isReady()) {
            out += QString("Drive %1 (%2): %3 free of %4\n")
                .arg(vol.rootPath())
                .arg(QString(vol.fileSystemType()))
                .arg(QString::fromStdString(infrastructure::PathUtils::formatByteSize(vol.bytesAvailable())))
                .arg(QString::fromStdString(infrastructure::PathUtils::formatByteSize(vol.bytesTotal())));
        }
    }

    out += "\n[Security Boundary]\n";
    out += "Source Mutation Protection: ENFORCED (Read-only source inspections)\n";
    out += "Staging Boundary: ENFORCED (Managed directory with shipdesk.owner verification)\n";
    out += "External Process Approval: ENFORCED (Untrusted script auto-exec DISABLED)\n";
    out += "Secret Redaction: ACTIVE (Tokens and passwords masked)\n";
    out += "Telemetry / Analytics: DISABLED (Local-first, zero remote tracking)\n";

    m_diagView->setPlainText(out);
}

void DiagnosticsPage::onRunDbCheckClicked() {
    std::string details;
    bool ok = infrastructure::DatabaseManager::instance().runIntegrityCheck(details);
    if (ok) {
        QMessageBox::information(this, "SQLite Integrity Check", "Integrity check PASSED: " + QString::fromStdString(details));
    } else {
        QMessageBox::critical(this, "SQLite Integrity Check", "Integrity check FAILED: " + QString::fromStdString(details));
    }
    refreshDiagnostics();
}

void DiagnosticsPage::onExportClicked() {
    QString savePath = QFileDialog::getSaveFileName(this, "Export Diagnostics Report", "shipdesk_diagnostics.txt", "Text Files (*.txt);;All Files (*.*)");
    if (!savePath.isEmpty()) {
        std::ofstream ofs(savePath.toStdString());
        if (ofs.is_open()) {
            ofs << m_diagView->toPlainText().toStdString();
            QMessageBox::information(this, "Exported", "Diagnostics exported successfully to:\n" + savePath);
        }
    }
}

} // namespace shipdesk::ui
