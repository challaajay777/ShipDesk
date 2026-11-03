#pragma once

#include "domain/release/release_record.h"
#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>

namespace shipdesk::ui {

class PublishDialog : public QDialog {
    Q_OBJECT
public:
    explicit PublishDialog(const QString& releaseRecordId, QWidget* parent = nullptr);

private slots:
    void onDestinationTypeChanged(int index);
    void onConfirmCheckChanged(bool checked);
    void onPublishClicked();

private:
    void setupUi();
    void loadReleaseData();

    std::string m_releaseRecordId;
    std::string m_artifactPath;
    std::string m_releaseTitle;
    std::string m_releaseVersion;
    std::string m_artifactSha256;

    QComboBox* m_destTypeCombo{nullptr};
    QLineEdit* m_localFolderEdit{nullptr};
    QPushButton* m_browseFolderBtn{nullptr};
    QLineEdit* m_ghRepoEdit{nullptr};
    QLineEdit* m_ghTagEdit{nullptr};
    QCheckBox* m_ghDraftCheck{nullptr};
    QCheckBox* m_ghPrereleaseCheck{nullptr};

    QTextEdit* m_previewEdit{nullptr};
    QCheckBox* m_confirmationCheck{nullptr};
    QPushButton* m_publishBtn{nullptr};
    QPushButton* m_cancelBtn{nullptr};
    QProgressBar* m_progressBar{nullptr};
    QLabel* m_resultLabel{nullptr};
};

} // namespace shipdesk::ui
