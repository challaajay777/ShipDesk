#include "ui/pages/workspaces_page.h"
#include "application/workspace/workspace_service.h"
#include "infrastructure/database/database_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>

namespace shipdesk::ui {

WorkspacesPage::WorkspacesPage(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    refreshData();
}

void WorkspacesPage::setupUi() {
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // Left pane: Workspaces List
    auto* leftWidget = new QWidget(splitter);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    auto* listHeaderRow = new QHBoxLayout();
    auto* listTitle = new QLabel("<b>Workspaces</b>", leftWidget);
    m_newBtn = new QPushButton("New", leftWidget);
    connect(m_newBtn, &QPushButton::clicked, this, &WorkspacesPage::onNewWorkspaceClicked);
    listHeaderRow->addWidget(listTitle);
    listHeaderRow->addStretch();
    listHeaderRow->addWidget(m_newBtn);
    leftLayout->addLayout(listHeaderRow);

    m_workspaceList = new QListWidget(leftWidget);
    connect(m_workspaceList, &QListWidget::currentItemChanged, this, &WorkspacesPage::onWorkspaceSelected);
    leftLayout->addWidget(m_workspaceList);

    splitter->addWidget(leftWidget);

    // Right pane: Workspace Details
    auto* rightWidget = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(12, 0, 0, 0);
    rightLayout->setSpacing(16);

    auto* detailsBox = new QGroupBox("Workspace Settings", rightWidget);
    auto* formLayout = new QFormLayout(detailsBox);

    m_nameEdit = new QLineEdit(detailsBox);
    m_categoryCombo = new QComboBox(detailsBox);
    m_categoryCombo->addItems({
        "General File Bundle",
        "Software Application",
        "Document Bundle",
        "Educational Resources",
        "Media Collection",
        "Source Code Release",
        "Game Mod",
        "Resource Pack",
        "Plugin/Extension",
        "Static Website Output",
        "Custom"
    });

    m_descEdit = new QTextEdit(detailsBox);
    m_descEdit->setMaximumHeight(70);

    m_statusLabel = new QLabel("Active", detailsBox);
    m_createdLabel = new QLabel("-", detailsBox);

    formLayout->addRow("Workspace Name:", m_nameEdit);
    formLayout->addRow("Deliverable Category:", m_categoryCombo);
    formLayout->addRow("Description:", m_descEdit);
    formLayout->addRow("Status:", m_statusLabel);
    formLayout->addRow("Created Date (UTC):", m_createdLabel);

    rightLayout->addWidget(detailsBox);

    // Source Roots Section
    auto* rootsBox = new QGroupBox("Source Roots (Selected for Packaging)", rightWidget);
    auto* rootsLayout = new QVBoxLayout(rootsBox);

    m_rootsTable = new QTableWidget(rootsBox);
    m_rootsTable->setColumnCount(3);
    m_rootsTable->setHorizontalHeaderLabels({"Display Name", "Path on Disk", "Type"});
    m_rootsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_rootsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_rootsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_rootsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_rootsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    rootsLayout->addWidget(m_rootsTable);

    auto* rootsBtnRow = new QHBoxLayout();
    m_addRootBtn = new QPushButton("Add Source Folder...", rootsBox);
    m_removeRootBtn = new QPushButton("Remove Selected Root", rootsBox);
    connect(m_addRootBtn, &QPushButton::clicked, this, &WorkspacesPage::onAddSourceRootClicked);
    connect(m_removeRootBtn, &QPushButton::clicked, this, &WorkspacesPage::onRemoveSourceRootClicked);
    rootsBtnRow->addWidget(m_addRootBtn);
    rootsBtnRow->addWidget(m_removeRootBtn);
    rootsBtnRow->addStretch();
    rootsLayout->addLayout(rootsBtnRow);

    rightLayout->addWidget(rootsBox, 1);

    // Bottom Action Row
    auto* actionRow = new QHBoxLayout();
    m_saveBtn = new QPushButton("Save Changes", rightWidget);
    m_archiveBtn = new QPushButton("Archive Workspace", rightWidget);
    m_deleteBtn = new QPushButton("Delete...", rightWidget);
    m_createReleaseBtn = new QPushButton("Create Release for Workspace", rightWidget);
    m_createReleaseBtn->setDefault(true);

    connect(m_saveBtn, &QPushButton::clicked, this, &WorkspacesPage::onSaveWorkspaceClicked);
    connect(m_archiveBtn, &QPushButton::clicked, this, &WorkspacesPage::onArchiveWorkspaceClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &WorkspacesPage::onDeleteWorkspaceClicked);
    connect(m_createReleaseBtn, &QPushButton::clicked, [this]() {
        if (!m_currentWorkspaceId.empty()) {
            emit createReleaseForWorkspace(QString::fromStdString(m_currentWorkspaceId));
        }
    });

    actionRow->addWidget(m_saveBtn);
    actionRow->addWidget(m_archiveBtn);
    actionRow->addWidget(m_deleteBtn);
    actionRow->addStretch();
    actionRow->addWidget(m_createReleaseBtn);
    rightLayout->addLayout(actionRow);

    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    mainLayout->addWidget(splitter);
}

void WorkspacesPage::refreshData() {
    auto workspaces = infrastructure::DatabaseManager::instance().listWorkspaces(true);

    std::string prevId = m_currentWorkspaceId;
    m_workspaceList->clear();

    int selectIndex = -1;
    for (size_t i = 0; i < workspaces.size(); ++i) {
        const auto& ws = workspaces[i];
        QString title = QString::fromStdString(ws.name());
        if (ws.isArchived()) {
            title += " [Archived]";
        }
        auto* item = new QListWidgetItem(title);
        item->setData(Qt::UserRole, QString::fromStdString(ws.id()));
        m_workspaceList->addItem(item);

        if (ws.id() == prevId) {
            selectIndex = static_cast<int>(i);
        }
    }

    if (selectIndex >= 0) {
        m_workspaceList->setCurrentRow(selectIndex);
    } else if (m_workspaceList->count() > 0) {
        m_workspaceList->setCurrentRow(0);
    } else {
        clearDetails();
    }
}

void WorkspacesPage::selectWorkspace(const QString& workspaceId) {
    for (int i = 0; i < m_workspaceList->count(); ++i) {
        if (m_workspaceList->item(i)->data(Qt::UserRole).toString() == workspaceId) {
            m_workspaceList->setCurrentRow(i);
            break;
        }
    }
}

void WorkspacesPage::onWorkspaceSelected(QListWidgetItem* current, QListWidgetItem* /*previous*/) {
    if (!current) {
        clearDetails();
        return;
    }

    std::string id = current->data(Qt::UserRole).toString().toStdString();
    auto optWs = infrastructure::DatabaseManager::instance().getWorkspace(id);
    if (optWs) {
        loadWorkspaceDetails(*optWs);
    }
}

void WorkspacesPage::loadWorkspaceDetails(const domain::Workspace& ws) {
    m_currentWorkspaceId = ws.id();
    m_nameEdit->setText(QString::fromStdString(ws.name()));
    m_categoryCombo->setCurrentText(QString::fromStdString(ws.category()));
    m_descEdit->setText(QString::fromStdString(ws.description()));
    m_statusLabel->setText(QString::fromStdString(ws.status()));
    m_createdLabel->setText(QString::fromStdString(ws.createdAtUtc()));

    m_archiveBtn->setText(ws.isArchived() ? "Restore Workspace" : "Archive Workspace");

    // Load roots
    auto roots = infrastructure::DatabaseManager::instance().getSourceRoots(ws.id());
    m_rootsTable->setRowCount(0);
    for (size_t i = 0; i < roots.size(); ++i) {
        const auto& r = roots[i];
        m_rootsTable->insertRow(static_cast<int>(i));

        auto* itemDisp = new QTableWidgetItem(QString::fromStdString(r.display_name));
        itemDisp->setData(Qt::UserRole, QString::fromStdString(r.id));

        m_rootsTable->setItem(static_cast<int>(i), 0, itemDisp);
        m_rootsTable->setItem(static_cast<int>(i), 1, new QTableWidgetItem(QString::fromStdString(r.configured_path)));
        m_rootsTable->setItem(static_cast<int>(i), 2, new QTableWidgetItem(QString::fromStdString(r.path_kind)));
    }
}

void WorkspacesPage::clearDetails() {
    m_currentWorkspaceId.clear();
    m_nameEdit->clear();
    m_descEdit->clear();
    m_statusLabel->setText("-");
    m_createdLabel->setText("-");
    m_rootsTable->setRowCount(0);
}

void WorkspacesPage::onNewWorkspaceClicked() {
    bool ok = false;
    QString name = QInputDialog::getText(this, "New Workspace", "Workspace Name:", QLineEdit::Normal, "", &ok);
    if (ok && !name.trimmed().isEmpty()) {
        application::WorkspaceService service;
        auto ws = service.createWorkspace(name.trimmed().toStdString());
        refreshData();
        selectWorkspace(QString::fromStdString(ws.id()));
    }
}

void WorkspacesPage::onSaveWorkspaceClicked() {
    if (m_currentWorkspaceId.empty()) return;

    QString newName = m_nameEdit->text().trimmed();
    if (newName.isEmpty()) {
        QMessageBox::warning(this, "Invalid Name", "Workspace name cannot be empty.");
        return;
    }

    auto optWs = infrastructure::DatabaseManager::instance().getWorkspace(m_currentWorkspaceId);
    if (!optWs) return;

    auto ws = *optWs;
    ws.setName(newName.toStdString());
    ws.setCategory(m_categoryCombo->currentText().toStdString());
    ws.setDescription(m_descEdit->toPlainText().toStdString());

    infrastructure::DatabaseManager::instance().saveWorkspace(ws);
    refreshData();
}

void WorkspacesPage::onArchiveWorkspaceClicked() {
    if (m_currentWorkspaceId.empty()) return;

    application::WorkspaceService service;
    auto optWs = service.getWorkspace(m_currentWorkspaceId);
    if (!optWs) return;

    if (optWs->isArchived()) {
        service.restoreWorkspace(m_currentWorkspaceId);
    } else {
        service.archiveWorkspace(m_currentWorkspaceId);
    }
    refreshData();
}

void WorkspacesPage::onDeleteWorkspaceClicked() {
    if (m_currentWorkspaceId.empty()) return;

    auto reply = QMessageBox::question(
        this, "Confirm Deletion",
        "Are you sure you want to delete this workspace configuration?\n"
        "Original source files and existing releases will remain completely untouched on disk.",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        application::WorkspaceService service;
        service.deleteWorkspace(m_currentWorkspaceId);
        m_currentWorkspaceId.clear();
        refreshData();
    }
}

void WorkspacesPage::onAddSourceRootClicked() {
    if (m_currentWorkspaceId.empty()) {
        QMessageBox::information(this, "Select Workspace", "Please select or create a workspace first.");
        return;
    }

    QString dir = QFileDialog::getExistingDirectory(this, "Select Source Directory to Register");
    if (!dir.isEmpty()) {
        application::WorkspaceService service;
        service.addSourceRoot(m_currentWorkspaceId, dir.toStdString());
        // Refresh roots
        auto optWs = service.getWorkspace(m_currentWorkspaceId);
        if (optWs) loadWorkspaceDetails(*optWs);
    }
}

void WorkspacesPage::onRemoveSourceRootClicked() {
    int row = m_rootsTable->currentRow();
    if (row < 0) return;

    auto* item = m_rootsTable->item(row, 0);
    if (!item) return;

    std::string rootId = item->data(Qt::UserRole).toString().toStdString();
    application::WorkspaceService service;
    service.removeSourceRoot(rootId);

    auto optWs = service.getWorkspace(m_currentWorkspaceId);
    if (optWs) loadWorkspaceDetails(*optWs);
}

} // namespace shipdesk::ui
