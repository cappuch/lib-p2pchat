#pragma once

#include "p2p/Identity.hpp"

#include <cstdint>
#include <vector>

namespace p2p {

struct Packet {
    PeerId sender{};
    PeerId dest{};
    uint8_t ttl{8};
    std::array<uint8_t, 64> signature{}; // sign(sender||dest||payload)
    std::vector<uint8_t> payload; // encrypted

    // serialize to bytes
    std::vector<uint8_t> serialize() const;
    static bool deserialize(const uint8_t* data, size_t len, Packet& out);
};

} // namespace p2p
