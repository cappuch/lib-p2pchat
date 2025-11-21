#pragma once

#include <array>
#include <string>
#include <vector>
#include <cstdint>

namespace p2p {

using KeyBytes = std::array<uint8_t, 32>;
using SignSecret = std::array<uint8_t, 64>;
using SignPublic = std::array<uint8_t, 32>;
using PeerId    = std::array<uint8_t, 32>;

struct Identity {
    KeyBytes privateKey{};
    KeyBytes publicKey{};

    SignSecret signSecret{};
    SignPublic signPublic{};
    PeerId   id{}; // hash(pub)

    static Identity generate();
};

// hex helpers
std::string toHex(const uint8_t* data, size_t len);
std::string toHex(const PeerId& id);

} // namespace p2p
