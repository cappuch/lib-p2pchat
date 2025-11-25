#include "p2p/Node.hpp"

#include <chrono>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif
#include <cstring>

namespace p2p {

Node::Node(const std::string& bindIp, uint16_t bindPort)
    : self_(Identity::generate()), router_(self_, transport_, peers_) {
    transport_.bind(bindIp, bindPort);
}

Node::~Node() { stop(); }

void Node::start() {
    if (running_) return;
    running_ = true;
    loop_ = std::thread([this]{
        auto lastBeacon = std::chrono::steady_clock::now();
        while (running_) {
            transport_.poll(100,
                [this](const Packet& pkt, const std::string& ip, uint16_t port){
                    router_.handleIncoming(pkt, ip, port);
                },
                [this](const std::vector<uint8_t>& bytes, const std::string& ip, uint16_t port){
                    // parse DISC beacon
                    if (bytes.size() < 4+2+32+32+32) return;
                    if (!(bytes[0]=='D'&&bytes[1]=='I'&&bytes[2]=='S'&&bytes[3]=='C')) return;
                    uint16_t p = (static_cast<uint16_t>(bytes[4])<<8) | bytes[5];
                    KeyBytes boxPub{}; std::memcpy(boxPub.data(), bytes.data()+6, 32);
                    SignPublic signPub{}; std::memcpy(signPub.data(), bytes.data()+6+32, 32);
                    PeerId pid{}; std::memcpy(pid.data(), bytes.data()+6+32+32, 32);
                    if (pid == self_.id) return; // ignore self
                    peers_.upsertAddrAndKeys(pid, ip, p, boxPub, signPub);
                }
            );
            // prune stale
            peers_.removeStale(std::chrono::seconds(120));

            // beacon every 2s
            auto now = std::chrono::steady_clock::now();
            if (now - lastBeacon > std::chrono::seconds(2)) {
                lastBeacon = now;
                std::vector<uint8_t> msg;
                msg.reserve(4+2+32+32+32);
                msg.push_back('D'); msg.push_back('I'); msg.push_back('S'); msg.push_back('C');
                uint16_t lp = htons(transport_.localPort());
                msg.push_back((lp>>8)&0xFF); msg.push_back(lp&0xFF);
                msg.insert(msg.end(), self_.publicKey.begin(), self_.publicKey.end());
                msg.insert(msg.end(), self_.signPublic.begin(), self_.signPublic.end());
                msg.insert(msg.end(), self_.id.begin(), self_.id.end());
                transport_.sendBroadcast(transport_.localPort(), msg);
            }
        }
    });
}

void Node::stop() {
    if (!running_) return;
    running_ = false;
    if (loop_.joinable()) loop_.join();
}

void Node::addPeer(const Peer& p) { peers_.addOrUpdate(p); }

bool Node::sendMessage(const PeerId& dest, const std::vector<uint8_t>& data) { return router_.sendMessage(dest, data); }

bool Node::sendText(const PeerId& dest, const std::string& text) {
    std::vector<uint8_t> payload(text.begin(), text.end());
    auto packed = packMessage(MessageType::TEXT, payload);
    return sendMessage(dest, packed);
}

} // namespace p2p
