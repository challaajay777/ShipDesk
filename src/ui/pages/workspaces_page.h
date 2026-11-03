#pragma once

#include "domain/workspace/workspace.h"
#include <QWidget>
#include <QListWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

namespace shipdesk::ui {

class WorkspacesPage : public QWidget {
    Q_OBJECT
public:
    explicit WorkspacesPage(QWidget* parent = nullptr);

    void refreshData();
    void selectWorkspace(const QString& workspaceId);

signals:
    void createReleaseForWorkspace(const QString& workspaceId);

private slots:
    void onWorkspaceSelected(QListWidgetItem* current, QListWidgetItem* previous);
    void onNewWorkspaceClicked();
    void onSaveWorkspaceClicked();
    void onArchiveWorkspaceClicked();
    void onDeleteWorkspaceClicked();
    void onAddSourceRootClicked();
    void onRemoveSourceRootClicked();

private:
    void setupUi();
    void loadWorkspaceDetails(const domain::Workspace& ws);
    void clearDetails();

    QListWidget* m_workspaceList{nullptr};
    QPushButton* m_newBtn{nullptr};

    QLineEdit* m_nameEdit{nullptr};
    QComboBox* m_categoryCombo{nullptr};
    QTextEdit* m_descEdit{nullptr};
    QLabel* m_statusLabel{nullptr};
    QLabel* m_createdLabel{nullptr};

    QTableWidget* m_rootsTable{nullptr};
    QPushButton* m_addRootBtn{nullptr};
    QPushButton* m_removeRootBtn{nullptr};

    QPushButton* m_saveBtn{nullptr};
    QPushButton* m_archiveBtn{nullptr};
    QPushButton* m_deleteBtn{nullptr};
    QPushButton* m_createReleaseBtn{nullptr};

    std::string m_currentWorkspaceId;
};

} // namespace shipdesk::ui
