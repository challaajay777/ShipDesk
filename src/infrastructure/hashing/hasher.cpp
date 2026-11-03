#include "infrastructure/hashing/hasher.h"
#include <QCryptographicHash>
#include <QFile>
#include <QByteArray>
#include <QByteArrayView>
#include <stdexcept>

namespace shipdesk::infrastructure {

std::string Hasher::sha256String(const std::string& input) {
    QByteArray data(input.data(), static_cast<int>(input.size()));
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    return hash.toHex().toStdString();
}

std::string Hasher::sha256File(
    const std::filesystem::path& filePath,
    std::function<void(uint64_t bytesRead, uint64_t totalBytes)> progress)
{
    QString qPath = QString::fromStdWString(filePath.wstring());
    QFile file(qPath);
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("Failed to open file for SHA-256 calculation: " + filePath.string());
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    uint64_t totalBytes = static_cast<uint64_t>(file.size());
    uint64_t bytesRead = 0;

    constexpr qint64 bufferSize = 64 * 1024; // 64 KB buffer
    char buffer[bufferSize];

    while (!file.atEnd()) {
        qint64 read = file.read(buffer, bufferSize);
        if (read < 0) {
            throw std::runtime_error("Read error during SHA-256 calculation: " + filePath.string());
        }
        if (read > 0) {
            hash.addData(QByteArrayView(buffer, read));
            bytesRead += read;
            if (progress) {
                progress(bytesRead, totalBytes);
            }
        }
    }

    file.close();
    return hash.result().toHex().toStdString();
}

} // namespace shipdesk::infrastructure
