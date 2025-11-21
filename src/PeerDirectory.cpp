#include "p2p/PeerDirectory.hpp"

#include <algorithm>

namespace p2p {

void PeerDirectory::addOrUpdate(const Peer& p) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = std::find_if(peers_.begin(), peers_.end(), [&](const Peer& x){ return x.id == p.id; });
    if (it == peers_.end()) peers_.push_back(p);
    else *it = p;
}

std::vector<Peer> PeerDirectory::list() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return peers_;
}

std::optional<Peer> PeerDirectory::findById(const PeerId& id) const {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = std::find_if(peers_.begin(), peers_.end(), [&](const Peer& x){ return x.id == id; });
    if (it == peers_.end()) return std::nullopt;
    return *it;
}

void PeerDirectory::removeStale(std::chrono::seconds maxAge) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto now = std::chrono::steady_clock::now();
    peers_.erase(std::remove_if(peers_.begin(), peers_.end(), [&](const Peer& p){
        if (p.lastSeen.time_since_epoch().count() == 0) return false;
        return (now - p.lastSeen) > maxAge;
    }), peers_.end());
}

void PeerDirectory::upsertAddrAndKeys(const PeerId& id, const std::string& ip, uint16_t port, const KeyBytes& boxPub, const SignPublic& signPub) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = std::find_if(peers_.begin(), peers_.end(), [&](const Peer& x){ return x.id == id; });
    Peer p{};
    if (it != peers_.end()) p = *it;
    p.id = id; p.ip = ip; p.port = port; p.publicKey = boxPub; p.signPublic = signPub; p.lastSeen = std::chrono::steady_clock::now();
    if (it == peers_.end()) peers_.push_back(p); else *it = p;
}

} // namespace p2p
