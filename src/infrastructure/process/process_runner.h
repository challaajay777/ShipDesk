#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QTimer>
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace shipdesk::infrastructure {

struct ProcessExecutionResult {
    bool started{false};
    bool timed_out{false};
    bool cancelled{false};
    int exit_code{-1};
    std::string termination_status;
    std::string stdout_output;
    std::string stderr_output;
    std::string combined_logs;
};

class ProcessRunner : public QObject {
    Q_OBJECT
public:
    explicit ProcessRunner(QObject* parent = nullptr);
    ~ProcessRunner() override;

    // Redacts secrets, tokens, passwords and API keys from a string
    static std::string redactSecrets(const std::string& input);

    // Executes process synchronously with event loop processing (keeps UI responsive)
    // or asynchronous execution.
    ProcessExecutionResult executeSync(
        const std::string& executable,
        const std::vector<std::string>& arguments,
        const std::string& workingDirectory,
        const std::map<std::string, std::string>& environmentOverrides,
        int timeoutSeconds = 300,
        size_t maxLogBytes = 2 * 1024 * 1024
    );

    void cancel();

signals:
    void outputReceived(const QString& line);
    void processFinished(int exitCode, const QString& status);

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();

private:
    QProcess* m_process{nullptr};
    bool m_cancelled{false};
    bool m_timedOut{false};
    size_t m_maxLogBytes{2 * 1024 * 1024};
    std::string m_stdoutBuffer;
    std::string m_stderrBuffer;
    std::string m_combinedLog;
};

} // namespace shipdesk::infrastructure
