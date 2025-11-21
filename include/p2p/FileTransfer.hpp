#pragma once

#include "p2p/Node.hpp"

#include <unordered_map>
#include <optional>
#include <fstream>

namespace p2p {

class FileTransfer {
public:
    explicit FileTransfer(Node& node);

    using FileHandler = std::function<void(const PeerId& from, const std::string& name, const std::vector<uint8_t>& data)>;
    void onFile(FileHandler cb) { onFile_ = std::move(cb); }

    bool sendFile(const PeerId& dest, const std::string& path, size_t chunkSize = 1024);
    bool sendBuffer(const PeerId& dest, const std::string& name, const std::vector<uint8_t>& data, size_t chunkSize = 1024);

private:
    Node& node_;
    FileHandler onFile_{};

    using FileId = std::array<uint8_t, 16>;
    struct Incoming {
        std::string name;
        uint32_t total{0};
        uint32_t received{0};
        std::vector<std::optional<std::vector<uint8_t>>> chunks;
    };
    std::unordered_map<std::string, Incoming> in_; // key: hex(fileId)

    static FileId randomId();
    static std::string toHex16(const FileId& id);
    static std::vector<uint8_t> buildChunk(const FileId& id, uint32_t index, uint32_t total,
                                           const std::string& name, const std::vector<uint8_t>& data);
    static bool parseChunk(const std::vector<uint8_t>& msg, FileId& id, uint32_t& index, uint32_t& total,
                           std::string& name, std::vector<uint8_t>& data);
};

} // namespace p2p
