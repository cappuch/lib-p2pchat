#pragma once

#include "p2p/Peer.hpp"

#include <vector>
#include <mutex>
#include <optional>

namespace p2p {

class PeerDirectory {
public:
    void addOrUpdate(const Peer& p);
    std::vector<Peer> list() const;
    std::optional<Peer> findById(const PeerId& id) const;
    void upsertAddrAndKeys(const PeerId& id, const std::string& ip, uint16_t port, const KeyBytes& boxPub, const SignPublic& signPub);
    void removeStale(std::chrono::seconds maxAge);

private:
    mutable std::mutex mtx_;
    std::vector<Peer> peers_;
};

} // namespace p2p
