#pragma once

#include "p2p/Transport.hpp"
#include "p2p/PeerDirectory.hpp"
#include "p2p/Crypto.hpp"
#include "p2p/Message.hpp"

#include <functional>

namespace p2p {

class Router {
public:
    using MessageHandler = std::function<void(const PeerId& from, const std::vector<uint8_t>& data)>;
    using TypedHandler = std::function<void(const PeerId& from, MessageType type, const std::vector<uint8_t>& payload)>;

    Router(const Identity& self, Transport& transport, PeerDirectory& peers);

    // forward or deliver
    void handleIncoming(const Packet& pkt, const std::string& fromIp, uint16_t fromPort);

    // send encrypted; forward if needed
    bool sendMessage(const PeerId& dest, const std::vector<uint8_t>& data);

    void onMessage(MessageHandler cb);
    void onTypedMessage(TypedHandler cb);
    // sign packet bytes
    static std::array<uint8_t,64> signPacket(const Identity& self, const Packet& pkt);
    static bool verifyPacket(const Peer& senderPeer, const Packet& pkt);

private:
    Identity self_;
    Transport& transport_;
    PeerDirectory& peers_;
    std::vector<MessageHandler> handlers_{};
    std::vector<TypedHandler> typedHandlers_{};

    bool forward(const Packet& pkt, const std::string& exceptIp, uint16_t exceptPort);
};

} // namespace p2p
