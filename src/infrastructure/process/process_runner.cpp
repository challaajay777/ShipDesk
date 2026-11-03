#include "infrastructure/process/process_runner.h"
#include <QRegularExpression>
#include <QEventLoop>
#include <QProcessEnvironment>
#include <QCoreApplication>

namespace shipdesk::infrastructure {

ProcessRunner::ProcessRunner(QObject* parent)
    : QObject(parent)
{
}

ProcessRunner::~ProcessRunner() {
    cancel();
}

std::string ProcessRunner::redactSecrets(const std::string& input) {
    if (input.empty()) return "";

    QString text = QString::fromStdString(input);

    // List of secret redaction regexes
    static const QList<QPair<QRegularExpression, QString>> patterns = {
        // GitHub Tokens: gho_..., ghp_..., github_pat_...
        {QRegularExpression(R"((gh[ops]_[A-Za-z0-9_]{16,}))"), "[REDACTED_GITHUB_TOKEN]"},
        {QRegularExpression(R"((github_pat_[A-Za-z0-9_]{22,}))"), "[REDACTED_GITHUB_PAT]"},
        // GitLab Personal Access Token: glpat-...
        {QRegularExpression(R"((glpat-[A-Za-z0-9_\-]{20,}))"), "[REDACTED_GITLAB_TOKEN]"},
        // Bearer tokens in headers
        {QRegularExpression(R"((Bearer\s+)[A-Za-z0-9\-_.~+/=]{16,})", QRegularExpression::CaseInsensitiveOption), "\\1[REDACTED_BEARER_TOKEN]"},
        // AWS Secret Access Key: 40 character base64 string
        {QRegularExpression(R"((aws_secret_access_key\s*=\s*)[A-Za-z0-9/+=]{40})", QRegularExpression::CaseInsensitiveOption), "\\1[REDACTED_AWS_SECRET]"},
        // Generic Password fields in CLI or configs
        {QRegularExpression(R"((password|passwd|secret|api_key|token)\s*[:=]\s*([^\s,;]+))", QRegularExpression::CaseInsensitiveOption), "\\1=[REDACTED]"},
        // URL with user:pass@host
        {QRegularExpression(R"(([a-zA-Z]+://[^:]+:)([^@]+)(@))"), "\\1[REDACTED_PASSWORD]\\3"},
        // Private keys
        {QRegularExpression(R"(-----BEGIN [A-Z ]+ PRIVATE KEY-----[^-]+-----END [A-Z ]+ PRIVATE KEY-----)"), "[REDACTED_PRIVATE_KEY]"}
    };

    for (const auto& [regex, replacement] : patterns) {
        text.replace(regex, replacement);
    }

    return text.toStdString();
}

ProcessExecutionResult ProcessRunner::executeSync(
    const std::string& executable,
    const std::vector<std::string>& arguments,
    const std::string& workingDirectory,
    const std::map<std::string, std::string>& environmentOverrides,
    int timeoutSeconds,
    size_t maxLogBytes)
{
    ProcessExecutionResult result;
    m_cancelled = false;
    m_timedOut = false;
    m_maxLogBytes = maxLogBytes;
    m_stdoutBuffer.clear();
    m_stderrBuffer.clear();
    m_combinedLog.clear();

    m_process = new QProcess(this);

    // Prepare arguments
    QStringList qArgs;
    for (const auto& arg : arguments) {
        qArgs.append(QString::fromStdString(arg));
    }

    // Prepare working directory
    if (!workingDirectory.empty()) {
        m_process->setWorkingDirectory(QString::fromStdString(workingDirectory));
    }

    // Prepare environment
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    for (const auto& [key, val] : environmentOverrides) {
        env.insert(QString::fromStdString(key), QString::fromStdString(val));
    }
    m_process->setProcessEnvironment(env);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &ProcessRunner::onReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &ProcessRunner::onReadyReadStandardError);

    // Timeout timer
    QTimer timer;
    timer.setSingleShot(true);
    if (timeoutSeconds > 0) {
        connect(&timer, &QTimer::timeout, [this]() {
            m_timedOut = true;
            if (m_process && m_process->state() != QProcess::NotRunning) {
                m_process->kill();
            }
        });
        timer.start(timeoutSeconds * 1000);
    }

    // Run nested event loop to process output while waiting
    QEventLoop loop;
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            &loop, &QEventLoop::quit);

    m_process->start(QString::fromStdString(executable), qArgs);

    if (!m_process->waitForStarted(5000)) {
        if (m_process->state() == QProcess::NotRunning && m_process->error() == QProcess::UnknownError) {
            // Process started and finished before waitForStarted returned
        } else {
            result.started = false;
            result.termination_status = "FailedToStart";
            result.combined_logs = "Process failed to start: " + executable;
            delete m_process;
            m_process = nullptr;
            return result;
        }
    }

    result.started = true;

    if (m_process->state() != QProcess::NotRunning) {
        loop.exec();
    }

    timer.stop();

    result.timed_out = m_timedOut;
    result.cancelled = m_cancelled;
    result.exit_code = m_process->exitCode();
    result.termination_status = (m_process->exitStatus() == QProcess::NormalExit) ? "NormalExit" : "CrashExit";
    if (m_timedOut) {
        result.termination_status = "TimedOut";
    } else if (m_cancelled) {
        result.termination_status = "Cancelled";
    }

    // Flush any remaining output
    onReadyReadStandardOutput();
    onReadyReadStandardError();

    result.stdout_output = redactSecrets(m_stdoutBuffer);
    result.stderr_output = redactSecrets(m_stderrBuffer);
    result.combined_logs = redactSecrets(m_combinedLog);

    delete m_process;
    m_process = nullptr;

    return result;
}

void ProcessRunner::cancel() {
    m_cancelled = true;
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(1000)) {
            m_process->kill();
        }
    }
}

void ProcessRunner::onReadyReadStandardOutput() {
    if (!m_process) return;
    QByteArray data = m_process->readAllStandardOutput();
    if (!data.isEmpty()) {
        std::string raw = data.toStdString();
        if (m_stdoutBuffer.size() < m_maxLogBytes) {
            m_stdoutBuffer.append(raw);
        }
        if (m_combinedLog.size() < m_maxLogBytes) {
            m_combinedLog.append(raw);
        }
        emit outputReceived(QString::fromStdString(redactSecrets(raw)));
    }
}

void ProcessRunner::onReadyReadStandardError() {
    if (!m_process) return;
    QByteArray data = m_process->readAllStandardError();
    if (!data.isEmpty()) {
        std::string raw = data.toStdString();
        if (m_stderrBuffer.size() < m_maxLogBytes) {
            m_stderrBuffer.append(raw);
        }
        if (m_combinedLog.size() < m_maxLogBytes) {
            m_combinedLog.append(raw);
        }
        emit outputReceived(QString::fromStdString(redactSecrets(raw)));
    }
}

} // namespace shipdesk::infrastructure
