#pragma once

#include <QWidget>
#include <QListWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>

namespace shipdesk::ui {

class OverviewPage : public QWidget {
    Q_OBJECT
public:
    explicit OverviewPage(QWidget* parent = nullptr);

    void refreshData();

signals:
    void createReleaseRequested();
    void newWorkspaceRequested();
    void openWorkspaceRequested(const QString& workspaceId);
    void openReleaseRequested(const QString& releaseId);

private slots:
    void onWorkspaceDoubleClicked(QListWidgetItem* item);
    void onReleaseDoubleClicked(QTableWidgetItem* item);

private:
    void setupUi();

    QListWidget* m_recentWorkspacesList{nullptr};
    QTableWidget* m_recentReleasesTable{nullptr};
    QPushButton* m_createReleaseBtn{nullptr};
    QPushButton* m_newWorkspaceBtn{nullptr};
    QLabel* m_statsLabel{nullptr};
};

} // namespace shipdesk::ui
