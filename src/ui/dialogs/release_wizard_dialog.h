#pragma once

#include "domain/types.h"
#include "domain/profile/release_profile.h"
#include "domain/source/inventory_item.h"
#include "domain/validation/validation_finding.h"
#include "application/release/release_service.h"
#include <QDialog>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QTableWidget>
#include <QProgressBar>
#include <atomic>

namespace shipdesk::ui {

class ReleaseWizardDialog : public QDialog {
    Q_OBJECT
public:
    explicit ReleaseWizardDialog(const QString& initialWorkspaceId = "", QWidget* parent = nullptr);
    ~ReleaseWizardDialog() override;

    std::string completedRecordId() const { return m_completedRecordId; }

private slots:
    void onNextClicked();
    void onBackClicked();
    void onBrowseSourceClicked();
    void onBrowseDestinationClicked();
    void onSearchInventoryChanged(const QString& text);
    void onStartExecutionClicked();

private:
    void setupUi();
    void setupStep1Category();
    void setupStep2Source();
    void setupStep3Inventory();
    void setupStep4Metadata();
    void setupStep5Validation();
    void setupStep6Format();
    void setupStep7Review();
    void setupStep8Execute();

    void updateNavButtons();
    void runInspectionForStep3();
    void runValidationForStep5();
    void updateReviewForStep7();

    QStackedWidget* m_stack{nullptr};
    QLabel* m_stepTitleLabel{nullptr};
    QPushButton* m_backBtn{nullptr};
    QPushButton* m_nextBtn{nullptr};
    QPushButton* m_cancelBtn{nullptr};

    // Step 1: Category
    QComboBox* m_categoryCombo{nullptr};

    // Step 2: Source
    QComboBox* m_workspaceCombo{nullptr};
    QLineEdit* m_sourcePathEdit{nullptr};
    QPushButton* m_browseSourceBtn{nullptr};

    // Step 3: Inventory
    QLineEdit* m_inventorySearchEdit{nullptr};
    QTableWidget* m_inventoryTable{nullptr};
    QLabel* m_inventorySummaryLabel{nullptr};
    std::vector<domain::SourceInventoryItem> m_inventoryItems;

    // Step 4: Metadata
    QLineEdit* m_titleEdit{nullptr};
    QLineEdit* m_versionEdit{nullptr};
    QComboBox* m_channelCombo{nullptr};
    QTextEdit* m_descEdit{nullptr};
    QTextEdit* m_notesEdit{nullptr};

    // Step 5: Validation
    QTableWidget* m_validationTable{nullptr};
    QLabel* m_validationStatusLabel{nullptr};
    std::vector<domain::ValidationFinding> m_currentFindings;

    // Step 6: Format & Destination
    QComboBox* m_formatCombo{nullptr};
    QLineEdit* m_destDirEdit{nullptr};
    QPushButton* m_browseDestBtn{nullptr};
    QLabel* m_estimateSizeLabel{nullptr};

    // Step 7: Review
    QTextEdit* m_reviewSummaryEdit{nullptr};

    // Step 8: Execution & Verification
    QPushButton* m_startExecBtn{nullptr};
    QProgressBar* m_progressBar{nullptr};
    QLabel* m_phaseLabel{nullptr};
    QTextEdit* m_execLogEdit{nullptr};
    QLabel* m_finalResultLabel{nullptr};

    std::string m_selectedWorkspaceId;
    std::atomic<bool> m_cancelExecutionFlag{false};
    std::string m_completedRecordId;
};

} // namespace shipdesk::ui
