#pragma once

#include "p2p/Packet.hpp"
#include "p2p/Peer.hpp"

#include <functional>
#include <string>
#include <vector>

namespace p2p {

class Transport {
public:
    using PacketHandler = std::function<void(const Packet& pkt, const std::string& fromIp, uint16_t fromPort)>;
    using RawHandler = std::function<void(const std::vector<uint8_t>& bytes, const std::string& fromIp, uint16_t fromPort)>;

    Transport();
    ~Transport();

    bool bind(const std::string& ip, uint16_t port);
    bool send(const std::string& ip, uint16_t port, const Packet& pkt);
    bool sendRaw(const std::string& ip, uint16_t port, const std::vector<uint8_t>& data);
    bool sendBroadcast(uint16_t port, const std::vector<uint8_t>& data);

    // poll without blocking
    void poll(int timeoutMs, const PacketHandler& pktHandler, const RawHandler& rawHandler);

    uint16_t localPort() const { return boundPort_; }

private:
    int sock_{-1};
    uint16_t boundPort_{0};
};

} // namespace p2p
