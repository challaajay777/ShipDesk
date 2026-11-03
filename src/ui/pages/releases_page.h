#pragma once

#include "domain/release/release_record.h"
#include <QWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>

namespace shipdesk::ui {

class ReleasesPage : public QWidget {
    Q_OBJECT
public:
    explicit ReleasesPage(QWidget* parent = nullptr);

    void refreshData();
    void selectRelease(const QString& releaseId);

signals:
    void publishRequested(const QString& releaseRecordId);

private slots:
    void onReleaseSelected(int row, int column);
    void onOpenFolderClicked();
    void onReverifyClicked();
    void onPublishClicked();

private:
    void setupUi();
    void loadReleaseDetails(const domain::ImmutableReleaseRecord& rec);
    void clearDetails();

    QTableWidget* m_releasesTable{nullptr};
    QLabel* m_titleLabel{nullptr};
    QLabel* m_versionLabel{nullptr};
    QLabel* m_dateLabel{nullptr};
    QLabel* m_manifestPathLabel{nullptr};
    QTextEdit* m_manifestView{nullptr};

    QPushButton* m_openFolderBtn{nullptr};
    QPushButton* m_reverifyBtn{nullptr};
    QPushButton* m_publishBtn{nullptr};

    std::string m_currentRecordId;
    std::string m_currentManifestPath;
};

} // namespace shipdesk::ui
