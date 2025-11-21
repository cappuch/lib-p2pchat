#pragma once

#include "p2p/Identity.hpp"

#include <string>
#include <chrono>

namespace p2p {

struct Peer {
    PeerId id{};
    KeyBytes publicKey{};
    SignPublic signPublic{};
    std::string ip;
    uint16_t port{0};
    std::chrono::steady_clock::time_point lastSeen{};
};

} // namespace p2p
