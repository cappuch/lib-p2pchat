#pragma once

#include <cstdint>
#include <vector>

namespace p2p {

enum class MessageType : uint8_t {
    TEXT = 0x01,
    FILE_CHUNK = 0xF1,
    USER_BASE = 0x80
};

inline std::vector<uint8_t> packMessage(MessageType type, const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> out;
    out.reserve(1 + payload.size());
    out.push_back(static_cast<uint8_t>(type));
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

inline bool unpackMessage(const std::vector<uint8_t>& bytes, MessageType& type, std::vector<uint8_t>& payload) {
    if (bytes.size() < 1) return false;
    type = static_cast<MessageType>(bytes[0]);
    payload.assign(bytes.begin() + 1, bytes.end());
    return true;
}

} // namespace p2p
