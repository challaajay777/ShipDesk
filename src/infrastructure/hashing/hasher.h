#pragma once

#include <string>
#include <filesystem>
#include <functional>
#include <cstdint>

namespace shipdesk::infrastructure {

class Hasher {
public:
    // Calculates SHA-256 of a string or buffer
    static std::string sha256String(const std::string& input);

    // Calculates SHA-256 of a file using streaming 64 KB buffers.
    // Supports progress callback: (bytesRead, totalBytes)
    static std::string sha256File(
        const std::filesystem::path& filePath,
        std::function<void(uint64_t bytesRead, uint64_t totalBytes)> progress = nullptr
    );
};

} // namespace shipdesk::infrastructure
