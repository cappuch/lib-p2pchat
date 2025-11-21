#include "p2p/Router.hpp"

#include <algorithm>
#include <sodium.h>

namespace p2p {

Router::Router(const Identity& self, Transport& transport, PeerDirectory& peers)
    : self_(self), transport_(transport), peers_(peers) {}

void Router::onMessage(MessageHandler cb) { handlers_.push_back(std::move(cb)); }
void Router::onTypedMessage(TypedHandler cb) { typedHandlers_.push_back(std::move(cb)); }

void Router::handleIncoming(const Packet& pkt, const std::string& fromIp, uint16_t fromPort) {
    // update lastSeen for matching addr
    auto lp = peers_.list();
    for (auto& p : lp) {
        if (p.ip == fromIp && p.port == fromPort) {
            Peer np = p; np.lastSeen = std::chrono::steady_clock::now();
            peers_.addOrUpdate(np); break;
        }
    }

    // deliver if for me
    if (pkt.dest == self_.id) {
        auto sp = peers_.findById(pkt.sender);
        if (!sp) return; // unknown sender
        if (!verifyPacket(*sp, pkt)) return; // bad sig
        auto plaintext = crypto::decrypt(self_.privateKey, sp->publicKey, pkt.payload);
        // typed first
        if (!typedHandlers_.empty()) {
            MessageType mt; std::vector<uint8_t> body;
            if (unpackMessage(plaintext, mt, body)) {
                for (auto& h : typedHandlers_) h(pkt.sender, mt, body);
            }
        }
        for (auto& h : handlers_) h(pkt.sender, plaintext);
        return;
    }

    // forward if ttl
    if (pkt.ttl == 0) return;
    Packet fwd = pkt; fwd.ttl = pkt.ttl - 1;
    forward(fwd, fromIp, fromPort);
}

bool Router::forward(const Packet& pkt, const std::string& exceptIp, uint16_t exceptPort) {
    bool any = false;
    for (const auto& p : peers_.list()) {
        if (p.ip == exceptIp && p.port == exceptPort) continue;
        if (p.ip.empty() || p.port == 0) continue;
        if (transport_.send(p.ip, p.port, pkt)) any = true;
    }
    return any;
}

bool Router::sendMessage(const PeerId& dest, const std::vector<uint8_t>& data) {
    auto dp = peers_.findById(dest);
    if (!dp) return false; // need target

    // encrypt for dest
    auto ct = crypto::encrypt(self_.privateKey, dp->publicKey, data);
    Packet pkt{}; pkt.sender = self_.id; pkt.dest = dest; pkt.ttl = 8; pkt.payload = std::move(ct);
    pkt.signature = signPacket(self_, pkt);

    // try direct else flood
    if (transport_.send(dp->ip, dp->port, pkt)) return true;
    return forward(pkt, "", 0);
}

std::array<uint8_t,64> Router::signPacket(const Identity& self, const Packet& pkt) {
    // sign sender||dest||payload
    std::vector<uint8_t> m;
    m.insert(m.end(), pkt.sender.begin(), pkt.sender.end());
    m.insert(m.end(), pkt.dest.begin(), pkt.dest.end());
    m.insert(m.end(), pkt.payload.begin(), pkt.payload.end());
    auto sig = crypto::sign(self.signSecret, m);
    std::array<uint8_t,64> out{};
    if (sig.size() == out.size()) std::copy(sig.begin(), sig.end(), out.begin());
    return out;
}

bool Router::verifyPacket(const Peer& senderPeer, const Packet& pkt) {
    std::vector<uint8_t> m;
    m.insert(m.end(), pkt.sender.begin(), pkt.sender.end());
    m.insert(m.end(), pkt.dest.begin(), pkt.dest.end());
    m.insert(m.end(), pkt.payload.begin(), pkt.payload.end());
    std::vector<uint8_t> sig(pkt.signature.begin(), pkt.signature.end());
    return crypto::verify(senderPeer.signPublic, m, sig);
}

} // namespace p2p
