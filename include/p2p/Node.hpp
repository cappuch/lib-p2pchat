#pragma once

#include "p2p/Identity.hpp"
#include "p2p/PeerDirectory.hpp"
#include "p2p/Transport.hpp"
#include "p2p/Router.hpp"
#include "p2p/Message.hpp"

#include <thread>
#include <atomic>
#include <functional>

namespace p2p {

class Node {
public:
    using MessageHandler = Router::MessageHandler;
    using TypedHandler = Router::TypedHandler;

    explicit Node(const std::string& bindIp = "", uint16_t bindPort = 0);
    ~Node();

    const Identity& identity() const { return self_; }
    uint16_t port() const { return transport_.localPort(); }

    void start();
    void stop();

    void addPeer(const Peer& p);
    bool sendMessage(const PeerId& dest, const std::vector<uint8_t>& data);
    bool sendText(const PeerId& dest, const std::string& text);
    void onMessage(MessageHandler cb) { router_.onMessage(std::move(cb)); }
    void onTypedMessage(TypedHandler cb) { router_.onTypedMessage(std::move(cb)); }

private:
    Identity self_{};
    PeerDirectory peers_{};
    Transport transport_{};
    Router router_;
    std::thread loop_{};
    std::atomic<bool> running_{false};
};

} // namespace p2p
